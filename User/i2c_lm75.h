
#ifndef _USER_I2C_LM75_H_
#define _USER_I2C_LM75_H_

#include "i2c_if.h"

struct i2c_lm75 {
	struct i2c_if *i2c;
	uint8_t address;	/* 7 bit i2c address, << 1 */
#if 0
	uint8_t register_buffer[1];
	uint8_t measurement_buffer[2];
	struct i2c_transaction msg[2];
	int		reg_selected;
#endif
};

int i2c_lm75_init(struct i2c_lm75 *lm75, struct i2c_if *i2c, uint8_t address);

int i2c_lm75_read(struct i2c_lm75 *lm75, int *out_val_mdeg);

int i2c_lm75_test(struct i2c_if *i2c);


#endif
