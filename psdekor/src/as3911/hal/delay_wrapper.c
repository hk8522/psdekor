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
 *  \brief Delay module.
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include <asf.h>
#include "platform.h"
#include "delay_wrapper.h"
#include "ic.h"
#include "logger.h"
#include "as3911_hw_config.h"

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/

/*! \brief Helper structure for dividing 32 bit integers in words (16 bits) and
 *         bytes.
 */
struct Uint32_s {
	union {
		u32 v32;
		struct {
			u16 lo16;
			u16 hi16;
		};
		struct {
			u8  v8_0;
			u8  v8_1;
			u8  v8_2;
			u8  v8_3;
		};
		struct {
			u8  _p1;
			u16 mi16;
			u8  _p2;
		};
	};
};

/*!
 * \brief Info structure for delay module.
 *
 * This structure holds all information necessary for the delay module to
 * operate
 */
struct DelayInfoStruct {
	volatile bool_t delayDone;  /**< Timeout has occured */
	volatile bool_t cascade;    /**< Use multiple interrupts to handle
	                             *   long timeout periods (cascade mode) */
	volatile u32    remaining;  /**< Remainder (only in cascade mode) */
	struct Uint32_s equ_hz;     /**< Frequency of the timer module in use
	                             *   (AS3911_DELAY_TIMER_MODULE) */
	u8              shiftUp;    /**< Used to determine divider settings */
	u8              shiftDown;  /**< Used to determine divider settings */
};

/*! \brief Divider settings of timer module */
struct TCModeSettings {
	u8 divMode;   /**< Configuration for specific divider setting */
	u8 divShift;  /**< Number of right-shift operations for specific divider
	               *   setting */
};

/*! \brief All Divider configurations for timer module */
const struct TCModeSettings ModeSettings[] = {
	{TC_CLKSEL_DIV1_gc, 0}, {TC_CLKSEL_DIV2_gc, 1},
	{TC_CLKSEL_DIV4_gc, 2}, {TC_CLKSEL_DIV8_gc, 3},
	{TC_CLKSEL_DIV64_gc, 6}, {TC_CLKSEL_DIV256_gc, 8},
	{TC_CLKSEL_DIV1024_gc, 10}
};

/*!
 * \brief Delay info structure that holds all status information for the
 *         delay module
 */
static struct DelayInfoStruct DelayInfo;


/*
******************************************************************************
* LOCAL CONSTANTS
******************************************************************************
*/

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

static s8 delaySetupParameters (u32 bus_hz);
static s8 delayStartTimerMS (u16 ms);

INTERRUPT(delayIsr)
{
	if (unlikely(DelayInfo.cascade)) {
		if ((DelayInfo.remaining >> 16) != U16_C(0)) {
			/* keep cascade flag set as there are more interrupts
			 * to wait for */
			AS3911_DELAY_TIMER_MODULE->PER = U16_C(0xffff);
			DelayInfo.remaining -= U16_C(0xffff);
			goto clear_int;
		} else if (unlikely((DelayInfo.remaining & U16_C(0xffff)) !=
		                     U16_C(0))) {
			/* Last cascade for delay - unset flag */
			AS3911_DELAY_TIMER_MODULE->PER = (DelayInfo.remaining & U16_C(0xffff));
			DelayInfo.cascade = false;
			goto clear_int;
		} /* Else remaining part is 0 -> finished!
		   * Don't care about cascade flag as it is set by start
		   * routine */
	}

	/* Delay is finished! */
	DelayInfo.delayDone = true;
	AS3911_DELAY_TIMER_MODULE->CTRLA = TC_CLKSEL_OFF_gc;
	icDisableInterrupt(IC_SOURCE_DELAY);
clear_int:
	icClearInterrupt (IC_SOURCE_DELAY);
}

/*!
 * \brief Sets up parameters for milliseconds delay
 * \param bus_hz System clock frequency for the timer module in use
 *               (AS3911_DELAY_TIMER_MODULE)
 * \return Negative error code on failure, else 0.
 */
static s8 delaySetupParameters (u32 bus_hz)
{
	DelayInfo.equ_hz.v32 = bus_hz;
	DelayInfo.shiftUp = 0;

	if ((DelayInfo.equ_hz.v8_3 != 0) || /* if equ_hz > '0x003f ffff': */
	    (DelayInfo.equ_hz.v8_2 > 0x3f)) {
		/* has to be shifted down three bits */
		DelayInfo.equ_hz.v32 = (DelayInfo.equ_hz.v32 / U32_C(125));
		DelayInfo.shiftDown = 3;
	} else {
		/* has to be shifted down ten bits */
		DelayInfo.equ_hz.v32 = (DelayInfo.equ_hz.v32 << 7) / U32_C(125);
		DelayInfo.shiftDown = 10;
	}

	while ((DelayInfo.equ_hz.v8_2 != 0) || (DelayInfo.equ_hz.v8_3 != 0)) {
		DelayInfo.equ_hz.v32 >>= 1;
		++DelayInfo.shiftUp;
	}

	/* Normalize shiftUp and shiftDown: */
	if (DelayInfo.shiftUp > DelayInfo.shiftDown) {
		DelayInfo.shiftUp -= DelayInfo.shiftDown;
		DelayInfo.shiftDown = 0;
	} else {
		DelayInfo.shiftDown -= DelayInfo.shiftUp;
		DelayInfo.shiftUp = 0;
	}

	if (unlikely(DelayInfo.equ_hz.v32 == 0)) {
		return ERR_REQUEST;
	}

	return ERR_NONE;
}

/*
 * \brief Starts delay timer.
 * \param ms Timeout period of the timer module (in milliseconds).
 * \note This method could be improved (as suggested in the source code) -
 *       however, it wasn't necessary and therefore only not done -
 *       Since it has already been evaluated which part of the algorithm
 *       could be improved and how much it affects the timing,
 *       these notes have been left in the source code - ON PURPOSE!
 */
static s8 delayStartTimerMS (u16 ms)
{
	const u8 settings_count = sizeof(ModeSettings) /
	                          sizeof(ModeSettings[0]);
	struct Uint32_s tmp;
	u8 i;
	u8 shift_down;
	u32 result;

	/* HINT: inline asm mul 16x16->32 (but only affects 20% of
	 *       extra execution time)
	 */
	result = ((u32)DelayInfo.equ_hz.lo16 * (u32)ms);
	AS3911_DELAY_TIMER_MODULE->CNT = U16_C(0);

	/* HINT: This loop is very inefficient (especially shifting 32 bits).
	 *       This loop affects ~80% of extra execution time
	 *       (shift operation).
	 */
	for (i = 0; i < settings_count; ++i) {
		shift_down = ModeSettings[i].divShift + DelayInfo.shiftDown;

		tmp.v32 = result;
		if (unlikely(shift_down < DelayInfo.shiftUp)) {
			tmp.v32 <<= (DelayInfo.shiftUp - shift_down);
		} else {
			tmp.v32 >>= (shift_down - DelayInfo.shiftUp);
		}

		if (tmp.hi16 == U16_C(0)) {
			if (unlikely(tmp.lo16 == U16_C(0))) {
				/* Since divider settings are monotonically
				 * nondecreasing, this means that none of the
				 * remaining settings matches the requested
				 * timeout (clock is too slow; -> unlikely)
				 */
				return ERR_REQUEST;
			}

			DelayInfo.cascade = false;
			AS3911_DELAY_TIMER_MODULE->PER = tmp.lo16;
			AS3911_DELAY_TIMER_MODULE->CTRLA = ModeSettings[i].divMode;
			goto exit_timer_running;
		}
	}

	/* Timeout period is too long for 16bit timer -> use extra 32 bits
	 * variable */
	AS3911_DELAY_TIMER_MODULE->PER = U16_C(0xffff);

	IRQ_INC_DISABLE();
	AS3911_DELAY_TIMER_MODULE->CTRLA = ModeSettings[settings_count - 1].divMode;
	DelayInfo.remaining = tmp.v32 - U32_C(0xffff);
	DelayInfo.cascade = true;
	IRQ_DEC_ENABLE();

exit_timer_running:
	DelayInfo.delayDone = false;
	icEnableInterrupt (IC_SOURCE_DELAY);

	return ERR_NONE;
}

s8 delayInitialize()
{
	AS3911_DELAY_TIMER_MODULE->CTRLA = TC_CLKSEL_OFF_gc;      /* Timer OFF */
	AS3911_DELAY_TIMER_MODULE->CTRLB = 0x00;                  /* CCxEN = false, WGMODE = NORMAL */
	AS3911_DELAY_TIMER_MODULE->CTRLC = 0x00;                  /* CMPx = 0 */
	AS3911_DELAY_TIMER_MODULE->CTRLD = 0x00;                  /* EVACT = OFF, EVDLY = 0,
	                                     * EVSEL = OFF */
	AS3911_DELAY_TIMER_MODULE->CTRLE = 0x00;                  /* BYTEM = NORMAL */
	AS3911_DELAY_TIMER_MODULE->INTCTRLA = TC_OVFINTLVL_LO_gc; /* ERRINTLVL = OFF,
	                                     * OVFINTLVL = LOW */
	AS3911_DELAY_TIMER_MODULE->INTCTRLB = 0x00;               /* CCxINTLVL = OFF */
	AS3911_DELAY_TIMER_MODULE->CNT = U16_C(0);
	AS3911_DELAY_TIMER_MODULE->PER = U16_C(0);

	DelayInfo.delayDone = false;
	DelayInfo.cascade = false;

	return delaySetupParameters (sysclk_get_peripheral_bus_hz (AS3911_DELAY_TIMER_MODULE));;
}

s8 delayDeinitialize()
{
	IRQ_INC_DISABLE();
	delayNMilliSecondsStop ();
	icDisableInterrupt (IC_SOURCE_DELAY);
	icClearInterrupt (IC_SOURCE_DELAY);
	IRQ_DEC_ENABLE();

	return ERR_NONE;
}

s8 delayNMilliSeconds(u16 ms)
{
	s8 err = delayStartTimerMS (ms);
	EVAL_ERR_NE_GOTO(err, ERR_NONE, out);

	cpu_irq_disable();
	while (!DelayInfo.delayDone) {
		SLEEP_CPU_LOCKED();
		cpu_irq_disable();
	}
	cpu_irq_enable();
out:
	return err;
}

void delayNMilliSecondsStop()
{
	AS3911_DELAY_TIMER_MODULE->CTRLA = TC_CLKSEL_OFF_gc;
}

s8 delayNMilliSecondsStart(u16 ms)
{
	return delayStartTimerMS (ms);
}

bool_t delayNMilliSecondsIsDone(bool_t do_sleep)
{
	cpu_irq_disable();
	if (do_sleep && !(DelayInfo.delayDone)) {
		SLEEP_CPU_LOCKED();
	} else {
		cpu_irq_enable();
	}

	 return DelayInfo.delayDone;
}

s8 delayNMicroSeconds(u16 us)
{
	delay_us (us);
	return ERR_NONE;
}
