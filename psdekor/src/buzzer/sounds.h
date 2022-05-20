/*
 * sounds.h
 *
 * Created: 19.11.2013 17:57:18
 *  Author: huber
 */

#include "platform.h"
#include "buzzer/buzzer.h"

extern const char SignalWakeUp[];

extern const char SignalError[];

extern const char SignalHello[];

extern const char SignalTestOn[];

extern const char SignalTestOff[];

/*!
 * \brief Fast beeping signal for learn mode and programming mode 1
 * \note Set the repeat flag in the buzzer module to continuously play
 *       this signal.
 */
extern const char SignalModeFast[];

/*!
 * \brief Fast beeping signal for programming mode 2
 * \note Set the repeat flag in the buzzer module to continuously play
 *       this signal.
 */
extern const char SignalModeSlow[];

extern const char SignalPositive[];

extern const char SignalNegative[];

extern const char SignalAllDeleted[];

extern const char SignalSetSoftware[];

extern const char SignalRepeatedAlarm[];

extern const char SignalFirstAlarm[];

extern const char SignalVccLow[];

/*!
 * \brief Initializes buzzer sub-system with predefined tones.
 * \return See buzzerInitialize()
 */
extern s8 SoundsInitialize (void);
