
#ifndef _USER_HD44780_LCD_H_
#define _USER_HD44780_LCD_H_

#include <stdint.h>
#include "hd44780_interface.h"

struct hd44780 {
	struct hd44780_interface *interface;
	uint8_t 		  	rows;
	uint8_t				function;
	uint8_t 		  	control;
	uint8_t 		  	mode;
	uint8_t 		  	backlight;
};


void hd44780_init(struct hd44780 *lcd, struct hd44780_interface *interface, uint8_t rows);
void hd44780_Clear(struct hd44780 *lcd);
void hd44780_Home(struct hd44780 *lcd);

void hd44780_SetDisplayVisible(struct hd44780 *lcd, int val);
void hd44780_SetCursorVisible(struct hd44780 *lcd, int val);
void hd44780_SetBlink(struct hd44780 *lcd, int val);

void hd44780_ScrollDisplayLeft(struct hd44780 *lcd);
void hd44780_ScrollDisplayRight(struct hd44780 *lcd);
void hd44780_PrintLeft(struct hd44780 *lcd);
void hd44780_PrintRight(struct hd44780 *lcd);
void hd44780_LeftToRight(struct hd44780 *lcd);
void hd44780_RightToLeft(struct hd44780 *lcd);
void hd44780_ShiftIncrement(struct hd44780 *lcd);
void hd44780_ShiftDecrement(struct hd44780 *lcd);
void hd44780_AutoScroll(struct hd44780 *lcd);
void hd44780_NoAutoScroll(struct hd44780 *lcd);

void hd44780_CreateSpecialChar(struct hd44780 *lcd, uint8_t, uint8_t[]);
void hd44780_PrintSpecialChar(struct hd44780 *lcd, uint8_t);

void hd44780_SetCursor(struct hd44780 *lcd, uint8_t, uint8_t);
void hd44780_SetBacklight(struct hd44780 *lcd, uint8_t new_val);
void hd44780_LoadCustomCharacter(struct hd44780 *lcd, uint8_t char_num, uint8_t *rows);
void hd44780_PrintStr(struct hd44780 *lcd, const char[]);

void hd44780_test(struct hd44780 *lcd);

#endif

