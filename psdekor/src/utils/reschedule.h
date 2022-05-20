/*
 * reschedule.h
 *
 * Created: 06.08.2014 10:01:17
 *  Author: huber
 */


#ifndef RESCHEDULE_H_
#define RESCHEDULE_H_

#include <stdint.h>

#define RSCHED_TIMER_UNIT       (&TCE0)
#define RSCHED_ISR              TCE0_CCA_vect

#ifndef RSCHED_PRIORITY
/*! \brief RSCHED interrupt priority (can either be 'high', 'medium' or 'low') */
# define RSCHED_PRIORITY         medium
#endif /* RSCHED_PRIORITY */

#define RSCHED_SOURCE_ISR 0
#define RSCHED_SOURCE_WAIT 1

typedef int8_t rsched_callback_t (uint8_t status);

int8_t rsched_init (uint8_t timer_div);

int8_t rsched_reschedule(uint16_t ticks, rsched_callback_t *callback);

void rsched_wait_pending_locked (void);


#endif /* RESCHEDULE_H_ */