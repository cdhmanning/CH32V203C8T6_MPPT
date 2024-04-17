/*
 * Pin map used by MPPT.
 */

#ifndef _USER_HW_MAP_H_
#define _USER_HW_MAP_H_

/*
 * Debug UART. Output only.
 * 115200, 230400, 460800, 921600
 */
#define DEBUG_UART_BAUD	921600
#define DEBUG_UART_PIN  GPIO_Pin_9
#define DEBUG_UART_PORT GPIOA
#define DEBUG_UART_IO   DEBUG_UART_PORT, DEBUG_UART_PIN

/*
 * Debug LED.
 * LED0 matches LED on nanoCH32V203 board.
 */
#define LED0_PIN     GPIO_Pin_15
#define LED0_PORT    GPIOA
#define LED0_IO      LED0_PORT, LED0_PIN

/*
 * PWM High side and low side.
 */
#define PWM_H_PIN     GPIO_Pin_8
#define PWM_H_PORT    GPIOA
#define PWM_L_PIN     GPIO_Pin_13
#define PWM_L_PORT    GPIOB


#endif
