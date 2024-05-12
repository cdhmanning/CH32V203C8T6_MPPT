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
	uint8_t *buffer;
	uint32_t ext_buffer_length;
	uint8_t *ext_buffer;
};


struct i2c_if {
	I2C_TypeDef *interface;
	struct i2c_transaction *transactions;
	int n_transactions;
	uint8_t *buffer;
	int n_bytes;
	int stop_sent;
	int have_bus;
	int ext_buffer_loaded;
};

int i2c_if_init(struct i2c_if *i2c, I2C_TypeDef *interface);

int i2c_if_transact(struct i2c_if *i2c, struct i2c_transaction * transaction, int n_transactions);

/*
 * i2c_if_check() checks if there is a device at the specified address.
 * Returns -1: some sort of failure
 *          0: no device there
 *          1: device seen
 */
int i2c_if_check(struct i2c_if *i2c, uint8_t addr);

/*
 * i2c_if_scan_bus()
 * Scans the bus and prints what devices are seen.
 */
int i2c_if_scan_bus(struct i2c_if *i2c);

#endif
