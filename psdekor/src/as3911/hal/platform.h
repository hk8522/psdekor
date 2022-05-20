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
 *      PROJECT:   AS3911
 *      $Revision: $
 *      LANGUAGE:  ANSI C
 */

/*! \file
 *
 *  \author Christian Eisendle (original PIC implementation)
 *  \author Manuel Huber (AVR port)
 *
 *  \brief Platform specific header file
 *
 *  \note Platform abstraction file has been ported to AVR.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "ams_types.h"
#include "as3911errno.h"
#include "utils.h"
#include <string.h>
#include "as3911_hw_config.h"

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
extern volatile umword IRQ_COUNT;

/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/
#define SLEEP_CPU_LOCKED() do { \
	sleep_set_mode(SLEEP_SMODE_IDLE_gc); \
	sleep_enable(); \
	cpu_irq_enable(); \
	sleep_enter(); \
	sleep_disable(); \
} while (0)

#define PSAVE_MCU_LOCKED() do { \
	sleep_set_mode(SLEEP_MODE_PWR_SAVE); \
	sleep_enable(); \
	cpu_irq_enable(); \
	sleep_enter(); \
	sleep_disable(); \
} while (0)

#define PDOWN_MCU_LOCKED() do { \
	sleep_set_mode(SLEEP_MODE_PWR_DOWN); \
	sleep_enable(); \
	cpu_irq_enable(); \
	sleep_enter(); \
	sleep_disable(); \
} while (0)

/*! macro which globally disables interrupts and increments interrupt count */
#define IRQ_INC_DISABLE() do {                                          \
	cpu_irq_disable ();                                             \
	IRQ_COUNT++;                                                    \
} while (0)

/*! macro to globally enable interrupts again if interrupt count is 0 */
#define IRQ_DEC_ENABLE() do {                                           \
	if (IRQ_COUNT != 0) IRQ_COUNT--;                                \
	if (IRQ_COUNT == 0) cpu_irq_enable ();                          \
} while (0)

/*! on pic platform, signature of interrupt service routing must include
 * __attribute__((interrupt, no_auto_psv))
 */

#define INTERRUPT(x) ISR(x)


#endif /* PLATFORM_H */
