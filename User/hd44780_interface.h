
#ifndef USER_HD44780_INTERFACE_H_
#define USER_HD44780_INTERFACE_H_

#include <stdint.h>

struct hd44780_interface {
	void *private;
	int (*output)(struct hd44780_interface *interface, uint8_t b);
};

#endif
