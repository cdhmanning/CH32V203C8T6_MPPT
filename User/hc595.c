/*
 * Logic controlling a 74HC595 serial in, parallel out sift register,
 * Controlled by 3 GPIO pins:
 *
 * Data : Data pin.
 * SerCLK:  Data clocked on rising edge.
 * OutCLK:  Shift register values stored to outputs on rising clock.
 *
 * Assume that all other pins are (nSRCLR, nOE) are wired to prevent outputs.
 *
 * The QA output is the last bit shifted in. This is the the LSB.
 * ie QH = D7, QA = D0
 */

#include "hc595.h"
#include "hw_map.h"
#include "ch32v20x_gpio.h"

static void hc595_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = HC595_DATA_PIN;
    GPIO_Init(HC595_DATA_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = HC595_SERCLK_PIN;
    GPIO_Init(HC595_SERCLK_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = HC595_OUTCLK_PIN;
    GPIO_Init(HC595_OUTCLK_PORT, &GPIO_InitStructure);
}

void hc595_init(uint8_t val)
{
	hc595_gpio_init();
	hc595_out(val);
}

/*
 * Shift bits out D7 first, D0 last.
 * NB: Timing is very important. THis has to be slow enough for
 * the 74HC595 at 3V3.
 */
void hc595_out_C(uint8_t val)
{
	int i;
	uint8_t bit;

	GPIO_WriteBit(HC595_OUTCLK_IO, 0);

	for(i = 0; i < 8; i++){
		bit = (val >> 7) & 1;

		GPIO_WriteBit(HC595_DATA_IO, bit);
		GPIO_WriteBit(HC595_SERCLK_IO, 0);
		GPIO_WriteBit(HC595_DATA_IO, bit);

		GPIO_WriteBit(HC595_SERCLK_IO, 1);
		val <<= 1;
	}
	GPIO_WriteBit(HC595_OUTCLK_IO, 0);
	GPIO_WriteBit(HC595_OUTCLK_IO, 0);
	GPIO_WriteBit(HC595_OUTCLK_IO, 1);
}
