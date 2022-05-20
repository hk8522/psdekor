/*
 * eeprom_compiler_checks.h
 *
 * Created: 21.11.2013 11:20:34
 *  Author: huber
 */ 

#ifndef EEPROM_COMPILER_CHECKS_H_
#define EEPROM_COMPILER_CHECKS_H_

#include "cardman/cards_manager.h"

#if EEPROM_PAGE_SIZE <= HEADER_ENTRY_SIZE
# error "EEPROM_PAGE_SIZE is too small (header_page)!"
#endif

#if EEPROM_PAGE_SIZE <= KEY_PAGE_SIZE
# error "EEPROM_PAGE_SIZE is too small (key_page/prog_page)!"
#endif

#if EEPROM_PAGE_SIZE <= SOFT_PAGE_SIZE
# error "EEPROM_PAGE_SIZE is too small (soft_page)!"
#endif

#if EEPROM_SIZE < (EEPROM_PAGE_SIZE * (MAX_NUMBER_OF_KEY_CARDS + ((MAX_NUMBER_OF_SOFT_CARDS / 2) + (MAX_NUMBER_OF_SOFT_CARDS % 2)) + 1 + 2))
# error "EEPROM too small for requested number of pages!"
#endif

#endif /* EEPROM_COMPILER_CHECKS_H_ */