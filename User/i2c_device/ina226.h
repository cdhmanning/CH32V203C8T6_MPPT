
#ifndef _USER_I2C_INA226_H_
#define _USER_I2C_INA226_H_

#include "i2c_if.h"


#define INA226_REG_CONFIG 	0x00
#define INA226_REG_SHUNTV 	0x01
#define INA226_REG_BUSV 	0x02


struct ina226 {
	char *name;
	struct i2c_if *i2c;
	uint8_t address;	/* 7 bit i2c address, << 1 */
	int shunt_multiplier;
	int shunt_shift;

	/* Buffers for measurement. */
	uint8_t shuntV_bytes[2];
	uint8_t busV_bytes[2];

	/* Output values */
	int mA;
	int mV;
};

int ina226_init(struct ina226 *ina226,
					struct i2c_if *i2c, uint8_t address,
					int shunt_uOhm,
					char *name);

int ina226_read_mV(struct ina226 *ina226, int *out_mV);
int ina226_read_mA(struct ina226 *ina226, int *out_mA);

int ina226_test(struct i2c_if *i2c);


#endif
