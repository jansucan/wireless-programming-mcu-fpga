/* Author: Jan Sucan */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <diagnostic_module/diagnostic_module.h>

/**
 * @brief Ziskanie cisla prikazu z bufferu pre data prijate cez YUP.
 */
#define PROTOCOL_GET_CMD_NUMBER(b) (b[0])

/**
 * @brief Ziskanie cisla stranky z bufferu pre data prijate cez YUP.
 */
#define PROTOCOL_GET_PAGE_NUMBER(b) (GET_UINT16_T_FROM_BYTES(b, 1))

/**
 * @brief Minimum prijatych bajtov, aby sa uz jednalo o nejaky platny prikaz.
 */
#define PROTOCOL_MINIMAL_SIZE        3

/**
 * @brief Maximum prijatych bajtov, aby sa este jednalo o nejaky platny prikaz.
 */
#define PROTOCOL_MAX_RECEIVED_BYTES  515

/**
 * @brief Maximum odosielanych bajtov.
 */
#define PROTOCOL_MAX_SEND_BYTES      513

/**
 * @brief Nazov aplikacie.
 *
 * Pouzity v identifikacnom retazci aplikacie.
 */
#define PROTOCOL_APPLICATION_NAME    "DMBootloader"

/**
 * @brief Verzia aplikacie.
 *
 * Pouzita v identifikacnom retazci aplikacie.
 */
#define PROTOCOL_APPLICATION_VERSION  "0.1.0"

enum protocol_return_codes {
	PROTOCOL_OK = 0x00,
	// Chyby protokolu
	PROTOCOL_ERROR_UNKNOWN_CMD = 0x10,
	PROTOCOL_ERROR_MISSING_PAGENUM,
	PROTOCOL_ERROR_PAGENUM_OUT_OF_BOUNDS,
	PROTOCOL_ERROR_MISSING_DATA,
	PROTOCOL_ERROR_EXTRA_DATA,
	// Chyby FLASH subsystemu
	PROTOCOL_ERROR_FLASH_WRITE = 0x30,
	PROTOCOL_ERROR_FLASH_ERASE,
	PROTOCOL_ERROR_FLASH_READ
};

enum protocol_command_codes {
	PROTOCOL_CMD_WRITE_PAGE = 0,
	PROTOCOL_CMD_ERASE_PAGE,
	PROTOCOL_CMD_READ_PAGE,
	PROTOCOL_CMD_EXEC_APPLICATION,
	PROTOCOL_CMD_GET_APPLICATION_ID = UINT8_MAX
};

/**
 * @brief Pocet aplikacnych prikazov podporovanych touto aplikaciou.
 */
#define PROTOCOL_CMD_COUNT 5

// Parametre dat pre kontrolu prikazov, minumum a maximum bajtov
typedef struct cmd_data_size_s {
	size_t min_bytes;
	size_t max_bytes;
} cmd_data_size_t;
	
const cmd_data_size_t cmd_data_sizes[PROTOCOL_CMD_COUNT] = {
	{PROTOCOL_MINIMAL_SIZE + 1, PROTOCOL_MAX_RECEIVED_BYTES}, // PROTOCOL_CMD_WRITE_PAGE
	{PROTOCOL_MINIMAL_SIZE, PROTOCOL_MINIMAL_SIZE}, // PROTOCOL_CMD_ERASE_PAGE
	{PROTOCOL_MINIMAL_SIZE, PROTOCOL_MINIMAL_SIZE}, // PROTOCOL_CMD_READ_PAGE
	{PROTOCOL_MINIMAL_SIZE, PROTOCOL_MINIMAL_SIZE}  // PROTOCOL_CMD_EXEC_APPLICATION
};

static bool protocol_is_cmd_code_valid(uint8_t cmd_code);
static void protocol_send_reply_with_data(uint8_t ret_code, uint8_t * const data, uint16_t data_size);
static void protocol_send_reply(uint8_t ret_code);
static bool protocol_cmd_has_minimal_size(uint8_t cmd_code, size_t cmd_bytes);
static bool protocol_cmd_has_extra_data(uint8_t cmd_code, size_t cmd_bytes);
static bool protocol_cmd_contains_page_number(size_t cmd_bytes);

/**
 * @brief Prikazova slucka aplikacneho protokolu bootloaderu.
 *
 * Podporovane prikazy:
 *   Zapis stranky:
 *      1 B - kod prikazu
 *      2 B - cislo stranky (od 0)
 *      1 az 512 B - dat stranky
 *     Odpoved:
 *      1 B - navratovy kod
 *
 *   Mazanie stranky:
 *      1 B - kod prikazu
 *      2 B - cislo stranky (od 0)
 *     Odpoved:
 *      1 B - navratovy kod
 *
 *   Citanie stranky:
 *      1 B - kod prikazu
 *      2 B - cislo stranky (od 0)
 *     Odpoved:
 *      1 B - navratovy kod
 *      0 alebo 512 B - data stranky, 512B len pri spravnom prikaze
 *
 *   Predanie riadenia na stranku:
 *      1 B - kod prikazu
 *      2 B - cislo stranky (od 0)
 *     Odpoved:
 *      1 B - navratovy kod
 *
 *   Ziskanie identifikacneho retazca aplikacie:
 *      1 B - kod prikazu
 *     Odpoved:
 *      ASCII znaky ID retazca bez ukoncovacieho znaku '\0'.
 */
void
protocol_loop(void)
{
	while (1) {
		// Najviac dat (1 + 2 + 512 = 515B) sa bude prijimat pre prikaz zapisu stranky
		uint8_t buf[PROTOCOL_MAX_RECEIVED_BYTES];
		size_t bytes_received;
		
		// Prijmu sa data prikazu
		while (data_receive(buf, PROTOCOL_MAX_RECEIVED_BYTES, &bytes_received) != DATA_OK);
		
		// Skontroluje sa cislo prikazu, urcite sa prijal aspon 1B
		// Funkciam pre prikazy sa uz nemusi predavat cislo prikazu
		// Kontrola ci je velkost dat prikazu v prislusnych medziach
		const uint8_t cmd_number = PROTOCOL_GET_CMD_NUMBER(buf);
		
		if (!protocol_is_cmd_code_valid(cmd_number)) {
			// Nezname cislo prikazu
			protocol_send_reply(PROTOCOL_ERROR_UNKNOWN_CMD);			
		}
		
		// Cislo prikazu je OK
		if (cmd_number == PROTOCOL_CMD_GET_APPLICATION_ID) {
			// Prikaz pre ziskanie identifikacneho retazca aplikacie je OK
			// Zostavi sa identifikacny retazec
			char id_string[64];
			int id_string_length = firmware_identification_string(id_string, PROTOCOL_APPLICATION_NAME, PROTOCOL_APPLICATION_VERSION);
			// Odosle sa
			data_send((uint8_t *) id_string, id_string_length);
			// Vsetky dalsie prikazy obsahuju viac bajtov pre kontrolu
		} else if (!protocol_cmd_contains_page_number(bytes_received)) {
			// Cislo stranky nie je pritomne
			protocol_send_reply(PROTOCOL_ERROR_MISSING_PAGENUM);			
		} else if (!FLASH_IS_VALID_VIRT_PAGE_NUMBER(PROTOCOL_GET_PAGE_NUMBER(buf))) {
			// Cislo stranky nie z platneho rozsahu
			protocol_send_reply(PROTOCOL_ERROR_PAGENUM_OUT_OF_BOUNDS);
		} else if (!protocol_cmd_has_minimal_size(cmd_number, bytes_received)) {
			// Prikazu chybaju data
			protocol_send_reply(PROTOCOL_ERROR_MISSING_DATA);
		} else if (protocol_cmd_has_extra_data(cmd_number, bytes_received)) {
			// Prikaz obsahuje data navyse
			protocol_send_reply(PROTOCOL_ERROR_EXTRA_DATA);
		} else {
			// Prikaz OK, cislo stranky OK, velkost dat OK
			int r = -1;
			const uint16_t page_num = PROTOCOL_GET_PAGE_NUMBER(buf);
			
			switch (cmd_number) {
				case PROTOCOL_CMD_WRITE_PAGE:
					// Od velkosti dat sa odcita 1B prikazu a 2B cisla stranky
					r = flash_write_virtual_page(page_num, buf + 3, bytes_received - 3);
					protocol_send_reply((r == 0) ? PROTOCOL_OK : PROTOCOL_ERROR_FLASH_WRITE);
					break;
				
				case PROTOCOL_CMD_ERASE_PAGE:
					r = flash_erase_virtual_page(page_num);
					protocol_send_reply((r == 0) ? PROTOCOL_OK : PROTOCOL_ERROR_FLASH_ERASE);
					break;
				
				case PROTOCOL_CMD_READ_PAGE:
					// Nacitavame az za 0. bajt, ten nechavame pre navratovy kod odpovede, aby sme recyklovali buffer
					r = flash_read_virtual_page(page_num, buf + 1);
					if (r == 0) {
						// Uspech, mame data
						protocol_send_reply_with_data(PROTOCOL_OK, buf, PROTOCOL_MAX_SEND_BYTES);
					} else {
						// Chyba
						protocol_send_reply(PROTOCOL_ERROR_FLASH_READ);
					}
					break;
				
				case PROTOCOL_CMD_EXEC_APPLICATION:
					// Odpoved sa musi poslat vopred, pretoze po spustenie uz strati Bootloader riadenie
					protocol_send_reply(PROTOCOL_OK);
					exec_application_firmware_at_virtual_page(page_num);
					break;
			}
		}
		
	}
}

/**
 * @brief Kontrola, ci je cislo prikazu platne.
 *
 * @param cmd_code   Cislo prikazu.
 *
 * @retval true   Cislo prikazu je platne.
 * @retval false  Neplatne cislo prikazu.
 */
bool
protocol_is_cmd_code_valid(uint8_t cmd_code)
{
	return ((cmd_code == PROTOCOL_CMD_WRITE_PAGE)
			|| (cmd_code == PROTOCOL_CMD_ERASE_PAGE)
			|| (cmd_code == PROTOCOL_CMD_READ_PAGE)
			|| (cmd_code == PROTOCOL_CMD_EXEC_APPLICATION)
			|| (cmd_code == PROTOCOL_CMD_GET_APPLICATION_ID));
}

/**
 * @brief Odoslanie odpovede aplikacneho protokolu, ktora nesie data.
 *
 * @param ret_code   Navratovy kod v odpovedi.
 * @param data       Ukazovatel na buffer dat pre odoslanie. Musi mat vyhradeny prvy bajt pre umiestnenie navratoveho kodu.
 * @param data_size  Velkost bufferu v bajtoch.
 */
void
protocol_send_reply_with_data(uint8_t ret_code, uint8_t * const data, uint16_t data_size)
{
	data[0] = ret_code;
	// Predpoklada sa, ze sa odosiela navratovy kod 1B + 512B dat stranky
	data_send(data, data_size);
}

/**
 * @brief Odoslanie odpovede aplikacneho protokolu bez pripojenych dat.
 *
 * @param ret_code  Navratovy kod v odpovedi.
 */
void
protocol_send_reply(uint8_t ret_code)
{
	// Nekontroluje sa uspesnost odoslania
	data_send(&ret_code, 1);
}

/**
 * @brief Kontrola, ci prijaty prikaz obsahuje aj cislo stranky.
 *
 * @param cmd_bytes  Velkost prijateho prikazu v bajtoch.
 *
 * @retval true   Prikaz obsahuje cislo stranky.
 * @retval false  Prikaz neobsahuje cislo stranky.
 */
bool
protocol_cmd_contains_page_number(size_t cmd_bytes) {
	return (cmd_bytes >= PROTOCOL_MINIMAL_SIZE);
}

/**
 * @brief Kontrola, ci konkretny prijaty prikaz obsahuje aspon minumum dat, aby bol platny.
 *
 * @param cmd_code   Kod prijateho prikazu.
 * @param cmd_bytes  Velkost prijateho prikazu v bajtoch.
 *
 * @retval true   Prikaz je platny.
 * @retval false  Prikaz nie je platny.
 */
bool
protocol_cmd_has_minimal_size(uint8_t cmd_code, size_t cmd_bytes)
{
	return (cmd_bytes >= cmd_data_sizes[cmd_code].min_bytes);
}

/**
 * @brief Kontrola, ci konkretny prijaty prikaz obsahuje zbytocne data navyse.
 *
 * @param cmd_code   Kod prijateho prikazu.
 * @param cmd_bytes  Velkost prijateho prikazu v bajtoch.
 *
 * @retval true   Prikaz je platny.
 * @retval false  Prikaz nie je platny pretoze obsahuje zbytocne data navyse.
 */
bool
protocol_cmd_has_extra_data(uint8_t cmd_code, size_t cmd_bytes)
{
	return (cmd_bytes > cmd_data_sizes[cmd_code].max_bytes);
}
