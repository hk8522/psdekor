/*
 * card_utils.h
 *
 * Created: 25.11.2013 12:22:44
 *  Author: huber
 */


#ifndef CARD_UTILS_H_
#define CARD_UTILS_H_

#include "cardman/cards_manager.h"

/*!
 * \brief Compares two card structures
 * \param uid UID of the first card
 * \param len Lenght of the UID's
 * \param cmp_card card structure to compare to
 * \return TRUE if both UID's are equal, else FALSE.
 */
extern bool_t cardIsEqual (const u8 *uid,
                           const u8 len,
                           const struct card *cmp_card);

/*!
 * \brief Copies a UID to a destination structure
 * \param uid UID to copy (source)
 * \param len length of the UID to copy.
 * \param dest Destination card structure to copy to.
 * \return ERR_PARAM if parameters are invalid.
 *         ERR_NONE on success.
 */
extern s8 cardCopy (const u8 *uid, const u8 len, struct card_entry *dest);

#endif /* CARD_UTILS_H_ */
