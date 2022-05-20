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
/*!
 * Abstraction of the board configuration, like IO port settings.
 */

#ifndef BOARD_WRAPPER_H
#define BOARD_WRAPPER_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "platform.h"
#include "board_wrapper.h"

/*
******************************************************************************
* GLOBAL DATATYPES
******************************************************************************
*/
/*!
 * list of on-chip peripherals with external interfaces used on the board.
 */
typedef enum {
	/*! spi 1 */
	BOARD_PERIPHERAL_SPI1,
	/*! uart 1 */
	BOARD_PERIPHERAL_UART1,
	/*! interrupt line of as3911 */
	BOARD_PERIPHERAL_AS3911_INT,
	/*! as3911 transparent mode */
	BOARD_PERIPHERAL_AS3911_TRANSPARENT_MODE,
} boardPeripheral_t;

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/
/*!
 *****************************************************************************
 *  \brief  Initialize board configuration module and sets default IO config.
 *
 *  \return ERR_NONE : No error.
 *
 *****************************************************************************
 */
extern s8 boardInitialize (void);

/*!
 *****************************************************************************
 *  \brief  Deinitialize board configuration.
 *  \note This function is not supported!
 *
 *  \return ERR_NOTSUPP
 *
 *****************************************************************************
 */
extern s8 boardDeinitialize (void);

/*!
 *****************************************************************************
 *  \brief  Initialize the peripheral specific pin configuration
 *
 *  This function is called by the particular low level drivers for
 *  hardware blocks with external interfaces. It configures the pin
 *  configuration, i.e. sets up pin multiplexing and so on.
 *
 *  \param[in] peri : Peripheral for which the pins should be configured.
 *
 *  \note This functions doesn't initialize anything since all initialization
 *        has already been done in boardInitialize.
 *
 *  \return ERR_PARAM : Given peripheral \a peri not available.
 *  \return ERR_NONE : No error, configuration set.
 *  \return ERR_NOTSUPP: For peripherals that can not be initialized.
 *
 *****************************************************************************
 */
extern s8 boardPeripheralPinInitialize (boardPeripheral_t peri);

/*!
 *****************************************************************************
 *  \brief  Deinitialize the peripheral specific pin configuration
 *
 *  This function is called by the particular low level drivers for
 *  hardware blocks with external interfaces. It deinitializes the pin
 *  configuration, i.e. resets pin multiplexing.
 *
 *  \param[in] peri : Peripheral for which the pins should be deinitialized.
 *
 *  \note Doesn't deinitialize anything!
 *
 *  \return ERR_PARAM : Given peripheral \a peri not available.
 *  \return ERR_NONE : No error.
 *  \return ERR_NOTSUPP : Not implemented
 *
 *****************************************************************************
 */
extern s8 boardPeripheralPinDeinitialize (boardPeripheral_t peri);

#endif /* BOARD_WRAPPER_H */
