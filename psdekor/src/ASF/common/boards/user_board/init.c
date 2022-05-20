/**
 * \file
 *
 * \brief User board initialization template
 *
 */

#include <asf.h>
#include <board.h>
#include <conf_board.h>

void board_init(void)
{
	/* This function is meant to contain board-specific initialization code
	 * for, e.g., the I/O pins. The initialization can rely on application-
	 * specific board configuration, found in conf_board.h.
	 */

	/* Set UART PIN directions */
	#ifdef CONF_ENABLE_DBG_UART
	ioport_configure_pin (UART_TX_PIN, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW);
	ioport_configure_pin (UART_RX_PIN, IOPORT_DIR_INPUT);
	#endif

	ioport_configure_pin (SPEAKER, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW | IOPORT_INV_ENABLED);

	ioport_configure_pin (MOTOR_PIN, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW);
	ioport_configure_pin (MOTOR_SLAVE, IOPORT_DIR_OUTPUT | IOPORT_INIT_HIGH);

	ioport_configure_pin (SW_LEARN, IOPORT_DIR_INPUT | IOPORT_PULL_UP);
	ioport_configure_pin (SW_DOOR_CLOSED, IOPORT_DIR_INPUT | IOPORT_PULL_UP);
	ioport_configure_pin (SW_DOOR_OPENED, IOPORT_DIR_INPUT | IOPORT_PULL_UP);
	ioport_configure_pin (SW_LOCK_CLOSED, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW);

	ioport_configure_pin (AS3911_SEN_PIN, IOPORT_DIR_OUTPUT | IOPORT_INIT_HIGH);
	ioport_configure_pin (AS3911_INTR_PIN, IOPORT_DIR_INPUT);
	ioport_configure_pin (AS3911_MISO_PIN, IOPORT_DIR_INPUT);
	ioport_configure_pin (AS3911_MOSI_PIN, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW);
	ioport_configure_pin (AS3911_SCLK_PIN, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW);

	ioport_configure_pin (SPIC_SS_PIN, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW);

	ioport_set_pin_sense_mode (AS3911_INTR_PIN, IOPORT_SENSE_RISING);
	ioport_set_pin_sense_mode(SW_LEARN, IOPORT_SENSE_FALLING);

	arch_ioport_set_pin_sense_mode(SW_DOOR_CLOSED, 0b011); /* LOW LEVEL interrupt */
	arch_ioport_set_pin_sense_mode(SW_DOOR_OPENED, 0b011); /* LOW LEVEL interrupt */
}
