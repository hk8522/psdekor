/*
 * card_utils.c
 *
 * Created: 25.11.2013 12:22:54
 *  Author: huber
 */

#include "cardman/card_utils.h"

bool_t cardIsEqual (const u8 *uid, const u8 len, const struct card *cmp_card)
{
	u8 i;

	if (len != cmp_card->card.len)
	return false;

	for (i = 0; i < len; ++i) {
		if (cmp_card->card.uid[i] != uid[i]) {
			return false;
		}
	}

	return true;
}

s8 cardCopy (const u8 *uid, const u8 len, struct card_entry *dest)
{
	u8 i;

	if (unlikely((uid == NULL) || (len > MAX_KEY_UID_LENGTH) ||
	    (dest == NULL))) {
		    return ERR_PARAM;
	}

	dest->len = len;
	for (i = 0; i < len; ++i) {
		dest->uid[i] = uid[i];
	}

	return ERR_NONE;
}
