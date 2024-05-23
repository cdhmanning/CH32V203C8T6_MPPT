
#ifndef _USER_I2C_INA226_H_
#define _USER_I2C_INA226_H_

#include "i2c_if.h"

struct i2c_ina226 {
	struct i2c_if *i2c;
	uint8_t address;	/* 7 bit i2c address, << 1 */
	int shunt_multiplier;
	int shunt_shift;
};

int i2c_ina226_init(struct i2c_ina226 *ina226,
					struct i2c_if *i2c, uint8_t address,
					int    shunt_mult);

int i2c_ina226_read_mV(struct i2c_ina226 *ina226, int *out_mV);
int i2c_ina226_read_mA(struct i2c_ina226 *ina226, int *out_mA);

int i2c_ina226_test(struct i2c_if *i2c);


#endif
