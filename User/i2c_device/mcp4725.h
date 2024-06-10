
#ifndef _USER_I2C_MCP4725_H_
#define _USER_I2C_MCP4725_H_

#include "i2c_bus.h"

struct mcp4725 {
	struct i2c_bus *i2c;
	uint8_t address;	/* 7 bit i2c address, << 1 */
#if 0
	uint8_t register_buffer[1];
	uint8_t measurement_buffer[2];
	struct i2c_transaction msg[2];
	int		reg_selected;
#endif
};

enum mcp4725_power_down {
	MCP4725_PD_NORMAL = 0,
	MCP4725_PD_1K = 1,
	MCP4725_PD_100K = 2,
	MCP4725_PD_500K = 3
};

struct mcp4725_result {
	uint16_t dac;
	enum mcp4725_power_down pd;
	uint16_t por;
	uint16_t ready;
};

int mcp4725_init(struct mcp4725 *mcp4725, struct i2c_bus *i2c, uint8_t address);

int mcp4725_set(struct mcp4725 *dev, uint16_t dac_val,
					enum mcp4725_power_down pd, uint32_t do_store);

int mcp4725_read(struct mcp4725 *dev, struct mcp4725_result *result);

int mcp4725_test(struct i2c_bus *i2c);

#endif
