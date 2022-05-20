/*
 * motor_control.h
 *
 * Created: 25.11.2013 17:28:17
 *  Author: huber
 *  Modified: Alain Stucki <as@bitlab.ch>
 */

/* Changelog:
 *	15.12.2014			Changed VCC_THRESHOLD from 500 to 400
 *
 *
 */


#ifndef MOTOR_CONTROL_H_
#define MOTOR_CONTROL_H_

#include "delay_wrapper.h"

/* HINT: Maybe these first order estimate have to be optimized */

/*!
 * \brief Threshold of ADC result 
 */
#define VCC_THRESHOLD              U16_C(450)
/*!* \note war bei 485 als wir noch nur den Motor hatten. Einzeltest: Bei 400 war die Schwelle bei 2,0V.



/*! \brief Number of samples to average */
#define VCC_NUM_AVERAGING          16 /*! \war bei 16 - 01.03.2017*/
/*! \brief Error code to indicate that VCC was low during motor control */
#define ERR_VCC_LOW               -70 /*! \war bei ....*/

/*!
 * \brief Open lock
 * \return ERR_VCC_LOW if VCC is low.
 *         ERR_TIMEOUT if RTC timeout occured.
 *         ERR_NONE if lock is open (or has already been open).
 */
extern s8 motorOpenLock (bool_t slave);

/*!
 * \brief Closes lock
 * \return ERR_VCC_LOW if VCC is low (or was low)
 *         ERR_TIMEOUT if RTC timeout occured
 *         ERR_NONE if lock is closed (or has already been closed).
 */
extern s8 motorCloseLock (bool_t slave);

/*!
 * \brief Checks if lock is open
 * \return TRUE if lock is open, else FALSE
 */
extern bool_t motorIsLockOpen (void);

/*!
 * \brief Sets state of slave lock
 * \param lock_open True if slave lock shall be opened, else false
 *        (if slave lock shall be closed)
 * \param slave_mode True if device if the slave lock shall be used, else false.
 */
extern void setSlaveLock (bool_t lock_open, bool_t slave_mode);

#endif /* MOTOR_CONTROL_H_ */
