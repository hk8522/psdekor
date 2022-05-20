/*
 * as3911_hw_config.h
 *
 * Created: 09.01.2014 15:13:44
 *  Author: huber
 */

/*! \file
 *
 *  \author Manuel Huber
 *
 *  \brief Hardware configuration for as3911
 *
 */

#ifndef AS3911_HW_CONFIG_H_
#define AS3911_HW_CONFIG_H_


/*! \brief Configured UART unit to be used by uart.h */
#define AS3911_HAL_DEBUG_UART           (&USARTD0)

/*! 
 * \brief Timer module which will be used for delay module
 *        (delay_wrapper.c)
 */
#define AS3911_DELAY_TIMER_MODULE        (&TCC0)

/*! \brief delayIsr (used in delay_wrapper.c) */
#define delayIsr TCC0_OVF_vect

/*! \brief as3911Isr (used in as3911_interrupt.h) */
#define as3911Isr PORTC_INT0_vect


/*!
 * \brief Defines the number of scan attempts in a row before
 *        unit1StartScan() stops scanning (defined in Rfid.h).
 */
#define RFID_UNIT1_MAX_SCAN_ATTEMPTS          20
/*! \brief baud rate of SPI bus (in Hertz) used in Rfid.h */
#define RFID_UNIT1_SPI_BAUDRATE          2000000
/*! \brief spi device instance to use in Rfid.h */
#define RFID_UNIT1_SPI_DEVICE              &SPIC


#endif /* AS3911_HW_CONFIG_H_ */