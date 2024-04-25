/*
 * Shift bits out D7 first, D0 last.
 * NB: Timing is very important. This has to be slow enough for
 * the 74HC595 at 3V3.
 *
 * This is written in assembly to get precise timing.

void hc595_out(uint8_t val)
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

 */

#include "hw_map.h"

.globl hc595_out

#define nop5	nop; nop; nop; nop; nop
#define nop10	nop5; nop5

.text


 # a0 = val argument
 # t0 = bit
 # t1 = DATA_PORT
 # t2 = DATA_PIN
 # t3 = SERCLK_PORT
 # t4 = SERCLK_PIN
 # t5 = OUTCLK_PORT
 # t6 = OUTCLK_PIN
 # a1 = 8 to 0 downcounter
 #
 # All these are caller saved, so we are free to trash them.
 #
 # The port registers we access are:
 # 0x10: GPIOx_BSHR used to set bits
 # 0x14: GPIOx_BCR used to clear bits.
hc595_out:
	li		t1, HC595_DATA_PORT
	li		t2, HC595_DATA_PIN
	li		t3, HC595_SERCLK_PORT
	li		t4, HC595_SERCLK_PIN
	li		t5, HC595_OUTCLK_PORT
	li		t6, HC595_OUTCLK_PIN

	li		a1, 8

	sw		t6, 0x14(t5) # GPIO_WriteBit(HC595_OUTCLK_IO, 0);

loop:

	sw		t4, 0x14(t3) # GPIO_WriteBit(HC595_SERCLK_IO, 0);

	andi	t0, a0, 0x80
	bne		t0, zero, set_bit
	sw		t2, 0x14(t1) # GPIO_WriteBit(HC595_DATA_IO, 0);
	j		end_bit_set
set_bit:
	sw		t2, 0x10(t1) # GPIO_WriteBit(HC595_DATA_IO, 1);
end_bit_set:

	slli	a0, a0, 1
	addi 	a1, a1, -1

	nop10
	nop10

	sw		t4, 0x10(t3) # GPIO_WriteBit(HC595_SERCLK_IO, 1);

	nop10

	bne 	a1, zero, loop

	nop10

	sw		t6, 0x10(t5) # GPIO_WriteBit(HC595_OUTCLK_IO, 1);

	ret

.end


