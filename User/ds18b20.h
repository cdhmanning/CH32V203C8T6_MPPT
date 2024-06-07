/*
 * ds18b20.h
 *
 *  Created on: 6/06/2024
 *      Author: charles
 */

#ifndef USER_DS18B20_H_
#define USER_DS18B20_H_

#include <stdint.h>

int ds18b20_init(void);
int ds18b20_reset(void);
int ds18b20_get_temperature_mC(int *temp_mC);

int ds18b20_test(void);

#endif /* USER_DS18B20_H_ */
