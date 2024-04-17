/*
 * main.h
 *
 *  Created on: 6/04/2024
 *      Author: charles
 */

#ifndef USER_MAIN_H_
#define USER_MAIN_H_

void systick_hook(void);

uint32_t get_tick(void);
void delay_ms(int n_ms);

#endif /* USER_MAIN_H_ */
