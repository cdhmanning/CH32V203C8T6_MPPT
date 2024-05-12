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


#define timed_out(start) ((get_tick() - (start)) > 5)

static int wait_flag(struct i2c_if *i2c, uint32_t flag, uint32_t value)
{
	int32_t start = get_tick();

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
					struct i2c_transaction *transactions,
					int n_transactions)
{
	int ret = 0;
	int ok_transactions = 0;
	uint8_t dummy;

	(void) dummy;

	if (!i2c)
		return -1;
	if (i2c->transactions)
		return -1; /* Busy */
	if (!transactions)
		return -1;
	if (n_transactions < 1)
		return -1;

	i2c->transactions = transactions;
	i2c->n_transactions = n_transactions;
	i2c->buffer = 0;
	i2c->n_bytes = 0;


	dummy = I2C_ReceiveData(i2c->interface);

	while(i2c->n_transactions > 0) {

		i2c->buffer = i2c->transactions->buffer;
		i2c->n_bytes = i2c->transactions->buffer_length;
		i2c->stop_sent = 0;
		i2c->ext_buffer_loaded = 0;

		got_to |= 1;
		if (!i2c->have_bus)
			ret = wait_flag(i2c, I2C_FLAG_BUSY, 0);

		if (ret < 0)
			goto fail;
		got_to |= 2;
		I2C_GenerateSTART(i2c->interface, 1);

		//delay_ms(5);

		ret = wait_event(i2c, I2C_EVENT_MASTER_MODE_SELECT);

		if (ret < 0)
			goto fail;

		i2c->have_bus = 1;

		if (i2c->transactions->flags & I2C_MSG_FLAG_READ) {
			/* Read mode */
			I2C_Send7bitAddress(i2c->interface,
								i2c->transactions->dest_addr7,
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
			        GPIO_WriteBit(LED0_IO, 1);
					//I2C_AcknowledgeConfig(i2c->interface, DISABLE );
					I2C_GenerateSTOP(i2c->interface, ENABLE );
			        GPIO_WriteBit(LED0_IO, 0);
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
			/* Write mode */
			I2C_Send7bitAddress(i2c->interface,
								i2c->transactions->dest_addr7,
								I2C_Direction_Transmitter);
			//delay_ms(10);

			ret = wait_event(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);

			got_to |= 0x10;

			if (ret < 0)
				goto fail;
			got_to |= 0x20;
			while(i2c->n_bytes > 0) {

				wait_flag(i2c, I2C_FLAG_TXE, 1);
				if (ret < 0)
					goto fail;
				got_to |= 0x40;

				I2C_SendData(i2c->interface, i2c->buffer[0]);

				i2c->n_bytes--;
				i2c->buffer++;

				if(i2c->n_bytes == 0 && !i2c->ext_buffer_loaded) {
					i2c->buffer = i2c->transactions->ext_buffer;
					i2c->n_bytes = i2c->transactions->ext_buffer_length;
					i2c->ext_buffer_loaded = 1;
				}
			}
			ret = wait_event(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED) ;
			if (ret < 0)
				goto fail;
			got_to |= 0x80;

			if(i2c->stop_sent == 0 &&
			  !(i2c->transactions->flags &I2C_MSG_FLAG_NO_STOP)) {
				I2C_GenerateSTOP(i2c->interface, ENABLE );
				i2c->stop_sent = 1;
				i2c->have_bus = 0;
			}
		}

		i2c->transactions++;
		i2c->n_transactions--;
		ok_transactions++;
	}

fail:

	if (!i2c->stop_sent) {
		I2C_GenerateSTOP( I2C1, ENABLE );
		i2c->stop_sent = 1;
	}

	i2c->transactions = NULL;

	return ok_transactions;
}


