/**
 * \file
 *
 * \brief User board definition template
 *
 */

 /* This file is intended to contain definitions and configuration details for
 * features and devices that are available on the board, e.g., frequency and
 * startup time for an external crystal, external memory devices, LED and USART
 * pins.
 */

#ifndef USER_BOARD_H
#define USER_BOARD_H

#include <conf_board.h>

// External oscillator settings.
// Uncomment and set correct values if external oscillator is used.

// External oscillator frequency
//#define BOARD_XOSC_HZ          8000000

// External oscillator type.
//!< External clock signal
//#define BOARD_XOSC_TYPE        XOSC_TYPE_EXTERNAL
//!< 32.768 kHz resonator on TOSC
//#define BOARD_XOSC_TYPE        XOSC_TYPE_32KHZ
//!< 0.4 to 16 MHz resonator on XTALS
//#define BOARD_XOSC_TYPE        XOSC_TYPE_XTAL

// External oscillator startup time
//#define BOARD_XOSC_STARTUP_US  500000

#ifdef CONF_ENABLE_DBG_UART
#  define UART_TX_PIN IOPORT_CREATE_PIN(PORTD, 3)
#  define UART_RX_PIN IOPORT_CREATE_PIN(PORTD, 2)
#endif

#define SPEAKER IOPORT_CREATE_PIN(PORTD, 4)


#define MOTOR_PIN_bp     0
#define MOTOR_SLAVE_bp   1
#define MOTOR_PIN_bm    (1 << MOTOR_PIN_bp)
#define MOTOR_SLAVE_bm  (1 << MOTOR_SLAVE_bp)
#define MOTOR_PIN IOPORT_CREATE_PIN(PORTE, MOTOR_PIN_bp)
#define MOTOR_SLAVE IOPORT_CREATE_PIN(PORTE, MOTOR_SLAVE_bp)

#define MOTOR_gc 0x03
#define MOTOR_PORT PORTE

#define LEARN_INTR_PIN_bp 2
#define LEARN_INTR_PIN_bm (1 << LEARN_INTR_PIN_bp)
#define LEARN_INTR_PORT PORTB

#define SW_LEARN IOPORT_CREATE_PIN(PORTB, LEARN_INTR_PIN_bp)

#define DOOR_CLOSE_INTR_PIN_bp 2
#define DOOR_CLOSE_INTR_PIN_bm (1 << DOOR_CLOSE_INTR_PIN_bp)
#define DOOR_CLOSE_INTR_PORT PORTA
#define SW_DOOR_CLOSED IOPORT_CREATE_PIN(PORTA, DOOR_CLOSE_INTR_PIN_bp)

#define DOOR_OPEN_INTR_PIN_bp 2
#define DOOR_OPEN_INTR_PIN_bm (1 << DOOR_OPEN_INTR_PIN_bp)
#define DOOR_OPEN_INTR_PORT PORTE
#define SW_DOOR_OPENED IOPORT_CREATE_PIN(PORTE, DOOR_OPEN_INTR_PIN_bp)

#define SW_LOCK_CLOSED IOPORT_CREATE_PIN(PORTA, 3)

#define AS3911_INTR_PIN_bp 2
#define AS3911_INTR_PIN_bm (1 << AS3911_INTR_PIN_bp)
#define AS3911_INTR_PORT   PORTC

#define AS3911_INTR_PIN  IOPORT_CREATE_PIN(PORTC, AS3911_INTR_PIN_bp)
#define AS3911_SEN_PIN   IOPORT_CREATE_PIN(PORTC, 3)
#define AS3911_MOSI_PIN  IOPORT_CREATE_PIN(PORTC, 5)
#define AS3911_MISO_PIN  IOPORT_CREATE_PIN(PORTC, 6)
#define AS3911_SCLK_PIN  IOPORT_CREATE_PIN(PORTC, 7)

#define SPIC_SS_PIN IOPORT_CREATE_PIN(PORTC, 4)

#endif // USER_BOARD_H
