/*
 * This code originally from github.com/eziya/STM32_HAL_I2C_HD44780
 * However it has been modified a bit...
 * Most importantly it has been modified to use an 74HC595
 * rather than I2C.
 */

#include "hd44780.h"
#include "delay_ms.h"
#include <stdio.h>

/*
 * Shift register field usage:
 * Data bits are D7..D4.
 * Backlight bit: D3
 * Enable bit:  D2
 * Register Select bit: D0
 *
 * D1: Unused
 *
 */
#define LCD_BACKLIGHT (1<<3)
#define ENABLE (1<<2)
#define RS (1<<0)


/* Command */
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

/* Entry Mode */
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

/* Display On/Off */
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

/* Cursor Shift */
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

/* Function Set */
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

static void SendCommand(struct hd44780 *lcd, uint8_t);
static void SendChar(struct hd44780 *lcd, uint8_t);
static void Send(struct hd44780 *lcd, uint8_t, uint8_t);
static void Write4Bits(struct hd44780 *lcd, uint8_t);
static void ExpanderWrite(struct hd44780 *lcd, uint8_t);
static void PulseEnable(struct hd44780 *lcd, uint8_t);


static uint8_t special0[8] = {
        0b00000,
        0b11001,
        0b11011,
        0b00110,
        0b01100,
        0b11011,
        0b10011,
        0b00000
};

static uint8_t special1[8] = {
        0b11000,
        0b11000,
        0b00110,
        0b01001,
        0b01000,
        0b01001,
        0b00110,
        0b00000
};

struct special_character_def {
	uint8_t character_id;
	uint8_t *bitmap;
};

struct special_character_def special_list[] = {
		{.character_id = 0, .bitmap = special0},
		{.character_id = 1, .bitmap = special1},
};

static const int n_special_char = sizeof(special_list)/sizeof(special_list[0]);

static void lcd_delay(uint32_t ms)
{
	delay_ms(ms);
}

void create_special_characters(struct hd44780 *lcd)
{
	int i;

	for (i = 0; i < n_special_char; i++)
		hd44780_CreateSpecialChar(lcd, special_list[i].character_id, special_list[i].bitmap);
}

void hd44780_init(struct hd44780 *lcd, struct hd44780_interface *interface, uint8_t rows)
{
	lcd->interface = interface;

	lcd->rows = rows;

	lcd->backlight = 0;

	lcd->function = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

	if (lcd->rows > 1)
	{
	lcd->function |= LCD_2LINE;
	}
	else
	{
	lcd->function |= LCD_5x10DOTS;
	}

	/* Should wait 50msec from power up. */
	lcd_delay(50);

	/* 4bit Mode.  3x 0x03, then 0x02.
	 * Some delays are needed here.
	 */
	Write4Bits(lcd, 0x03 << 4); lcd_delay(5);
	Write4Bits(lcd, 0x03 << 4); lcd_delay(2);
	Write4Bits(lcd, 0x03 << 4); lcd_delay(2);
	Write4Bits(lcd, 0x02 << 4); lcd_delay(2);

	/* Display Control */
	SendCommand(lcd, LCD_FUNCTIONSET | lcd->function);

	lcd->control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	hd44780_SetDisplayVisible(lcd, 1);
	hd44780_Clear(lcd);

	/* Display Mode */
	lcd->mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
	lcd_delay(5);

	//create_special_characters(lcd);

	hd44780_Home(lcd);
}

void hd44780_Clear(struct hd44780 *lcd)
{
  SendCommand(lcd, LCD_CLEARDISPLAY);
  lcd_delay(2);
}

void hd44780_Home(struct hd44780 *lcd)
{
  SendCommand(lcd, LCD_RETURNHOME);
  lcd_delay(3);
}

void hd44780_SetCursor(struct hd44780 *lcd, uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if (row >= lcd->rows)
    row = lcd->rows-1;
  SendCommand(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

static void hd44780_SetControl(struct hd44780 *lcd, uint8_t ctl_bit, int val)
{
	if (val)
		lcd->control |= ctl_bit;
	else
		lcd->control &= ~ctl_bit;

	SendCommand(lcd, LCD_DISPLAYCONTROL | lcd->control);

}

void hd44780_SetDisplayVisible(struct hd44780 *lcd, int val)
{
	hd44780_SetControl(lcd, LCD_DISPLAYON, val);
}

void hd44780_SetCursorVisible(struct hd44780 *lcd, int val)
{
	hd44780_SetControl(lcd, LCD_CURSORON, val);
}

void hd44780_SetBlink(struct hd44780 *lcd, int val)
{
	hd44780_SetControl(lcd, LCD_BLINKON, val);
}

void hd44780_ScrollDisplayLeft(struct hd44780 *lcd)
{
  SendCommand(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void hd44780_ScrollDisplayRight(struct hd44780 *lcd)
{
  SendCommand(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void hd44780_LeftToRight(struct hd44780 *lcd)
{
  lcd->mode |= LCD_ENTRYLEFT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void hd44780_RightToLeft(struct hd44780 *lcd)
{
  lcd->mode &= ~LCD_ENTRYLEFT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void hd44780_AutoScroll(struct hd44780 *lcd)
{
  lcd->mode |= LCD_ENTRYSHIFTINCREMENT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void hd44780_NoAutoScroll(struct hd44780 *lcd)
{
  lcd->mode &= ~LCD_ENTRYSHIFTINCREMENT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void hd44780_CreateSpecialChar(struct hd44780 *lcd, uint8_t location, uint8_t charmap[])
{
  location &= 0x7;
  SendCommand(lcd, LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++)
  {
    SendChar(lcd, charmap[i]);
  }
}

void hd44780_PrintSpecialChar(struct hd44780 *lcd, uint8_t index)
{
  SendChar(lcd, index);
}

void hd44780_LoadCustomCharacter(struct hd44780 *lcd, uint8_t char_num, uint8_t *rows)
{
  hd44780_CreateSpecialChar(lcd, char_num, rows);
}

void hd44780_PrintStr(struct hd44780 *lcd, const char c[])
{
  while(*c) {
	  SendChar(lcd, *c);
	  c++;
  }
}

void hd44780_SetBacklight(struct hd44780 *lcd, uint8_t new_val)
{
  lcd->backlight=new_val ? LCD_BACKLIGHT : 0;
  ExpanderWrite(lcd, 0);
}

static void SendCommand(struct hd44780 *lcd, uint8_t cmd)
{
  Send(lcd, cmd, 0);
}

static void SendChar(struct hd44780 *lcd, uint8_t ch)
{
  Send(lcd, ch, RS);
}

static void Send(struct hd44780 *lcd, uint8_t value, uint8_t mode)
{
  uint8_t highnib = value & 0xF0;
  uint8_t lownib = (value<<4) & 0xF0;
  Write4Bits(lcd, (highnib)|mode);
  Write4Bits(lcd, (lownib)|mode);
}

static void Write4Bits(struct hd44780 *lcd, uint8_t value)
{
  //ExpanderWrite(lcd, value);
  PulseEnable(lcd, value);
}

static void ExpanderWrite(struct hd44780 *lcd, uint8_t _data)
{
  uint8_t data = _data | lcd->backlight;

  lcd->interface->output(lcd->interface, data);
}

static void PulseEnable(struct hd44780 *lcd, uint8_t _data)
{
  ExpanderWrite(lcd, _data | ENABLE);
  ExpanderWrite(lcd, _data & ~ENABLE);
}


void hd44780_test(struct hd44780 *lcd)
{
	uint32_t start;

	hd44780_SetBacklight(lcd, 1);
	/* Clear buffer */
	hd44780_Clear(lcd);

	/* Hide characters */
	hd44780_SetDisplayVisible(lcd, 0);
	hd44780_SetCursorVisible(lcd, 1);
	hd44780_SetCursor(lcd, 0,0);
	hd44780_PrintStr(lcd, "HELLO Micro!!!");
	hd44780_PrintSpecialChar(lcd, 0);

	/* Show characters */
	hd44780_SetDisplayVisible(lcd, 1);

	/* Move position */
	hd44780_SetCursor(lcd, 0, 1);
	hd44780_PrintStr(lcd, "Line 1");
	hd44780_PrintSpecialChar(lcd, 1);
	hd44780_SetCursor(lcd, 0, 2);
	hd44780_PrintStr(lcd, "Line 2");

	hd44780_SetCursor(lcd, 0, 3);
	hd44780_PrintStr(lcd, "Line 3-8901234567890");
	/* Blink cursor */
	hd44780_SetBlink(lcd,1);
	hd44780_SetBlink(lcd, 0);
	hd44780_SetCursorVisible(lcd, 1);
	hd44780_SetCursorVisible(lcd, 0);
	hd44780_SetBacklight(lcd,0);
	hd44780_SetBacklight(lcd, 1);

	lcd_delay(2000);
	hd44780_Clear(lcd);
	lcd_delay(2000);

	start = get_tick();
	hd44780_Clear(lcd);
	hd44780_SetCursor(lcd, 0, 0);
	hd44780_PrintStr(lcd, "Line 0-Hello on top!");
	hd44780_SetCursor(lcd, 0, 1);
	hd44780_PrintStr(lcd, "Line 1-ABCDEFGHIJKLM");
	hd44780_SetCursor(lcd, 0, 2);
	hd44780_PrintStr(lcd, "Line 2-NOPQRSTUVWXYZ");
	hd44780_SetCursor(lcd, 0, 3);
	hd44780_PrintStr(lcd, "Line 3-8901234567890");
	printf("Clear and display whole screen took %d msec\n",
			(int)(get_tick() - start));
}


