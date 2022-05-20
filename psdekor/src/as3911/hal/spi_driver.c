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
 *  \author Wolfgang Reichart (original implementation on PIC)
 *  \author Manuel Huber (AVR port)
 *
 *  \brief SPI driver for AVR (ported from PIC).
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <asf.h>
#include "ams_types.h"
#include "errno.h"
#include "spi_driver.h"
#include "logger.h"
#include "ams_types.h"
#include "platform.h"

/*
******************************************************************************
* LOCAL MACROS
******************************************************************************
*/

/*
******************************************************************************
* LOCAL DATATYPES
******************************************************************************
*/

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/

static struct spiConfig current_config = {
	.spi_dev = NULL
};

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

s8 spiInitialize (const spiConfig_t *config)
{
	struct spi_device tmp;

	if (unlikely (config == NULL) ||unlikely (config->spi_dev == NULL)) {
		SPI_LOG ("[spi] Invalid config\r\n");
		return ERR_PARAM;
	}

	if (current_config.spi_dev != NULL) {
		spiDeinitialize ();
	}

	tmp.id = config->sen;
	current_config = *config;

	current_config.needs_reinit = false;

	spi_master_init (config->spi_dev);
	spi_master_setup_device (config->spi_dev,
	                         &tmp,
	                         config->flags,
	                         config->baudrate,
	                         0 /* Not used */);
	spi_enable (config->spi_dev);

	SPI_LOG ("[spi] Activated SPI channel");

    return ERR_NONE;
}

s8 spiReinitialize (void)
{
	struct spi_device tmp;

	if (unlikely(current_config.spi_dev == NULL)) {
		SPI_LOG ("[spi] hasn't been initialized\r\n");
		return ERR_REQUEST;
	} else {
		IRQ_INC_DISABLE();
		if (current_config.needs_reinit) {
			current_config.needs_reinit = false;
			tmp.id = current_config.sen;

			spi_master_init (current_config.spi_dev);
			spi_master_setup_device (current_config.spi_dev,
			                         &tmp,
			                         current_config.flags,
			                         current_config.baudrate,
			                         0 /* Not used */);
		}
		IRQ_DEC_ENABLE();

		return ERR_NONE;
	}
}

s8 spiPause (void)
{
	if (unlikely(current_config.spi_dev == NULL)) {
		SPI_LOG ("[spi] hasn't been initialized\r\n");
		return ERR_REQUEST;
	} else {
		IRQ_INC_DISABLE();
		if (!current_config.needs_reinit) {
			/* No need to actually disable the spi interface */
			current_config.needs_reinit = true;
		}
		IRQ_DEC_ENABLE();

		return ERR_NONE;
	}
}

s8 spiDeinitialize (void)
{
	if (unlikely (current_config.spi_dev == NULL)) {
		SPI_LOG ("[spi] hasn't been initialized\r\n");
		return ERR_REQUEST;
	}

	spi_disable (current_config.spi_dev);
	current_config.spi_dev = NULL;

	return ERR_NONE;
}

s8 spiTxRx (const u8 *txData, u8 *rxData, u16 length)
{
	u8 tmp;
	u16 i = 0;

	if (unlikely (txData == NULL) || unlikely (length == 0)) {
		SPI_LOG ("[spi] Illegal parameter in 'spiTxRx': txData=%x, rxData=%x, length=%u\r\n",
		         txData, rxData, length);
		return ERR_PARAM;
	}

	if (unlikely (current_config.spi_dev == NULL)) {

		SPI_LOG ("[spi] SPI has not been initialized; Request failed!\r\n");
		return ERR_REQUEST;
	}

	SPI_LOG("[spi] write:");
	for (i = 0; i < length; ++i) {

		SPI_LOG (" %hhx", txData [i]);
		spi_put (current_config.spi_dev, txData [i]);

		/* HINT: Busy waiting here is not the best solution
		 *       however, previous implementation was written similar.
		 *       Change is not easy here - (interrupts etc.)
		 */
		while (!spi_is_rx_full(current_config.spi_dev));

		tmp = spi_get (current_config.spi_dev);
		if (rxData != NULL)
			rxData [i] = tmp;
	}

	SPI_LOG ("\r\n");
#if USE_LOGGER
	if (rxData) {
		SPI_LOG ("[spi] receive:");

		for (i = 0; i < length; i++)
		{
			SPI_LOG(" %hhx", rxData[i]);
		}
		SPI_LOG ("\r\n");
	}
#endif

	return ERR_NONE;
}

void spiActivateSEN (void)
{
	bool_t lvl_en;
	if (unlikely (current_config.spi_dev == NULL)) {
		SPI_LOG ("[spi] Not initialized\r\n");
		return;
	}

	lvl_en = current_config.invert_sen ? 0 : 1;
	ioport_set_pin_level (current_config.sen, lvl_en);
}

void spiDeactivateSEN (void)
{
	bool_t lvl_dis;
	if (unlikely (current_config.spi_dev == NULL)) {
		SPI_LOG ("[spi] Not initialized\r\n");
		return;
	}

	lvl_dis = current_config.invert_sen ? 1 : 0;
	ioport_set_pin_level (current_config.sen, lvl_dis);
}
