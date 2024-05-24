
#ifndef _USER_I2C_INA226_H_
#define _USER_I2C_INA226_H_

#include "i2c_if.h"

struct ina226 {
	struct i2c_if *i2c;
	uint8_t address;	/* 7 bit i2c address, << 1 */
	int shunt_multiplier;
	int shunt_shift;
};

int ina226_init(struct ina226 *ina226,
					struct i2c_if *i2c, uint8_t address,
					int    shunt_mult);

int ina226_read_mV(struct ina226 *ina226, int *out_mV);
int ina226_read_mA(struct ina226 *ina226, int *out_mA);

int ina226_test(struct i2c_if *i2c);


#endif
