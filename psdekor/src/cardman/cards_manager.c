/*
 * cards_manager.c
 *
 * Created: 21.11.2013 11:12:14
 *  Author: huber
 */
#include "cardman/cards_manager.h"
#include "cardman/compiler_checks.h"
#include "cardman/card_utils.h"
#include "application/software_functions.h"

/************************************************************************/
/* LOCAL DEFINITIONS                                                    */
/************************************************************************/

#define CARDMAN_DEBUG
#ifdef CARDMAN_DEBUG
# include "utils/debug.h"
# define CARDMAN_DLOG(...) DLOG(__VA_ARGS__)
#else
# define CARDMAN_DLOG(...)
#endif


static struct card sw_std_group =  {
	.card.uid = { 0x5e, 0xa6, 0x3d, 0xeb },
	.card.len = 4,
	.extra[0] = SW_FUNCTION_SLAVE
};

static struct card sw_bolt =  {
	.card.uid = { 0xda, 0xcf, 0x32, 0xe9 },
	.card.len = 4,
	.extra[0] = SW_FUNCTION_LATCH
};

static struct card sw_alarm =  {
	.card.uid = { 0xae, 0xec, 0x3d, 0xeb },
	.card.len = 4,
	.extra[0] = SW_FUNCTION_ALARM
};

static struct card gym_software_card = {
	.card.uid = { 0xF7, 0xA8, 0x33, 0x3B },
	.card.len = 4,
	.extra[0] = 0x00
};

static struct card gym_software_card_6 = {
	.card.uid = { 0x6e, 0x4f, 0x26, 0xab },
	.card.len = 4,
	.extra[0] = 0x00
};
static struct card gym_software_card_12 = {
	.card.uid = { 0x1e, 0x12, 0x2a, 0xcc },
	.card.len = 4,
	.extra[0] = 0x00
};
static struct card gym_software_card_24 = {
	.card.uid = { 0x6e, 0x8c, 0x26, 0xce },
	.card.len = 4,
	.extra[0] = 0x00
};

#define DATABASE_CURRENT_VERSION  1
#define DATABASE_UNINITIALIZED    U16_C(0xffff)
#define DATABASE_PROG_START_PAGE  2
#define DATABASE_SOFT_START_PAGE  3
#define DATABASE_KEY_START_PAGE   6
#define DATABASE_PROG_START_ADDR  (EEPROM_PAGE_SIZE * DATABASE_PROG_START_PAGE)
#define DATABASE_SOFT_START_ADDR  (EEPROM_PAGE_SIZE * DATABASE_SOFT_START_PAGE)
#define DATABASE_KEY_START_ADDR   (EEPROM_PAGE_SIZE * DATABASE_KEY_START_PAGE)

#define ENTRY_USED                1

#define HEADER_1_PAGE             0
#define HEADER_2_PAGE             1

#define HEADER_ADDR_1             U16_C(HEADER_1_PAGE)
#define HEADER_ADDR_2             (EEPROM_PAGE_SIZE * HEADER_2_PAGE)

#define HEADER_1                 0
#define HEADER_2                 1

#define HEADER_STATE_VALUE_X              0xaa
#define HEADER_STATE_VALUE_Z              0x55
#define HEADER_STATE_VALUE_UNINITIALIZED  0xff

enum header_state_e {
	HEADER_STATE_X = 1,
	HEADER_STATE_Z = 2,
	HEADER_STATE_I = 0,
	HEADER_STATE_F = 4
};

#define HEADER_STATE2_FF	((HEADER_STATE_F << 4) | HEADER_STATE_F)
#define HEADER_STATE2_XF	((HEADER_STATE_X << 4) | HEADER_STATE_F)
#define HEADER_STATE2_XX	((HEADER_STATE_X << 4) | HEADER_STATE_X)
#define HEADER_STATE2_XZ	((HEADER_STATE_X << 4) | HEADER_STATE_Z)
#define HEADER_STATE2_XI	((HEADER_STATE_X << 4) | HEADER_STATE_I)
#define HEADER_STATE2_ZX	((HEADER_STATE_Z << 4) | HEADER_STATE_X)
#define HEADER_STATE2_ZZ	((HEADER_STATE_Z << 4) | HEADER_STATE_Z)
#define HEADER_STATE2_ZI	((HEADER_STATE_Z << 4) | HEADER_STATE_I)
#define HEADER_STATE2_IX	((HEADER_STATE_I << 4) | HEADER_STATE_X)
#define HEADER_STATE2_IZ	((HEADER_STATE_I << 4) | HEADER_STATE_Z)
#define HEADER_STATE2_II	((HEADER_STATE_I << 4) | HEADER_STATE_I)
/* HINT: Not all states are currently listed here (only the ones that are
 *       used! (This doesn't mean the software doesn't handle these)
 */


/************************************************************************/
/* LOCAL DATASTRUCTURES                                                 */
/************************************************************************/

static struct cardman_ctrl_s {
	struct header_entry header;
	u8 next_header_index;
	u8 comb_header_state;
	union {
		struct card mapped_card[1 + MAX_NUMBER_OF_SOFT_CARDS +
		                        MAX_NUMBER_OF_KEY_CARDS];
		struct {
			struct card prog_card;
			struct card soft_card[MAX_NUMBER_OF_SOFT_CARDS];
			struct card key_card[MAX_NUMBER_OF_KEY_CARDS];
		};
	};
}cardman_ctrl;

union union_card_page{
	struct key_page key;
	struct soft_page soft;
	struct key_page prog;
};

/************************************************************************/
/* LOCAL FUNCTION DECLARATIONS                                          */
/************************************************************************/

/*!
 * \brief Reads header from eeprom
 * \return ERR_BAD_DATA if an invalid header has been found.
 *         ERR_NONE if no error occured.
 */
static s8 read_header (void);

/*!
 * \brief Decodes value from eeprom to state variable
 * \param state_value 8 bit state variable from eeprom.
 * \return Returns a header_state_e
 */
static enum header_state_e decode_header_state (u8 state_value);

/*!
 * \brief Tries to load a version 1 database
 * \return ERR_BAD_DATA if there is an obvious problem with header
 *         information (Corrupted data).
 *         ERR_NONE if successfull.
 *
 * This function doesn't check the actual header version, it just tries to load
 * the database. It should allow to change the actual database at any time
 * without loosing any data.
 */
static s8 load_db_version1 (void);

/*!
 * \brief Initializes a new database (version 1)
 * \return Error if eeprom_write_buffer_to_page fails, else ERR_NONE.
 */
static s8 init_db_version1 (void);

/*!
 * \brief Copies uid and len to entry address
 * \param entry destination entry address
 * \param uid UID to copy to entry.
 * \param len Length of uid.
 */
static void copy_to_entry (struct card_entry *entry,
                           const u8 *uid,
                           const u8 len);

/*!
 * \brief Prepares the next write of the header
 * Determines the next header-state and which header will be replaced.
 */
static void prepare_next_write (void);

/*!
 * \brief Writes header to eeprom.
 */
static void write_header (void);

/*!
 * \brief Writes a page to the eeprom
 * \param page_addr Address of the eeprom-page
 * \param buffer Pointer to the data to write.
 * \param len Length of the data to write (Must be less than EEPROM_PAGE_SIZE
 *            and more than 0).
 * \return ERR_PARAM if len is invalid or buffer is NULL.
 *         ERR_NONE if call was successful.
 */
static s8 eeprom_write_buffer_to_page (u8 page_addr, void *buffer, u8 len);

/************************************************************************/
/* LOCAL FUNCTIONS DEFINITIONS                                          */
/************************************************************************/

static s8 eeprom_write_buffer_to_page (u8 page_addr, void *buffer, u8 len)
{
	u8 i = 0;
	u8 *ptr = buffer;

	if (unlikely((len > EEPROM_PAGE_SIZE) ||
	    (page_addr > (EEPROM_SIZE/EEPROM_PAGE_SIZE)))) {
		return ERR_PARAM;
	}

	nvm_wait_until_ready ();
	nvm_eeprom_flush_buffer ();

	for (i = 0; i < len; ++i, ++ptr) {
		nvm_eeprom_load_byte_to_buffer (i, *ptr);
	}
	nvm_eeprom_atomic_write_page (page_addr);

	return ERR_NONE;
}

static enum header_state_e decode_header_state (u8 state_value)
{
	switch (state_value) {

	case HEADER_STATE_VALUE_X:
		return HEADER_STATE_X;
	case HEADER_STATE_VALUE_Z:
		return HEADER_STATE_Z;
	case HEADER_STATE_VALUE_UNINITIALIZED:
		return HEADER_STATE_F;
	default:
		return HEADER_STATE_I;
	}
}

static void prepare_next_write (void)
{
	enum header_state_e decoded_state;

	switch (cardman_ctrl.comb_header_state) {
	/* next: H1 */
	case HEADER_STATE2_FF:
		cardman_ctrl.next_header_index = HEADER_1;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_X;
		break;

	case HEADER_STATE2_XX:
		cardman_ctrl.next_header_index = HEADER_1;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_Z;
		break;

	case HEADER_STATE2_ZZ:
		cardman_ctrl.next_header_index = HEADER_1;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_X;
		break;

	case HEADER_STATE2_IX:
		cardman_ctrl.next_header_index = HEADER_1;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_Z;
		break;

	case HEADER_STATE2_IZ:
		cardman_ctrl.next_header_index = HEADER_1;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_X;
		break;

	/* next: H2 */
	case HEADER_STATE2_XF:
		cardman_ctrl.next_header_index = HEADER_2;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_X;
		break;

	case HEADER_STATE2_XZ:
		cardman_ctrl.next_header_index = HEADER_2;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_X;
		break;

	case HEADER_STATE2_XI:
		cardman_ctrl.next_header_index = HEADER_2;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_X;
		break;

	case HEADER_STATE2_ZX:
		cardman_ctrl.next_header_index = HEADER_2;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_Z;
		break;

	case HEADER_STATE2_ZI:
		cardman_ctrl.next_header_index = HEADER_2;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_Z;
		break;

	case HEADER_STATE2_II:
	default:
		cardman_ctrl.next_header_index = HEADER_1;
		cardman_ctrl.header.state = HEADER_STATE_VALUE_X;
		break;
	}

	decoded_state = decode_header_state (cardman_ctrl.header.state);

	if (cardman_ctrl.next_header_index == HEADER_1) {
		cardman_ctrl.comb_header_state = (
			(cardman_ctrl.comb_header_state & 0x0f) |
			(decoded_state << 4)
		);
	} else {
		cardman_ctrl.comb_header_state = (
			(cardman_ctrl.comb_header_state & 0xf0) |
			decoded_state
		);
	}
}

static s8 read_header (void)
{
	struct header_entry entry[2];

	nvm_eeprom_read_buffer (HEADER_ADDR_1, &entry[0], HEADER_ENTRY_SIZE);
	nvm_eeprom_read_buffer (HEADER_ADDR_2, &entry[1], HEADER_ENTRY_SIZE);

	cardman_ctrl.comb_header_state = (
		(decode_header_state (entry[0].state) << 4) |
		decode_header_state (entry[1].state)
	);

	CARDMAN_DLOG("[cardman] header state: %hhx\r\n",
	             cardman_ctrl.comb_header_state);

	switch (cardman_ctrl.comb_header_state) {
	/* next: H1 */
	case HEADER_STATE2_FF:
	case HEADER_STATE2_XX:
	case HEADER_STATE2_ZZ:
	case HEADER_STATE2_IX:
	case HEADER_STATE2_IZ:
		cardman_ctrl.header = entry[1];
		DLOG("read header page=%u size=%u delay=%u\r\n", HEADER_ADDR_2, sizeof (cardman_ctrl.header), cardman_ctrl.header.open_delay);
		break;

	/* next: H2 */
	case HEADER_STATE2_XF:
	case HEADER_STATE2_XZ:
	case HEADER_STATE2_XI:
	case HEADER_STATE2_ZX:
	case HEADER_STATE2_ZI:
		cardman_ctrl.header = entry[0];
		DLOG("read header page=%u size=%u delay=%u\r\n", HEADER_ADDR_1, sizeof (cardman_ctrl.header), cardman_ctrl.header.open_delay);
		break;

	case HEADER_STATE2_II:
	default:
		return ERR_BAD_DATA;
	}

	return ERR_NONE;
}

static s8 load_db_version1 (void)
{
	const u8 number_of_soft_pages = (
		(MAX_NUMBER_OF_SOFT_CARDS >> 1) +
		(MAX_NUMBER_OF_SOFT_CARDS & 0x1)
	);

	u8 i;
	union union_card_page tmp;
	u8 nsoft = 0;
	u8 nkeys = 0;

	if (cardman_ctrl.header.nkeys > MAX_NUMBER_OF_KEY_CARDS)
		return ERR_BAD_DATA;

	if (cardman_ctrl.header.nsoft_cards > MAX_NUMBER_OF_SOFT_CARDS)
		return ERR_BAD_DATA;

	nvm_eeprom_read_buffer (DATABASE_PROG_START_ADDR, &tmp.prog,
	                        sizeof(tmp.prog));

	cardman_ctrl.prog_card.card = tmp.prog.card;
	cardman_ctrl.prog_card.extra[0] = (tmp.prog.used == ENTRY_USED) ? 1 : 0;

	for (i = 0; i < number_of_soft_pages; ++i) {
		u8 page = DATABASE_SOFT_START_PAGE + i;
		u16 addr = (EEPROM_PAGE_SIZE * (u16)page);

		nvm_eeprom_read_buffer (addr, &tmp.soft, sizeof(tmp.soft));
		if (tmp.soft.card[0].used == ENTRY_USED) {
			cardman_ctrl.soft_card[nsoft].card = tmp.soft.card[0].card;
			cardman_ctrl.soft_card[nsoft].extra[0] = tmp.soft.card[0].sw_function;
			++nsoft;
		}

		if (tmp.soft.card[1].used == ENTRY_USED) {
			cardman_ctrl.soft_card[nsoft].card = tmp.soft.card[1].card;
			cardman_ctrl.soft_card[nsoft].extra[0] = tmp.soft.card[1].sw_function;
			++nsoft;
		}
	}

	for (i = 0; i < MAX_NUMBER_OF_KEY_CARDS; ++i) {
		u8 page = DATABASE_KEY_START_PAGE + i;
		u16 addr = (EEPROM_PAGE_SIZE * (u16)page);

		nvm_eeprom_read_buffer (addr, &tmp.key, sizeof(tmp.key));
		if (tmp.key.used == ENTRY_USED) {
			cardman_ctrl.key_card[nkeys].card = tmp.key.card;
			cardman_ctrl.key_card[nkeys].extra[0] = page;
			++nkeys;
		}
	}

	/* NOTE: Currently there are no consistency checks with header
	 *       information. (What should be done if there is
	 *       a missmatch??
	 */
	cardman_ctrl.header.nkeys = nkeys;
	cardman_ctrl.header.nsoft_cards = nsoft;

	return ERR_NONE;
}

static s8 init_db_version1 (void)
{
	cardman_ctrl.header.nkeys = 0;
	cardman_ctrl.header.nsoft_cards = 0;
	cardman_ctrl.header.db_version = DATABASE_CURRENT_VERSION;
	cardman_ctrl.header.state = HEADER_STATE_VALUE_X;
	cardman_ctrl.header.current_sw = DEFAULT_SOFTWARE_FUNCTION;
	cardman_ctrl.next_header_index = HEADER_1;
	cardman_ctrl.comb_header_state = HEADER_STATE2_XF;
	nvm_eeprom_erase_bytes_in_page (HEADER_2_PAGE);

	return eeprom_write_buffer_to_page (HEADER_1_PAGE,
	                                    &cardman_ctrl.header,
	                                    sizeof(cardman_ctrl.header));
}

static void copy_to_entry (struct card_entry *entry,
                           const u8 *uid,
                           const u8 len)
{
	u8 i;

	entry->len = len;
	for (i = 0; i < len; ++i) {
		entry->uid[i] = uid[i];
	}
}

static void write_header (void)
{
	u8 page_addr;

	prepare_next_write();
	switch (cardman_ctrl.next_header_index) {
	case HEADER_2:
		page_addr = HEADER_2;
		break;

	case HEADER_1:
	default:
		page_addr = HEADER_1;
		break;
	}

	DLOG("write header page=%u size=%u delay=%u\r\n", page_addr, sizeof (cardman_ctrl.header), cardman_ctrl.header.open_delay);
	eeprom_write_buffer_to_page(page_addr, &cardman_ctrl.header,
	                            sizeof (cardman_ctrl.header));
}

/************************************************************************/
/* GLOBAL FUNCTIONS                                                     */
/************************************************************************/

s8 cardmanInitialize (void)
{
	s8 err;

	err = read_header ();
	EVAL_ERR_NE_GOTO (err, ERR_NONE, out)

	switch (cardman_ctrl.header.db_version) {

	case U16_C(0xffff): /* EEPROM will be initialized to 0xffff */
		/* Initialize database */
		CARDMAN_DLOG("[cardman] Init db (first time)\r\n");
		err =  init_db_version1 ();
		break;

	case DATABASE_CURRENT_VERSION:
		CARDMAN_DLOG("[cardman] Load db version %hd\r\n",
			     cardman_ctrl.header.db_version);
		err = load_db_version1 ();
		if (err) {
			CARDMAN_DLOG("[cardman] Error %hhd, reinitializing db\r\n", err);
			err = init_db_version1();
		}
		break;

	default:
		err = ERR_NOTSUPP;
		break;
	}

out:
	return err;
}

s8 cardmanAddKey (const u8 *uid, const u8 len)
{
	u8 i;
	struct key_page key;
	enum card_type type = cardmanGetCardType (uid, len, NULL);

	switch (type) {
	case CARD_TYPE_UNKNOWN:
		/* Go ahead */
		if (unlikely(cardman_ctrl.header.nkeys >=
		             MAX_NUMBER_OF_KEY_CARDS)) {
			return ERR_NO_MEMORY;
		}

		/* HINT: Search strategy could be optimized */
		for (i = 0; i < MAX_NUMBER_OF_KEY_CARDS; ++i) {
			u16 addr = DATABASE_KEY_START_ADDR + (EEPROM_PAGE_SIZE * i);
			nvm_eeprom_read_buffer (addr, &key, sizeof(key));
			if (key.used != ENTRY_USED) {
				u8 page_addr = DATABASE_KEY_START_PAGE + i;
				copy_to_entry (&key.card, uid, len);
				key.used = 1;
				eeprom_write_buffer_to_page (page_addr,
				                             &key,
				                             sizeof(key));
				cardman_ctrl.key_card[cardman_ctrl.header.nkeys].card = key.card;
				cardman_ctrl.key_card[cardman_ctrl.header.nkeys].extra[0] = page_addr;
				++cardman_ctrl.header.nkeys;
				write_header ();
				CARDMAN_DLOG("HEADER: db: %hd, nkeys: %hhd, nsoft: %hhd, comb_h: %hhx\r\n",
				             cardman_ctrl.header.db_version,
				             cardman_ctrl.header.nkeys,
				             cardman_ctrl.header.nsoft_cards,
				             cardman_ctrl.comb_header_state);
				return ERR_NONE;
			}
		}

		return ERR_BAD_DATA;

	case CARD_TYPE_PROGRAMMING_CARD:
	case CARD_TYPE_SOFTWARE_CARD:
	case CARD_TYPE_KEY:
	default:
		return ERR_REQUEST;
	}
}

s8 cardmanDeleteKey (const u8 *uid, const u8 len)
{
	u8 page_addr;
	struct card *ptr;
	enum card_type type = cardmanGetCardType (uid, len, &ptr);

	switch (type) {
	case CARD_TYPE_KEY:
		/* Go ahead */
		page_addr = ptr->extra[0];
		CARDMAN_DLOG("Delete page %hhd\r\n", page_addr);
		nvm_eeprom_fill_buffer_with_value(0xff);
		nvm_eeprom_erase_bytes_in_page (page_addr);
		nvm_eeprom_flush_buffer ();
		--cardman_ctrl.header.nkeys;
		CARDMAN_DLOG("HEADER: db: %hd, nkeys: %hhd, nsoft: %hhd, comb_h: %hhx\r\n",
		             cardman_ctrl.header.db_version,
		             cardman_ctrl.header.nkeys,
		             cardman_ctrl.header.nsoft_cards,
		             cardman_ctrl.comb_header_state);
		write_header ();
		CARDMAN_DLOG("HEADER: db: %hd, nkeys: %hhd, nsoft: %hhd, comb_h: %hhx\r\n",
		             cardman_ctrl.header.db_version,
		             cardman_ctrl.header.nkeys,
		             cardman_ctrl.header.nsoft_cards,
		             cardman_ctrl.comb_header_state);

		/* HINT: This is VERY inefficient -> Could be optimized */
		cardmanInitialize ();
		CARDMAN_DLOG("HEADER: db: %hd, nkeys: %hhd, nsoft: %hhd, comb_h: %hhx\r\n",
		             cardman_ctrl.header.db_version,
		             cardman_ctrl.header.nkeys,
		             cardman_ctrl.header.nsoft_cards,
		             cardman_ctrl.comb_header_state);
		return ERR_NONE;

	case CARD_TYPE_UNKNOWN:
		return ERR_NONE;

	case CARD_TYPE_PROGRAMMING_CARD:
	case CARD_TYPE_SOFTWARE_CARD:
	default:
		return ERR_REQUEST;
	}
}

s8 cardmanDeleteAllKeys (void)
{
	u8 i;

	cardman_ctrl.header.nkeys = 0;
	write_header ();

	for (i = 0; i < MAX_NUMBER_OF_KEY_CARDS; ++i) {
		/* HINT: Could be optimized if only pages that have actually
		 *       been used.
		 */
		nvm_eeprom_fill_buffer_with_value(0xff);
		nvm_eeprom_erase_bytes_in_page (DATABASE_KEY_START_PAGE + i);
		nvm_eeprom_flush_buffer ();
	}

	return ERR_NONE;
}

enum card_type cardmanGetCardType (const u8 *uid,
                                   const u8 len,
                                   struct card **result)
{
	u8 i;


	if (cardIsEqual (uid, len, &sw_std_group )) {
		if (result != NULL)
		*result = &sw_std_group ;
		return CARD_TYPE_SOFTWARE_CARD;
	}

	if (cardIsEqual (uid, len, &sw_bolt )) {
		if (result != NULL)
		*result = &sw_bolt ;
		return CARD_TYPE_SOFTWARE_CARD;
	}

	if (cardIsEqual (uid, len, &sw_alarm )) {
		if (result != NULL)
		*result = &sw_alarm ;
		return CARD_TYPE_SOFTWARE_CARD;
	}

	if (cardIsEqual (uid, len, &gym_software_card)) {
		if (result != NULL)
		*result = &gym_software_card;
		return CARD_TYPE_GYM_SOFTWARD_CARD;
	}
	

	
	if (cardIsEqual (uid, len, &gym_software_card_6)) {
		if (result != NULL)
		*result = &gym_software_card_6;
		return CARD_TYPE_GYM_SOFTWARD_CARD_6;
	}

	if (cardIsEqual (uid, len, &gym_software_card_12)) {
		if (result != NULL)
		*result = &gym_software_card_12;
		return CARD_TYPE_GYM_SOFTWARD_CARD_12;
	}

	if (cardIsEqual (uid, len, &gym_software_card_24)) {
		if (result != NULL)
		*result = &gym_software_card_24;
		return CARD_TYPE_GYM_SOFTWARD_CARD_24;
	}

	if (cardman_ctrl.prog_card.extra[0] &&
	    cardIsEqual (uid, len, &cardman_ctrl.prog_card)) {
		if (result != NULL)
			*result = &cardman_ctrl.prog_card;
		return CARD_TYPE_PROGRAMMING_CARD;
	}

	for (i = 0; i < cardman_ctrl.header.nsoft_cards; ++i) {
		if (cardIsEqual (uid, len, &cardman_ctrl.soft_card[i])) {
			if (result != NULL)
				*result = &cardman_ctrl.soft_card[i];
			return CARD_TYPE_SOFTWARE_CARD;
		}
	}

	for (i = 0; i < cardman_ctrl.header.nkeys; ++i) {
		if (cardIsEqual (uid, len, &cardman_ctrl.key_card[i])) {
			if (result != NULL)
				*result = &cardman_ctrl.key_card[i];
			return CARD_TYPE_KEY;
		}
	}

	return CARD_TYPE_UNKNOWN;
}

s8 cardmanSetProgrammingCard (const u8 *uid, const u8 len)
{
	struct key_page prog_card;
	enum card_type type = cardmanGetCardType (uid, len, NULL);

	prog_card.used = ENTRY_USED;

	switch (type) {
	case CARD_TYPE_PROGRAMMING_CARD:
		return ERR_NONE;

	case CARD_TYPE_KEY: /* As in old HW */
	case CARD_TYPE_UNKNOWN:
		/* Save new programming card to eeprom */
		copy_to_entry (&prog_card.card, uid, len);
		cardman_ctrl.prog_card.card = prog_card.card;
		cardman_ctrl.prog_card.extra[0] = prog_card.used;
		eeprom_write_buffer_to_page (DATABASE_PROG_START_PAGE,
		                             &prog_card,
		                             sizeof(prog_card));
		return ERR_NONE;

	
	case CARD_TYPE_SOFTWARE_CARD:
	default:
		return ERR_REQUEST;
	}
}

u8 cardmanGetSoftwareFunction (void)
{
	return cardman_ctrl.header.current_sw;
}

void cardmanSetSoftwareFunction (u8 sw_function)
{
	cardman_ctrl.header.current_sw = sw_function;
	write_header();
}

void cardmanSetSoftwareFunctionOpenDelay (u8 sw_function, u8 delay)
{
	cardman_ctrl.header.open_delay = delay;
	cardman_ctrl.header.current_sw = sw_function;
	write_header();
}

u8 cardmanGetOpenDelay() {
	return cardman_ctrl.header.open_delay;
}

u8 cardmanGetNumberOfKeys (void)
{
	return cardman_ctrl.header.nkeys;
}
