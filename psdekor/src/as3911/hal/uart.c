/*
 *****************************************************************************
 * Copyright by ams AG                                                       *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************
 */
/*
 *      PROJECT:   AS3911 firmware
 *      $Revision: $
 *      LANGUAGE:  ANSI C
 */

/*! \file
 *
 *  \author Manuel Huber
 *
 *  \brief uart driver implementation for ATXMEGA
 *
 */
/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "platform.h"
#include "uart.h"
#include "board_wrapper.h"
#include "as3911_hw_config.h"
#include <asf.h>

/*
******************************************************************************
* LOCAL MACROS
******************************************************************************
*/


/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

s8 uartInitialize (u32 baudrate, u32* actbaudrate)
{
	usart_rs232_options_t opt = {
		.baudrate = baudrate,
		.charlength = USART_CHSIZE_8BIT_gc,
		.paritytype = USART_PMODE_DISABLED_gc,
		.stopbits = false /* only one stop bit */
	};

	sysclk_enable_peripheral_clock (AS3911_HAL_DEBUG_UART);

	/* Configure IO pins. */
	boardPeripheralPinInitialize (BOARD_PERIPHERAL_UART1);

	/* NOTE: actual baud rate won't be calculated (It was not required for
	 *       the application). The parameter has been kept to not have to
	 *       modify the generic part of the driver but it could be removed
	 *       easily
	 */

	if (usart_init_rs232 (AS3911_HAL_DEBUG_UART, &opt))
		return ERR_NONE;
	else
		return ERR_PARAM;
}

s8 uartDeinitialize (void)
{
	boardPeripheralPinInitialize (BOARD_PERIPHERAL_UART1);

	/* NOTE: The stack doesn't use this function. It's not clear
	 *       what this function should exactly do - but since it's
	 *       not required by the application nor by the stack it has NOT
	 *       been investigated deeply. If this function is required (and it
	 *       has been defined what it should do) there may be some change
	 *       necessary.
	 */
	usart_tx_disable (AS3911_HAL_DEBUG_UART);
	usart_rx_disable (AS3911_HAL_DEBUG_UART);

	return ERR_NONE;
}

s8 uartTxByte (u8 dat)
{
	return uartTxNBytes (&dat, 1);
}

s8 uartTxNBytes (const u8* buffer, u16 length)
{
	while (length--)
	{
		char c = *buffer;
		++buffer;

		usart_putchar (AS3911_HAL_DEBUG_UART, c);

		/* Wait until transmit shift register is empty
		 *  - usart_putchar does that already */
	}

	return ERR_NONE;
}
