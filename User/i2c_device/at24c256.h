/*
 *
 */


#ifndef _USER_I2C_AT24C256_H_
#define _USER_I2C_AT24C2565_H_

#include <i2c_bus/i2c_bus.h>

struct at24c256 {
	struct i2c_bus *i2c;
	uint8_t address;	/* 7 bit i2c address, << 1 */
};

int at24c256_init(struct at24c256 *dev, struct i2c_bus *i2c, uint8_t address);

int at24c256_read_buffer(struct at24c256 *dev, uint32_t address,
							uint8_t *buffer, int n_bytes);
int at24c256_write_buffer(struct at24c256 *dev, uint32_t address,
							uint8_t *buffer, int n_bytes);

int at24c256_test(struct i2c_bus *i2c);


#endif
