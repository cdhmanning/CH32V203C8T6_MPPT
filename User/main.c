
/*
 * Note: This include must be first.
 */
#include <hd44780.h>
#include <i2c_bus/i2c_if.h>
#include <i2c_bus/i2c_bus_gpio.h>
#include <i2c_device/hd44780_if_i2c.h>
#include "hw_map.h"

#include "critical.h"

#include "i2c_device/at24c256.h"
#include "i2c_device/ina226.h"
#include "i2c_device/lm75.h"
#include "i2c_device/mcp4725.h"
#include "pwm_controller.h"
#include "ds18b20.h"

#include "core_riscv.h"
#include "ch32v20x_gpio.h"
#include "delay_ms.h"

#include "main.h"

#include "debug.h"


static struct hd44780_interface_i2c  __lcd_interface_i2c;
static struct hd44780 __lcd;

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

void IIC_Init(u32 bound, u16 address)
{
	GPIO_InitTypeDef GPIO_InitStructure={0};
	I2C_InitTypeDef I2C_InitTSturcture={0};

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE );
	//GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_I2C1, ENABLE );

	GPIO_InitStructure.GPIO_Pin = I2C1_SCL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(I2C1_SCL_PORT, &GPIO_InitStructure );

	GPIO_InitStructure.GPIO_Pin = I2C1_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(I2C1_SDA_PORT, &GPIO_InitStructure );

	I2C_InitTSturcture.I2C_ClockSpeed = bound;
	I2C_InitTSturcture.I2C_Mode = I2C_Mode_I2C;
	I2C_InitTSturcture.I2C_DutyCycle = I2C_DutyCycle_16_9;
	I2C_InitTSturcture.I2C_OwnAddress1 = address;
	I2C_InitTSturcture.I2C_Ack = I2C_Ack_Enable;
	I2C_InitTSturcture.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init( I2C1, &I2C_InitTSturcture );

	I2C_Cmd( I2C1, ENABLE );
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
    loop_counter++;

	if (loop_counter & 7)
		return;
	printf("%lu\n", loop_counter);
}

struct i2c_if i2c_if1;
struct i2c_bus_gpio i2c_gpio;

int main(void)
{
    peripheral_clock_init();

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    USART_Printf_Init(DEBUG_UART_BAUD);
    gpio_init();
    systick_init();
    printf("\n\n* * *\n\n"
    	   "PWM Test\n"
    	   "Built: " __DATE__  " " __TIME__ "\n"
		   "SystemCoreClock:%lu\n"
		   "Debug UART%d baud:%u\n",
		   SystemCoreClock, DEBUG_UART_ID, DEBUG_UART_BAUD);
    printf( "ChipID:%08lx\r\n", DBGMCU_GetCHIPID() );

    IIC_Init(100000, 0x12);
    i2c_if_init(&i2c_if1, I2C1);

    //i2c_scan_bus(&i2c_if1.bus);
    //ina226_test(&i2c_if1.bus);
    i2c_bus_gpio_init(&i2c_gpio, GPIOA, 1 << 10, GPIOA, 1 << 9);

    i2c_scan_bus(&i2c_gpio.bus);
    //ina226_test(&i2c_gpio.bus);
    //ds18b20_init();
    //ds18b20_test();


    //ina226_test(&i2c_if1);
    //mcp4725_test(&i2c_if1);
    //at24c256_test(&i2c_if1);

    //lm75_test(&i2c_if1);

    hd44780_interface_i2c_init(&__lcd_interface_i2c,
    			&i2c_gpio.bus,
				//&i2c_if1.bus,
    			0x27 <<1);

    hd44780_init(&__lcd, &__lcd_interface_i2c.interface, 4);
    hd44780_test(&__lcd);

    while(1) {
    	delay_ms(500);
    	//hd44780_SetBacklight(&__lcd, 0);
    	delay_ms(500);
    	hd44780_SetBacklight(&__lcd, 1);
    }

   // lcd_test(lcd);



    pwm_init(2048, 0);

    while(1)
    {
        //GPIO_WriteBit(LED0_IO, 0);
    	__WFI();
        //GPIO_WriteBit(LED0_IO, 1);
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
    //GPIO_WriteBit(LED0_IO, 1);
	tick_counter++;
	ms_poll();
}
