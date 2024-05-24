

#include "i2c_device/ina226.h"
#include <string.h>

#include "delay_ms.h"

#define mA_SHIFT	10

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
					int shunt_uOhm)
{
	int ret = 0;
	uint16_t cfg_value;

	memset(ina226, 0, sizeof(*ina226));
	ina226->i2c = i2c;
	ina226->address = address;
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

static void ina226_msg_cfg(struct i2c_msg *m,
				uint8_t addr, uint8_t *reg_buf, uint8_t *buffer)
{
	m->dest_addr7 = addr;
	m->flags = I2C_MSG_FLAG_WRITE | I2C_MSG_FLAG_NO_STOP;
	m->buffer = reg_buf;
	m->buffer_length = 1;
	m++;

	m->dest_addr7 = addr;
	m->flags = I2C_MSG_FLAG_READ;
	m->buffer = buffer;
	m->buffer_length = 2;
	m++;
}

int ina226_quick_read(struct i2c_if *i2c,
						struct ina226 *ina226_0,
						struct ina226 *ina226_1)
{
	uint8_t mA_buffer0[2];
	uint8_t mV_buffer0[2];
	uint8_t mA_buffer1[2];
	uint8_t mV_buffer1[2];
	uint8_t reg_buf0[1];
	uint8_t reg_buf1[1];
	struct i2c_msg msg[8];
	int ret;
	uint8_t addr0 = 0x80;
	uint8_t addr1 = 0x88;

	memset(msg, 0, sizeof(msg));
	memset(mA_buffer0, 0, sizeof(mA_buffer0));
	memset(mV_buffer0, 0, sizeof(mV_buffer0));
	memset(mA_buffer1, 0, sizeof(mA_buffer1));
	memset(mV_buffer1, 0, sizeof(mV_buffer1));

	reg_buf0[0] = 1;
	reg_buf1[0] = 2;

	ina226_msg_cfg(&msg[0], addr0, reg_buf0, mA_buffer0);
	ina226_msg_cfg(&msg[2], addr0, reg_buf1, mV_buffer0);

	ina226_msg_cfg(&msg[4], addr1, reg_buf0, mA_buffer1);
	ina226_msg_cfg(&msg[6], addr1, reg_buf1, mV_buffer1);

	ret = i2c_if_transact(i2c, msg, 8);

	if (ret >= 0) {
		printf("0 mV buffer %02x %02x mA buffer %02x %02x\n",
					mV_buffer0[0], mV_buffer0[1], mA_buffer0[0], mA_buffer0[1]);
		printf("1 mV buffer %02x %02x mA buffer %02x %02x\n",
							mV_buffer1[0], mV_buffer1[1], mA_buffer1[0], mA_buffer1[1]);

		printf("0 mV %d mA %d\n",
				u16_to_mV(ina226_0, mV_buffer0[0] << 8 | mV_buffer0[1]),
				u16_to_mA(ina226_0, mA_buffer0[0] << 8 | mA_buffer0[1]));
		printf("1 mV %d mA %d\n",
				u16_to_mV(ina226_1, mV_buffer1[0] << 8 | mV_buffer1[1]),
				u16_to_mA(ina226_1, mA_buffer1[0] << 8 | mA_buffer1[1]));
	}

	return ret;

}

int ina226_test(struct i2c_if *i2c)
{
	struct ina226 ina226_0;
	struct ina226 ina226_1;
	int ret;
	int i;
	int mV_0;
	int mA_0;
	int mW_0;
	int mV_1;
	int mA_1;
	int mW_1;

	ret = ina226_init(&ina226_0, i2c, 0x80, 100000);
	ret = ina226_init(&ina226_1, i2c, 0x88, 100000);

	printf("ina226 init returned %d\n", ret);

	ina226_quick_read(i2c, &ina226_0, &ina226_1);

	for(i = 0; i < 0; i++) {
		ret = ina226_read_mV(&ina226_0, &mV_0);
		ret = ina226_read_mA(&ina226_1, &mA_0);
		mW_0 = mV_0 * mA_0 /1000;
		ret = ina226_read_mV(&ina226_0, &mV_1);
		ret = ina226_read_mA(&ina226_1, &mA_1);
		mW_1 = mV_1 * mA_1 /1000;

		printf("ina226 read_mV0 ret %d, %d mV\n", ret, mV_0);
		printf("ina226 read_mA0 ret %d, %d mA\n", ret, mA_0);
		printf("Power %d mW0\n", mW_0);

		printf("ina226 read_mV1 ret %d, %d mV\n", ret, mV_1);
		printf("ina226 read_mA1 ret %d, %d mA\n", ret, mA_1);
		printf("Power %d mW1\n", mW_1);
		delay_ms(10);
		//ret = ina226_read(&bad, &temp_mdeg);
		//printf("Bad ina226 read ret %d, %d mdegrees\n", ret, temp_mdeg);
	}

	return ret;
}
