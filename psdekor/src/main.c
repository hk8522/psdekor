/**
 * \file
 *
 * \brief PS2-101 PsDekor - application entry point
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
#include <asf.h>
#include "as3911.h"
#include "as3911_com.h"
#include "as3911_interrupt.h"
#include "iso14443a.h"
#include "clock.h"
#include "ic.h"
#include "uart.h"
#include "delay_wrapper.h"
#include "sorex_hal/Communication/Rfid.h"
#include "utils/debug.h"
#include "utils/reschedule.h"
#include "buzzer/sounds.h"
#include "cardman/cards_manager.h"
#include "application/application.h"
#include "application/rtc_timeout.h"
#include "application/software_functions.h"

/*! \brief Firmware Version string (will be written to UART while initializing) */
#define FIRMWARE_VERSION_STR "v3.3.1"
/*!
 * \brief Short timeout before initializing/calibrating the RFID unit
 * Has been introduced to get better calibration results...
 */
#define STARTUP_WAIT_TIME_MS 1500


/************************************************************************/
/* GLOBAL VARIABLES                                                     */
/************************************************************************/

volatile umword IRQ_COUNT = 0;


/************************************************************************/
/* LOCAL FUNCTION PROTOTYPES                                            */
/************************************************************************/

/*! \brief Initialize main application
 */
static void AppInitialize (void);

static s8 calibrate_antenna (void);

static s8 switch_rffield (s8 on);

static void dump_phase (void);

/************************************************************************/
/* IMPLEMENTATION                                                       */
/************************************************************************/

#define ERR_CALIBRATION -100

/*!
 * \brief Switches on the RF field
 * \param on Indicates if the RF field shall be turned on or off.
 * \return 0 if there was no error, else returns the error code.
 */
static s8 switch_rffield (s8 on)
{
	s8 err;
	if (on) {
		err = as3911ModifyRegister (AS3911_REG_OP_CONTROL,
		                            0,
		                            AS3911_REG_OP_CONTROL_en);
	} else {
		err = as3911ModifyRegister (AS3911_REG_OP_CONTROL,
		                            AS3911_REG_OP_CONTROL_en,
		                            0);
	}

	return err;
}

/*!
 * \brief Debug function which measures the current phase
 * A value of 255 indicates that phase shift is less than 30°.
 * A value of 0 indicates that phase shift is bigger than 150°
 * All other values can be converted to degree by following
 * formular: (((255 - result) * 2.0)/(255 * 3)+(1/6.0)) * 180
 */
static void dump_phase (void)
{
	u8 result;

	switch_rffield (1);
	delayNMilliSeconds (500);
	as3911MeasureAntennaResonance (&result);

	DLOG("Phase %hhd\r\n", result);
	switch_rffield (0);
}

/*!
 * \brief Debug function - calibrates antenna and displays trim config
 * \return 0 if calibration was successfull, else an error-code will be
 *         returned. ERR_CALIBRATION means that the calibration command
 *         of the AS3911 has reported that calibration failed.
 */
static s8 calibrate_antenna (void)
{
	s8 err;
	u8 result;
	u8 trim;
	u8 trim_err;
	u8 trim_cap;
	err = as3911ModifyRegister(AS3911_REG_ANT_CAL_CONTROL,
	                           AS3911_REG_ANT_CAL_CONTROL_trim_s,
	                           0x0);
	if (ERR_NONE == err)
	{
		err = as3911CalibrateAntenna(&result);
		trim = (result & (AS3911_REG_ANT_CAL_RESULT_tri_0 |
		                  AS3911_REG_ANT_CAL_RESULT_tri_1 |
		                  AS3911_REG_ANT_CAL_RESULT_tri_2 |
		                  AS3911_REG_ANT_CAL_RESULT_tri_3)) >> 4;
		trim_err = (result & AS3911_REG_ANT_CAL_RESULT_tri_err) >> 3;

		DLOG("\r\nCalibrate Antenna returned %hhx -> trim: %hhx, trim_err: %hhx\r\n",
		     result, trim, trim_err);

		trim_cap = 0;
		if (trim & 0x1)
			trim_cap += 6;
		if (trim & 0x2)
			trim_cap += 12;
		if (trim & 0x4)
			trim_cap += 22;
		if (trim & 0x8)
			trim_cap += 47;

		DLOG("Trim-Cap: %hhd pf\r\n", trim_cap);

		if (trim_err)
			err = ERR_CALIBRATION;
	}

	return err;
}

static void dump_all_regs (void)
{
	const u8 cols = 8;
	const u8 rows = 8;

	u8 i = 0;
	u8 j;
	u8 idx;
	u8 result;
	s8 err;

	DLOG("READ Registers:\r\n");
	for (i = 0; i < rows; ++i) {
		for (j = 0; j < cols; ++j) {
			idx = j * rows + i;
			if (idx >= 0x40)
				goto out;

			err = as3911ReadRegister (idx, &result);
			if (!err) {
				DLOG("'%02hhx': %02hhx\t", idx, result);
			} else {
				DLOG("\r\nError reading register %02hhx", idx);
				goto out;
			}
		}
		DLOG("\r\n");
	}

out:
	DLOG("\r\n");
}

/*!
 * \brief Initializes the application
 * All initial initialization is done here!
 */
static void AppInitialize (void)
{
	bool_t init_succeeded = true;
	s8 err;

	board_init();
	clkInitialize();
	icInitialize();
	uartInitialize(115200, NULL);
	delayInitialize();
	err = rsched_init(TC_CLKSEL_DIV1024_gc);
	DLOG("[rsched] Initialize returned %hhd\r\n", err);

	err = SoundsInitialize();
	DLOG ("[buzzer] Initialize returned %hhd\r\n", err);

	err = cardmanInitialize();
	DLOG("[cardman] init returned %hhd\r\n", err);

	DLOG("Firmware Version " FIRMWARE_VERSION_STR "\r\n");
	SoftwarePrintVersion();

	err = RfidInitialize(RFID_UNIT_1, NULL);
	if (err) {
		DLOG("Rfid Initialization FAILED [err:%hhd]\r\n", err);
		buzzerStart(SignalError, false);
		buzzerWaitTillFinished();
		init_succeeded = false;
	} else {
		DLOG("Rfid Initialization DONE\r\n");
	}

	err = calibrate_antenna();
	if (err) {
		buzzerStart(SignalError, false);
		buzzerWaitTillFinished();
		init_succeeded = false;
	}

	if (init_succeeded) {
		buzzerStart(SignalHello, false);
		buzzerWaitTillFinished();
	}

	delayNMilliSeconds(STARTUP_WAIT_TIME_MS);

	calibrate_antenna();

	dump_phase();
}

static Status TestReadCallback (UnitId unitId, byte scanId, void *result)
{
	static u8 counter = 0;
	u8 i;
	iso14443AProximityCard_t *card = result;

	buzzerStart (SignalWakeUp, false);
	buzzerWaitTillFinished ();
	DLOG("\r\n  > Found RFID tag [unit=%hhu, count=%hhx]: ", unitId, counter++);

	DLOG("%02hhx", card->uid[0]);
	for (i = 1; i < card->actlength; ++i) {
		DLOG(":%02hhx", card->uid[i]);
	}
	DLOG ("\r\n");

	return ERR_NONE;
}

static enum test_state {
	TEST_INIT,
	TEST_MEASURE_ALL,
	TEST_MEASURE_CAPACITY,
	TEST_MEASURE_INDUCTANCE,
	TEST_TRY_READ,
	EXIT_TEST_MODE
} g_state = TEST_INIT;

static u8 ref_rf_amplitude = 0;
static u8 ref_rf_phase = 0;
static u8 ref_capacity = 0;

static u8 g_card_present = 0;

static void inductive_measure (u8 *measured_amplitude, u8 *measured_phase)
{
	u16 phase_avg = U16_C(0);
	u16 amplitude_avg = U16_C(0);
	u8 phase;
	u8 amplitude;

	as3911WriteRegister (AS3911_REG_OP_CONTROL, AS3911_REG_OP_CONTROL_en);
	for (u8 i = 0; i < 255; ++i)
	{
		as3911MeasureAntennaResonance (&phase);
		as3911MeasureRF (&amplitude);
		phase_avg += phase;
		amplitude_avg += amplitude;
	}
	phase = phase_avg >> 8;
	amplitude = amplitude_avg >> 8;
	DLOG("    \r\n******* ANTENNA Measurement ********\r\n");
	DLOG("[debug] Antenna Phase: %hhu\r\n", phase);
	DLOG("[debug] Antenna Amplitude: %hhu\r\n", amplitude);
	DLOG("[debug] Difference Phase: %hd\r\n", (s16)phase - (s16)ref_rf_phase);
	DLOG("[debug] Difference Amplitude: %hd\r\n", (s16)amplitude - (s16)ref_rf_amplitude);
	DLOG("************************************\r\n");
	as3911WriteRegister (AS3911_REG_OP_CONTROL, 0x0);

	if (measured_amplitude != NULL)
		*measured_amplitude = amplitude;

	if (measured_phase != NULL)
		*measured_phase = phase;
}

static void capacitiv_measure (u8 *measured_capacity)
{
	u8 value;

	as3911ExecuteCommandAndGetResult (AS3911_CMD_MEASURE_CAPACITANCE,
	AS3911_REG_AD_RESULT, 100, &value);
	DLOG("    \r\n******* Capacity Measurement ********\r\n");
	DLOG("[debug] Capacity  %hhu\r\n", value);
	DLOG("[debug] Difference Capacity: %hd\r\n", (s16)value - (s16)ref_capacity);
	DLOG("************************************\r\n");

	if (measured_capacity != NULL)
		*measured_capacity = value;
}

static Status CheckCardCallback (UnitId unitId, byte scanId, void *result)
{
	switch (scanId) {
	case 0:
		buzzerStart ("iiiiiiaaaaaa", false);
		buzzerWaitTillFinished ();
		
		DLOG("Enter PsDekor Application!\n");
		g_state = EXIT_TEST_MODE;
		break;

	case 1:
		g_card_present = 1;
		break;
	}
	

	return ERR_NONE;
}

static void measure_all (void)
{
	static u8 counter = 0;
	u16 phase_avg = U16_C(0);
	u16 amplitude_avg = U16_C(0);
	u8 phase;
	u8 amplitude;
	u8 capacity;

	as3911ExecuteCommandAndGetResult (AS3911_CMD_MEASURE_CAPACITANCE,
					  AS3911_REG_AD_RESULT, 100, &capacity);

	as3911WriteRegister (AS3911_REG_OP_CONTROL, AS3911_REG_OP_CONTROL_en);
	for (u8 i = 0; i < 255; ++i)
	{
		as3911MeasureAntennaResonance (&phase);
		as3911MeasureRF (&amplitude);
		phase_avg += phase;
		amplitude_avg += amplitude;
	}
	as3911WriteRegister (AS3911_REG_OP_CONTROL, 0x0);

	g_card_present = 0;
	RfidStartScan (RFID_UNIT_1, 1, CheckCardCallback);

	phase = phase_avg >> 8;
	amplitude = amplitude_avg >> 8;
	if (g_card_present) {
		DLOG("    \r\n****[%3d]*** Measure ALL ***[CARD]***\r\n", counter);
	} else {
		DLOG("    \r\n****[%3d]*** Measure ALL ************\r\n", counter);
	}
	DLOG("[debug] Antenna Phase: %hhu\r\n", phase);
	DLOG("[debug] Antenna Amplitude: %hhu\r\n", amplitude);
	DLOG("[debug] Capacity  %hhu\r\n", capacity);
	DLOG("[debug] Difference Phase: %hd\r\n", (s16)phase - (s16)ref_rf_phase);
	DLOG("[debug] Difference Amplitude: %hd\r\n", (s16)amplitude - (s16)ref_rf_amplitude);
	DLOG("[debug] Difference Capacity: %hd\r\n", (s16)capacity - (s16)ref_capacity);
	DLOG("*************************************\r\n");

	++counter;
}

static void TryReadTest (void)
{
	s8 err;

	err = RfidStartScan (RFID_UNIT_1, 0, TestReadCallback);
	if (err) {
		DLOG("RfidStartScan failed: %hhd\r\n", err);
	}

	delayNMilliSeconds(1000);
}

static void TestStateMachine (void)
{
	u8 change = 0;
	u8 val;

	if (ioport_get_pin_level(SW_LEARN) == 0)
	{
		RfidStartScan (RFID_UNIT_1, 0, CheckCardCallback);
		buzzerStart ("iaiaia", false);
		buzzerWaitTillFinished ();
		change = 1;
	}

	switch (g_state) {
	case TEST_INIT:
		as3911ExecuteCommandAndGetResult(AS3911_CMD_CALIBRATE_C_SENSOR,
		AS3911_REG_CAP_SENSOR_RESULT, 255, &val);

		/* Determine Reference: */
		capacitiv_measure (&ref_capacity);
		inductive_measure (&ref_rf_amplitude, &ref_rf_phase);

		for (val = 10; val > 0; --val) {
			delayNMilliSeconds (500);
			DLOG("                               \rWAITING %hhd seconds", (val + 1)/2);
		}
		DLOG("\r\n");

		g_state = TEST_MEASURE_ALL;
		break;

	case TEST_MEASURE_ALL:
		measure_all ();
		if (change)
			g_state = TEST_MEASURE_INDUCTANCE;
		break;

	case TEST_MEASURE_INDUCTANCE:
		inductive_measure (NULL, NULL);
		if (change)
			g_state = TEST_MEASURE_CAPACITY;
		break;

	case TEST_MEASURE_CAPACITY:
		capacitiv_measure (NULL);
		if (change)
			g_state = TEST_TRY_READ;
		break;

	case TEST_TRY_READ:
		TryReadTest ();
		if (change) {
			g_state = TEST_MEASURE_INDUCTANCE;
		}
		break;

	case EXIT_TEST_MODE:
		break;
	}
}

/*!
 * \brief main entry point
 * \return Does not return!
 */
int main (void)
{
	u8 test_mode = 0;

	/* for test-mode */
	ioport_configure_pin (SW_LEARN, IOPORT_DIR_INPUT | IOPORT_PULL_UP);

	if (ioport_get_pin_level(SW_LEARN) == 0)
		test_mode = 1;

	AppInitialize ();

	dump_all_regs ();

	sei ();

	if (test_mode) {
		buzzerStart ("iaiaiap", false);
		buzzerWaitTillFinished ();
		
		while (g_state != EXIT_TEST_MODE) {
			TestStateMachine ();
		}
	}

	while (1) {
		MainStateMachine ();
	}
}
