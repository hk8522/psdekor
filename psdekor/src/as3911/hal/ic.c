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
 *  \brief Interrupt controller driver for AVR
 *
 *  Uses INT0 interrupt for as3911.
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <asf.h>
#include "platform.h"
#include "ic.h"
#include "as3911_hw_config.h"

/************************************************************************/
/* LOCAL VARIABLES                                                      */
/************************************************************************/
static volatile u8 irq_state = 0;

/************************************************************************/
/* LOCAL FUNCTIONS                                                      */
/************************************************************************/

static s8 enable_irq_source (icSource_t src, bool_t save);
static s8 disable_irq_source (icSource_t src, bool_t save);

static s8 enable_irq_source (icSource_t src, bool_t save)
{
	switch (src) {

	case IC_SOURCE_AS3911:
		IRQ_INC_DISABLE();
		ioport_set_pin_sense_mode (AS3911_INTR_PIN, IOPORT_SENSE_RISING);
		AS3911_INTR_PORT.INT0MASK |= AS3911_INTR_PIN_bm;
		AS3911_INTR_PORT.INTCTRL |= PORT_INT0LVL_LO_gc;
		if (save)
			irq_state |= (1 << IC_SOURCE_AS3911);
		IRQ_DEC_ENABLE();
		return ERR_NONE;

	case IC_SOURCE_DELAY:
		IRQ_INC_DISABLE();
		AS3911_DELAY_TIMER_MODULE->INTCTRLA = TC_OVFINTLVL_LO_gc | TC_ERRINTLVL_OFF_gc;
		if (save)
			irq_state |= (1 << IC_SOURCE_DELAY);
		IRQ_DEC_ENABLE();
		return ERR_NONE;

	case IC_SOURCE_TRANSPARENT_MODE:
		return ERR_NOTSUPP;

	case IC_SOURCE_UART:
		return ERR_NOTSUPP;

	default:
		return ERR_PARAM;
	}
}

static s8 disable_irq_source (icSource_t src, bool_t save)
{
	switch (src) {

	case IC_SOURCE_AS3911:
		IRQ_INC_DISABLE();
		AS3911_INTR_PORT.INT0MASK &= ~AS3911_INTR_PIN_bm;
		AS3911_INTR_PORT.INTCTRL &= ~PORT_INT0LVL_LO_gc;
		if (save)
			irq_state &= ~(1 << IC_SOURCE_AS3911);
		IRQ_DEC_ENABLE();
		return ERR_NONE;

	case IC_SOURCE_DELAY:
		IRQ_INC_DISABLE();
		AS3911_DELAY_TIMER_MODULE->INTCTRLA = TC_OVFINTLVL_OFF_gc | TC_ERRINTLVL_OFF_gc;
		if (save)
			irq_state &= ~(1 << IC_SOURCE_DELAY);
		IRQ_DEC_ENABLE();
		return ERR_NONE;

	case IC_SOURCE_TRANSPARENT_MODE:
		return ERR_NOTSUPP;

	case IC_SOURCE_UART:
		return ERR_NOTSUPP;

	default:
		return ERR_PARAM;
	}
}

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

s8 icInitialize()
{
	pmic_init ();
	sei ();
	return ERR_NONE;
}

s8 icDeinitialize()
{
	icDisableInterrupt (IC_SOURCE_AS3911);
	icDisableInterrupt (IC_SOURCE_DELAY);

	return ERR_NONE;
}

s8 icEnableInterrupt(icSource_t src)
{
	return enable_irq_source (src, true);
}

s8 icDisableInterrupt(icSource_t src)
{
	return disable_irq_source (src, true);
}

s8 icClearInterrupt(icSource_t src)
{
	switch (src) {
	case IC_SOURCE_AS3911:
		/* clear interrupt flag */
		AS3911_INTR_PORT.INTFLAGS |= PORT_INT0IF_bm;
		return ERR_NONE;

	case IC_SOURCE_DELAY:
		/* clear interrupt flag */
		AS3911_DELAY_TIMER_MODULE->INTFLAGS |= TC0_OVFIF_bm;
		return ERR_NONE;

	case IC_SOURCE_UART:
		return ERR_NOTSUPP;

	default:
		return ERR_PARAM;
	}
}

s8 icDisableInterruptSave (icSource_t src)
{
	return disable_irq_source (src, false);
}

s8 icEnableInterruptRestore (icSource_t src)
{
	if (irq_state & (1 << src)) {
		return enable_irq_source (src, false);
	}

	return ERR_NONE;
}