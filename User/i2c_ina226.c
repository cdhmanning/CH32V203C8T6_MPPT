

#include "i2c_ina226.h"
#include <string.h>

#include "delay_ms.h"

#define mA_SHIFT	10

static int i2c_ina226_read_reg(struct i2c_ina226 *ina226,
							   uint32_t reg,
							   uint32_t *out_value)
{
	return i2c_if_read_reg(ina226->i2c, ina226->address, reg, 1, out_value, 2);
}

static int i2c_ina226_write_reg(struct i2c_ina226 *ina226,
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
static int calc_shunt_multiplier(struct i2c_ina226 *ina226, int shunt_uOhm)
{
	 if (shunt_uOhm < 1)
		 shunt_uOhm = 1;

	 ina226->shunt_multiplier = (2500 * (1<<mA_SHIFT) + shunt_uOhm/2)/shunt_uOhm;
	 ina226->shunt_shift = mA_SHIFT;

	 return 0;
}

int i2c_ina226_init(struct i2c_ina226 *ina226,
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

	ret = i2c_ina226_write_reg(ina226, 0x00, cfg_value);

	return ret;
}


/*
 * Read the bus voltage register and calculate mV.
 * The value can only be positive.
 * LSB is 1.25mV.
 */
int i2c_ina226_read_mV(struct i2c_ina226 *ina226, int *out_mV)
{
	uint32_t umV;
	int mV;
	int ret;

	ret = i2c_ina226_read_reg(ina226, 0x02, &umV);

	if (ret >= 0) {
		/*
		 * LSB is 1.25mV, so take the value and add a quarter of the value to
		 * that to get the 0.25mV.
		 */
		mV = (int)umV;
		mV += mV/4;
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
int i2c_ina226_read_mA(struct i2c_ina226 *ina226, int *out_mA)
{
	uint32_t umA;
	int mA;
	int ret;

	ret = i2c_ina226_read_reg(ina226, 0x01, &umA);

	if (ret >= 0) {
		/*
		 *  LSB is 2.5uV. Need to convert to mA.
		 */
		mA = (int16_t)((uint16_t) umA);
		mA *= ina226->shunt_multiplier;
		mA >>= ina226->shunt_shift;
	} else {
		mA = -1;
	}

	if (out_mA)
		*out_mA = mA;
	return ret;
}


int i2c_ina226_test(struct i2c_if *i2c)
{
	struct i2c_ina226 ina226;
	int ret;
	int i;
	int mV;
	int mA;
	int mW;

	ret = i2c_ina226_init(&ina226, i2c, 0x88, 100000);

	printf("ina226 init returned %d\n", ret);

	for(i = 0; i < 10; i++) {
		ret = i2c_ina226_read_mV(&ina226, &mV);
		ret = i2c_ina226_read_mA(&ina226, &mA);
		mW = mV * mA /1000;
		printf("ina226 read_mV ret %d, %d mV\n", ret, mV);
		printf("ina226 read_mA ret %d, %d mA\n", ret, mA);
		printf("Power %d mW\n", mW);
		delay_ms(10);
		//ret = i2c_ina226_read(&bad, &temp_mdeg);
		//printf("Bad ina226 read ret %d, %d mdegrees\n", ret, temp_mdeg);
	}

	return ret;
}
