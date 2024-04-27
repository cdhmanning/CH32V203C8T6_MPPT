/*
 * This code originally from github.com/eziya/STM32_HAL_I2C_HD44780
 * However it has been modified a bit...
 * Most importantly it has been modified to use an 74HC595
 * rather than I2C.
 */

#include "hc595_lcd.h"
#include "hc595.h"
#include "delay_ms.h"

/*
 * Shift register field usage:
 * Data bits are D7..D4.
 * Backlight bit: D3
 * Enable bit:  D2
 * Register Select bit: D1
 *
 * D0: Unused
 *
 */
/* Backlight : D3*/
#define LCD_BACKLIGHT (1<<3)
#define ENABLE (1<<2)
#define RS (1<<1)


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




static void SendCommand(struct hc595_lcd *lcd, uint8_t);
static void SendChar(struct hc595_lcd *lcd, uint8_t);
static void Send(struct hc595_lcd *lcd, uint8_t, uint8_t);
static void Write4Bits(struct hc595_lcd *lcd, uint8_t);
static void ExpanderWrite(struct hc595_lcd *lcd, uint8_t);
static void PulseEnable(struct hc595_lcd *lcd, uint8_t);


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


static void create_special_characters(struct hc595_lcd *lcd)
{
	int i;

	for (i = 0; i < n_special_char; i++)
		HD44780_CreateSpecialChar(lcd, special_list[i].character_id, special_list[i].bitmap);
}

void HD44780_Init(struct hc595_lcd *lcd, uint8_t rows)
{
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
  hc595_init(0);
  delay_ms(10);

  ExpanderWrite(lcd, lcd->backlight);
  delay_ms(1);

  /* 4bit Mode.  3x 0x03, the 0x02*/
  Write4Bits(lcd, 0x03 << 4); delay_ms(5);
  Write4Bits(lcd, 0x03 << 4); delay_ms(5);
  Write4Bits(lcd, 0x03 << 4); delay_ms(5);
  Write4Bits(lcd, 0x02 << 4); delay_ms(5);

  /* Display Control */
  SendCommand(lcd, LCD_FUNCTIONSET | lcd->function);

  lcd->control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  HD44780_SetDisplayVisible(lcd, 1);
  HD44780_Clear(lcd);

  /* Display Mode */
  lcd->mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
  delay_ms(5);

  create_special_characters(lcd);

  HD44780_Home(lcd);
}

void HD44780_Clear(struct hc595_lcd *lcd)
{
  SendCommand(lcd, LCD_CLEARDISPLAY);
  delay_ms(2);
}

void HD44780_Home(struct hc595_lcd *lcd)
{
  SendCommand(lcd, LCD_RETURNHOME);
  delay_ms(2);
}

void HD44780_SetCursor(struct hc595_lcd *lcd, uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if (row >= lcd->rows)
    row = lcd->rows-1;
  SendCommand(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

static void HD44780_SetControl(struct hc595_lcd *lcd, uint8_t ctl_bit, int val)
{
	if (val)
		lcd->control |= ctl_bit;
	else
		lcd->control &= ~ctl_bit;

	SendCommand(lcd, LCD_DISPLAYCONTROL | lcd->control);

}

void HD44780_SetDisplayVisible(struct hc595_lcd *lcd, int val)
{
	HD44780_SetControl(lcd, LCD_DISPLAYON, val);
}

void HD44780_SetCursorVisible(struct hc595_lcd *lcd, int val)
{
	HD44780_SetControl(lcd, LCD_CURSORON, val);
}

void HD44780_SetBlink(struct hc595_lcd *lcd, int val)
{
	HD44780_SetControl(lcd, LCD_BLINKON, val);
}

void HD44780_ScrollDisplayLeft(struct hc595_lcd *lcd)
{
  SendCommand(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void HD44780_ScrollDisplayRight(struct hc595_lcd *lcd)
{
  SendCommand(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void HD44780_LeftToRight(struct hc595_lcd *lcd)
{
  lcd->mode |= LCD_ENTRYLEFT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void HD44780_RightToLeft(struct hc595_lcd *lcd)
{
  lcd->mode &= ~LCD_ENTRYLEFT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void HD44780_AutoScroll(struct hc595_lcd *lcd)
{
  lcd->mode |= LCD_ENTRYSHIFTINCREMENT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void HD44780_NoAutoScroll(struct hc595_lcd *lcd)
{
  lcd->mode &= ~LCD_ENTRYSHIFTINCREMENT;
  SendCommand(lcd, LCD_ENTRYMODESET | lcd->mode);
}

void HD44780_CreateSpecialChar(struct hc595_lcd *lcd, uint8_t location, uint8_t charmap[])
{
  location &= 0x7;
  SendCommand(lcd, LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++)
  {
    SendChar(lcd, charmap[i]);
  }
}

void HD44780_PrintSpecialChar(struct hc595_lcd *lcd, uint8_t index)
{
  SendChar(lcd, index);
}

void HD44780_LoadCustomCharacter(struct hc595_lcd *lcd, uint8_t char_num, uint8_t *rows)
{
  HD44780_CreateSpecialChar(lcd, char_num, rows);
}

void HD44780_PrintStr(struct hc595_lcd *lcd, const char c[])
{
  while(*c) {
	  SendChar(lcd, *c);
	  c++;
  }
}

void HD44780_SetBacklight(struct hc595_lcd *lcd, uint8_t new_val)
{
  lcd->backlight=new_val ? LCD_BACKLIGHT : 0;
  ExpanderWrite(lcd, 0);
}

static void SendCommand(struct hc595_lcd *lcd, uint8_t cmd)
{
  Send(lcd, cmd, 0);
}

static void SendChar(struct hc595_lcd *lcd, uint8_t ch)
{
  Send(lcd, ch, RS);
}

static void Send(struct hc595_lcd *lcd, uint8_t value, uint8_t mode)
{
  uint8_t highnib = value & 0xF0;
  uint8_t lownib = (value<<4) & 0xF0;
  Write4Bits(lcd, (highnib)|mode);
  Write4Bits(lcd, (lownib)|mode);
}

static void Write4Bits(struct hc595_lcd *lcd, uint8_t value)
{
  //ExpanderWrite(lcd, value);
  PulseEnable(lcd, value);
}

static void ExpanderWrite(struct hc595_lcd *lcd, uint8_t _data)
{
  uint8_t data = _data | lcd->backlight;

  hc595_out(data);
}

static void PulseEnable(struct hc595_lcd *lcd, uint8_t _data)
{
  ExpanderWrite(lcd, _data | ENABLE);
  ExpanderWrite(lcd, _data & ~ENABLE);
}


void lcd_test(struct hc595_lcd *lcd)
{
	 /* Clear buffer */
	  HD44780_Clear(lcd);

	  /* Hide characters */
	  HD44780_SetDisplayVisible(lcd, 0);
	  HD44780_SetCursorVisible(lcd, 1);
	  HD44780_SetCursor(lcd, 0,0);
	  HD44780_PrintStr(lcd, "HELLO STM32!!!");
	  HD44780_PrintSpecialChar(lcd, 0);

	  /* Show characters */
	  HD44780_SetDisplayVisible(lcd, 1);

	  /* Move position */
	  HD44780_SetCursor(lcd, 0, 1);
	  HD44780_PrintStr(lcd, "BYE STM32!!!");
	  HD44780_PrintSpecialChar(lcd, 1);
	  HD44780_SetCursor(lcd, 0, 2);
	  HD44780_PrintStr(lcd, "Line 3");

	  HD44780_SetCursor(lcd, 0, 3);
	  HD44780_PrintStr(lcd, "Line 4-8901234567890");
	  /* Blink cursor */
	  HD44780_SetBlink(lcd,1);
	  HD44780_SetBlink(lcd, 0);
	  HD44780_SetCursorVisible(lcd, 1);
	  HD44780_SetCursorVisible(lcd, 0);
	  HD44780_SetBacklight(lcd,0);
	  HD44780_SetBacklight(lcd, 1);

	  delay_ms(20);
	  HD44780_SetCursor(lcd, 0, 0);
	  HD44780_PrintStr(lcd, "Line 1-Hello on top!");
	  HD44780_SetCursor(lcd, 0, 1);
	  HD44780_PrintStr(lcd, "Line 2-ABCDEFGHIJKLM");
	  HD44780_SetCursor(lcd, 0, 2);
	  HD44780_PrintStr(lcd, "Line 3-NOPQRSTUVWXYZ");
	  HD44780_SetCursor(lcd, 0, 3);
	  HD44780_PrintStr(lcd, "Line 4-8901234567890");
}


