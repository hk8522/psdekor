/*
 * application.h
 *
 * Created: 22.11.2013 17:40:42
 *  Author: huber
 */ 


#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "cardman/cards_manager.h"
#include "sorex_hal/Communication/Rfid.h"

enum MainState {
	MSTATE_INITIALIZE,				/* Implemented */

	MSTATE_PREPARE_WAKE_UP,				/* Implemented */
	MSTATE_DEEP_SLEEP,				/* Implemented */
	MSTATE_WOKE_UP,					/* Implemented */
	MSTATE_ENTER_PROGRAMMING_MODE_1,
	MSTATE_ENTER_PROGRAMMING_MODE_2,
	MSTATE_WAIT_PROG_MODE_1,			/* Add/Remove key */
	MSTATE_WAIT_PROG_MODE_2,			/* Delete all keys */
	MSTATE_ENTER_SOFTWARE_KEY,			/* Implemented */
	MSTATE_ENTER_SOFTWARE_RTC,			/* Implemented */
	MSTATE_ENTER_SOFTWARE_DOOR,			/* Implemented */
	MSTATE_FOUND_UNKNOWN_CARD,			/* Implemented */
	MSTATE_EXIT_PROG_OR_LEARN_MODE,			/* Implemented */
	MSTATE_ENTER_LEARN_MODE,			/* Implemented */
	MSTATE_WAIT_LEARN_MODE,				/* Implemented */

	MSTATE_ERROR_SET_PROG_CARD_FAILED,		/* Implemented */
	MSTATE_ERROR_MULTIPLE_CARDS,			/* Implemented */
};

struct scan_result_s {
	struct card_entry card;
	enum card_type type;
	u8 found_card_counter;
	u8 extra;
};

extern struct scan_result_s scan_result;

extern Status IdentifyCardCallback (UnitId unitId, byte scanId, void *result);

extern void MainStateMachine (void);


#endif /* APPLICATION_H_ */