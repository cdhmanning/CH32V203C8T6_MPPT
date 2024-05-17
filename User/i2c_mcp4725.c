#include "i2c_mcp4725.h"
#include <string.h>


int i2c_mcp4725_init(struct i2c_mcp4725 *dev, struct i2c_if *i2c, uint8_t address)
{
	memset(dev, 0, sizeof(*dev));
	dev->i2c = i2c;
	dev->address = address;
	return 0;
}

static int i2c_mcp4725_set_fast(struct i2c_mcp4725 *dev, uint16_t dac_val,
							enum mcp4725_power_down pd)
{
	uint32_t value;
	int ret;

	value = (((uint32_t)pd) & 3) <<  12;
	value |= (dac_val & 0xFFF);

	ret = i2c_if_write_reg(dev->i2c, dev->address, 0, 0, value, 2);
	//ret = i2c_if_write_reg(dev->i2c, dev->address, value >> 8, 1, value, 1);

	return ret;
}

int i2c_mcp4725_set(struct i2c_mcp4725 *dev, uint16_t dac_val,
					enum mcp4725_power_down pd, uint32_t do_store)
{
	uint32_t value;
	int ret;

	if (!do_store)
		return  i2c_mcp4725_set_fast(dev, dac_val, pd);

	value  = 3 << 21; /* C2 = 0, C1 = 1, C0 = 1 */
	value |= (((uint32_t)pd) & 3) <<  17;
	value |= (dac_val & 0xFFF) << 4;

	ret = i2c_if_write_reg(dev->i2c, dev->address, 0, 0, value, 3);

	return ret;
}

int i2c_mcp4725_read(struct i2c_mcp4725 *dev, struct mcp4725_result *result)
{
	int ret;
	uint32_t value;

	if (result) {
		result->dac = 0;
		result->pd = 0;
		result->por = 0;
		result->ready = 0;
	}

	ret  = i2c_if_read_reg(dev->i2c, dev->address, 0, 0, &value, 3);

	if (ret < 0)
		return ret;

	if (result) {
		result->dac = (value >> 4) & 0xfff;
		result->pd  = (value >>17) & 0x03;
		result->por = (value>>22) & 1;
		result->ready = (value >>23) & 1;
	}

	return ret;
}

int i2c_mcp4725_test(struct i2c_if *i2c)
{
	struct i2c_mcp4725 dev;
	int ret;
	int i;
	uint16_t dac;

	ret = i2c_mcp4725_init(&dev, i2c, 0xC0);

	printf("mcp4725 init returned %d\n", ret);

	for(i = 0; i < 100; i++) {
		dac = (i * 500) & 0xFFF;
		ret = i2c_mcp4725_set(&dev, i * 100, MCP4725_PD_NORMAL, 0);
		printf("Set DAC to %d returned %d\n", dac, ret);
	}

	return ret;
}

