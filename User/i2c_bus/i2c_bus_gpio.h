
#ifndef _I2C_BUS_I2C_GPIO_H_
#define _I2C_BUS_I2C_GPIO_H_

#include <i2c_bus/i2c_bus.h>
#include "ch32v20x.h"
#include <stdint.h>

struct i2c_bus_gpio {
	struct i2c_bus bus;
	GPIO_TypeDef   *sda_port;
	uint32_t		sda_pin;
	GPIO_TypeDef   *scl_port;
	uint32_t		scl_pin;
	struct i2c_msg *msgs;
	int n_msgs;
	uint8_t *buffer;
	int n_bytes;
	int stop_sent;
	int have_bus;
	int ext_buffer_loaded;
};

int i2c_bus_gpio_init(struct i2c_bus_gpio *i2c,
		GPIO_TypeDef   *sda_port,
		uint32_t		sda_pin,
		GPIO_TypeDef   *scl_port,
		uint32_t		scl_pin);


#endif
