/* Author: Jan Sucan */

#include <avr32/io.h>
#include <stdbool.h>

#include <flash/flash.h>

/**
 * @brief Prevod virtualneho cisla na realne cislo stranky.
 */
#define FLASH_REAL_PAGE_NUMBER_FROM_VIRT(n)  (DM_APPLICATION_REAL_MIN_PAGE + n)

static bool flash_is_error(void);
static void flash_wait_until_ready(void);
static int flash_execute_command(int command, uint16_t page_num);

static int flash_erase_page(uint16_t page_num);
static int flash_clear_page_buffer(void);
static int flash_write_page_buffer(uint16_t page_num);
static int flash_write_page(uint16_t page_num, const uint8_t * data, size_t data_len);
static int flash_read_page(uint16_t page_num, uint8_t * const data);

/**
 * @brief Zisti, ci nastala chyba pri vykonavani prikazu pre kontroler FLASH pamate.
 *
 * @return true   Nastala chyba.
 * @return false  Ziadna chyba nenastala.
 */
bool
flash_is_error(void)
{
	return (((AVR32_FLASHC.fcr & AVR32_FLASHC_PROGE_MASK) == 1)
			|| ((AVR32_FLASHC.fcr & AVR32_FLASHC_LOCKE_MASK) == 1));	
}

/**
 * @brief Zisti, ci je kontroler FLASH pripraveny pre dalsi prikaz.
 *
 * @return true   Kontroler je pripraveny.
 * @return false  Kontroler nie je pripraveny.
 */
void
flash_wait_until_ready(void)
{
	while ((AVR32_FLASHC.fsr & AVR32_FLASHC_FSR_FRDY_MASK) == 0);
}

/**
 * @brief Vykona zadany prikaz s FLASH pamatou na zadanej stranke.
 *
 * @param command   Cislo prikazu pre vykonanie.
 * @param page_num  Cislo stranky, na ktore sa aplikuje prikaz, ak vyuziva cislo stranky.
 *
 * @return -1  Chyba pri vykonavani prikazu.
 * @return  0  Prikaz prebehol v poriadku.
 */
int
flash_execute_command(int command, uint16_t page_num)
{
	flash_wait_until_ready();
	
	AVR32_FLASHC.fcmd = (AVR32_FLASHC_KEY_KEY << AVR32_FLASHC_FCMD_KEY_OFFSET) | (page_num << AVR32_FLASHC_FCMD_PAGEN_OFFSET) | command;
	
	/* 
		Po spusteni prikazu sa musi vzdy najprv pockat. Akakolvek ina akcia ma za nasledok nespravne vykonanie prikazu.
	*/
	flash_wait_until_ready();
	
	if (flash_is_error()) {
		return -1;
	} else {
		return 0;	
	}
}

/**
 * @brief Zmaze stranku FLASH.
 *
 * @param page_num  Cislo stranky pre zmazanie.
 *
 * @return -1  Zle cislo stranky alebo chyba pri vykonavani prikazu.
 * @return  0  Mazanie prebehlo v poriadku.
 */
int
flash_erase_page(uint16_t page_num) {
	/* Stranka musi byt v rozsahu stranok */
	if (page_num >= (AVR32_FLASH_SIZE / AVR32_FLASH_PAGE_SIZE)) {
		return -1;
	}

	return flash_execute_command(AVR32_FLASHC_FCMD_CMD_EP, page_num);
}

/**
 * @brief Vycisti obsah page bufferu pre data zapisovane do stranky FLASH.
 *
 * @return -1  Chyba pri vykonavani prikazu.
 * @return  0  Page buffer uspesne vycisteny.
 */
int
flash_clear_page_buffer(void) {
	// Na stranke FLASH nezalezi
	return flash_execute_command(AVR32_FLASHC_FCMD_CMD_CPB, -1);
}
	
/**
 * @brief Zapise obsah page bufferu na danu stranku vo FLASH.
 *
 * @param page_num  Cislo stranky vo FLASH, kde ma byt zapisany obsah page bufferu.
 *
 * @return -1  Neplatne cislo stranky alebo chyba pri zapise page bufferu do FLASH.
 * @return  0  Obsah page bufferu uspesne zapisany na danu stranku @p page_num.
 */	
int
flash_write_page_buffer(uint16_t page_num)
{
	// Stranka musi byt platna
	if (page_num >= (AVR32_FLASH_SIZE / AVR32_FLASH_PAGE_SIZE)) {
		return -1;
	}
	
	return flash_execute_command(AVR32_FLASHC_FCMD_CMD_WP, page_num);
}

/**
 * @brief Zapis dat do stranky vo FLASH.
 *
 * @param page_num  Cislo stranky vo FLASH, kde maju byt data z @p data zapisane.
 * @param data      Ukazovatel na data pre zapis do FLASH.
 * @param data_len  Pocet bajtov z @p data pre zapis do FLASH.
 *
 * @return -1  Neplatne cislo stranky alebo chyba pri zapise do FLASH.
 * @return  0  Data uspesne zapisane.
 */	
int
flash_write_page(uint16_t page_num, const uint8_t * data, size_t data_len)
{	
	if (flash_erase_page(page_num) || flash_clear_page_buffer()) {
		return -1;
	}
	
	// Adresa pre pristup do page bufferu. Moze byt adresa stranky 0, pretoze stranku specifikujem explicitne.
	volatile uint32_t * page_buf_addr = (volatile uint32_t *) AVR32_FLASH_ADDRESS;		
		
	while (data_len > 0) {
		size_t chunk_size = ((data_len >= sizeof(uint32_t)) ? sizeof(uint32_t) : data_len);
		uint32_t w = 0xFFFFFFFF;
		uint8_t *dst = (uint8_t *) &w;
		
		for (size_t i = 0; i < chunk_size; ++i) {
			*(dst++) = *(data++);
		} 
		
		*(page_buf_addr++) = w;
		
		data_len -= chunk_size;
	}

	// Zapise sa page buffer do stranky FLASH 
	if (flash_write_page_buffer(page_num)) {
		return -1;
	}
		
	return 0;
}

/**
 * @brief Precitanie dat zo stranky vo FLASH.
 *
 * @param page_num  Cislo stranky vo FLASH, odkial sa budu citat data do @p data.
 * @param data      Ukazovatel kam bude ulozenych 512 B stranky.
 *
 * @return 0  Data precitane.
 */	
int
flash_read_page(uint16_t page_num, uint8_t * const data)
{
	uint32_t address = AVR32_FLASH_ADDRESS + (page_num * AVR32_FLASH_PAGE_SIZE);
	
	for (size_t i = 0; i < AVR32_FLASH_PAGE_SIZE; ++i, ++address) {
		data[i] = *((uint8_t *) address);
	}
	
	return 0;
}

/**
 * @brief Zapis dat do virtualnej stranky vo FLASH.
 *
 * @param page_num  Cislo virtualnej stranky vo FLASH, kde maju byt data z @p data zapisane.
 * @param data      Ukazovatel na data pre zapis do FLASH.
 * @param data_len  Pocet bajtov z @p data pre zapis do FLASH.
 *
 * @return -1  Neplatne cislo stranky.
 * @return -1  Ukazovatel @o data je NULL
 * @return -1  Dlzka dat je 0.
 * @return -1  Dlzka dat je vacsia ako velkost stranky.
 * @return  0  Data uspesne zapisane.
 */
int
flash_write_virtual_page(uint16_t page_num, const uint8_t * const data, size_t data_len)
{
	if (!FLASH_IS_VALID_VIRT_PAGE_NUMBER(page_num) || (data == NULL) || (data_len == 0) || (data_len > AVR32_FLASH_PAGE_SIZE)) {
		return -1;
	}
	// Prepocita sa cislo virtualnej stranky na cislo realnej stranky a zpisu sa tam data
	return flash_write_page(FLASH_REAL_PAGE_NUMBER_FROM_VIRT(page_num), data, data_len);
}

/**
 * @brief Precitanie dat z virtualnej stranky vo FLASH.
 *
 * @param page_num  Cislo virtualnej stranky vo FLASH, odkial sa budu citat data do @p data.
 * @param data      Ukazovatel kam bude ulozenych 512 B stranky.
 *
 * @return -1  Cislo virtualnej stranky je mimo rozsahu.
 * @return -1  Ukazovatel @p data je NULL
 * @return  0  Data precitane.
 */
int
flash_read_virtual_page(uint16_t page_num, uint8_t * const data)
{
	if (!FLASH_IS_VALID_VIRT_PAGE_NUMBER(page_num) || (data == NULL)) {
		return -1;
	}
	// Prepocita sa cislo virtualnej stranky na cislo realnej stranky a zpisu sa tam data
	return flash_read_page(FLASH_REAL_PAGE_NUMBER_FROM_VIRT(page_num), data);
}

/**
 * @brief Zmaze virtualnu stranku z FLASH.
 *
 * @param page_num  Cislo virtualnej stranky pre zmazanie.
 *
 * @return -1  Cislo virtualnej stranky mimo rozsahu.
 * @return -1  Chyba mazania.
 * @return  0  Mazanie prebehlo v poriadku.
 */
int
flash_erase_virtual_page(uint16_t page_num)
{
	if (!FLASH_IS_VALID_VIRT_PAGE_NUMBER(page_num)) {
		return -1;
	}
	return flash_erase_page(FLASH_REAL_PAGE_NUMBER_FROM_VIRT(page_num));
}