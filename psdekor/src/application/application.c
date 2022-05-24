/*
 * application.c
 *
 * Created: 22.11.2013 17:39:24
 *  Author: huber
 *  Modified: Alain Stucki <as@bitlab.ch>
 */ 

/* Changelog:
 *	Dec. 2014			Added Phase Detect functionality 
 *
 *
 */

#include "application/application.h"
#include "as3911.h"
#include "as3911_com.h"
#include "as3911_interrupt.h"
#include "iso14443a.h"
#include "clock.h"
#include "ic.h"
#include "uart.h"
#include "delay_wrapper.h"
#include "utils/debug.h"
#include "utils/reschedule.h"
#include "buzzer/sounds.h"
#include "spi_driver.h"
#include "cardman/card_utils.h"
#include "application/rtc_timeout.h"
#include "application/software_functions.h"
#include "application/timeouts.h"
#include "cardman/cards_manager.h"

/*! uncomment this line for using capactive detection */
#define PHASE_DETECT

/*! wake-up-timer range				0 = 100ms, 1 = 10ms */
#define PHASE_DETECT_WUR			0

/*! typical wake-up time (see datasheet, page 108, figure 101: typical wakeup time) */
#define PHASE_DETECT_WUT0			0
#define PHASE_DETECT_WUT1			0
#define PHASE_DETECT_WUT2			1

/* If enabled a beep will be triggered at every wake-up */
//#define DBG_BEEP_ON_WAKE_UP 

/*! \brief Number of iterations used to check door-state */
#define DOOR_NUMBER_OF_CHECKS 10

/*! \brief Timeout (in milliseconds) between door-state checks */
#define DOOR_ITERATION_TIMEOUT_MS 1

/*! \brief Timeout after door-interrupt until interrupts are activated again */
#define DOOR_PULL_UP_TIMEOUT_TICKS 10

/*! \brief main application state of this application. */
static enum MainState current_state = MSTATE_INITIALIZE;

/*! \brief This variable will be set, if a learn interrupt has been registered */
static volatile u8 learn = 0;

/*! \brief This variable will be set, if a door-change interrupt has been registered */
static volatile u8 door_changed = 0;

/*! \brief This variable will be set, if an capacitive wake up interrupt has been registered */
static u8 wcap_intr = 0;

/*! \brief This variable will be set, if an RTC interrupt has been registered */
static u8 rtc_intr = 0;

//mu 08.10.2017
/*! \brief This variable will count the bad detections from the RFID-Chip, if the value is higher then 5 --> Delay for 2 seconds (for the battery) */
static u8 bad_detection_counter = 0;
/*! \brief This define sets the max. allowed wrong RFID detections before the timer will be started */
#define Bad_Detection_Counter_Execute_Value 3 /*! \G?ther 18.12.2017: ich starte die Verz?erung bereits nach 3 Leseversuchen*/
/*! \brief This define sets the time delay which will block the code execution to save some energy */
#define Bad_Detection_Counter_Execute_Time_ms 2000

/*! \brief Holds last scan result (RFID card) */
struct scan_result_s scan_result;

#define MAIN_STATE_TRANSITION(next_state) do {								\
	current_state = next_state;									\
} while (0)

/*!
 * \brief Disables capacitive wake-up (and disables operations and interrupts of AS3911)
 * \note This function resets all operation control bits and all enabled interrupts, so it's
 *       name might be a little miss-leading
 */
static inline void disable_wakeup (void)
{
	as3911WriteRegister(AS3911_REG_OP_CONTROL, 0);
	as3911DisableInterrupts(AS3911_IRQ_MASK_ALL);
	as3911ClearInterrupts ();

	wcap_intr = 0;
}

/*!
 * \brief Enables learn interrupt
 */
static inline void enable_learn_interrupt (void)
{
	ioport_set_pin_sense_mode (SW_LEARN, IOPORT_SENSE_FALLING);

	IRQ_INC_DISABLE();
	LEARN_INTR_PORT.INTFLAGS |= PORT_INT0IF_bm;
	LEARN_INTR_PORT.INT0MASK |= LEARN_INTR_PIN_bm;	/* learn */
	LEARN_INTR_PORT.INTCTRL  |= (PORT_INT0LVL_LO_gc);
	IRQ_DEC_ENABLE();
}

/*!
 * \brief Disables and resets learn interrupt
 */
static inline void disable_learn_interrupt (void)
{
	IRQ_INC_DISABLE();
	LEARN_INTR_PORT.INT0MASK &= ~LEARN_INTR_PIN_bm;
	LEARN_INTR_PORT.INTFLAGS |= PORT_INT0IF_bm;
	IRQ_DEC_ENABLE();

	learn = 0;
}

/*!
 * \brief Callback function to check cards
 * \param unitId ID of unit which should be used to scan.
 * \param scanId not used.
 * \param result The card that has been found.
 * \return Always ERR_NONE.
 */
Status IdentifyCardCallback (UnitId unitId, byte scanId, void *result)
{
	static u8 counter = 0;
	u8 i;
	iso14443AProximityCard_t *card = result;
	struct card *c;

	DLOG("\r\n  > Found RFID tag [unit=%hhu, count=%hhx]: ", unitId, counter++);

	DLOG("%02hhx", card->uid[0]);
	for (i = 1; i < card->actlength; ++i) {
		DLOG(":%02hhx", card->uid[i]);
	}
	DLOG ("\r\n");

	cardCopy (card->uid, card->actlength, &scan_result.card);
	scan_result.type = cardmanGetCardType (card->uid, card->actlength, &c);
	scan_result.extra = c->extra[0];

	if (scan_result.found_card_counter < 0xff)	/* Do not overflow! */
		++scan_result.found_card_counter;

	return ERR_NONE;
}

/*! \brief Interrupt used to register that the learn button has been pressed/released */
ISR(PORTB_INT0_vect)
{
	learn = 1;
}

enum TriggerSource {
	TRIGGER_SOURCE_SW_OPEN,
	TRIGGER_SOURCE_SW_CLOSED
};

static volatile enum {
	DOOR_STATE_UNKNOWN,
	DOOR_STATE_OPEN,
	DOOR_STATE_CLOSED
}s_door_state = DOOR_STATE_UNKNOWN;

/************************************************************************/
/*                                                                      */
/************************************************************************/
static int8_t door_intr_activate_open_cb (uint8_t status)
{
	DOOR_OPEN_INTR_PORT.INT1MASK |= DOOR_OPEN_INTR_PIN_bm;
	return 0;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static int8_t door_intr_activate_closed_cb (uint8_t status)
{
	DOOR_CLOSE_INTR_PORT.INT1MASK |= DOOR_CLOSE_INTR_PIN_bm;
	return 0;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static void door_intr_change_trigger_locked(enum TriggerSource trigger)
{
	DOOR_OPEN_INTR_PORT.INT1MASK &= ~(DOOR_OPEN_INTR_PIN_bm);
	DOOR_CLOSE_INTR_PORT.INT1MASK &= ~(DOOR_CLOSE_INTR_PIN_bm);

	switch (trigger) {
	case TRIGGER_SOURCE_SW_OPEN:
		ioport_configure_pin (SW_DOOR_CLOSED, IOPORT_DIR_INPUT | IOPORT_PULL_DOWN);
		ioport_configure_pin (SW_DOOR_OPENED, IOPORT_DIR_INPUT | IOPORT_PULL_UP);
		DOOR_OPEN_INTR_PORT.INT1MASK |= DOOR_OPEN_INTR_PIN_bm;
		s_door_state = DOOR_STATE_CLOSED;
		rsched_reschedule(DOOR_PULL_UP_TIMEOUT_TICKS, door_intr_activate_open_cb);
		break;

	case TRIGGER_SOURCE_SW_CLOSED:
		ioport_configure_pin (SW_DOOR_CLOSED, IOPORT_DIR_INPUT | IOPORT_PULL_UP);
		ioport_configure_pin (SW_DOOR_OPENED, IOPORT_DIR_INPUT | IOPORT_PULL_DOWN);
		s_door_state = DOOR_STATE_OPEN;
		rsched_reschedule(DOOR_PULL_UP_TIMEOUT_TICKS, door_intr_activate_closed_cb);
		break;
	}
}

/*! \brief Interrupt used to register that the door close state has changed */
ISR(PORTA_INT1_vect)
{
	door_changed = 1;
	door_intr_change_trigger_locked(TRIGGER_SOURCE_SW_OPEN);
}

/*! \brief Interrupt used to register door open state changes */
ISR(PORTE_INT1_vect)
{
	door_changed = 1;
	door_intr_change_trigger_locked(TRIGGER_SOURCE_SW_CLOSED);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
bool_t isDoorClosed (void)
{
	u8 closed = 0;
	u8 open = 0;
	u8 i;

	for (i = 0; i < DOOR_NUMBER_OF_CHECKS; ++i) {
		delayNMilliSeconds(DOOR_ITERATION_TIMEOUT_MS);

		switch (s_door_state) {
		case DOOR_STATE_OPEN:
			++open;
			break;

		case DOOR_STATE_CLOSED:
			++closed;
			break;

		default:
			break;
		}
	}

	if ((open == 0) && (closed == 0)) {
		DLOG("SW_DOOR: Unplausible door-state\r\n");
	}

	DLOG("SW_DOOR: open: %hhu, closed: %hhu\r\n", open, closed);

	/* Assume _closed_ if open and close numbers are equal */
	return (closed >= open);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static bool_t is_function_control_enabled (void)
{
	return (cardmanGetNumberOfKeys() == 0);
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static void door_intr_print_state (void)
{
	switch (s_door_state) {
	case DOOR_STATE_OPEN:
		DLOG("SW_DOOR: open\r\n");
		break;

	case DOOR_STATE_CLOSED:
		DLOG("SW_DOOR: closed\r\n");
		break;

	case DOOR_STATE_UNKNOWN:
		DLOG("SW_DOOR: unknown\r\n");
		break;
	default:
		DLOG("SW_DOOR: Unspecified state\r\n");
		break;
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static void preprocess_door_intr (void)
{
	door_intr_print_state();
	rsched_wait_pending_locked();
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
static void postprocess_door_intr (void)
{
	door_intr_print_state();
}


/************************************************************************/
/*                                                                      */
/************************************************************************/
void MainStateMachine (void)
{
	static u32 woke_counter = U32_C(0);
	s8 err;
	u8 val;
	u32 ret;

	u8 sw = cardmanGetSoftwareFunction();
	
	switch (current_state) {
	case MSTATE_INITIALIZE:
		/* enable door interrupt */
		//ioport_set_pin_sense_mode (SW_DOOR_CLOSED, IOPORT_SENSE_BOTHEDGES);
			/* door */
		DOOR_CLOSE_INTR_PORT.INTCTRL = PORT_INT1LVL_LO_gc;
		DOOR_OPEN_INTR_PORT.INTCTRL = PORT_INT1LVL_LO_gc;

		door_intr_change_trigger_locked(TRIGGER_SOURCE_SW_CLOSED);

		SoftwareStateMachine(SW_TRIGGER_INIT_SOFTWARE);
		MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
		break;

	case MSTATE_PREPARE_WAKE_UP:
		enable_learn_interrupt ();
		as3911DisableInterrupts(AS3911_IRQ_MASK_ALL);
		as3911ClearInterrupts ();
		
		
		
		
		
#ifdef PHASE_DETECT		

		as3911ExecuteCommandAndGetResult (AS3911_CMD_MEASURE_PHASE, AS3911_REG_AD_RESULT, 100, &val);

		DLOG("Phase: %hu\r\n", (u16)val);
		
		uint8_t wup_timer_reg = 0x00	| AS3911_REG_WUP_TIMER_CONTROL_wph | (PHASE_DETECT_WUR << 7)
										| (PHASE_DETECT_WUT2 << 6) | (PHASE_DETECT_WUT1 << 5)
										| (PHASE_DETECT_WUT0 << 4);
		
				
		as3911WriteRegister (AS3911_REG_PHASE_MEASURE_REF, val);
		as3911WriteRegister (AS3911_REG_PHASE_MEASURE_CONF, 0b00111001); //0b00111001
		
		as3911WriteRegister (AS3911_REG_WUP_TIMER_CONTROL, wup_timer_reg);
		as3911WriteRegister (AS3911_REG_OP_CONTROL, AS3911_REG_OP_CONTROL_wu);
		as3911ClearInterrupts ();
		as3911EnableInterrupts(AS3911_IRQ_MASK_WPH);
		
#else

		as3911WriteRegister(AS3911_REG_CAP_SENSOR_CONTROL, 0x01); /* maximal gain, auto calibration */

		as3911ExecuteCommandAndGetResult(AS3911_CMD_CALIBRATE_C_SENSOR,
		AS3911_REG_CAP_SENSOR_RESULT, 255, &val);

		as3911ExecuteCommandAndGetResult (AS3911_CMD_MEASURE_CAPACITANCE,
		AS3911_REG_AD_RESULT, 100, &val);

		DLOG("Capacity: %hu\r\n", (u16)val);

		as3911WriteRegister (AS3911_REG_CAPACITANCE_MEASURE_REF, val);
		as3911WriteRegister (AS3911_REG_CAPACITANCE_MEASURE_CONF, 0b00111001); /* cm_ae, cm_aew0, cm_aam, cm_d0 */
		as3911WriteRegister (AS3911_REG_WUP_TIMER_CONTROL,0b11000000 | AS3911_REG_WUP_TIMER_CONTROL_wcap ); /* 50 ms */
		as3911WriteRegister (AS3911_REG_OP_CONTROL, AS3911_REG_OP_CONTROL_wu);
		as3911ClearInterrupts ();
		as3911EnableInterrupts(AS3911_IRQ_MASK_WCAP);

#endif
		buzzerStop(); /* Make sure, buzzer is off */
		MAIN_STATE_TRANSITION(MSTATE_DEEP_SLEEP);
		break;

	case MSTATE_DEEP_SLEEP:
		spiPause ();

		SoftwareCheckAlarm();

		cpu_irq_disable();
		
	//TESTEN UM ENERGIE zu sparen!!!
		// mu 16.10.2017
		
		if(bad_detection_counter >= Bad_Detection_Counter_Execute_Value)
		{
			rtcStartINTR(Bad_Detection_Counter_Execute_Time_ms / 1000);
			
			while(!rtcGetInterrupt())
			{
				SoftwareSleep();
			}

			if(cardmanGetNumberOfKeys() == 0)
			{
				buzzerStart(SignalWakeUp, false);
				buzzerWaitTillFinished();
				delayNMilliSeconds(70);
			}
			//buzzerStart(SignalWakeUp, false);
			//buzzerWaitTillFinished();
			//delayNMilliSeconds(70);
		}
		

		preprocess_door_intr();
		DLOG("SLEEP\r\n");
		if ((learn == 0) && (door_changed == 0) && (rtc_intr == 0) &&
			(wcap_intr == 0) && (!rtcPollInterrupt_LOCKED())) {
			SoftwareSleep ();

			//mu 08.10.2017
			//IF COUNTER >= 5 sleep 2 Sekunden. (TODO: go to deep Sleep and wake up only by a timer)
			//if(bad_detection_counter >= Bad_Detection_Counter_Execute_Value)
			//{
				//delayNMilliSeconds(2000); //Bad_Detection_Counter_Execute_Time_ms);
			//}

			postprocess_door_intr();

			if (woke_counter < U32_C(0xffffffff))
				++woke_counter;
#ifdef DBG_BEEP_ON_WAKE_UP
			if (is_function_control_enabled()) {
				buzzerStart(SignalWakeUp, false);
				buzzerWaitTillFinished();
				delayNMilliSeconds(70);
			}
#endif

			spiReinitialize ();
			uartInitialize (115200, NULL);
			DLOG("\r\n====> Wake-Up Counter: %lu\r\n", woke_counter);
		} else {
			cpu_irq_enable(); /* Turn on IRQ's again */
		}

		if (rtcGetInterrupt ())
			rtc_intr = 1;

#ifdef PHASE_DETECT 
		ret = as3911GetInterrupt (AS3911_IRQ_MASK_WPH);
		disable_wakeup ();

		if (ret & AS3911_IRQ_MASK_WPH)
		wcap_intr = 1;			
#else
		ret = as3911GetInterrupt (AS3911_IRQ_MASK_WCAP);
		disable_wakeup ();

		if (ret & AS3911_IRQ_MASK_WCAP)
			wcap_intr = 1;
			

#endif 

		
		
		if (learn) {
			disable_learn_interrupt ();
			MAIN_STATE_TRANSITION(MSTATE_ENTER_LEARN_MODE);
		} else if (door_changed) {
			door_changed = 0;
			disable_learn_interrupt ();
			MAIN_STATE_TRANSITION(MSTATE_ENTER_SOFTWARE_DOOR);
		} else if (rtc_intr) {
			rtc_intr = 0;
			MAIN_STATE_TRANSITION(MSTATE_ENTER_SOFTWARE_RTC);
		} else if (wcap_intr) {
			wcap_intr = 0;
			MAIN_STATE_TRANSITION(MSTATE_WOKE_UP);
		} else {
			MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
		}
		break;

	case MSTATE_ENTER_LEARN_MODE:
		buzzerStart(SignalModeFast, true);
		DLOG("Learn Mode\r\n");
		rtcStart (TIMEOUT_RTC_LEARN_MODE);
		MAIN_STATE_TRANSITION(MSTATE_WAIT_LEARN_MODE);
		break;

	case MSTATE_WAIT_LEARN_MODE:
		if (rtcIsFinished ()) {
			buzzerStop ();
			delayNMilliSeconds (500);
			MAIN_STATE_TRANSITION(MSTATE_EXIT_PROG_OR_LEARN_MODE);
		}
		scan_result.found_card_counter = 0;
		err = RfidStartScan (RFID_UNIT_1, 0, IdentifyCardCallback);
		if (scan_result.found_card_counter == 1) {
			if (scan_result.type == CARD_TYPE_GYM_SOFTWARD_CARD) {
				DLOG("GYM mode card NO-AUTO\r\n");

				////////////////////////
				val = rtcXSecondsPassed(10) ? 1 : 0;
				if (val) { /* Wait 2 seconds before changing state... */
					buzzerStop();
					rtcStop ();
					DLOG("Delete all keys!\r\n");
					cardmanDeleteAllKeys ();
					buzzerStart(SignalAllDeleted, false);
					buzzerWaitTillFinished ();
					delayNMilliSeconds(200);
					SoftwareStateMachine(SW_TRIGGER_CHANGE_NCARDS);
				}
				////////////////////////
				cardmanSetSoftwareFunctionOpenDelay(SW_FUNCTION_GYM, 0);
				buzzerStart(SignalAllDeleted, false);
				buzzerWaitTillFinished();
				CCP = CCP_IOREG_gc;
				RST.CTRL =  RST_SWRST_bm;
				for(;;);
			} else {
				buzzerStop ();
				rtcStop ();
				
				if (scan_result.type == CARD_TYPE_GYM_SOFTWARD_CARD) {
					
				} else if (scan_result.type == CARD_TYPE_GYM_SOFTWARD_CARD_6) {
					
					DLOG("GYM mode card 6 hours\r\n");
					cardmanSetSoftwareFunctionOpenDelay(SW_FUNCTION_GYM, 6);
					buzzerStart(SignalAllDeleted, false);
					buzzerWaitTillFinished();
					CCP = CCP_IOREG_gc;
					RST.CTRL =  RST_SWRST_bm;
					for(;;);
				} else if (scan_result.type == CARD_TYPE_GYM_SOFTWARD_CARD_12) {
					
					DLOG("GYM mode card 12 hours\r\n");
					cardmanSetSoftwareFunctionOpenDelay(SW_FUNCTION_GYM, 12);
					buzzerStart(SignalAllDeleted, false);
					buzzerWaitTillFinished();
					CCP = CCP_IOREG_gc;
					RST.CTRL =  RST_SWRST_bm;
					for(;;);
				} else if (scan_result.type == CARD_TYPE_GYM_SOFTWARD_CARD_24) {
				
					DLOG("GYM mode card 24 hours\r\n");
					cardmanSetSoftwareFunctionOpenDelay(SW_FUNCTION_GYM, 24);
					buzzerStart(SignalAllDeleted, false);
					buzzerWaitTillFinished();
					CCP = CCP_IOREG_gc;
					RST.CTRL =  RST_SWRST_bm;
					for(;;);

					
				} else  {
				
					err = cardmanSetProgrammingCard (scan_result.card.uid, scan_result.card.len);
					if (err) {
						DLOG("Couldn't set Programming card [err: %hhd]\r\n", err);
						delayNMilliSeconds (500);
						MAIN_STATE_TRANSITION(MSTATE_ERROR_SET_PROG_CARD_FAILED);
					} else {
						DLOG("Programming card has been set!\r\n");
						delayNMilliSeconds (500);
						buzzerStart (SignalPositive, false);
						buzzerWaitTillFinished ();
						MAIN_STATE_TRANSITION(MSTATE_EXIT_PROG_OR_LEARN_MODE);
					}
				
				}
			}
		}
		break;

	case MSTATE_WOKE_UP:
		scan_result.found_card_counter = 0;
		delayNMilliSeconds (10);
		err = RfidStartScan (RFID_UNIT_1, 0, IdentifyCardCallback);
		if (scan_result.found_card_counter == 0) {
			if (learn == 1) {
				disable_learn_interrupt ();
				disable_wakeup ();
				MAIN_STATE_TRANSITION(MSTATE_ENTER_LEARN_MODE);
			} else {
				MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
				if(bad_detection_counter <= Bad_Detection_Counter_Execute_Value) bad_detection_counter++;	//mu 08.10.2017
			}
		} else if (scan_result.found_card_counter == 1) {
			bad_detection_counter = 0;	//mu 08.10.2017
			disable_learn_interrupt ();

			if (sw & SW_FUNCTION_GYM && scan_result.type != CARD_TYPE_SOFTWARE_CARD) {
							door_intr_activate_open_cb(1);
							door_intr_activate_closed_cb(1);
							
				gym_card_callback(&scan_result);
				MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
				
			} else {

				switch (scan_result.type) {
					case CARD_TYPE_KEY:
						DLOG("It's a key [page: %hhd]!\r\n", scan_result.extra);
						MAIN_STATE_TRANSITION(MSTATE_ENTER_SOFTWARE_KEY);
						break;

					case CARD_TYPE_PROGRAMMING_CARD:
						DLOG("It's a programming card [used: %hhd]!\r\n",scan_result.extra);
						MAIN_STATE_TRANSITION(MSTATE_ENTER_PROGRAMMING_MODE_1);
						break;

					case CARD_TYPE_SOFTWARE_CARD:
					case CARD_TYPE_GYM_SOFTWARD_CARD:
						DLOG ("It's a software card [sw-function: %hhd]!\r\n", scan_result.extra);

						buzzerStart (SignalSetSoftware, false);
						buzzerWaitTillFinished ();
						SoftwareStateMachine (SW_TRIGGER_DEINIT_SOFTWARE);
						cardmanSetSoftwareFunction (scan_result.extra);
						DLOG("Change software version:\r\n");
						SoftwarePrintVersion();
						SoftwareStateMachine (SW_TRIGGER_INIT_SOFTWARE);
						MAIN_STATE_TRANSITION(MSTATE_EXIT_PROG_OR_LEARN_MODE);

						//MAIN_STATE_TRANSITION(MSTATE_WAIT_PROG_MODE_1);
						break;
					
					case CARD_TYPE_UNKNOWN:
						MAIN_STATE_TRANSITION(MSTATE_FOUND_UNKNOWN_CARD);
						break;
				}
			}
		} else {
			disable_learn_interrupt ();
			MAIN_STATE_TRANSITION(MSTATE_ERROR_MULTIPLE_CARDS);
		}
		break;

	case MSTATE_ENTER_SOFTWARE_KEY: {
		
		buzzerStart(SignalPositive, false);
		buzzerWaitTillFinished();
		SoftwareStateMachine (SW_TRIGGER_KEY);

		MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
		break;
	}

	case MSTATE_ENTER_SOFTWARE_RTC:
		SoftwareStateMachine (SW_TRIGGER_RTC);
		gym_rtc_callback();
		MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
		break;

	case MSTATE_ENTER_SOFTWARE_DOOR:
		if (is_function_control_enabled()) {
			if (isDoorClosed()) {
				buzzerStart(SignalTestOn, false);
			} else {
				buzzerStart(SignalTestOff, false);
			}
			buzzerWaitTillFinished();
		}
		SoftwareStateMachine (SW_TRIGGER_DOOR);
		MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
		break;

	case MSTATE_FOUND_UNKNOWN_CARD:
		DLOG("I don't know this card!\r\n");
		buzzerStart (SignalNegative, false);
		buzzerWaitTillFinished ();
		MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
		break;

	case MSTATE_ENTER_PROGRAMMING_MODE_2:
		rtcStart (TIMEOUT_RTC_DELETE_ALL);
		buzzerStart(SignalModeSlow, true);
		MAIN_STATE_TRANSITION(MSTATE_WAIT_PROG_MODE_2);
		break;

	case MSTATE_ENTER_PROGRAMMING_MODE_1:
		rtcStart (TIMEOUT_RTC_PROG_MODE_1);
		buzzerStart (SignalModeFast, true);
		MAIN_STATE_TRANSITION(MSTATE_WAIT_PROG_MODE_1);
		break;

	case MSTATE_WAIT_PROG_MODE_1:
		scan_result.found_card_counter = 0;
		err = RfidStartScan (RFID_UNIT_1, 0, IdentifyCardCallback);
		if (scan_result.found_card_counter == 1) {
			val = rtcXSecondsPassed(TIMEOUT_RTC_SWITCH_TO_PROG2) ? 1 : 0;

			switch (scan_result.type) {
			case CARD_TYPE_PROGRAMMING_CARD:
				if (val) { /* Wait 2 seconds before changing state... */
					rtcStop ();
					MAIN_STATE_TRANSITION(MSTATE_ENTER_PROGRAMMING_MODE_2);
				}
				break;

			case CARD_TYPE_KEY:
				DLOG("Delete card %hhd\r\n", scan_result.extra);
				cardmanDeleteKey (scan_result.card.uid, scan_result.card.len);
				buzzerStart (SignalNegative, false);
				buzzerWaitTillFinished ();
				SoftwareStateMachine(SW_TRIGGER_CHANGE_NCARDS);
				MAIN_STATE_TRANSITION(MSTATE_EXIT_PROG_OR_LEARN_MODE);
				break;

			case CARD_TYPE_UNKNOWN:
				DLOG("Add card!\r\n");
				cardmanAddKey (scan_result.card.uid, scan_result.card.len);
				buzzerStart (SignalPositive, false);
				buzzerWaitTillFinished ();
				SoftwareStateMachine(SW_TRIGGER_CHANGE_NCARDS);
				MAIN_STATE_TRANSITION(MSTATE_EXIT_PROG_OR_LEARN_MODE);
				break;

			case CARD_TYPE_SOFTWARE_CARD:
				buzzerStart (SignalSetSoftware, false);
				buzzerWaitTillFinished ();
				SoftwareStateMachine (SW_TRIGGER_DEINIT_SOFTWARE);
				cardmanSetSoftwareFunction (scan_result.extra);
				DLOG("Change software version:\r\n");
				SoftwarePrintVersion();
				SoftwareStateMachine (SW_TRIGGER_INIT_SOFTWARE);
				MAIN_STATE_TRANSITION(MSTATE_EXIT_PROG_OR_LEARN_MODE);
				break;

			default:
				DLOG("Illegal card type %hhx\r\n", (u8)scan_result.type);
				MAIN_STATE_TRANSITION(MSTATE_EXIT_PROG_OR_LEARN_MODE);
				break;
			}
		} else if (scan_result.found_card_counter == 0) {
			if (rtcIsFinished ()) {
				buzzerStop ();
				rtcStop ();
				MAIN_STATE_TRANSITION(MSTATE_EXIT_PROG_OR_LEARN_MODE);
			}
		} else {
			buzzerStop();			
			rtcStop();
			MAIN_STATE_TRANSITION(MSTATE_ERROR_MULTIPLE_CARDS);
		}
		break;

	case MSTATE_WAIT_PROG_MODE_2:
		scan_result.found_card_counter = 0;
		err = RfidStartScan (RFID_UNIT_1, 0, IdentifyCardCallback);
		if (scan_result.found_card_counter == 1) {
			switch (scan_result.type) {
			case CARD_TYPE_PROGRAMMING_CARD:
				if (rtcIsFinished ()) {
					// buzzerStop();
					// DLOG("Delete all keys!\r\n");
					// cardmanDeleteAllKeys ();
					// buzzerStart (SignalAllDeleted, false);
					// buzzerWaitTillFinished ();
					// delayNMilliSeconds(200);
					// SoftwareStateMachine(SW_TRIGGER_CHANGE_NCARDS);
					MAIN_STATE_TRANSITION(MSTATE_EXIT_PROG_OR_LEARN_MODE);
				}
				break;

			case CARD_TYPE_KEY:
			case CARD_TYPE_UNKNOWN:
			case CARD_TYPE_SOFTWARE_CARD:
			case CARD_TYPE_GYM_SOFTWARD_CARD:
			default:
				MAIN_STATE_TRANSITION(MSTATE_ENTER_PROGRAMMING_MODE_1);
				break;
			}
		} else {
			buzzerStop ();			
			rtcStop ();
			MAIN_STATE_TRANSITION(MSTATE_ENTER_PROGRAMMING_MODE_1);
		}
		break;

	case MSTATE_ERROR_MULTIPLE_CARDS:
		DLOG("Multiple Cards\r\n");
	case MSTATE_EXIT_PROG_OR_LEARN_MODE:
		buzzerStop();
		rtcStop();
		SoftwareRestartAlarm();
		MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
		break;

	case MSTATE_ERROR_SET_PROG_CARD_FAILED:
		DLOG("Couldn't set Programming Card\r\n");
		buzzerStart(SignalError, false);
		buzzerWaitTillFinished ();
		rtcStop();
		SoftwareRestartAlarm();
		MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
		break;

	default:
		DLOG("Unexpected state %hhu\r\n", current_state);
		MAIN_STATE_TRANSITION(MSTATE_PREPARE_WAKE_UP);
	}
}