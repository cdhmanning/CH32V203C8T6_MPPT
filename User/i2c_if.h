/*
 * i2c_if.h
 */

#ifndef _USER_I2C_IF_H_
#define _USER_I2C_IF_H_

#include "ch32v20x.h"
#include <stdint.h>

struct i2c_transaction {
	uint8_t	dest_addr7; /* Shifted so that 0x80 is MSB */
	uint32_t flags;
#define I2C_MSG_FLAG_WRITE		0x0001
#define I2C_MSG_FLAG_READ		0x0002
/* Not yet supported. */
#define I2C_MSG_FLAG_NO_STOP	0x0010
	uint32_t buffer_length;
	char *buffer;
	uint32_t ext_buffer_length;
	char *ext_buffer;
};


struct i2c_if {
	I2C_TypeDef *interface;
	struct i2c_transaction *transactions;
	int n_transactions;
	char *buffer;
	int n_bytes;
	int stop_sent;
	int have_bus;
	int ext_buffer_loaded;
};

int i2c_if_init(struct i2c_if *i2c, I2C_TypeDef *interface);

int i2c_if_transact(struct i2c_if *i2c, struct i2c_transaction * transaction, int n_transactions);

int i2c_if_test(struct i2c_if *i2c);

#endif
