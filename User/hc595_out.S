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

#define GPIO_SET	0x10
#define GPIO_CLEAR	0x14

.globl hc595_out


# Define a short delay macro.
# Note this uses t0
.macro short_delay n
	li		t0, \n
1:	add		t0, t0, -1
	bne		t0, zero, 1b
.endm

.text


 # a0 = val argument
 # t0 = scratch register: bit, delay_counter
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

	sw		t6, GPIO_CLEAR(t5) # GPIO_WriteBit(HC595_OUTCLK_IO, 0);

loop:

	sw		t4, GPIO_CLEAR(t3) # GPIO_WriteBit(HC595_SERCLK_IO, 0);

	andi	t0, a0, 0x80
	bne		t0, zero, set_bit
	sw		t2, 0x14(t1) # GPIO_WriteBit(HC595_DATA_IO, 0);
	j		end_bit_set
set_bit:
	sw		t2, GPIO_SET(t1) # GPIO_WriteBit(HC595_DATA_IO, 1);
end_bit_set:

	slli	a0, a0, 1
	addi 	a1, a1, -1

	short_delay	10

	sw		t4, GPIO_SET(t3) # GPIO_WriteBit(HC595_SERCLK_IO, 1);

	short_delay	5

	bne 	a1, zero, loop

	short_delay	5

	sw		t6, GPIO_SET(t5) # GPIO_WriteBit(HC595_OUTCLK_IO, 1);

	ret

.end


