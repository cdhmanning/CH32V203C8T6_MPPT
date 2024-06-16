
#ifndef USER_I2C_DEVICE_HD44780_IF_I2C_H_
#define USER_I2C_DEVICE_HD44780_IF_I2C_H_

#include "i2c_bus/i2c_bus.h"
#include "hd44780_interface.h"

struct hd44780_interface_i2c {
	struct hd44780_interface interface;
	struct i2c_bus *i2c;
	uint8_t address;	/* 7 bit i2c address, << 1 */
};

void hd44780_interface_i2c_init(struct hd44780_interface_i2c *this_dev,
								struct i2c_bus *bus,
								uint8_t address);

#endif
