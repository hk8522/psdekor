/*
 * software_functions.c
 *
 * Created: 25.11.2013 15:55:28
 *  Author: huber
 */
#include "platform.h"
#include "application/application.h"
#include "application/software_functions.h"
#include "utils/debug.h"
#include "application/rtc_timeout.h"
#include "application/timeouts.h"
#include "application/motor_control.h"
#include "buzzer/sounds.h"
#include "cardman/cards_manager.h"

enum alarm_state_e {
	ALARM_STATE_OFF,
	ALARM_STATE_20S,
	ALARM_STATE_01S,
	ALARM_STATE_DISABLED
};

typedef enum { GYM_STATE_OPEN, GYM_STATE_LOCK } gym_state_e;

gym_state_e					gym_state = GYM_STATE_OPEN;
struct card_entry			gym_user_card = { .len = 0 };

static struct sw1_state_s {
	enum alarm_state_e alarm_state;
}sw1_state = {ALARM_STATE_OFF};



uint32_t			auto_open_delay = 0;
uint32_t			auto_open_delay_remain = 0;
uint8_t				auto_open_delay_steps = 60;


/************************************************************************/
/*                                                                      */
/************************************************************************/
void gym_rtc_callback(void) {
	

	if (gym_state != GYM_STATE_LOCK)
		return;

	auto_open_delay_remain--;

	DLOG("gym_rtc_callback auto_open_delay_remain=%u\r\n", auto_open_delay_remain);


	if (gym_state == GYM_STATE_OPEN) { 
		auto_open_delay_remain = 0;
		return;
	}

	if (auto_open_delay_remain == 0) {
		gym_state = GYM_STATE_OPEN;
		motorOpenLock(true);
		buzzerStart(SignalPositive, false);
		buzzerWaitTillFinished();
	} else {
		rtcGetInterrupt();
		rtcStartINTR(auto_open_delay_steps);
	}
}


/************************************************************************/
/*                                                                      */
/************************************************************************/
void gym_card_callback(struct scan_result_s *scan) {
			
	if (scan->type == CARD_TYPE_PROGRAMMING_CARD) {
		if (gym_state == GYM_STATE_LOCK) {
			
			gym_state = GYM_STATE_OPEN;
			motorOpenLock(true);
			buzzerStart(SignalPositive, false);
			buzzerWaitTillFinished();
			
		} else {
			buzzerStart(SignalError, false);
			buzzerWaitTillFinished();
		}
	} else if (scan->type == CARD_TYPE_GYM_SOFTWARD_CARD || scan->type == CARD_TYPE_SOFTWARE_CARD) {
			buzzerStart(SignalError, false);
			buzzerWaitTillFinished();
		return;
	} else {
		if (gym_state == GYM_STATE_OPEN) {


			if (!isDoorClosed()) {
				buzzerStart(SignalError, false);
				buzzerWaitTillFinished();
				return;
			}
			
		
			memcpy(&gym_user_card,  &scan->card, sizeof(struct card_entry));
			motorCloseLock(true);
			buzzerStart(SignalPositive, false);
			buzzerWaitTillFinished();

			auto_open_delay = cardmanGetOpenDelay();

			DLOG("Open Delay: %u hours\r\n", auto_open_delay);

			if (auto_open_delay > 0) {
				auto_open_delay_remain = (auto_open_delay * 60 * 60) / auto_open_delay_steps;
				rtcGetInterrupt();
				rtcStartINTR(auto_open_delay_steps);
			}
		
		
			gym_state = GYM_STATE_LOCK;
		} else {
			if (memcmp(&gym_user_card, &scan->card,  sizeof(struct card_entry)) == 0) {
				memset(&gym_user_card, 0x00, sizeof(struct card_entry));
				motorOpenLock(true);	
				buzzerStart(SignalPositive, false);
				buzzerWaitTillFinished();
				auto_open_delay_remain = 0;
			
				gym_state = GYM_STATE_OPEN;
			} else {
				buzzerStart(SignalNegative, false);
				buzzerWaitTillFinished();
				return;
			}
		}
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static s8 open_door (bool_t slave)
{
	s8 err;
	/* TODO: Implement slave function */

	err = motorOpenLock(slave);
	if (err != ERR_NONE)
		return err;

	rtcGetInterrupt ();
	rtcStartINTR (TIMEOUT_RTC_LOCK_OPEN_WAIT_TIME);
	cpu_irq_disable();
	while (!rtcGetInterrupt ()) {
		PSAVE_MCU_LOCKED();
		cpu_irq_disable();
	}
	cpu_irq_enable();

	err = motorCloseLock(slave);

	return err;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static void nkeys_changed (void)
{
	u8 sw = cardmanGetSoftwareFunction();

	if (cardmanGetNumberOfKeys() > 0) {
		if (sw & SW_FUNCTION_GYM) {
			motorOpenLock((sw & SW_FUNCTION_SLAVE) ? 1 : 0);
		} else {
			if ((sw & SW_FUNCTION_LATCH) == 0) {
				motorCloseLock((sw & SW_FUNCTION_SLAVE) ? 1 : 0);
			}
		}
	} else {
		motorOpenLock((sw & SW_FUNCTION_SLAVE) ? 1 : 0);
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static void key_function (void)
{
	u8 sw = cardmanGetSoftwareFunction();

	if (sw & SW_FUNCTION_ALARM) {
		switch (sw1_state.alarm_state) {
		case ALARM_STATE_01S:
			sw1_state.alarm_state = ALARM_STATE_DISABLED;
			rtcStop ();
			rtcGetInterrupt ();
			DLOG("ALARM: Disabled alarm by key (from main)\r\n");
			break;

		case ALARM_STATE_OFF:
		case ALARM_STATE_20S:
		default:
			open_door((sw & SW_FUNCTION_SLAVE) ? 1 : 0);
			sw1_state.alarm_state = ALARM_STATE_OFF;
			DLOG("ALARM: Switch off alarm (key)\r\n");
			break;
		}
	} else if (sw & SW_FUNCTION_LATCH) {
		if (motorIsLockOpen ()) {
			motorCloseLock((sw & SW_FUNCTION_SLAVE) ? 1 : 0);
		} else {
			motorOpenLock((sw & SW_FUNCTION_SLAVE) ? 1 : 0);
		}
		
	} else if (sw & SW_FUNCTION_GYM) {
		DLOG("should never reach this!\r\n");
 	} else {
		open_door((sw & SW_FUNCTION_SLAVE) ? 1 : 0);
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static void door_function (void)
{
	if (isDoorClosed()) {
		/* Door has been closed */
		DLOG("DOOR_TRIGGER: Door has been closed!\r\n");
	} else {
		/* Door has been opened */
		DLOG("DOOR_TRIGGER: Door has been opened!\r\n");
	}
}


/************************************************************************/
/*                                                                      */
/************************************************************************/
static void rtc_function (void)
{
	rtcStop ();
	rtcGetInterrupt ();

	if ((cardmanGetSoftwareFunction() & SW_FUNCTION_ALARM) == 0) {
		DLOG("RTC: Spurious RTC...\r\n");
		return;
	}

	if (!isDoorClosed()) {
		/* Door is open */
		switch (sw1_state.alarm_state) {
		case ALARM_STATE_DISABLED:
		case ALARM_STATE_OFF:
			return;

		case ALARM_STATE_20S:
			DLOG("ALARM: RTC triggered first alarm (20s)\r\n");
			sw1_state.alarm_state = ALARM_STATE_01S;
			buzzerStart(SignalFirstAlarm, false);
			break;

		case ALARM_STATE_01S:
			DLOG("ALARM: RTC triggered furhter alarm (1s)\r\n");
			buzzerStart(SignalRepeatedAlarm, false);
			break;
		}

		while (buzzerIsRunning()) {
			if (isDoorClosed ()) {
				sw1_state.alarm_state = ALARM_STATE_OFF;
				DLOG("ALARM: Door has been closed\r\n");
				return;
			}

			scan_result.found_card_counter = 0;
			RfidStartScan (RFID_UNIT_1, 0, IdentifyCardCallback);
			if ((scan_result.found_card_counter == 1) &&
			    (scan_result.type == CARD_TYPE_KEY)) {
				sw1_state.alarm_state = ALARM_STATE_DISABLED;
				buzzerStart(SignalPositive, false);
				buzzerWaitTillFinished();
				DLOG("ALARM: Disabled by key\r\n");
				return;
			}
		}

		rtcStartINTR(TIMEOUT_RTC_REPEATING_ALARM);
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static void init_function (void)
{
	u8 sw = cardmanGetSoftwareFunction();

	nkeys_changed();

	if (sw & SW_FUNCTION_ALARM) {
		sw1_state.alarm_state = ALARM_STATE_OFF;
		SoftwareCheckAlarm();
	}

	setSlaveLock(motorIsLockOpen(), sw & SW_FUNCTION_SLAVE);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static void deinit_function (void)
{
	rtcStop ();
	rtcGetInterrupt ();
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
void SoftwareCheckAlarm (void)
{
	if (cardmanGetSoftwareFunction() & SW_FUNCTION_ALARM) {
		if (cardmanGetNumberOfKeys() == 0) {
			if (sw1_state.alarm_state != ALARM_STATE_OFF) {
				sw1_state.alarm_state = ALARM_STATE_OFF;
				rtcStop();
				rtcGetInterrupt(); /* Clear IRQ Flag */
				DLOG("ALARM: Disable alarm (no keys set)\r\n");
			}
		} else if (isDoorClosed()) {
			switch (sw1_state.alarm_state) {
			case ALARM_STATE_OFF:
				break;
			
			case ALARM_STATE_01S:
			case ALARM_STATE_20S:
				rtcStop();
				rtcGetInterrupt(); /* Clear IRQ Flag */
				DLOG("ALARM: Door has been closed (CheckAlarm)\r\n");
			case ALARM_STATE_DISABLED:
				sw1_state.alarm_state = ALARM_STATE_OFF;
				DLOG("ALARM: Reset alarm to OFF\r\n");
				break;
			}
		} else {
			switch (sw1_state.alarm_state) {
			case ALARM_STATE_DISABLED:
				break;

			case ALARM_STATE_OFF:
				DLOG("ALARM: Enable alarm in 20s (CheckAlarm)\r\n");
				sw1_state.alarm_state = ALARM_STATE_20S;
				rtcStartINTR(TIMEOUT_RTC_FIRST_ALARM);
				break;

			case ALARM_STATE_01S:
			case ALARM_STATE_20S:
				break;
			}
		}
	}
}

void SoftwareStateMachine (enum sw_trigger trigger)
{
	switch (trigger) {
	case SW_TRIGGER_RTC:
		rtc_function();
		break;

	case SW_TRIGGER_DOOR:
		door_function();
		break;

	case SW_TRIGGER_KEY:
		key_function();
		break;

	case SW_TRIGGER_INIT_SOFTWARE:
		init_function();
		break;

	case SW_TRIGGER_CHANGE_NCARDS:
		nkeys_changed();
		break;

	case SW_TRIGGER_DEINIT_SOFTWARE:
		deinit_function();
		break;
	}
}

void SoftwareSleep (void)
{
	PSAVE_MCU_LOCKED();
	//PDOWN_MCU_LOCKED();
/*
	if (cardmanGetSoftwareFunction() & SW_FUNCTION_ALARM) {
		if ((sw1_state.alarm_state == ALARM_STATE_OFF) ||
		(sw1_state.alarm_state == ALARM_STATE_DISABLED)) {
			PDOWN_MCU_LOCKED();
		} else {
			PSAVE_MCU_LOCKED();
		}
	} else {
		PDOWN_MCU_LOCKED();
	}
	*/
}

void SoftwarePrintVersion (void)
{
	u8 sw = cardmanGetSoftwareFunction();

	DLOG("Software configuration: ");
	if (sw & SW_FUNCTION_ALARM) {
		DLOG("ALARM ");
	}

	if (sw & SW_FUNCTION_LATCH) {
		DLOG("LATCH ");
	}

	if (sw & SW_FUNCTION_SLAVE) {
		DLOG("SLAVE ");
	}
	
	if (sw & SW_FUNCTION_GYM) {
		DLOG("GYM ");
	}

	DLOG("(%hhx)\r\n", sw);
}

void SoftwareRestartAlarm (void)
{
	if (cardmanGetSoftwareFunction() & SW_FUNCTION_ALARM) {
		switch (sw1_state.alarm_state) {
		case ALARM_STATE_DISABLED:
		case ALARM_STATE_OFF:
			break;
		
		case ALARM_STATE_01S:
			sw1_state.alarm_state = ALARM_STATE_20S;
		case ALARM_STATE_20S:
			DLOG("ALARM: Reset Alarm (20s)\r\n");
			rtcStartINTR(TIMEOUT_RTC_FIRST_ALARM);
			break;
		}
	}
}