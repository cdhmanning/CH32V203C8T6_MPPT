/*
 * i2c_if.h
 */

#ifndef _USER_I2C_IF_H_
#define _USER_I2C_IF_H_

#include <i2c_bus/i2c_bus.h>
#include "ch32v20x.h"
#include <stdint.h>

struct i2c_if {
	struct i2c_bus bus;
	I2C_TypeDef *interface;
	struct i2c_msg *msgs;
	int n_msgs;
	uint8_t *buffer;
	int n_bytes;
	int stop_sent;
	int have_bus;
	int ext_buffer_loaded;
};

int i2c_if_init(struct i2c_if *i2c, I2C_TypeDef *interface);


#endif
