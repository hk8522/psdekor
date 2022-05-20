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
 *      $Revision: $
 *      LANGUAGE:  ANSI C
 */

/*! \file
 *
 *  \author Wolfgang Reichart (original PIC implementation)
 *  \author Manuel Huber (AVR port)
 *
 *  \brief spi driver declaration file
 *
 */
/*!
 *
 * The spi driver provides basic functionality for sending and receiving
 * data via SPI interface. All four common SPI modes are supported.
 * Moreover, only master mode and 8 bytes blocks are supported.
 *
 * API:
 * - Initialize SPI driver: #spiInitialize
 * - Deinitialize SPI driver: #spiDeinitialize
 * - Transmit data: #spiTxRx
 */

#ifndef SPI_H
#define SPI_H

/*! \brief Structur to hold all information about spi module */
struct spiConfig {
	SPI_t *spi_dev;
	port_pin_t sen;         /**< positive logic if not inverted
	                         * (chip selected -> 1, chip deselected -> 0) */
	Bool invert_sen;        /**< true if logic is inverted (chip select) */
	spi_flags_t flags;      /**< SPI mode */
	unsigned long baudrate; /**< SPI baudrate */
	Bool needs_reinit;      /**< internal flag, used to support deep sleep
	                         *   modes. DO NOT MODIFY! */
};

typedef struct spiConfig spiConfig_t;

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "ams_types.h"

/*
******************************************************************************
* DEFINES
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL DATATYPES
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

/*!
 *****************************************************************************
 *  \brief  activate SEN for configured SPI device
 *
 *  Activate the SEN for the configured SPI device. Used by stream_disptcher.h.
 *
 *****************************************************************************
 */
extern void spiActivateSEN (void);

/*!
 *****************************************************************************
 *  \brief  deactivate SEN for configured SPI device
 *
 *  Deactivate the SEN for the configured SPI device. Used stream_disptcher.h.
 *
 *****************************************************************************
 */
extern void spiDeactivateSEN (void);

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/

/*!
 *****************************************************************************
 *  \brief  Initialize SPI interface
 *
 *  This function initializes the SPI interface, i.e. sets the requested
 *  frequency and SPI mode.
 *
 *  \param[in]  sysClk : The system clock currently in use
 *  \param[in]  spiConfigIn  : structure holding configuration parameter
 *                             (see #spiConfig_t)
 *  \param[out] spiConfigOut : structure returning previous configuration
 *                             parameter
 *
 *  \return ERR_NONE : No error, SPI initialized.
 *
 *****************************************************************************
 */
extern s8 spiInitialize (const spiConfig_t *config);

/*!
 *****************************************************************************
 * \brief Deinitializes an interface but keeps the configuration. Use it
 *        before deep sleep.
 *
 * If the spi interface has already been paused, no error will be returned.
 *
 * \return Error code or ERR_NONE on success.
 *****************************************************************************
 */
extern s8 spiPause (void);

/*!
 *****************************************************************************
 * \brief Reinitializes an already initialized spi interface
 *        (after deep sleep)
 *
 * If the spi interface has not been paused, no error will be returned since
 * the interface is initialized. If no spi interface has been configured,
 * ERR_REQUEST will be returned.
 *
 * \return Error code or ERR_NONE on success.
 */
extern s8 spiReinitialize (void);

/*!
 *****************************************************************************
 *  \brief  Deinitialize SPI interface
 *
 *  Calling this function deinitializes the SPI interface.
 *  SPI transfers are then not possible any more.
 *
 *  \return ERR_NONE : No error, SPI deinitialized.
 *
 *****************************************************************************
 */
extern s8 spiDeinitialize (void);

/*!
 *****************************************************************************
 *  \brief  Transfer a buffer of given length
 *
 *  This function is used to shift out \a length bytes of \a outbuf and
 *  write latched in data to \a inbuf.
 *  \note Due to the nature of SPI \a inbuf has to have the same size as
 *  \a outbuf, i.e. the number of bytes shifted out equals the number of
 *  bytes latched in again.
 *
 *  \param[in] txData: Buffer of size \a length to be transmitted.
 *  \param[out] rxData: Buffer of size \a length where received data is
 *              written to OR NULL in order to perform a write-only operation.
 *  \param[in] length: Number of bytes to be transfered.
 *
 *  \return ERR_IO : Error during SPI communication, returned data may be
 *                   invalid.
 *  \return ERR_NONE : No error, all \a length bytes transmitted/received.
 *
 *****************************************************************************
 */
extern s8 spiTxRx (const u8* txData, u8* rxData, u16 length);

#endif /* SPI_H */
