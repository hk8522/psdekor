/*!
 * \file Rfid.h
 * \author Manuel Huber
 * \brief Sorex HAL RFID implementation
 */

#ifndef __RFID_H
#define __RFID_H

#include "sorex_hal/Core/Types.h"
#include "sorex_hal/Core/Result.h"

/** \brief First AS3911 unit */
#define RFID_UNIT_1             0

/**
 * \brief Function called when scanning for RFID tags has finished.
 *
 * \param unitId Implementation/hardware specific identifier of the
 *               <b>RFID</b> unit.
 * \param scanId Identifier for the scanning process
 * \param result Implementation specific result.
 *
 * \todo (*RfidScanFinishedCallback)(.... <b> void *result</b> );
 *
 * \return Hardware specific return value. Should return S_OK if the operation
 *         succeeded, otherwise a negative value describing the cause of the
 *         problem.
 */
typedef Status (*RfidScanFinishedCallback)(UnitId unitId,
                                           byte scanId,
                                           void *result);

/**
 * \brief Function called when the RFID Hardware is initialized
 *
 * \param unitId Implementation/hardware specific identifier of the
 *               <b>RFID</b> unit.
 * \param result Implementation specific result.
 *
 * \return typedef Status
 */
typedef Status (*RfidInitializedCallBack)(UnitId unitId, void *result);

/**
 * \brief Initializes the RFID unit. Should be called only once.
 *
 * \param unitId Implementation/hardware specific identifier of the
 *               <b>RFID</b> unit.
 * \param callback  Function called when the RFID Hardware is initialized
 *
 * \return Hardware specific return value. Should return S_OK if the operation
 *         succeeded, otherwise a negative value describing the cause of the
 *         problem.
 */
Status RfidInitialize (UnitId unitId, RfidInitializedCallBack callback);

/**
 * \brief Resets the RFID unit.
 *
 * \param unitId Implementation/hardware specific identifier of the
 *               <b>RFID</b> unit.
 *
 * \return Hardware specific return value. Should return S_OK if the operation
 *         succeeded, otherwise a negative value describing the cause of the
 *         problem.
 */
Status RfidReset (UnitId unitId);

/**
 * \brief Starts scanning for RFID tags.
 *
 * \param unitId Implementation/hardware specific identifier of the
 *               <b>RFID</b> unit.
 * \param scanId Identifier for the scanning process
 * \param scanFinishedCallback Function called when the hardware finished
 *                             scanning.
 *
 * \return Hardware specific return value. Should return S_OK if the operation
 *         succeeded, otherwise a negative value describing the cause of the
 *         problem.
 */
Status RfidStartScan (UnitId unitId,
                      byte scanId,
                      RfidScanFinishedCallback scanFinishedCallback);

/**
 * \brief Stops/interrupts scanning for RFID tags.
 *
 * \param unitId Implementation/hardware specific identifier of the
 *               <b>RFID</b> unit.
 *
 * \return Hardware specific return value. Should return S_OK if the operation
 *         succeeded, otherwise a negative value describing the cause of the
 *         problem.
 */
Status RfidStopScan (UnitId unitId);

/**
 * \brief Retrieves type information from the latest scanned RFID tag.
 * This method should be called within a \see RfidScanFinishedCallback.
 *
 * \param unitId Implementation/hardware specific identifier of the
 *               <b>RFID</b> unit.
 * \param tagType Received data.
 *
 * \return Hardware specific return value. Should return S_OK if the operation
 *         succeeded, otherwise a negative value describing the cause of the
 *         problem.
 */
Status RfidGetCardType (UnitId unitId, byte *tagType);

/**
 * \brief Reads or writes bytes from/to RFID unit.
 *
 * \param unitId Implementation/hardware specific identifier of the
 *               <b>RFID</b> unit.
 * \param transferMode Specifies if the function should read or write data.
 * \param address Start address of the data.
 * \param bytes The data, byte array.
 * \param size Size of the data/byte array.
 *
 * \return Hardware specific return value. Should return S_OK if the operation
 *         succeeded, otherwise
 * a negative value describing the cause of the problem.
 */
Status RfidTransferBytes (UnitId unitId,
                          TransferMode transferMode,
                          unsigned int address,
                          byte *bytes,
                          byte *size);

#endif // __RFID_H
