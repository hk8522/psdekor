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
 *  \brief Clock driver for AVR.
 *  \note Only wraps around ASF functions.
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "platform.h"
#include "clock.h"
#include "as3911_hw_config.h"

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

/*! \brief Initializes clocks according to conf_clock.h
 *  \return Always returns ERR_NONE.
 *
 *  You have to enable peripheral clocks in this file for
 *  every module you are going to use.
 */
s8 clkInitialize ()
{
	sysclk_init ();
	/* HINT: If there is a reason to disable clocks again, all modules here
	 *       may be disabled in some clkDeinitialize function. It was NOT
	 *       required and is therefore NOT implemented - but it could be
	 *       done
	 */
	sysclk_enable_peripheral_clock (AS3911_DELAY_TIMER_MODULE);

	return ERR_NONE;
}

s8 clkSetClockSource(clkSource_t source)
{
#ifdef CONF_CHANGE_CLOCK_ALLOWED
	IRQ_INC_DISABLE();

	switch (source) {

	case CLK_SOURCE_INTERNAL:
		sysclk_set_source (SYSCLK_SRC_RC2MHZ);
		break;

	case CLK_SOURCE_EXTERNAL:
		sysclk_set_source (SYSCLK_SRC_XOSC);
		break;
	}
    IRQ_DEC_ENABLE();
#else
    /* HINT: Not implemented in original software -> therefore not
     *       implemented here! It's not feasible to implement this as the
     *       ASF library doesn't support it
     */
#endif

    return ERR_NONE;
}
