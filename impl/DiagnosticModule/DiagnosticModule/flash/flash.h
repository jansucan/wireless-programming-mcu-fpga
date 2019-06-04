/* Author: Jan Sucan */

#ifndef FLASH_H_
#define FLASH_H_

#include <stdint.h>
#include <stdlib.h>
#include <diagnostic_module/parameters.h>

/**
 * @brief Kontrola, ci je cislo virtualnej stranky FLASH z platneho rozsahu.
 */
#define	FLASH_IS_VALID_VIRT_PAGE_NUMBER(n)  (n <= DM_APPLICATION_VIRT_MAX_PAGE)

int  flash_write_virtual_page(uint16_t page_num, const uint8_t * const data, size_t data_len);
int  flash_read_virtual_page(uint16_t page_num, uint8_t * const data);
int  flash_erase_virtual_page(uint16_t page_num);

#endif /* FLASH_H_ */