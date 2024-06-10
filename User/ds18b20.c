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


#define N_ROM_BITS 64
#define N_ROM_BYTES (N_ROM_BITS/8)


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
	(void)i;
	(void)j;
    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 0);
    delay_ms(2);
    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);

    i = ds18b20_wait_level(1);
    j = ds18b20_wait_level(0);
    k = ds18b20_wait_level(1);

    //printf("Pulse rise %d, high %d, low %d\n", i, j, k);
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


static void ds18b20_receive_bit(uint8_t *bit_out)
{
	int i;
	volatile uint32_t crit_flags;
	uint8_t x;

	/* Send high between bits. */
	for(i = 0; i < 1300; i++) {
		GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
	}

	crit_flags = critical_lock();

	/* Write a low pulse, then return to high and delay a short while */
	for(i = 0; i < 20; i++) {
		GPIO_WriteBit(GPIOA, GPIO_Pin_2, 0);
	}
	for(i = 0; i < 20; i++) {
		GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
	}
	x = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2);
	critical_unlock(crit_flags);

	*bit_out = x ? 1: 0;
}

static void ds18b20_receive_byte(uint8_t *b_out)
{
	int bit_count;
	uint8_t x;
	uint8_t b = 0;

	for(bit_count = 0; bit_count < 8; bit_count++) {

		ds18b20_receive_bit(&x);

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


static int conversion_started = 0;

int ds18b20_wait_conversion(void)
{
	uint32_t start;
	uint32_t conv_ms;
	uint8_t b;

	start = get_tick();

	while((conv_ms = get_tick() - start) < 1000) {
		ds18b20_receive_bit(&b);
		if (b)
			break;
		delay_ms(1);
	}

	conversion_started = 0;

	printf("Conversion delay %lu ms\n", conv_ms);
	return conv_ms;
}


int ds18b20_start_conversion(void)
{
	int ret;

	ret = ds18b20_reset();
	ds18b20_send(0xCC, 8);
	ds18b20_send(0x44, 8);

	conversion_started = 1;

	return ret;
}

int ds18b20_get_temperature_mC(int *temp_mC_out)
{
	int ret;
	uint8_t b0, b1;
	uint16_t temp_u16;
	int temp_mC;

	if (!conversion_started)
		ds18b20_start_conversion();

	ds18b20_wait_conversion();

	ret = ds18b20_reset();
	ds18b20_send(0xCC, 8);
	ds18b20_send(0xBE, 8);

	ds18b20_receive_byte(&b0);
    ds18b20_receive_byte(&b1);

    temp_u16 = b0 | (b1 << 8);

    temp_mC = (int16_t) temp_u16;

    /* Temp is now properly signed and scaled to 16ths of a degree.
     * Convert to milliC by:
     * multiplying by 125 and dividing by 2.
     */

    temp_mC = (temp_mC * 125) / 2;

    //printf("Bytes are %02x %02x, temp is %d mC\n", b0, b1, temp_mC);

	*temp_mC_out = temp_mC;

	ds18b20_start_conversion();

	return ret;
}

int ds18b20_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};


    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    ds18b20_start_conversion();

    return 0;
}



static void print_buffer(uint8_t *bits, int n)
{
	int i;

	for(i = 0; i < n; i++)
		printf(" %02x", bits[i]);
	printf("\n");
}


uint8_t get_bit(uint8_t *rom, int bit_id)
{
	return (rom[bit_id/8] >> (bit_id & 7)) & 1;
}


void set_bit(uint8_t *rom, int bit_id, int bit_val)
{
	uint8_t mask = 1 << (bit_id & 7);

	if (bit_val & 1)
		rom[bit_id/8] |= mask;
	else
		rom[bit_id/8] &= ~mask;
}


int read_rom(void)
{
	uint8_t rom[N_ROM_BYTES];
	int i;

	ds18b20_reset();
	ds18b20_send(0x33, 8);

	for(i = 0; i < N_ROM_BYTES; i++)
		ds18b20_receive_byte(&rom[i]);

	printf("ROM is "); print_buffer(rom, 8);

	return 0;
}

int scan_bus(void)
{
	uint8_t scan_bits[N_ROM_BYTES];
	uint8_t conflict_bits[N_ROM_BYTES];

	uint8_t bit;
	uint8_t comp_bit;
	uint8_t send_bit;
	int scan_pos;
	int i;
	int is_conflict;
	int found = 0;

	scan_pos = 0;

	while(scan_pos >= 0) {
		ds18b20_reset();
		ds18b20_send(0xF0, 8);

		/*
		 * Output bits up to where we start scanning.
		 */
		for(i = 0; i < scan_pos; i++) {
			/*
			 * Dummy reads... we're ignoring them...
			 */
			ds18b20_receive_bit(&bit);
			ds18b20_receive_bit(&comp_bit);
			delay_ms(2);
			ds18b20_send(get_bit(scan_bits, i), 1);
		}

		/*
		 * Do scanning.
		 */
		for(i = scan_pos; i < N_ROM_BITS; i++) {
			/*
			 * Read the bit and its complement.
			 * If there is a conflict then both
			 * bits will be the same.
			 * If there is no conflict then one will be 1 and
			 * the other 0.
			 */
			ds18b20_receive_bit(&bit);
			ds18b20_receive_bit(&comp_bit);

			if (bit == comp_bit) {
				/* Conflict, there are both a 1 and a 0 here.
				 * we will try 0 first and will set up to try 1
				 * later.
				 */
				is_conflict = 1;
				send_bit = 0;
			} else {
				is_conflict = 0;
				send_bit = bit;
			}

			set_bit(scan_bits, i, bit);
			set_bit(conflict_bits, i, is_conflict);
			delay_ms(2);
			ds18b20_send(send_bit, 1);

			//printf("pos %2d bit %u comp %u send %u conflict %u\n", i, bit, comp_bit, send_bit, is_conflict);
		}

		printf("Found     "), print_buffer(scan_bits, 8);
		printf("Conflicts "), print_buffer(conflict_bits, 8);
		found++;
		/*
		 * Scan backwards to find last conflict.
		 */
		scan_pos = -1;

		for(i = N_ROM_BITS - 1; i >= 0; i--) {
			/*
			 * If we find a conflict, then...
			 *   set the bit to 1 (because we already tried 0),
			 *   clear the conflict and
			 *   scan from i + 1.
			 */
			if (get_bit(conflict_bits, i) == 1) {
				set_bit(conflict_bits, i, 0);
				set_bit(scan_bits, i, 1);
				scan_pos = i + 1;
				break;
			}
		}
		/*
		 * When we get here we are set up for the next scan.
		 * If scan_pos is -1, there are no more conflicts and
		 * nothing more to search.
		 */

	}

	printf("Found %d devices\n", found);
	return found;

}

int ds18b20_test(void)
{

	int ret;

	//ret = read_rom();
	ret = scan_bus();
#if 0
	int temp_mC;
	int i;

	printf("Testing DS18B20 temp sensor.\n");

	for (i = 0; i < 10; i++) {

		ret = ds18b20_get_temperature_mC(&temp_mC);

		printf("Get temperature returned %d, temp %d\n", ret, temp_mC);

		delay_ms(1000);
	}
#endif

	return ret;
}
