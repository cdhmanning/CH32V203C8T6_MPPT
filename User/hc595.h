/*
 * Logic controlling a 74HC595 serial in, parallel out sift register,
 * Controlled by 3 GPIO pins:
 *
 * Data : Data pin.
 * SerCLK:  Data clocked on rising edge.
 * OutCLK:  Shift register values stored to outputs on rising clock.
 *
 * Assume that all other pins are (nSRCLR, nOE) are wired to prevent outputs.
 */
#ifndef USER_HC595_H_
#define USER_HC595_H_

#include <stdint.h>

void hc595_init(uint8_t val);
void hc595_out(uint8_t val);
void hc595_out_C(uint8_t val);

#endif
