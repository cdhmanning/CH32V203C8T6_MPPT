

#include <i2c_at24c256.h>
#include "delay_ms.h"

#include <string.h>


int i2c_at24c256_init(struct i2c_at24c256 *dev, struct i2c_if *i2c, uint8_t address)
{
	memset(dev, 0, sizeof(*dev));
	dev->i2c = i2c;
	dev->address = address;

	return 0;
}

#define PAGE_SIZE 64
int i2c_at24c256_write_buffer(struct i2c_at24c256 *dev, uint32_t address,
							uint8_t *buffer, int n_bytes)
{
	int this_write;
	int left;
	int ret = 0;

	/* Write up to PAGE_SIZE at a time, PAGE_SIZE aligned.
	 * The first and last pages written might be partial pages.
	 */
	//printf("Total write is %x bytes at %04lx\n", n_bytes, (uint32_t) address);
	while (n_bytes > 0) {

		this_write = PAGE_SIZE - (address & (PAGE_SIZE-1));
		if (this_write > n_bytes)
			this_write = n_bytes;
		left = n_bytes - this_write;
		//printf("Write 0x%02x bytes at 0x%04lx, %d left\n", this_write, address, left);
		ret = i2c_if_write_reg_buffer(dev->i2c, dev->address,
									  address, 2,
									  buffer, this_write);
		// TODO: handle ret.
		address += this_write;
		n_bytes = left;
		buffer += this_write;
		delay_ms(20); // TODO: replace with a busy check.
	}

	return ret;
}

int i2c_at24c256_read_buffer(struct i2c_at24c256 *dev, uint32_t address,
							uint8_t *buffer, int n_bytes)
{

	return i2c_if_write_reg_buffer(dev->i2c, dev->address,
									  address, 2,
									  buffer, n_bytes);
}

void dump_buffer(uint8_t *buffer, int size)
{
	while (size > 0) {
		printf(" %02x", *buffer);
		buffer++;
		size--;
	}
}

int i2c_at24c256_test(struct i2c_if *i2c)
{
	struct i2c_at24c256 dev;
	int ret;
	int i;
	uint32_t addr;
	uint8_t wb[64];
	uint8_t rb[64];


	ret = i2c_at24c256_init(&dev, i2c, 0xa0);


	printf("24c256 init returned %d\n", ret);

	for(i = 0; i < 16; i++)
		wb[i] = 0x11 * i + 1;

	addr = 30*1024 + 128 + 64;

	for(i = 0; i < 10; i++) {
		ret = i2c_if_read_reg_buffer(dev.i2c, dev.address, addr,2, rb, 16);

		printf("24c256 read ret %d, ", ret);
		dump_buffer(rb, 16);
		printf("\n");

		ret = i2c_if_write_reg_buffer(dev.i2c, dev.address, addr,2, wb, 16);

		printf("24c256 write ret %d\n", ret);

		delay_ms(20);

		ret = i2c_if_read_reg_buffer(dev.i2c, dev.address, addr,2, rb, 16);

		printf("24c256 read ret %d, ", ret);
		dump_buffer(rb, 16);
		printf("\n");

	}

	for(i = 0; i < 1024; i++) {
		printf("%04x:", i * 32);
		ret = i2c_if_read_reg_buffer(dev.i2c, dev.address, i * 32,2, rb, 32);
		dump_buffer(rb, 32);
		printf("\n");
	}

	printf("Writing 4000 bytes\n");
	i2c_at24c256_write_buffer(&dev, 1000, (uint8_t *)i2c_at24c256_test, 4000);

	printf("Test done\n");

	return ret;
}
