
#ifndef USER_DELAY_MS_H_
#define USER_DELAY_MS_H_

#include <stdint.h>

void systick_hook(void);

int32_t get_tick(void);

void delay_ms(int n_ms);
#endif
