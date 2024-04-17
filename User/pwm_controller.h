
#ifndef _USER_PWM_CONTROLLER_H_
#define _USER_PWM_CONTROLLER_H_

#include <stdint.h>

void pwm_init(uint16_t period, uint16_t prescaler);
int update_pwm(uint32_t pwm_val);


#endif
