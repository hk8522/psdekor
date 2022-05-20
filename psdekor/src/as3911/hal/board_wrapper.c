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
 *  \author Christian Eisendle (original PIC implementation)
 *  \author Manuel Huber (AVR port)
 *
 *  \brief Board specific configuration
 *
 *  \note Board will be initialized the Atmel way via board_init ().
 *        Deinitialization is not supported (and doesn't make sense).
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "platform.h"
#include "board_wrapper.h"

/*
******************************************************************************
* LOCAL DEFINES
******************************************************************************
*/

s8 boardInitialize ()
{
	/* NOTE: All initialization is done here (That's the atmel way of doing
	 *       it
	 */
	board_init ();

	return ERR_NONE;
}

s8 boardDeinitialize ()
{
	/* NOTE: Not supported (doesn't make sense; When would you call it)
	 */
	return ERR_NOTSUPP;
}

s8 boardPeripheralPinInitialize (boardPeripheral_t peri)
{
	/* NOTE: Not supported -> see boardInitialize() for more information */

	switch (peri) {

	case BOARD_PERIPHERAL_SPI1:
		return ERR_NOTSUPP;

	case BOARD_PERIPHERAL_AS3911_TRANSPARENT_MODE:
		return ERR_NOTSUPP;

	case BOARD_PERIPHERAL_UART1:
		/* Do nothing; Already done by board.h */
		return ERR_NONE;

	case BOARD_PERIPHERAL_AS3911_INT:
		return ERR_NOTSUPP;

	default:
		return ERR_PARAM;
	}
}

s8 boardPeripheralPinDeinitialize(boardPeripheral_t peri)
{
	/* NOTE: Not supported -> see boardInitialize() for more information */

	switch (peri) {
	case BOARD_PERIPHERAL_AS3911_TRANSPARENT_MODE:
		return ERR_NOTSUPP;

	case BOARD_PERIPHERAL_SPI1:
		return ERR_NOTSUPP;

	case BOARD_PERIPHERAL_UART1:
		return ERR_NOTSUPP;

	case BOARD_PERIPHERAL_AS3911_INT:
		return ERR_NOTSUPP;

	default:
		return ERR_PARAM;
	}
}
