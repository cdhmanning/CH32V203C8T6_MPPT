/*
 * I2C driver that operates in polled mode with time outs.
 * This implements I2C with 2 GPIOs.
 */

#include <i2c_bus/i2c_bus.h>
#include <i2c_bus/i2c_bus_gpio.h>
#include "hw_map.h"

#include <stddef.h>
#include <string.h>
#include "delay_ms.h"

#define SDA_IO(x)	x->sda_port, x->sda_pin
#define SCL_IO(x)	x->scl_port, x->scl_pin

static void do_delay(void)
{
	volatile int x;

	for (x = 0; x < 500; x++) {
		/* Spin. */
	}
}



static void write_sda(struct i2c_bus_gpio *i2c, uint32_t bit)
{
	GPIO_WriteBit(SDA_IO(i2c), bit);
}

static void write_scl(struct i2c_bus_gpio *i2c, uint32_t bit)
{
	GPIO_WriteBit(SCL_IO(i2c), bit);
}

static int read_bit(struct i2c_bus_gpio *i2c)
{
	int x;

	write_sda(i2c, 1);
	do_delay();
	write_scl(i2c, 1);
	do_delay();
	x = GPIO_ReadInputDataBit(SDA_IO(i2c));
	write_scl(i2c, 0);
	do_delay();

	return x;
}

static void write_bit(struct i2c_bus_gpio *i2c, uint8_t b)
{
	write_sda(i2c, b & 1);
	do_delay();
	write_scl(i2c, 1);
	do_delay();
	write_scl(i2c, 0);
	do_delay();
}

static int write_byte(struct i2c_bus_gpio *i2c, uint8_t b, uint32_t with_ack)
{
	int i;
	int ack_bit;

	for (i = 0; i < 8; i++) {
		write_bit(i2c, (b >> 7) & 1);
		b <<= 1;
	}

	ack_bit = read_bit(i2c);

	if (with_ack) {
		/* Return -1 if the ack bit is high (ie. not acknowledged) */
		if (ack_bit)
			return -1;
	}
	return 0;
}

static uint8_t read_byte(struct i2c_bus_gpio *i2c, uint32_t with_ack)
{
	int i;

	uint8_t b = 0;

	for (i = 0; i < 8; i++) {
		b <<= 1;
		b |= read_bit(i2c);
	}

	write_bit(i2c, with_ack ? 0 : 1);

	return b;
}

static void generate_start(struct i2c_bus_gpio *i2c)
{
	/* Starting in idle (both high),
	 * drive SDA high->low while SCL is high.
	 * then SCL low;
	 * End with both low.
	 */
	write_sda(i2c, 1);
	write_scl(i2c, 1);
	do_delay();
	write_sda(i2c, 0);
	do_delay();
	write_scl(i2c, 0);
}

static void generate_stop(struct i2c_bus_gpio *i2c)
{
	/* Starting with both low
	 * Drive SCL high
	 * drive SDA low->high while SCL is high.
	 * End with both high.
	 */
	write_sda(i2c, 0);
	write_scl(i2c, 0);
	do_delay();
	write_scl(i2c, 1);
	do_delay();
	write_sda(i2c, 1);
}

static int i2c_bus_gpio_transact(struct i2c_bus *i2c_bus,
					struct i2c_msg *msgs,
					int n_msgs)
{
	int ret = 0;
	int ok_transactions = 0;
	uint8_t dummy;
	struct i2c_bus_gpio *i2c = (struct i2c_bus_gpio *)(i2c_bus->private);

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

	while(i2c->n_msgs > 0) {

		i2c->buffer = i2c->msgs->buffer;
		i2c->n_bytes = i2c->msgs->buffer_length;
		i2c->stop_sent = 0;
		i2c->ext_buffer_loaded = 0;

		if (!i2c->have_bus) {
		}

		generate_start(i2c);

		i2c->have_bus = 1;

		if (i2c->msgs->flags & I2C_MSG_FLAG_READ) {
			/*
			 *  Read mode - send address byte ored with 1 for read,
			 * expecting an ack.
			 */
			ret = write_byte(i2c, i2c->msgs->dest_addr7 | 1, 1);

			if (ret < 0)
				goto fail;

			/*
			 * Now read the bytes, sending acks on all bytes.
			 * TODO Maybe we don't send one on the last_byte?
			 */
			while(i2c->n_bytes > 0) {
				i2c->buffer[0] = read_byte(i2c, 1);
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
			/*
			 *  Write mode - send address byte,
			 * expecting an ack.
			 */
			ret = write_byte(i2c, i2c->msgs->dest_addr7, 1);

			if (ret < 0)
				goto fail;

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
					if (ret < 0)
						goto fail;
					ret = write_byte(i2c, i2c->buffer[0], 1);

					i2c->n_bytes--;
					i2c->buffer++;

					/*
					 * If we've done with the first buffer, then set up the
					 * second buffer if there is one.
					 */
					if(i2c->n_bytes == 0 && !i2c->ext_buffer_loaded) {
						i2c->buffer = i2c->msgs->ext_buffer;
						i2c->n_bytes = i2c->msgs->ext_buffer_length;
						i2c->ext_buffer_loaded = 1;
					}
				}
			}

			if (ret < 0)
				goto fail;
		}

		if(!(i2c->msgs->flags &I2C_MSG_FLAG_NO_STOP)) {
			generate_stop(i2c);
			i2c->have_bus = 0;
		}

		i2c->msgs++;
		i2c->n_msgs--;
		ok_transactions++;
	}

fail:

	if (i2c->have_bus) {
		generate_stop(i2c);
		i2c->have_bus = 0;
	}

	i2c->msgs = NULL;

	return ok_transactions;
}


/* Check bus by sending out a read address and
 * seeing if there is an ACK.
 * Returns:
 * 		 1: Device acked
 * 		 0: Device did not ACK.
 * 	    -1: Something else failed.
 */
int i2c_bus_gpio_check(struct i2c_bus *i2c_bus, uint8_t addr)
{
	struct i2c_bus_gpio *i2c = (struct i2c_bus_gpio *)i2c_bus->private;
	int ret = 0;

	if (!i2c)
		return -1;
	if (i2c->msgs)
		return -1; /* Busy */

	generate_start(i2c);

	ret = write_byte(i2c, addr, 1);

	generate_stop(i2c);

	if (ret < 0)
		return 0; /* No ACK. */

	return 1; /* ACKed, device present. */
}

int i2c_bus_gpio_init(struct i2c_bus_gpio *i2c,
		GPIO_TypeDef   *sda_port,
		uint32_t		sda_pin,
		GPIO_TypeDef   *scl_port,
		uint32_t		scl_pin)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};

	memset(i2c, 0, sizeof(*i2c));
	i2c->sda_port = sda_port;
	i2c->sda_pin  = sda_pin;
	i2c->scl_port = scl_port;
	i2c->scl_pin  = scl_pin;
	i2c->bus.private = (void *)i2c;
	i2c->bus.transact_fn = i2c_bus_gpio_transact;
	i2c->bus.check_fn = i2c_bus_gpio_check;

	/* Initialise the GPIO pins. Open Driver. */
    write_sda(i2c, 1);
    write_scl(i2c, 1);

	GPIO_InitStructure.GPIO_Pin = sda_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(sda_port, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = scl_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(scl_port, &GPIO_InitStructure);


	return 0;
}



