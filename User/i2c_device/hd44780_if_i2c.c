#include "hd44780_if_i2c.h"


static int hd44780_interface_i2c_output(struct hd44780_interface *interface,
										uint8_t b)
{
	struct hd44780_interface_i2c *this_dev =
				(struct hd44780_interface_i2c *)interface->private;

	return i2c_write_reg(this_dev->i2c, this_dev->address, 0, 0, b, 1);
}

void hd44780_interface_i2c_init(struct hd44780_interface_i2c *this_dev,
								struct i2c_bus *bus,
								uint8_t address)
{
	this_dev->i2c = bus;
	this_dev->address = address;
	this_dev->interface.output = hd44780_interface_i2c_output;
	this_dev->interface.private = (void *) this_dev;
}

