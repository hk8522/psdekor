/*
 * Rfid.c
 *
 * Created: 29.10.2013 11:15:54
 *  Author: m.huber
 */
/*!
 * \file Rfid.c
 * \author Manuel Huber
 * \brief Sorex HAL RFID implementation
 */
#include <asf.h>
#include "sorex_hal/Communication/Rfid.h"
#include "spi_driver.h"
#include "ic.h"
#include "delay_wrapper.h"
#include "as3911.h"
#include "as3911_com.h"
#include "iso14443a.h"
#include "iso14443b.h"
#include "as3911_hw_config.h"

/*! \brief Describes the state of Unit1 */
enum Unit1State
{
	Unit1StateOff,      /**< Unit1 isn't initialized */
	Unit1StateReady,    /**< Unit1 is initialized and ready to operate */
	Unit1StateScanning  /**< Unit1 scans for new cards */
};

/*!
 * \brief This structure holds information about Unit1
 *
 * This structure holds information as the last card that has been found and
 * status information about the current state of Unit1.
 */
struct Unit1InfoStruct
{
	/*! Current state of Unit1 */
	volatile enum Unit1State State;
	/*! TRUE during a scan as long as #RfidStopScan() hasn't been called */
	volatile bool_t StoppingScan;

	union {
		iso14443AProximityCard_t card_a;
		iso14443BProximityCard_t card_b;
	};

};

/*! \brief Information structure of Unit1 */
static struct Unit1InfoStruct Unit1Info;

/*!
 * \brief Initializes Unit1 (AS3911 stack)
 *
 * \return Error code if any error occured, else ERR_NONE
 *
 * Initializes following modules:
 *  - delay module
 *  - spi module
 *  - as3911 stack
 */
static s8 unit1Initialize (void);

/*!
 * \brief Deinitializes Unit1
 * \return Error code if any error occured, else ERR_NONE
 *
 * Deinitializes Interrupts and spi module
 */
static s8 unit1Deinitialize (void);

/*!
 * \brief Starts a scan operation.
 * \param scanId   Custom identification number that will be passed to
 *                 \a callback if a card has been found. Can be used to
 *                 distinguish different calls (from different locations
 *                 in the source code) that use the same callback functions.
 * \param callback Callback function that will be called for every card that
 *                 has been found.
 * \return Error code if an enumeration is currently running, or if some error
 *         in the as3911 stack has occured, else ERR_NONE.
 *
 * This functions will wake up all cards in range and try to enumerate them.
 * It tries to select a card #RFID_UNIT1_MAX_SCAN_ATTEMPTS times. Everytime a card
 * is found, it starts over again - until all cards have been found.
 */
static s8 unit1StartScan (byte scanId, RfidScanFinishedCallback callback);


Status RfidInitialize (UnitId unitId, RfidInitializedCallBack callback)
{
	s8 err;

	switch (unitId) {
	case RFID_UNIT_1:
		err = unit1Initialize ();
		break;

	default:
		err = ERR_PARAM;
	}

	if (callback)
		callback (unitId, &err);

	return err;
}

static s8 unit1Initialize (void)
{
	const spiConfig_t spi_config = {
		.spi_dev = RFID_UNIT1_SPI_DEVICE,
		.invert_sen = 1,
		.sen = AS3911_SEN_PIN,
		.flags = SPI_MODE_1,
		.baudrate = RFID_UNIT1_SPI_BAUDRATE
	};

	s8 err;
	u16 mV;
	u8 result;


	if (unlikely (Unit1Info.State != Unit1StateOff)) {
		err = ERR_REQUEST;
		goto out;
	}

	err = delayInitialize ();
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out);

	err = spiInitialize (&spi_config);
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out);

	err = icEnableInterrupt (IC_SOURCE_AS3911);
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out_err_spi);

	err = as3911Initialize ();
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out_err_intr);

	err = as3911ModifyRegister (AS3911_REG_REGULATOR_CONTROL,
	                            AS3911_REG_REGULATOR_CONTROL_reg_s,
	                            0x0);
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out_err_intr);

	err = as3911AdjustRegulators (&mV);
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out_err_intr);

	err = as3911ModifyRegister (AS3911_REG_ANT_CAL_CONTROL,
	                            AS3911_REG_ANT_CAL_CONTROL_trim_s,
	                            0x0);
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out_err_intr);

	err = as3911CalibrateAntenna (&result);
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out_err_intr);

	Unit1Info.State = Unit1StateReady;
	goto out;

out_err_intr:
	icDisableInterrupt (IC_SOURCE_AS3911);
out_err_spi:
	spiDeinitialize ();
out:
	return err;
}

static s8 unit1Deinitialize (void)
{
	s8 err;

	if (unlikely (Unit1Info.State != Unit1StateReady)) {
		err = ERR_REQUEST;
		goto out;
	}

	icDisableInterrupt (IC_SOURCE_AS3911);
	err = as3911Deinitialize ();
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out);

	spiDeinitialize ();
	Unit1Info.State = Unit1StateOff;
out:
	return err;
}

Status RfidReset (UnitId unitId)
{
	s8 err;

	switch (unitId) {
	case RFID_UNIT_1:
		err = unit1Deinitialize ();
		if (!err)
			err = unit1Initialize ();
		break;

	default:
		err = ERR_PARAM;
	}

	return err;
}

Status RfidStartScan (UnitId unitId,
                      byte scanId,
                      RfidScanFinishedCallback scanFinishedCallback)
{
	s8 err;

	switch (unitId) {
	case RFID_UNIT_1:
		err = unit1StartScan (scanId, scanFinishedCallback);
		break;

	default:
		err = ERR_PARAM;
	}

	return err;
}

static s8 unit1StartScan (byte scanId, RfidScanFinishedCallback callback)
{
	u8 count = RFID_UNIT1_MAX_SCAN_ATTEMPTS;
	s8 err;

	if (unlikely (Unit1Info.State != Unit1StateReady)) {
		err = ERR_REQUEST;
		goto out_no_cleanup;
	}

	/* Stopping is only valid during a scan operation! */
	Unit1Info.StoppingScan = false;
	Unit1Info.State = Unit1StateScanning;

	err = iso14443AInitialize ();
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out_no_cleanup);

	delayNMilliSeconds (5);

	/* Wake cards! */
	err = iso14443ASelect (ISO14443A_CMD_WUPA, &Unit1Info.card_a);
	while (1) {
		if (err == ERR_NONE) {
			count = RFID_UNIT1_MAX_SCAN_ATTEMPTS;
			callback (RFID_UNIT_1, scanId, &Unit1Info.card_a);
			/* TODO: What should be done with the error code ????
			 *       RfidScanFinishedCallback lacks documentation!
			 *       What error codes are expected? What should be
			 *       done by this function?
			 */

			err = iso14443ASendHlta ();
			EVAL_ERR_NE_GOTO (err, ERR_NONE, out_deinit_protocol);
		} else {
			--count;
			delayNMilliSeconds (5);
		}

		if ((count == 0) || Unit1Info.StoppingScan) {
			break;
		}

		/* Only check cards that haven't been read */
		err = iso14443ASelect (ISO14443A_CMD_REQA, &Unit1Info.card_a);
	}

	err = ERR_NONE;

out_deinit_protocol:
	iso14443ADeinitialize (0);
	Unit1Info.State = Unit1StateReady;

out_no_cleanup:
	return err;
}

Status RfidStopScan (UnitId unitId)
{
	switch (unitId) {

	case RFID_UNIT_1:
		if (Unit1Info.State == Unit1StateScanning) {
			Unit1Info.StoppingScan = true;
			return ERR_NONE;
		} else {
			return ERR_REQUEST;
		}

	default:
		return ERR_PARAM;
	}
}

Status RfidGetCardType (UnitId unitId, byte *tagType)
{
	/* TODO: What means 'tagType'? iso14443A/iso14443B ?*/
	return ERR_NOTSUPP;
}

Status RfidTransferBytes (UnitId unitId,
                          TransferMode transferMode,
                          unsigned int address,
                          byte *bytes,
                          byte *size)
{
	/* TODO: Only MiFare Ul Tags?? Protocol? Specification? */
	return ERR_NOTSUPP;
}
