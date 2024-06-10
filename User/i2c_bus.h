/*
 * General "class" for an i2c bus.
 */

#ifndef USER_I2C_BUS_H_
#define USER_I2C_BUS_H_

#include <stdint.h>

struct i2c_msg {
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

struct i2c_bus {
	void *private;
	int  (*transact_fn)(struct i2c_bus *i2c, struct i2c_msg *msg, int n_msg);
	int  (*check_fn)(struct i2c_bus *i2c, uint8_t dev_addr);
};

int i2c_transact(struct i2c_bus *i2c, struct i2c_msg *msg, int n_msg);
int i2c_scan_bus(struct i2c_bus *i2c);


int i2c_read_reg_buffer(struct i2c_bus *i2c, uint8_t dev_addr,
					uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					uint8_t *buffer, uint32_t n_bytes);

int i2c_write_reg_buffer(struct i2c_bus *i2c,  uint8_t dev_addr,
					 uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					 uint8_t *buffer, int n_bytes);

int i2c_read_reg(struct i2c_bus *i2c, uint8_t dev_addr,
				    uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					uint32_t *val_out, uint32_t val_out_n_bytes);
int i2c_write_reg(struct i2c_bus *i2c, uint8_t dev_addr,
					 uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					 uint32_t val, uint32_t val_n_bytes);


#endif
