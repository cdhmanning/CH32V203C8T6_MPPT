#include <i2c_bus/i2c_bus.h>
#include <string.h>
#include <stdio.h>

/*
 * Load a value into a buffer, MSB first.
 * The buffer is then a big endian value.
 */
static void load_buffer_msbf(uint8_t *buffer, uint32_t value, int n_bytes)
{
	int i;
	uint8_t *x;

	x = buffer + n_bytes - 1;

	for(i = 0; i < n_bytes; i++) {
		*x = value & 0xff;
		x--;
		value >>= 8;
	}
}

/*
 * Extract a value from a buffer.
 * The buffer is a big endian value.
 */
static uint32_t buffer_to_u32_msbf(uint8_t *buffer, int n_bytes)
{
	int i;
	uint32_t val;

	val = 0;

	for(i = 0; i < n_bytes; i++) {
		val <<= 8;
		val |= *buffer;
		buffer++;
	}
	return val;
}

int i2c_read_reg_buffer(struct i2c_bus *i2c, uint8_t dev_addr,
					uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					uint8_t *buffer, uint32_t n_bytes)
{
	struct i2c_msg msg[2];
	uint8_t addr_buffer[4];
	int ret;

	memset(msg, 0, sizeof(msg));
	load_buffer_msbf(addr_buffer, reg_addr, reg_addr_n_bytes);
	msg[0].dest_addr7 = dev_addr;
	msg[0].flags = I2C_MSG_FLAG_WRITE | I2C_MSG_FLAG_NO_STOP;
	msg[0].buffer_length = reg_addr_n_bytes;
	msg[0].buffer = addr_buffer;

	msg[1].dest_addr7 = dev_addr;
	msg[1].flags = I2C_MSG_FLAG_READ;
	msg[1].buffer_length = n_bytes;
	msg[1].buffer = buffer;

	if (reg_addr_n_bytes > 0)
		ret = i2c_transact(i2c, msg, 2);
	else
		ret = i2c_transact(i2c, &msg[1], 1);

	if (ret < 1)
		return -1;

	return 0;
}

int i2c_write_reg_buffer(struct i2c_bus *i2c,  uint8_t dev_addr,
					 uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					 uint8_t *buffer, int n_bytes)
{
	struct i2c_msg msg;
	uint8_t addr_buffer[4];
	int ret;

	load_buffer_msbf(addr_buffer, reg_addr, reg_addr_n_bytes);
	msg.dest_addr7 = dev_addr;
	msg.flags = I2C_MSG_FLAG_WRITE | I2C_MSG_FLAG_NO_STOP;
	msg.buffer = addr_buffer;
	msg.buffer_length = reg_addr_n_bytes;
	msg.ext_buffer = buffer;
	msg.ext_buffer_length = n_bytes;

	ret = i2c_transact(i2c, &msg, 1);

	if (ret < 1)
		return -1;

	return 0;
}

int i2c_read_reg(struct i2c_bus *i2c, uint8_t dev_addr,
					uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					uint32_t *val_out, uint32_t val_out_n_bytes)
{
	uint8_t val_buffer[4];
	uint32_t val;
	int ret;

	ret = i2c_read_reg_buffer(i2c, dev_addr,
								 reg_addr, reg_addr_n_bytes,
								 val_buffer, val_out_n_bytes);

	if (ret < 0)
		return -1;
	val = buffer_to_u32_msbf(val_buffer, val_out_n_bytes);

	if (val_out)
		*val_out = val;

	return 0;
}

int i2c_write_reg(struct i2c_bus *i2c, uint8_t dev_addr,
					uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					uint32_t val, uint32_t val_n_bytes)
{
	uint8_t val_buffer[4];
	int ret;

	load_buffer_msbf(val_buffer, val, val_n_bytes);
	ret = i2c_write_reg_buffer(i2c, dev_addr,
								 reg_addr, reg_addr_n_bytes,
								 val_buffer, val_n_bytes);

	if (ret < 0)
		return -1;

	return 0;
}


int i2c_transact(struct i2c_bus *i2c, struct i2c_msg *msg, int n_msg)
{
	return i2c->transact_fn(i2c, msg, n_msg);
}

int i2c_check(struct i2c_bus *i2c, uint8_t dev_addr)
{
	return i2c->check_fn(i2c, dev_addr);
}


/*
 * i2c_scan_bus()
 * Scans the bus and prints what devices are seen.
 */
int i2c_scan_bus(struct i2c_bus *i2c)
{
	uint8_t i;
	int n = 0;
	int result;

	printf("I2C Bus scan\n");
	for( i = 0x2; i < 0xFE; i+= 2) {
		result = i2c_check(i2c, i);
		if (result > 0) {
			printf("Device at 0x%02x\n", i);
			n++;
		}
	}
	printf("%d i2C devices found\n", n);
	return n;
}

