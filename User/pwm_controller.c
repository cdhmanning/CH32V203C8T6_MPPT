

#include "pwm_controller.h"

#include "hw_map.h"

#include "ch32v20x.h"



void pwm_init(uint16_t period, uint16_t prescaler)
{
	GPIO_InitTypeDef GPIO_InitStructure={0};
	TIM_OCInitTypeDef TIM_OCInitStructure={0};
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure={0};
	TIM_BDTRInitTypeDef     TIM_BDTRInitStructure = {0};

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA |
							RCC_APB2Periph_GPIOB |
							RCC_APB2Periph_TIM1, ENABLE );

	GPIO_InitStructure.GPIO_Pin = PWM_H_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init( PWM_H_PORT, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Pin = PWM_L_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init( PWM_L_PORT, &GPIO_InitStructure );

	TIM_TimeBaseInitStructure.TIM_Period = period;
	TIM_TimeBaseInitStructure.TIM_Prescaler = prescaler;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit( TIM1, &TIM_TimeBaseInitStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
	TIM_OC1Init( TIM1, &TIM_OCInitStructure);

	TIM_BDTRInitStructure.TIM_DeadTime = 0x20;
	TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);

	TIM_CtrlPWMOutputs(TIM1, ENABLE );
	TIM_OC1PreloadConfig( TIM1, TIM_OCPreload_Disable );
	TIM_ARRPreloadConfig( TIM1, ENABLE );
	TIM_Cmd( TIM1, ENABLE );
}



int update_pwm(uint32_t pwm_val)
{
    TIM1->CH1CVR = pwm_val;

    return 0;
}
