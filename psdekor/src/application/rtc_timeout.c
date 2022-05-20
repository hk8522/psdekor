/*
 * rtc_timeout.c
 *
 * Created: 25.11.2013 10:28:52
 *  Author: huber
 */ 
#include "application/rtc_timeout.h"
#include "utils/debug.h"
										\
#define RTC_IRQ_DISABLE() do {                                                  \
	IRQ_INC_DISABLE();                                                      \
	RTC.INTCTRL = RTC_OVFINTLVL_OFF_gc | RTC_COMPINTLVL_OFF_gc;             \
	IRQ_DEC_ENABLE();                                                       \
} while (0)

#define RTC_IRQ_ENABLE() do {                                                   \
	IRQ_INC_DISABLE();                                                      \
	RTC.INTCTRL = RTC_OVFINTLVL_LO_gc | RTC_COMPINTLVL_OFF_gc;              \
	IRQ_DEC_ENABLE();                                                       \
} while (0)

static void wait_for_rtc (void)
{
	while (RTC.STATUS & RTC_SYNCBUSY_bm) {
		delayNMilliSeconds (5);
	}
}

#define TIMEOUT_INTR_DISABLED 0
#define TIMEOUT_INTR_ENABLED 1
#define TIMEOUT_INTR_OCCURED 2

static volatile bool_t timeoutDone = true;
static volatile u8 timeout_intr = TIMEOUT_INTR_DISABLED;


ISR(RTC_OVF_vect)
{
	rtcStop ();
	if (timeout_intr == TIMEOUT_INTR_ENABLED)
		timeout_intr = TIMEOUT_INTR_OCCURED;
}

static void start (const u8 seconds);

static void start (const u8 seconds)
{
	u16 tmp;

	DLOG("RTC start %u sec\r\n", seconds);

	if (unlikely(seconds == 0)) {
		rtcStop ();
		return;
	}

	tmp = seconds;
	tmp *= 125;

	if (timeoutDone) {
		sysclk_enable_peripheral_clock (&RTC);
		CLK.RTCCTRL = CLK_RTCSRC_ULP_gc | CLK_RTCEN_bm;
	}

	RTC.CTRL = RTC_PRESCALER_OFF_gc;
	wait_for_rtc ();
	RTC.PER = tmp;
	wait_for_rtc ();
	RTC.CNT = 0;
	wait_for_rtc ();

	timeoutDone = false;
	RTC.CTRL = RTC_PRESCALER_DIV8_gc;
	wait_for_rtc ();

	RTC.INTFLAGS |= RTC_OVFIF_bm;
	RTC_IRQ_ENABLE();
}

void rtcStartINTR (const u8 seconds)
{
	RTC_IRQ_DISABLE();
	timeout_intr = TIMEOUT_INTR_ENABLED;
	start (seconds);
}

void rtcStart (const u8 seconds)
{
	RTC_IRQ_DISABLE();
	timeout_intr = TIMEOUT_INTR_DISABLED;
	start (seconds);
}

void rtcStop (void)
{
	RTC_IRQ_DISABLE();

	DLOG("RTC STOP\r\n");

	timeoutDone = true;
	CLK.RTCCTRL = 0;
	sysclk_disable_peripheral_clock(&RTC);
}

bool_t rtcIsFinished (void)
{
	return timeoutDone;
}

bool_t rtcXSecondsPassed (const u8 x)
{
	u16 tmp = x;

	tmp *= 125;
	
	if (RTC.CNT > tmp)
		return true;
	else
		return timeoutDone;
}

bool_t rtcGetInterrupt (void)
{
	u8 intr;

	IRQ_INC_DISABLE();
	intr = timeout_intr;
	if (intr == TIMEOUT_INTR_OCCURED)
		timeout_intr = TIMEOUT_INTR_DISABLED;
	IRQ_DEC_ENABLE();

	return (intr == TIMEOUT_INTR_OCCURED);
}

bool_t rtcPollInterrupt_LOCKED (void)
{
	return (timeout_intr == TIMEOUT_INTR_OCCURED);
}