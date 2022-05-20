/*
 * reschedule.c
 *
 * Created: 06.08.2014 10:01:27
 *  Author: huber
 */

#include "utils/reschedule.h"
#include "asf.h"

static uint8_t s_timer_div = TC_CLKSEL_OFF_gc;
static uint16_t s_ticks = 0;
static rsched_callback_t *s_callback = NULL;

static volatile enum {
	STATUS_STOPPED,
	STATUS_RUNNING
} s_status = STATUS_STOPPED;


static void deactivate_interrupts (void);

ISR(RSCHED_ISR)
{
	RSCHED_TIMER_UNIT->CTRLA = TC_CLKSEL_OFF_gc;
	deactivate_interrupts();
	sysclk_disable_peripheral_clock(RSCHED_TIMER_UNIT);
	s_status = STATUS_STOPPED;

	if (s_callback) {
		s_callback(RSCHED_SOURCE_ISR);
	}
}

static void deactivate_interrupts (void)
{
	RSCHED_TIMER_UNIT->INTCTRLB = TC_CCAINTLVL_OFF_gc | TC_CCBINTLVL_OFF_gc |
	TC_CCCINTLVL_OFF_gc | TC_CCDINTLVL_OFF_gc;
}

int8_t rsched_init (uint8_t timer_div)
{
	switch (timer_div) {
	case TC_CLKSEL_DIV1_gc:
	case TC_CLKSEL_DIV2_gc:
	case TC_CLKSEL_DIV4_gc:
	case TC_CLKSEL_DIV8_gc:
	case TC_CLKSEL_DIV64_gc:
	case TC_CLKSEL_DIV256_gc:
	case TC_CLKSEL_DIV1024_gc:
		break;

	default:
		return ERR_INVALID_ARG;
	}

	RSCHED_TIMER_UNIT->CTRLA = TC_CLKSEL_OFF_gc;
	RSCHED_TIMER_UNIT->PER = 0xffff;
	RSCHED_TIMER_UNIT->INTCTRLA = TC_OVFINTLVL_OFF_gc | TC_ERRINTLVL_OFF_gc;
	deactivate_interrupts();
	s_timer_div = timer_div;

	return 0;
}

int8_t rsched_reschedule(uint16_t ticks, rsched_callback_t *callback)
{
	RSCHED_TIMER_UNIT->CTRLA = TC_CLKSEL_OFF_gc;

	sysclk_enable_peripheral_clock (RSCHED_TIMER_UNIT);

	RSCHED_TIMER_UNIT->CNT = 0;
	RSCHED_TIMER_UNIT->CCA = ticks;
	s_ticks = ticks;
	s_callback = callback;

#if RSCHED_PRIORITY == high
	RSCHED_TIMER_UNIT->INTCTRLB = TC_CCAINTLVL_HI_gc;
#elif RSCHED_PRIORITY == medium
	RSCHED_TIMER_UNIT->INTCTRLB = TC_CCAINTLVL_MED_gc;
#elif RSCHED_PRIORITY == low
	RSCHED_TIMER_UNIT->INTCTRLB  = TC_CCAINTLVL_LO_gc;
#else
# error Unsupported priority setting
#endif
	s_status = STATUS_RUNNING;
	RSCHED_TIMER_UNIT->CTRLA = s_timer_div;

	return 0;
}

void rsched_wait_pending_locked (void)
{
	deactivate_interrupts();

	while ((s_status == STATUS_RUNNING) &&
	       (RSCHED_TIMER_UNIT->CNT < s_ticks));

	RSCHED_TIMER_UNIT->CTRLA = TC_CLKSEL_OFF_gc;

	if (s_callback) {
		s_callback(RSCHED_SOURCE_WAIT);
	}
}