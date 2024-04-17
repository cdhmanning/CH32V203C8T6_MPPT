
/*
 * Note: THis include must be first.
 */
#include "ch32v20x.h"

#include "pwm_controller.h"

#include "hw_map.h"

#include "core_riscv.h"
#include "ch32v20x_gpio.h"

#include "main.h"


#include "debug.h"


void gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = LED_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &GPIO_InitStructure);
}


void systick_init(void)
{
    //SetVTFIRQ((u32)SysTick_Handler,SysTicK_IRQn,0,ENABLE);
    SysTick->CTLR=0x1;
    SysTick->SR=0;
    SysTick->CNT=0;
    SysTick->CMP= SystemCoreClock/1000 - 1;
    SysTick->CTLR= 0x0f;
    NVIC_EnableIRQ(SysTicK_IRQn);
}

int main(void)
{
    uint32_t i = 0;
    uint32_t pwm_val;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    USART_Printf_Init(115200);

    printf("\n\n* * *\n\n"
    	   "PWM Test\n"
    	   "Built: " __DATE__  " " __TIME__ "\n"
		   "SystemCoreClock:%lu\n", SystemCoreClock);

    printf("PWM Test\n");
    gpio_init();
    systick_init();
    pwm_init(2048, 0);

#if 1
    while(1)
    {
        delay_ms(i & 1 ? 20 : 5);
        GPIO_WriteBit(LED_IO, (i & 1) ? 1 : 0);
        pwm_val = ( i & 1) ? (2048 * 9000)/75000 : (2048 * 14500)/15000;
        update_pwm(pwm_val);
        i++;
    }
#else
    while(1)
    {
        __WFI();
        GPIO_WriteBit(LED_IO, (i & 1) ? 1 : 0);
        i++;
    }
#endif
}

static uint32_t tick_counter;

uint32_t get_tick(void)
{
	return tick_counter;
}

void delay_ms(int n_ms)
{
	int  end = (int) get_tick() + n_ms;

	while (end > (int) get_tick()) {
		__WFI();
	}
}

void systick_hook(void)
{
	/*
	 * The load ticks are split in two parts.
	 */
	//load_tick0();

	tick_counter++;


	/* Place any more tick processing here. */

	//load_tick1();
}
