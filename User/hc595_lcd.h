
#ifndef _USER_HC595_LCD_H_
#define _USER_HC595_LCD_H_

#include <stdint.h>

struct hc595_lcd {
	uint8_t 		  	rows;
	uint8_t				function;
	uint8_t 		  	control;
	uint8_t 		  	mode;
	uint8_t 		  	backlight;
};


void HD44780_Init(struct hc595_lcd *lcd, uint8_t rows);
void HD44780_Clear(struct hc595_lcd *lcd);
void HD44780_Home(struct hc595_lcd *lcd);

void HD44780_SetDisplayVisible(struct hc595_lcd *lcd, int val);
void HD44780_SetCursorVisible(struct hc595_lcd *lcd, int val);
void HD44780_SetBlink(struct hc595_lcd *lcd, int val);

void HD44780_ScrollDisplayLeft(struct hc595_lcd *lcd);
void HD44780_ScrollDisplayRight(struct hc595_lcd *lcd);
void HD44780_PrintLeft(struct hc595_lcd *lcd);
void HD44780_PrintRight(struct hc595_lcd *lcd);
void HD44780_LeftToRight(struct hc595_lcd *lcd);
void HD44780_RightToLeft(struct hc595_lcd *lcd);
void HD44780_ShiftIncrement(struct hc595_lcd *lcd);
void HD44780_ShiftDecrement(struct hc595_lcd *lcd);
void HD44780_AutoScroll(struct hc595_lcd *lcd);
void HD44780_NoAutoScroll(struct hc595_lcd *lcd);

void HD44780_CreateSpecialChar(struct hc595_lcd *lcd, uint8_t, uint8_t[]);
void HD44780_PrintSpecialChar(struct hc595_lcd *lcd, uint8_t);

void HD44780_SetCursor(struct hc595_lcd *lcd, uint8_t, uint8_t);
void HD44780_SetBacklight(struct hc595_lcd *lcd, uint8_t new_val);
void HD44780_LoadCustomCharacter(struct hc595_lcd *lcd, uint8_t char_num, uint8_t *rows);
void HD44780_PrintStr(struct hc595_lcd *lcd, const char[]);

void lcd_test(struct hc595_lcd *lcd);

#endif

