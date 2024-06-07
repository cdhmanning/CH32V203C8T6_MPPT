/*
 * I2C driver that operates in polled mode with time outs.
 * It would probably be a good idea for the main transaction handler to
 * check if the AF flag is set (acknowledgement fail) for
 * detecting if the device is not present/ready.
 */

#include "i2c_if.h"

#include "hw_map.h"

#include <stddef.h>
#include <string.h>
#include "delay_ms.h"

int i2c_if_init(struct i2c_if *i2c, I2C_TypeDef *interface)
{
	memset(i2c, 0, sizeof(*i2c));
	i2c->interface = interface;

	return 0;
}


#define timed_out(start) ((get_tick() - (start)) > 2)

static int wait_flag(struct i2c_if *i2c, uint32_t flag, uint32_t value)
{
	uint32_t start = get_tick();

	while(I2C_GetFlagStatus( i2c->interface, flag ) != value ) {
		if (timed_out(start))
			return -1;
	}
	return 0;
}

static int wait_event(struct i2c_if *i2c, uint32_t event)
{
	int32_t start = get_tick();

	while(!I2C_CheckEvent( i2c->interface, event)) {
		if (timed_out(start))
			return -1;
	}
	return 0;
}

volatile uint32_t got_to;

int i2c_if_transact(struct i2c_if *i2c,
					struct i2c_msg *msgs,
					int n_msgs)
{
	int ret = 0;
	int ok_transactions = 0;
	uint8_t dummy;

	(void) dummy;

	if (!i2c)
		return -1;
	if (i2c->msgs)
		return -1; /* Busy */
	if (!msgs)
		return -1;
	if (n_msgs < 1)
		return -1;

	i2c->msgs = msgs;
	i2c->n_msgs = n_msgs;
	i2c->buffer = 0;
	i2c->n_bytes = 0;


	/*
	 * Clear any stale state out.
	 */
	dummy = I2C_ReceiveData(i2c->interface);
	I2C_ClearFlag(i2c->interface,I2C_FLAG_AF);

	while(i2c->n_msgs > 0) {

		i2c->buffer = i2c->msgs->buffer;
		i2c->n_bytes = i2c->msgs->buffer_length;
		i2c->stop_sent = 0;
		i2c->ext_buffer_loaded = 0;

		got_to |= 1;
		if (!i2c->have_bus) {
	        GPIO_WriteBit(LED0_IO, 1);
			//ret = wait_flag(i2c, I2C_FLAG_BUSY, 0);
	        GPIO_WriteBit(LED0_IO, 0);
		}

		if (ret < 0)
			goto fail;
		got_to |= 2;
		I2C_GenerateSTART(i2c->interface, 1);

		//delay_ms(5);

		ret = wait_event(i2c, I2C_EVENT_MASTER_MODE_SELECT);

		if (ret < 0)
			goto fail;

		i2c->have_bus = 1;

		if (i2c->msgs->flags & I2C_MSG_FLAG_READ) {
			/* Read mode */
			I2C_Send7bitAddress(i2c->interface,
								i2c->msgs->dest_addr7,
								I2C_Direction_Receiver);

			//delay_ms(10);
			ret = wait_event(i2c, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
			if (ret < 0)
				goto fail;
			got_to |= 4;

			/*
			 * After reading the second last byte we request the stop condition.
			 * We do that because it will be applied when the read that we just started
			 * (ie. the last read), is complete.
			 * If we don't do this then we get one more read on the wire than we wanted.
			 */
			while(i2c->n_bytes > 0) {

				if(i2c->n_bytes == 1) {
					//I2C_AcknowledgeConfig(i2c->interface, DISABLE );
					I2C_GenerateSTOP(i2c->interface, ENABLE );
					i2c->stop_sent = 1;
					i2c->have_bus = 0;
					//delay_ms(2);
				}

				ret = wait_flag(i2c, I2C_FLAG_RXNE, 1);
				if (ret < 0)
					goto fail;
				got_to |= 8;

		        //GPIO_WriteBit(LED0_IO, 1);


		        //GPIO_WriteBit(LED0_IO, 0);


				i2c->buffer[0] = I2C_ReceiveData(i2c->interface);

				i2c->n_bytes--;
				i2c->buffer++;
			}

		} else {
			/* Write mode
			 * There are two buffers to write,
			 * buffer: the first buffer
			 * ext_buffer: the second buffer
			 * One or both of those buffers might be empty, in which case
			 * that needs to be handled.
			 */
			I2C_Send7bitAddress(i2c->interface,
								i2c->msgs->dest_addr7,
								I2C_Direction_Transmitter);
			//delay_ms(10);

			ret = wait_event(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);

			got_to |= 0x10;

			if (ret < 0)
				goto fail;
			got_to |= 0x20;

			/*
			 * If there is something in the buffers to send, then send it.
			 */
			if (i2c->n_bytes > 0 || i2c->msgs->ext_buffer_length) {
				/*
				 * If the first buffer is empty (eg. chip does not have a
				 * register address) then skip to the second buffer.
				 */
				if(i2c->n_bytes == 0 && !i2c->ext_buffer_loaded) {
					i2c->buffer = i2c->msgs->ext_buffer;
					i2c->n_bytes = i2c->msgs->ext_buffer_length;
					i2c->ext_buffer_loaded = 1;
				}

				while(i2c->n_bytes > 0) {

					wait_flag(i2c, I2C_FLAG_TXE, 1);
					if (ret < 0)
						goto fail;
					got_to |= 0x40;

					I2C_SendData(i2c->interface, i2c->buffer[0]);

					i2c->n_bytes--;
					i2c->buffer++;

					/*
					 * If we've done with the first buffer, then set up the
					 * second buffer.
					 */
					if(i2c->n_bytes == 0 && !i2c->ext_buffer_loaded) {
						i2c->buffer = i2c->msgs->ext_buffer;
						i2c->n_bytes = i2c->msgs->ext_buffer_length;
						i2c->ext_buffer_loaded = 1;
					}
				}
				ret = wait_event(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED) ;
			}

			if (ret < 0)
				goto fail;
			got_to |= 0x80;

			if(i2c->stop_sent == 0 &&
			  !(i2c->msgs->flags &I2C_MSG_FLAG_NO_STOP)) {
				I2C_GenerateSTOP(i2c->interface, ENABLE );
				i2c->stop_sent = 1;
				i2c->have_bus = 0;
			}
		}

		i2c->msgs++;
		i2c->n_msgs--;
		ok_transactions++;
	}

fail:

	if (!i2c->stop_sent) {
		I2C_GenerateSTOP( I2C1, ENABLE );
		i2c->stop_sent = 1;
	}

	i2c->msgs = NULL;

	return ok_transactions;
}


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

int i2c_if_read_reg_buffer(struct i2c_if *i2c, uint8_t dev_addr,
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
		ret = i2c_if_transact(i2c, msg, 2);
	else
		ret = i2c_if_transact(i2c, &msg[1], 1);

	if (ret < 1)
		return -1;

	return 0;
}

int i2c_if_write_reg_buffer(struct i2c_if *i2c,  uint8_t dev_addr,
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

	ret = i2c_if_transact(i2c, &msg, 1);

	if (ret < 1)
		return -1;

	return 0;
}

int i2c_if_read_reg(struct i2c_if *i2c, uint8_t dev_addr,
					uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					uint32_t *val_out, uint32_t val_out_n_bytes)
{
	uint8_t val_buffer[4];
	uint32_t val;
	int ret;

	ret = i2c_if_read_reg_buffer(i2c, dev_addr,
								 reg_addr, reg_addr_n_bytes,
								 val_buffer, val_out_n_bytes);

	if (ret < 0)
		return -1;
	val = buffer_to_u32_msbf(val_buffer, val_out_n_bytes);

	if (val_out)
		*val_out = val;

	return 0;
}

int i2c_if_write_reg(struct i2c_if *i2c, uint8_t dev_addr,
					uint32_t reg_addr, uint32_t reg_addr_n_bytes,
					uint32_t val, uint32_t val_n_bytes)
{
	uint8_t val_buffer[4];
	int ret;

	load_buffer_msbf(val_buffer, val, val_n_bytes);
	ret = i2c_if_write_reg_buffer(i2c, dev_addr,
								 reg_addr, reg_addr_n_bytes,
								 val_buffer, val_n_bytes);

	if (ret < 0)
		return -1;

	return 0;
}



/* Check bus by sending out a read address and
 * seeing if there is an ACK.
 */
#if 1
int i2c_if_check(struct i2c_if *i2c, uint8_t addr)
{

	int ret = 0;
	uint8_t dummy;
	uint32_t start;

	(void) dummy;

	if (!i2c)
		return -1;
	if (i2c->msgs)
		return -1; /* Busy */

	dummy = I2C_ReceiveData(i2c->interface);
	I2C_ClearFlag(i2c->interface,I2C_FLAG_AF);

	ret = wait_flag(i2c, I2C_FLAG_BUSY, 0);

	if (ret < 0)
			goto fail;

	I2C_GenerateSTART(i2c->interface, 1);

	ret = wait_event(i2c, I2C_EVENT_MASTER_MODE_SELECT);

	if (ret < 0)
			goto fail;

	I2C_Send7bitAddress(i2c->interface,
								addr,
								I2C_Direction_Transmitter);

	start = get_tick();

	while (!timed_out(start)) {
		/* Acknowledged by device */
		if (I2C_CheckEvent( i2c->interface, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			I2C_GenerateSTOP( I2C1, ENABLE );
			return 1;
		}
		/* Acknowledge failed */
		if (I2C_GetFlagStatus(i2c->interface,I2C_FLAG_AF)) {
			I2C_GenerateSTOP( I2C1, ENABLE );
			return 0;
		}
	}
fail:
	I2C_GenerateSTOP( I2C1, ENABLE );
	return -1;
}
#else
int i2c_if_check(struct i2c_if *i2c, uint8_t addr)
{
	return i2c_if_read_reg(i2c, addr, 0, 0, 0, 0);
}
#endif

/*
 * i2c_if_scan_bus()
 * Scans the bus and prints what devices are seen.
 */
int i2c_if_scan_bus(struct i2c_if *i2c)
{
	uint8_t i;
	int n = 0;
	int result;

	printf("I2C Bus scan\n");
	for( i = 0x2; i < 0xFE; i+= 2) {
		result = i2c_if_check(i2c, i);
		if (result > 0) {
			printf("Device at 0x%02x\n", i);
			n++;
		}
	}
	printf("%d i2C devices found\n", n);
	return n;
}


