/* Author: Jan Sucan */

#include <flash/exec.h>
#include <diagnostic_module/parameters.h>

static void exec_application_firmware_at_address(uint32_t address);

/**
 * @brief Preda riadenie na zadanu adresu vo FLASH.
 *
 * Funkcia uz nepreda riadenie volajucemu.
 *
 * @param address  Adresa v adresovom priestore MCU, kam ma byt predane riadenie. 
 */
void
exec_application_firmware_at_address(uint32_t address)
{
	void (*application_firmware)(void) = (void (*)(void)) (address);
	application_firmware();
}

/**
 * @brief Preda riadenie na nulte slovo zadanej stranky z FLASH pamate programu.
 *
 * Funkcia uz nepreda riadenie volajucemu.
 *
 * @param page_num  Cislo virtualnej stranky, kam ma byt riadenie predane. Hodnota sa osetruje modulo pocet virtualnych stranok.
 */
void
exec_application_firmware_at_virtual_page(uint16_t page_num)
{	
	exec_application_firmware_at_address(AVR32_FLASH_ADDRESS + (AVR32_FLASH_PAGE_SIZE * (DM_APPLICATION_REAL_MIN_PAGE + page_num)));
}