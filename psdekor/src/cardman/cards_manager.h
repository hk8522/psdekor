/*!
 * \file cards_manager.h
 * The cards-manager modules handles the management of key-, soft- and
 * programming cards. It makes use of the eeprom to permanently save all
 * sorts of cards. The cards-manager supports a simple method to recover
 * from power failure during
 *  \author huber
 */

#ifndef CARDS_MANAGER_H_
#define CARDS_MANAGER_H_

#include "platform.h"
#include "iso14443a.h"

/************************************************************************/
/* CONFIGURATION                                                        */
/************************************************************************/

/*! \brief Maximum size of UID of cards */
#define MAX_KEY_UID_LENGTH         (ISO14443A_MAX_UID_LENGTH)

/*! \brief Maximum number of key cards */
#define MAX_NUMBER_OF_KEY_CARDS    50

/*! \brief Number of software cards must be a multiple of 2 */
#define MAX_NUMBER_OF_SOFT_CARDS    6

/*! \brief Default software function */
#define DEFAULT_SOFTWARE_FUNCTION  0x01

/************************************************************************/
/* GLOBAL DATASTRUCTURES                                                */
/************************************************************************/

/*! \brief enum of different types of cards */
enum card_type {
	CARD_TYPE_UNKNOWN,					//0
	CARD_TYPE_KEY,						//1
	CARD_TYPE_PROGRAMMING_CARD,			//2
	CARD_TYPE_SOFTWARE_CARD,			//3
	CARD_TYPE_GYM_SOFTWARD_CARD,		//4
	CARD_TYPE_GYM_SOFTWARD_CARD_6,		//5
	CARD_TYPE_GYM_SOFTWARD_CARD_12,		//6
	CARD_TYPE_GYM_SOFTWARD_CARD_24,		//7
};

/*! \brief Structure to hold information regarding cards */
struct card_entry {
	u8 uid[MAX_KEY_UID_LENGTH];
	u8 len;
};
/*! \brief Size of card_entry structure */
#define CARD_ENTRY_SIZE            (MAX_KEY_UID_LENGTH + 1)

/*! \brief Structure to hold information regarding cards */
struct card {
	struct card_entry card;
	u8 extra [1];
};

/*! \brief Structure that holds information of a header entry */
struct header_entry {
	u8 state;
	u16 db_version;
	u8 nkeys;
	u8 nsoft_cards;
	u8 current_sw;
	u8 open_delay;
	u8 dummy;
};
/*! \brief Size of header structure */
#define HEADER_ENTRY_SIZE          8

/*! \brief Structure that holds information of one key card */
struct key_page {
	u8 used;
	struct card_entry card;
};
/*! \brief Size of a key card structure */
#define KEY_PAGE_SIZE              (CARD_ENTRY_SIZE + 1)

/*! \brief Structure to hold information of one software card */
struct soft_card_entry {
	u8 used;
	struct card_entry card;
	u8 sw_function;
};
/*! \brief Size of one software card entry */
#define SOFT_CARD_ENTRY_SIZE       (CARD_ENTRY_SIZE + 2)

/*! \brief Structure that holds two software cards */
struct soft_page {
	struct soft_card_entry card[2];
};
/*! \brief Size of a soft_page structure */
#define SOFT_PAGE_SIZE             (SOFT_CARD_ENTRY_SIZE * 2)

/************************************************************************/
/* GLOBAL FUNCTIONS                                                     */
/************************************************************************/

/*!
 * \brief Initializes the cards-manager module
 * \return ERR_NOTSUP if database version is not supported by the firmeware.
 *         ERR_BAD_DATA if both headers are corrupted.
 *         Error codes from eeprom_write_buffer_to_page could also be returned
 *         if there an error occured.
 *         ERR_NONE if initialization was successfull.
 *
 * This functions tries to load database from eeprom.
 * This function has to be called (successfully) before using any other
 * function defined in this module or you will experience unspecified
 * behaviour.
 */
extern s8 cardmanInitialize (void);

/*!
 * \brief Adds a key card to the database
 * \param uid UID of the card to add.
 * \param len Length of the UID of the card to add.
 * \return ERR_REQUEST if card can't be added to the database
 *         because it's already in the database.
 *         ERR_BAD_DATA if no empty slot could be found although
 *         the database header inidicated that there should be
 *         space left.
 *         ERR_NO_MEMORY if the header indicates that the maximum
 *         number of cards has been stored in the database.
 *         ERR_NONE if key card has been added to database.
 */
extern s8 cardmanAddKey (const u8 *uid, const u8 len);

/*!
 * \brief Deletes a key card from database
 * \param uid UID of the card to delete
 * \param len Length of the UID of the card to delete.
 * \return ERR_REQUEST if card can't be deleted (because it's
 *         a software or programming card)
 *         ERR_NONE if card has been deleted successfully (or
 *         has never existed in database).
 */
extern s8 cardmanDeleteKey (const u8 *uid, const u8 len);

/*!
 * \brief Deletes all key cards from database
 * \return ERR_NONE
 */
extern s8 cardmanDeleteAllKeys (void);

/*!
 * \brief Retrieves the type of card (by checking the database)
 * \param uid UID of the card to check.
 * \param len Length of the UID of the card to check.
 * \param result pointer to address of the result structure.
 * \return Returns a type code if the card could be found in the
 *         database (or CARD_TYPE_UNKNOWN if not found).
 */
extern enum card_type cardmanGetCardType (const u8 *uid,
                                          const u8 len,
                                          struct card **result);

/*!
 * \brief Store programming card to database
 * \param uid UID of the card to use as programming card.
 * \param len length of the UID of the card that will be
 *            used as programming card.
 * \return ERR_REQUEST if card is already a key or software card.
 *         ERR_NONE if card has been set as programming card.
 *
 * If the specified card is already the programming card, ERR_NONE will
 * be returned.
 */
extern s8 cardmanSetProgrammingCard (const u8 *uid, const u8 len);

/*!
 * \brief Retrieve current software function
 * \return Returns the current software function code.
 */
extern u8 cardmanGetSoftwareFunction (void);

/*!
 * \brief Set current software function code.
 * \param sw_function software function code to set.
 */
extern void cardmanSetSoftwareFunction (u8 sw_function);

/*!
 * \brief Retrieves number of keys in database.
 * \return Number of keys in the database.
 */
extern u8 cardmanGetNumberOfKeys (void);

extern void cardmanSetSoftwareFunctionOpenDelay (u8 sw_function, u8 delay);
extern u8 cardmanGetOpenDelay();
#endif /* CARDS_MANAGER_H_ */
