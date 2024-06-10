

#include "i2c_device/lm75.h"
#include <string.h>
#include <stdio.h>


int lm75_init(struct lm75 *lm75, struct i2c_bus *i2c, uint8_t address)
{
	memset(lm75, 0, sizeof(*lm75));
	lm75->i2c = i2c;
	lm75->address = address;
#if 0
	lm75->msg[0].dest_addr7 = lm75->address;
	lm75->msg[0].buffer = lm75->register_buffer;
	lm75->msg[0].buffer_length = 1;
	lm75->msg[0].flags = I2C_MSG_FLAG_WRITE | I2C_MSG_FLAG_NO_STOP;

	lm75->msg[1].dest_addr7 = lm75->address;
	lm75->msg[1].buffer = lm75->measurement_buffer;
	lm75->msg[1].buffer_length = 2;
	lm75->msg[1].flags = I2C_MSG_FLAG_READ;
#endif
	return 0;
}


#if 0
int lm75_read(struct lm75 *lm75, int *out_val_mdeg)
{
	int ret;
	uint16_t utemp;
	int temp;

	if (out_val_mdeg)
		*out_val_mdeg = -1;
	/* If the address has been set, then no need to send it again,
	 * just do the read.
	 */
	if (!lm75->reg_selected)
		ret = i2c_if_transact(lm75->i2c, lm75->msg, 2);
	else
		ret = i2c_if_transact(lm75->i2c, &lm75->msg[1], 1);

	if (ret == 2)
		lm75->reg_selected = 1;

	if (ret == 0)
		return -1;

	// byte 0 = MSB
	// byte 1 = LSB
	// Least significant 7 bits are discarded.
	// Sign extend.
	// Value is in 0.5 degrees.

	utemp = (lm75->measurement_buffer[0] << 8) | lm75->measurement_buffer[1];
	utemp >>= 7;
	if (utemp & (1<<8))
		utemp |= 0xfe00;

	temp = (int16_t) utemp;
	temp = temp * 500;

	if (out_val_mdeg)
		*out_val_mdeg = temp;

	return 0;
}

#else

int lm75_read(struct lm75 *lm75, int *out_val_mdeg)
{
	int ret;
	uint32_t utemp;
	int temp;

	if (out_val_mdeg)
		*out_val_mdeg = -1;

	ret  = i2c_read_reg(lm75->i2c, lm75->address, 0, 1, &utemp, 2);

	temp = (int16_t) utemp;
	temp >>= 7;
	temp = temp * 500;

	if (out_val_mdeg)
		*out_val_mdeg = temp;


	return ret;
}


#endif


int lm75_test(struct i2c_bus *i2c)
{
	struct lm75 lm75;
	struct lm75 bad;
	int ret;
	int temp_mdeg;
	int i;

	ret = lm75_init(&bad, i2c, 0x98);
	ret = lm75_init(&lm75, i2c, 0x90);

	printf("LM75 init returned %d\n", ret);

	for(i = 0; i < 100; i++) {
		ret = lm75_read(&lm75, &temp_mdeg);
		printf("LM75 read ret %d, %d mdegrees\n", ret, temp_mdeg);
		//ret = lm75_read(&bad, &temp_mdeg);
		//printf("Bad LM75 read ret %d, %d mdegrees\n", ret, temp_mdeg);
	}

	return ret;
}
