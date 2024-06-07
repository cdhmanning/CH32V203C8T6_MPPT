

#include "i2c_device/ina226.h"
#include <string.h>

#include "delay_ms.h"

#define mA_SHIFT	10

#define N_BACKGROUND_MSG 8
static struct {
	struct i2c_if *i2c;
	struct i2c_msg msg[N_BACKGROUND_MSG];
	int n_msg;
	uint8_t reg_busV;
	uint8_t reg_shuntV;
} background_processing;

static int ina226_read_reg(struct ina226 *ina226,
							   uint32_t reg,
							   uint32_t *out_value)
{
	return i2c_if_read_reg(ina226->i2c, ina226->address, reg, 1, out_value, 2);
}

static int ina226_write_reg(struct ina226 *ina226,
								uint32_t reg,
								uint16_t out_value)
{
	return i2c_if_write_reg(ina226->i2c, ina226->address, reg, 1, out_value, 2);
}

/*
 * mA = 1000 * reading * 2.5E-6/(shunt_uOhm * 1E-6)
 *
 * mA = 2500 * reading / uOhm
 *
 * mA = 2500/uOhm * reading
 *
 * But because 2500/uOhm is very low and we want integer maths, let's scale it
 * by 2**10.
 */
static int calc_shunt_multiplier(struct ina226 *ina226, int shunt_uOhm)
{
	 if (shunt_uOhm < 1)
		 shunt_uOhm = 1;

	 ina226->shunt_multiplier = (2500 * (1<<mA_SHIFT) + shunt_uOhm/2)/shunt_uOhm;
	 ina226->shunt_shift = mA_SHIFT;

	 return 0;
}

int ina226_init(struct ina226 *ina226,
					struct i2c_if *i2c, uint8_t address,
					int shunt_uOhm,
					char *name)
{
	int ret = 0;
	uint16_t cfg_value;

	memset(ina226, 0, sizeof(*ina226));
	ina226->i2c = i2c;
	ina226->address = address;
	ina226->name = strdup(name);

	calc_shunt_multiplier(ina226, shunt_uOhm);

	/*
	 * Configure to give a reasonably smoothed average every 1 and a bit msec.
	 */
	cfg_value = 0x01 << 9 | // 4 sample averaging
				0x02 << 6 | // 332usec bus conversion time
				0x02 << 3 | // 332usec shunt conversion time
				0x07 << 0 ; // bus and shunt continuous conversion

	ret = ina226_write_reg(ina226, 0x00, cfg_value);

	return ret;
}


/*
 * Read the bus voltage register and calculate mV.
 * The value can only be positive.
 * LSB is 1.25mV.
 */

int u16_to_mV(struct ina226 *ina226, uint16_t u16_mV)
{
	int mV;

	(void) ina226;
	/*
	 * LSB is 1.25mV, so take the value and add a quarter of the value to
	 * that to get the 0.25mV.
	 */
	mV = (int)u16_mV;
	mV += mV/4;
	return mV;
}

int ina226_read_mV(struct ina226 *ina226, int *out_mV)
{
	uint32_t umV;
	int mV;
	int ret;

	ret = ina226_read_reg(ina226, 0x02, &umV);

	if (ret >= 0) {
		mV = u16_to_mV(ina226, umV);
	} else {
		mV = -1;
	}

	if (out_mV)
		*out_mV = mV;
	return ret;
}

/*
 * Read shunt voltage register and convert the value into mA.
 */

int u16_to_mA(struct ina226 *ina226, uint16_t u16_mA)
{
	int mA;

	/*
	 *  LSB is 2.5uV. Need to convert to mA.
	 */
	mA = (int16_t)((uint16_t) u16_mA);
	mA *= ina226->shunt_multiplier;
	mA >>= ina226->shunt_shift;

	return mA;
}

int ina226_read_mA(struct ina226 *ina226, int *out_mA)
{
	uint32_t umA;
	int mA;
	int ret;

	ret = ina226_read_reg(ina226, 0x01, &umA);

	if (ret >= 0) {
		mA = u16_to_mA(ina226, umA);
	} else {
		mA = -1;
	}

	if (out_mA)
		*out_mA = mA;
	return ret;
}

void ina226_update_values(struct ina226 *ina226)
{
	uint16_t u16_shuntV;
	uint16_t u16_busV;

	u16_shuntV = ina226->shuntV_bytes[0] << 8 | ina226->shuntV_bytes[1];
	u16_busV = ina226->busV_bytes[0] << 8 | ina226->busV_bytes[1];
	ina226->mA = u16_to_mA(ina226, u16_shuntV);
	ina226->mV = u16_to_mV(ina226, u16_busV);
}

void ina226_print_values(struct ina226 *ina226)
{
	printf("%s  %dmA %dmV\n", ina226->name, ina226->mA, ina226->mV);
}

static void ina226_msg_cfg(int msg_pos, struct ina226 *ina226)
{
	struct i2c_msg *m = & background_processing.msg[msg_pos];

	m->dest_addr7 = ina226->address;
	m->flags = I2C_MSG_FLAG_WRITE | I2C_MSG_FLAG_NO_STOP;
	m->buffer = &background_processing.reg_shuntV;
	m->buffer_length = 1;
	m++;

	m->dest_addr7 = ina226->address;
	m->flags = I2C_MSG_FLAG_READ;
	m->buffer = ina226->shuntV_bytes;
	m->buffer_length = 2;
	m++;

	m->dest_addr7 = ina226->address;
	m->flags = I2C_MSG_FLAG_WRITE | I2C_MSG_FLAG_NO_STOP;
	m->buffer = &background_processing.reg_busV;
	m->buffer_length = 1;
	m++;

	m->dest_addr7 = ina226->address;
	m->flags = I2C_MSG_FLAG_READ;
	m->buffer = ina226->busV_bytes;
	m->buffer_length = 2;
	m++;

}

int ina226_quick_read_init(struct ina226 *ina226_0,
						   struct ina226 *ina226_1)
{
	memset(&background_processing, 0, sizeof(background_processing));

	background_processing.i2c = ina226_0->i2c;

	background_processing.reg_busV = INA226_REG_BUSV;
	background_processing.reg_shuntV = INA226_REG_SHUNTV;

	ina226_msg_cfg(0, ina226_0);
	ina226_msg_cfg(4, ina226_1);

	background_processing.n_msg = 8;

	return 0;
}

int ina226_perform_quick_read(struct ina226 *ina226_0,
		   	   	   	   	      struct ina226 *ina226_1)
{
	int ret;

	ret = i2c_if_transact(background_processing.i2c,
						  background_processing.msg,
						  background_processing.n_msg);

	if (ret > 0) {
		ina226_update_values(ina226_0);
		ina226_update_values(ina226_1);
		ina226_print_values(ina226_0);
		ina226_print_values(ina226_1);
	}

	return ret;

}

int ina226_test(struct i2c_if *i2c)
{
	struct ina226 ina226_0;
	struct ina226 ina226_1;
	int ret;


	ret = ina226_init(&ina226_0, i2c, 0x80, 100000, "Main battery");
	ret = ina226_init(&ina226_1, i2c, 0x88, 100000, "Secondary battery");

	printf("ina226 init returned %d\n", ret);

	ina226_quick_read_init(&ina226_0, &ina226_1);
	ina226_perform_quick_read(&ina226_0, &ina226_1);

	return ret;
}
