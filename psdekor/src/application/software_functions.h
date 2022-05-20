/*
 * software_functions.h
 *
 * Created: 25.11.2013 15:55:41
 *  Author: huber
 */ 


#ifndef SOFTWARE_FUNCTIONS_H_
#define SOFTWARE_FUNCTIONS_H_

#include "cardman/cards_manager.h"

#define SW_FUNCTION_ALARM       (1 << 0)
#define SW_FUNCTION_SLAVE       (1 << 1)
#define SW_FUNCTION_LATCH		(1 << 2)
#define SW_FUNCTION_GYM			(1 << 3)

#define SW_FUNCTION_1           SW_FUNCTION_ALARM
#define SW_FUNCTION_2           SW_FUNCTION_LATCH
#define SW_FUNCTION_3          (SW_FUNCTION_SLAVE | SW_FUNCTION_ALARM)
#define SW_FUNCTION_4          SW_FUNCTION_GYM

enum sw_trigger {
	SW_TRIGGER_KEY,
	SW_TRIGGER_RTC,
	SW_TRIGGER_DOOR,
	SW_TRIGGER_INIT_SOFTWARE,
	SW_TRIGGER_DEINIT_SOFTWARE,
	SW_TRIGGER_CHANGE_NCARDS
};


extern void SoftwareStateMachine (enum sw_trigger trigger);

extern void SoftwareSleep (void);

extern void SoftwareCheckAlarm (void);

extern bool_t isDoorClosed(void);

extern void SoftwarePrintVersion (void);

extern void SoftwareRestartAlarm (void);

extern void gym_card_callback(struct scan_result_s *scan);
extern void gym_rtc_callback(void);

#endif /* SOFTWARE_FUNCTIONS_H_ */