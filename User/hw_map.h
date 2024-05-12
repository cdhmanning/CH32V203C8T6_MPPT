/*
 * Pin map used by MPPT.
 */

#ifndef _USER_HW_MAP_H_
#define _USER_HW_MAP_H_

#ifdef  __ASSEMBLER__
#define GPIOA	0x40010800
#define GPIOB	0x40010C00
#define GPIOC	0x40011000
#else
#include "ch32v20x.h"
#endif

/*
 * Debug UART. Output only.
 * 115200, 230400, 460800, 921600
 * Faster speeds might be lossy.
 */
#define DEBUG_UART_BAUD	115200
#define DEBUG_UART_PIN  (1<<10)
#define DEBUG_UART_PORT GPIOB
#define DEBUG_UART_IO   DEBUG_UART_PORT, DEBUG_UART_PIN

/*
 * Debug LED.
 * LED0 matches LED on nanoCH32V203 board.
 */
#define LED0_PIN     (1 << 15)
#define LED0_PORT    GPIOA
#define LED0_IO      LED0_PORT, LED0_PIN

/*
 * PWM High side and low side.
 */
#define PWM_H_PIN     (1 << 8)
#define PWM_H_PORT    GPIOA
#define PWM_L_PIN     (1 << 13)
#define PWM_L_PORT    GPIOB

/*
 * 74HC595 for LED etc.
 */

#define HC595_DATA_PIN  (1 << 5)
#define HC595_DATA_PORT GPIOB
#define HC595_DATA_IO   HC595_DATA_PORT, HC595_DATA_PIN

#define HC595_SERCLK_PIN  (1 << 4)
#define HC595_SERCLK_PORT GPIOB
#define HC595_SERCLK_IO   HC595_SERCLK_PORT, HC595_SERCLK_PIN

#define HC595_OUTCLK_PIN  (1 << 3)
#define HC595_OUTCLK_PORT GPIOB
#define HC595_OUTCLK_IO   HC595_OUTCLK_PORT, HC595_OUTCLK_PIN


#endif
