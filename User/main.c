
/*
 * Note: This include must be first.
 */
#include "hw_map.h"

#include <critical.h>

#include "pwm_controller.h"
#include "hc595.h"
#include "hc595_lcd.h"

#include "core_riscv.h"
#include "ch32v20x_gpio.h"
#include "delay_ms.h"

#include "main.h"

#include "debug.h"


static struct hc595_lcd __lcd;
struct hc595_lcd *lcd = &__lcd;

void peripheral_clock_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
			   	   	   	   RCC_APB2Periph_GPIOB |
						   RCC_APB2Periph_GPIOC |
						   RCC_APB2Periph_TIM1, ENABLE );
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
}

void gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.GPIO_Pin = LED0_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED0_PORT, &GPIO_InitStructure);
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


static uint32_t loop_counter;
static uint32_t tick_counter;

void main_poll(void)
{
	static uint32_t pwm_val;

    pwm_val = ( loop_counter & 1) ? (2048 * 9000)/75000 : (2048 * 14500)/15000;
    update_pwm(pwm_val);
    hc595_out(loop_counter);
    loop_counter++;

	if (loop_counter & 7)
		return;
	printf("%lu\n", loop_counter);
}

int main(void)
{
    peripheral_clock_init();

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    USART_Printf_Init(DEBUG_UART_BAUD);
    gpio_init();
    systick_init();

    hc595_init(0x55);

    HD44780_Init(lcd, 4);

   // lcd_test(lcd);

    printf("\n\n* * *\n\n"
    	   "PWM Test\n"
    	   "Built: " __DATE__  " " __TIME__ "\n"
		   "SystemCoreClock:%lu\n"
		   "Debug UART%d baud:%u\n",
		   SystemCoreClock, DEBUG_UART_ID, DEBUG_UART_BAUD);

    pwm_init(2048, 0);

    while(1)
    {
        GPIO_WriteBit(LED0_IO, 0);
    	__WFI();
        GPIO_WriteBit(LED0_IO, 1);
    	main_poll();
    }
}



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

void ms_poll(void)
{

}

void systick_hook(void)
{
    GPIO_WriteBit(LED0_IO, 1);
	tick_counter++;
	ms_poll();
}
