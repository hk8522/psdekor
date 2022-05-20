/*
 * motor_control.c
 *
 * Created: 25.11.2013 17:28:07
 *  Author: huber
 */
#include "application/motor_control.h"
#include "utils/debug.h"
#include "application/rtc_timeout.h"
#include "application/timeouts.h"
#include "buzzer/sounds.h"
#include "buzzer/buzzer.h"
#include "cardman/cards_manager.h"
#include "application/software_functions.h"


#define MOTOR_DLOG DLOG
//#define MOTOR_DLOG(...)

/* Motor + MOSFet */
static void motor_off (void)
{
	MOTOR_PORT.OUTCLR = MOTOR_PIN_bm;
}

static void motor_on (void)
{
	MOTOR_PORT.OUTSET = MOTOR_PIN_bm;
}

/*! \brief PWM on/off time (milliseconds) */
#define MOTOR_PWM_MS	                20
/*! \brief Initial on-time (milliseconds) */
#define MOTOR_START_HALF_MS	        40

/*! \brief Timeout to wait for lock-pin to change from output to input (in microseconds) */
#define LOCK_PIN_CHANGE_TIMEOUT         10

/*! ADC to measure voltage on VCC */
#define MY_ADC (&ADCA)

static volatile bool_t adc_running = false;
static volatile bool_t adc_vcc_low = false;

/*! \brief This variable is set, if the internal pull up on SW_LOCK_CLOSED is activated */
static volatile u8 lock_pin_enabled = 0;

/* DEBUG */
static volatile u16 adc_vcc_avg = 0;
static volatile u8 adc_vcc_avg_counter = 0;

/*!
 * \brief Reads calibration data for ADC channels
 * \param index adc channel index
 * \return calibration data
 *
 * Implementation from ATMEL App-Note
 */
static uint8_t ReadCalibrationByte (uint8_t index);

/*!
 * \brief adc callback method when adc conversion is finished
 * \param adc pointer to ADC configuration
 * \param ch_mask channel mask
 * \param res result of ADC conversion
 */
static void adc_callback (ADC_t *adc, uint8_t ch_mask, adc_result_t res);

/*!
 * \brief Initializes VCC measurement
 */
static void adcInitVCCMeasurement (void);

/*!
 * \brief Deinitializes ADC measurement and disabling ADC module
 */
static void adcDeinit (void);

/*!
 * \brief Starts ADC Conversions
 */
static void adcStart (void);

/*!
 * \brief Stops ADC Conversions
 */
static void adcStop (void);

/*!
 * \brief Drives motor (either open or close lock)
 * \param open_lock True if lock shall be opened, false if lock shall be closed.
 * \return ERR_VCC_LOW if VCC was low during driving the motor.
 *         ERR_TIMEOUT if RTC timeout occures (couldn't open/close lock
 *         in time).
 *         ERR_NONE if open/close was successful.
 */
static s8 drive_motor (bool_t open_lock);

/*! \brief Enables internal pull up to read state of SW_LOCK_CLOSED. */
static void enable_lock_pull_up (void)
{
	if (!lock_pin_enabled) {
		MOTOR_DLOG("MOTOR: Enable pull-up on SW_LOCK_CLOSED\r\n");
		ioport_configure_pin (SW_LOCK_CLOSED, IOPORT_DIR_INPUT | IOPORT_PULL_UP);
		delayNMicroSeconds(LOCK_PIN_CHANGE_TIMEOUT);
		lock_pin_enabled = 1;
	}
}

/*! \brief Disables internal pull up on SW_LOCK_CLOSED (to save power) */
static void disable_lock_pull_up (void)
{
	ioport_configure_pin (SW_LOCK_CLOSED, IOPORT_DIR_OUTPUT | IOPORT_INIT_LOW);

	if (lock_pin_enabled) {
		MOTOR_DLOG("MOTOR: Disable pull-up on SW_LOCK_CLOSED\r\n");
	}

	lock_pin_enabled = 0;
}

/*!
 * \brief Read SW_LOCK_CLOSED to see if motor lock is open
 * \return True if motor lock is open, else false.
 *
 * \note This function enables the internal pull up on SW_LOCK_CLOSED if necessary, but doesn't
 *       deactivate them. This is useful to check lock state while driving the motor.
 */
static bool_t is_motor_lock_open_no_disable (void)
{
	enable_lock_pull_up();
	return ioport_get_pin_level (SW_LOCK_CLOSED) == 0;
}

/*!
 * \brief Checks state of the lock pin.
 * \param lock_open Checks if lock has this state (true means open,
 *                  false means closed).
 * \return TRUE if lock state is open_lock, else FALSE.
 *
 * \note This function eventually enables the internal pull-up on SW_LOCK_CLOSED
 *       but doesn't disable it afterwards.
 */
static bool_t check_motor_lock_state_no_disable (bool_t lock_open)
{
	return (is_motor_lock_open_no_disable() == lock_open);
}

bool_t motorIsLockOpen (void)
{
	bool_t ret;

	ret = is_motor_lock_open_no_disable();
	disable_lock_pull_up();

	return ret;
}

void setSlaveLock (bool_t lock_open, bool_t slave_mode)
{
	MOTOR_DLOG("setSlaveLock open=0x%02x slave_mode=0x%02x\r\n", lock_open, slave_mode);
	if (lock_open && slave_mode) {
		MOTOR_PORT.OUTCLR = MOTOR_SLAVE_bm;
	} else { /* !do_open or not in slave mode */
		MOTOR_PORT.OUTSET = MOTOR_SLAVE_bm;
	}
}

static uint8_t ReadCalibrationByte(uint8_t index) {

	uint8_t result;
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	result = pgm_read_byte(index);

	NVM_CMD = NVM_CMD_NO_OPERATION_gc;

	return(result);
}

static void adc_callback (ADC_t *adc, uint8_t ch_mask, adc_result_t res)
{
	if (!adc_running)
		return;

	++adc_vcc_avg_counter;
	adc_vcc_avg += res;

	if (adc_vcc_avg_counter >= VCC_NUM_AVERAGING) {
		adc_vcc_avg /= adc_vcc_avg_counter;
		if (adc_vcc_avg < VCC_THRESHOLD) {
			adc_vcc_low = true;
			/*
			MOTOR_DLOG("ADC: VCC low detected! %u < %u (treshold)\r\n",
			     adc_vcc_avg, VCC_THRESHOLD);
				 */
		} else {
			//MOTOR_DLOG("ADC: VCC okay %u\r\n", adc_vcc_avg);
		}
		adc_vcc_avg_counter = 0;
		adc_vcc_avg = 0;
	}

	adc_start_conversion(MY_ADC, ADC_CH0);
}

static void adcInitVCCMeasurement (void)
{
	struct adc_config adc_conf;
	struct adc_channel_config adcch_conf;

	/* FROM ATMEL APP-NOTE */
	// Calibration values are stored at production time
	// Load stored bytes into the calibration registers
	// First NVM read is junk and must be thrown away
	MY_ADC->CALL = ReadCalibrationByte (ADCACAL0);
	MY_ADC->CALH = ReadCalibrationByte (ADCACAL1);
	MY_ADC->CALL = ReadCalibrationByte (ADCACAL0);
	MY_ADC->CALH = ReadCalibrationByte (ADCACAL1);
	adc_read_configuration (MY_ADC, &adc_conf);
	adcch_read_configuration (MY_ADC, ADC_CH0, &adcch_conf);

	/* FROM ATMEL: APP-NOTE */
	// Calibration values are stored at production time
	// Load stored bytes into the calibration registers
	// First NVM read is junk and must be thrown away
	MY_ADC->CALL = ReadCalibrationByte (ADCACAL0);
	MY_ADC->CALH = ReadCalibrationByte (ADCACAL1);
	MY_ADC->CALL = ReadCalibrationByte (ADCACAL0);
	MY_ADC->CALH = ReadCalibrationByte (ADCACAL1);

	adc_set_conversion_parameters (&adc_conf,
	                               ADC_SIGN_ON,
	                               ADC_RES_12,
	                               ADC_REF_BANDGAP);

	adc_set_conversion_trigger (&adc_conf, ADC_TRIG_MANUAL, 1, 0);
	adc_set_clock_rate (&adc_conf, 20000UL);
	adcch_set_input (&adcch_conf, ADCCH_POS_SCALED_VCC, ADCCH_NEG_NONE, 1);
	adcch_enable_interrupt (&adcch_conf);
	adc_write_configuration (MY_ADC, &adc_conf);
	adcch_write_configuration (MY_ADC, ADC_CH0, &adcch_conf);

	adc_running = false;

	adc_set_callback (MY_ADC, adc_callback);
	adc_enable (MY_ADC);

	adcStart ();
	delayNMilliSeconds (20); /* Capture a few samples */
	adcStop ();

	//MOTOR_DLOG("ADC: Init\r\n");
}


static void adcDeinit (void)
{
	//MOTOR_DLOG("ADC: Deinit\r\n");
	adc_running = false;
	adc_disable (MY_ADC);
}

static void adcStart (void)
{
	IRQ_INC_DISABLE();
	adc_vcc_avg = 0;
	adc_vcc_avg_counter = 0;
	IRQ_DEC_ENABLE();

	adc_running = true;
	//MOTOR_DLOG("ADC: Start\r\n");
	adc_start_conversion (MY_ADC, ADC_CH0);
}

static void adcStop (void)
{
	adc_running = false;
	//MOTOR_DLOG("ADC: Stop\r\n");
}

static s8 drive_motor (bool_t open_lock)
{
	s8 err = ERR_NONE;

	enable_lock_pull_up();

	while (true) 
	{
		//delayNMilliSeconds (MOTOR_PWM_MS);
		if (check_motor_lock_state_no_disable (open_lock)) 
		{
			err = ERR_NONE;
			goto out;
		}

		delayNMilliSecondsStart (MOTOR_PWM_MS);
		motor_on();
		adcStart ();
		while (!delayNMilliSecondsIsDone (false)) 
		{
			if (adc_vcc_low) {
				err = ERR_VCC_LOW;
				/* TRICKY: Continue to open/close the lock
				 *         since it's not safe to stop now...
				 *         Therefore keep error message and
				 *         (unless timeout occurs) and reopen
				 *         lock if VCC is low.
				 */
			}
			if (rtcIsFinished ()) {
				err = ERR_TIMEOUT;
				goto out;
			}
			if (check_motor_lock_state_no_disable (open_lock)) {
				goto out;
			}
		}
		adcStop ();
		//motor_off();
	}

out:
	adcStop ();
	motor_off();
	disable_lock_pull_up();
	return err;
}

static void handle_low_vcc (void)
{
	MOTOR_DLOG("MOTOR: low VCC detected\r\n");
	buzzerStart (SignalVccLow, false);
	buzzerWaitTillFinished ();
}

static void handle_timeout_error (void)
{
	MOTOR_DLOG("MOTOR: Timeout occured while driving the motor\r\n");
	/* TODO: What should be done? */
}

s8 motorOpenLock (bool_t slave)
{
	s8 err;

	
	
	/* evil hack :( */
	u8 sw = cardmanGetSoftwareFunction();
	if (sw & SW_FUNCTION_LATCH) {
		slave = true;
	}

	//setSlaveLock(true, slave);

	if (motorIsLockOpen ())
		return ERR_NONE;

	adcInitVCCMeasurement ();

	rtcStart(TIMEOUT_RTC_MOTOR);
	motor_on();
	delayNMilliSeconds (MOTOR_START_HALF_MS);
	adcStart ();
	delayNMilliSeconds (MOTOR_START_HALF_MS);
	adcStop ();
	motor_off();

	err = drive_motor (true);

	rtcStop ();
	adcDeinit ();
	
	setSlaveLock(true, slave);

	if (adc_vcc_low) {
		handle_low_vcc();
	}

	if (err == ERR_TIMEOUT)
		handle_timeout_error();

	return err;
}

s8 motorCloseLock (bool_t slave)
{
	s8 err;
	
	DLOG("motorCloseLock\r\n");

	
	/* evil hack :( */
	u8 sw = cardmanGetSoftwareFunction();
	if (sw & SW_FUNCTION_LATCH) {
		slave = true;
		setSlaveLock(false, slave);
	}


	if (!motorIsLockOpen())
		return ERR_NONE;

	if (adc_vcc_low) {
		handle_low_vcc();
		return ERR_VCC_LOW;
	}

	//setSlaveLock(false, slave);

	adcInitVCCMeasurement ();

	rtcStart (TIMEOUT_RTC_MOTOR);
	motor_on();
	delayNMilliSeconds (MOTOR_START_HALF_MS);
	adcStart ();
	delayNMilliSeconds (MOTOR_START_HALF_MS);
	adcStop ();
	motor_off();

	err = drive_motor (false);
	rtcStop ();
	adcDeinit ();

	setSlaveLock(false, slave);

	if (adc_vcc_low) /* Has already been moved */
		motorOpenLock(slave); /* Will automatically issue the signal! */

	if (err == ERR_TIMEOUT)
		handle_timeout_error();

	return err;
}
