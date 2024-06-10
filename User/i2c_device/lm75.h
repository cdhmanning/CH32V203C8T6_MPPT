
#ifndef _USER_I2C_LM75_H_
#define _USER_I2C_LM75_H_

#include "i2c_bus.h"

struct lm75 {
	struct i2c_bus *i2c;
	uint8_t address;	/* 7 bit i2c address, << 1 */
#if 0
	uint8_t register_buffer[1];
	uint8_t measurement_buffer[2];
	struct i2c_transaction msg[2];
	int		reg_selected;
#endif
};

int lm75_init(struct lm75 *lm75, struct i2c_bus *i2c, uint8_t address);

int lm75_read(struct lm75 *lm75, int *out_val_mdeg);

int lm75_test(struct i2c_bus *i2c);


#endif
