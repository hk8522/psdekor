/*
 * rtc_timeout.h
 *
 * Created: 25.11.2013 10:28:41
 *  Author: huber
 */ 


#ifndef RTC_TIMEOUT_H_
#define RTC_TIMEOUT_H_

#include "platform.h"
#include "delay_wrapper.h"

/*!
 * \brief Starts (or restarts) the RTC timer.
 * \param seconds Timeout in seconds.
 */
extern void rtcStart (const u8 seconds);

/*!
 * \brief Starts (or restarts the RTC timer and sets a flag when the
 *        interrupt occurs.
 * \param seconds Timeout in seconds.
 */
extern void rtcStartINTR (const u8 seconds);

/*!
 * \brief Stops the RTC timer.
 */
extern void rtcStop (void);

/*!
 * \brief Checks if the configured timeout period has experied.
 * \return True, if configured period is over or the timer has
 *         not been started, False otherwise.
 */
extern bool_t rtcIsFinished (void);

/*!
 * \brief Checks if x seconds have passed since the timer has
 *        been started.
 * \param x Number of seconds.
 * \return True if x seconds have passed since the timer has been started.
 *         If the timer has not been started or has already expired (and
 *         is therefore stopped) this function will always return true.
 *
 * This function can be used to check multiple timeouts within a longer
 * period. You must start the timer with the longest timeout period
 * otherwise this function will return true as soon as the first
 * period expired.
 */
extern bool_t rtcXSecondsPassed (const u8 x);

/*!
 * \brief Checks if interrupt has occurred
 */
extern bool_t rtcGetInterrupt (void);

extern bool_t rtcPollInterrupt_LOCKED (void);

#endif /* RTC_TIMEOUT_H_ */