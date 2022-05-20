/*
 * as3911_enhanced_irq_control.h
 *
 * Created: 22.11.2013 09:25:00
 *  Author: huber
 */ 


#ifndef AS3911_ENHANCED_IRQ_CONTROL_H_
#define AS3911_ENHANCED_IRQ_CONTROL_H_

#include "as3911_interrupt.h"
#include "ic.h"

extern volatile umword AS3911_IRQ_COUNT;


#define AS3911_IRQ_DEC_ENABLE() do {							\
	if (AS3911_IRQ_COUNT != 0) --AS3911_IRQ_COUNT;					\
	if (AS3911_IRQ_COUNT == 0) {							\
		IRQ_INC_DISABLE();							\
		icEnableInterruptRestore (IC_SOURCE_AS3911);				\
		while (ioport_get_pin_level (AS3911_INTR_PIN) == 1) {			\
			icDisableInterruptSave (IC_SOURCE_AS3911);			\
			icClearInterrupt (IC_SOURCE_AS3911);				\
			IRQ_DEC_ENABLE();						\
			as3911_interrupt_handler ();					\
			IRQ_INC_DISABLE();						\
			icEnableInterruptRestore (IC_SOURCE_AS3911);			\
		}									\
		IRQ_DEC_ENABLE();							\
	}										\
} while (0)

#define AS3911_IRQ_INC_DISABLE() do {							\
	icDisableInterruptSave (IC_SOURCE_AS3911);					\
	++AS3911_IRQ_COUNT;								\
} while (0)


/*
#define AS3911_IRQ_DEC_ENABLE() IRQ_DEC_ENABLE()
#define AS3911_IRQ_INC_DISABLE() IRQ_INC_DISABLE()
*/

extern void as3911_interrupt_handler (void);

#endif /* AS3911_ENHANCED_IRQ_CONTROL_H_ */