/*
 * Access a DS18B20 using a UART inhlaf duplex mode.
 * Here we are using USART2 on pin PA2.
 *
 * For the init we use 9600 baud and for the rest of the comms
 * we use 115200 baud.
 */

#include "ds18b20.h"
#include "delay_ms.h"
#include "ch32v20x.h"
#include "critical.h"
#include <stdint.h>


int ds18b20_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};


    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    return 0;
}


static int ds18b20_wait_level(uint32_t wait_high)
{
	int i;
	uint8_t x;

    for(i = 0; i < 5000; i++) {
    	x = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2);
    	if (wait_high ? x : !x)
    		return i;
    }

    return -1;

}

static int ds18b20_send_reset_pulse(void)
{
	int i;
	int j;
	int k;

    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 0);
    delay_ms(2);
    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);

    i = ds18b20_wait_level(1);
    j = ds18b20_wait_level(0);
    k = ds18b20_wait_level(1);

    printf("Pulse rise %d, high %d, low %d\n", i, j, k);
    if (k < 0)
    	return -1;
    return 0;
}

/*
 * Send up to 32 bits of data.
 * Data is sent LSB first.
 */
static void ds18b20_send(uint32_t data, uint32_t n_bits)
{
	int i;
	volatile uint32_t crit_flags;

	/* Inter bit delay. */
	for(i = 0; i < 20; i++)
	    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);

	while(n_bits > 0) {

		/* Send bit low. */
		i = (data & 1) ? 50 : 500;

		crit_flags = critical_lock();
		while (i > 0) {
		    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 0);
		    i--;
		}
	    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
	    critical_unlock(crit_flags);

		/* Send high for rest of slot. */
		i = (data & 1) ? 700 : 250;

		while (i > 0) {
		    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
		    i--;
		}

		n_bits--;
		data >>= 1;
	}
}


static void ds18b20_receive_byte(uint8_t *b_out)
{
	int i;
	int bit_count;
	volatile uint32_t crit_flags;
	uint8_t x;
	uint8_t b = 0;

	for(bit_count = 0; bit_count < 8; bit_count++) {

		/* Send high between bits. */
		for(i = 0; i < 1200; i++) {
		    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
		}

		crit_flags = critical_lock();

		/* Write a low pulse, then return to high and delay a short while */
		for(i = 0; i < 20; i++) {
		    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 0);
		}
		for(i = 0; i < 100; i++) {
		    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
		}
    	x = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2);
	    critical_unlock(crit_flags);

		b >>= 1;
		if (x)
			b |= 0x80;
	}

	*b_out = b;
}


int ds18b20_reset(void)
{
    int ret;

    ret = ds18b20_send_reset_pulse();

    if (ret >= 0)
    	ret = 0;

    return ret;
}

int ds18b20_get_temperature_mC(int *temp_mC_out)
{
	int ret;
	uint8_t b0, b1;
	uint16_t temp_u16;
	int temp_mC;

	ret = ds18b20_reset();
    delay_ms(2);
	ds18b20_send(0xCC, 8);
    delay_ms(2);
	ds18b20_send(0x44, 8);
    delay_ms(2);
	ret = ds18b20_reset();
    delay_ms(2);
	ds18b20_send(0xCC, 8);
    delay_ms(2);
	ds18b20_send(0xBE, 8);
    delay_ms(2);
    ds18b20_receive_byte(&b0);
    ds18b20_receive_byte(&b1);

    temp_u16 = b0 | (b1 << 8);

    temp_mC = (int16_t) temp_u16;

    /* Temp is now properly signed and scaled to 16ths of a degree.
     * Convert to milliC by:
     * multiplying by 125 and dividing by 2.
     */

    temp_mC = (temp_mC * 125) / 2;

    printf("Bytes are %02x %02x, temp is %d mC\n", b0, b1, temp_mC);

	*temp_mC_out = temp_mC;
	return ret;
}


int ds18b20_test(void){
	int ret;
	int temp_mC;

	ret = ds18b20_get_temperature_mC(&temp_mC);

	printf("Get temperature returned %d, temp %d\n", ret, temp_mC);
	return ret;
}
