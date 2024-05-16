/*
 *
 */


#ifndef _USER_I2C_24C256_H_
#define _USER_I2C_24C2565_H_

#include "i2c_if.h"

struct i2c_24c256 {
	struct i2c_if *i2c;
	uint8_t address;	/* 7 bit i2c address, << 1 */
};

int i2c_24c256_init(struct i2c_24c256 *dev, struct i2c_if *i2c, uint8_t address);

int i2c_24c256_read_buffer(struct i2c_24c256 *dev, uint32_t address,
							uint8_t *buffer, int n_bytes);
int i2c_24c256_write_buffer(struct i2c_24c256 *dev, uint32_t address,
							uint8_t *buffer, int n_bytes);

int i2c_24c256_test(struct i2c_if *i2c);


#endif
