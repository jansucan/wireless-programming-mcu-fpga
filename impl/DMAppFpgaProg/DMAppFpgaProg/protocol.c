/* Author: Jan Sucan */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <diagnostic_module/diagnostic_module.h>

#include <protocol.h>
#include <application_state.h>
#include <protocol_return_codes.h>
#include <directc_wrapper.h>
#include <setjmp_buffers.h>
#include <fpga.h>

#include <dpcom.h> // Pre pristup k hodnote PAGE_BUFFER_SIZE

/**
 * @brief Pocet bajtov platneho prikazu pre spustenie DirectC operacie pre FPGA.
 *
 * 1B je kod prikazu v aplikacnom protokole a 1B je kod DirectC operacie.
 */
#define PROTOCOL_CMD_EXECUTE_OPERATION_BYTES  2U

/**
 * @brief Pocet bajtov prikazu pre ziskanie textoveho vypisu o priebehu naposledy spustenej DirectC operacie.
 */
#define PROTOCOL_CMD_GET_OPERATION_LOG_BYTES 1U

/**
 * @brief Pocet bajtov prikazu pre ziskanie stavu aplikacie.
 */
#define PROTOCOL_CMD_GET_APPLICATION_STATUS_BYTES  1U

/**
 * @brief Pocet bajtov prikazu pre zrusenie aktualne prebiehajucej DirectC operacie.
 */
#define PROTOCOL_CMD_CANCEL_CURRENT_OPERATION_BYTES  1U

/**
 * @brief Maximalny pocet bajtov prikazu pre dodanie stranky .DAT suboru do aktualne prebiehajucej DirectC operacie.
 *
 * 1B je kod prikazu v aplikacnom protokole a PAGE_BUFFER_SIZE su data z prislusnej stranky .DAT suboru.
 */
#define PROTOCOL_CMD_DELIVER_DATA_TO_OPERATION_MAX_BYTES  (1U + PAGE_BUFFER_SIZE)

/**
 * @brief Pocet bajtov platneho prikazu pre ovladanie napajania FPGA.
 *
 * 1B je kod prikazu v aplikacnom protokole a 1B je priznak zapnutia/vypnutia napajania.
 */
#define PROTOCOL_CMD_FPGA_POWER_SUPPLY_CTRL_BYTES  2U

/**
 * @brief Pocet bajtov najvacsieho platneho prikazu.
 *
 * Najvacsi platny prikaz je prikaz pre spustenie DirectC operacie. Ma 2 bajty (kod prikzu, kod DirectC operacie).
 */
#define PROTOCOL_CMD_MAX_BYTES  PROTOCOL_CMD_DELIVER_DATA_TO_OPERATION_MAX_BYTES

/**
 * @brief Nazov aplikacie.
 *
 * Pouzity v identifikacnom retazci aplikacie.
 */
#define PROTOCOL_APPLICATION_NAME    "DMAppFpgaProg"

/**
 * @brief Verzia aplikacie.
 *
 * Pouzita v identifikacnom retazci aplikacie.
 */
#define PROTOCOL_APPLICATION_VERSION  "0.1.0"

enum protocol_command_codes {
	PROTOCOL_CMD_EXECUTE_OPERATION = 0,
	PROTOCOL_CMD_GET_OPERATION_LOG,
	PROTOCOL_CMD_GET_APPLICATION_STATUS,
	PROTOCOL_CMD_CANCEL_CURRENT_OPERATION,
	PROTOCOL_CMD_DELIVER_DATA_TO_OPERATION,
    PROTOCOL_CMD_FPGA_POWER_SUPPLY_CTRL,
	PROTOCOL_CMD_GET_APPLICATION_ID = UINT8_MAX
};

static void protocol_cmd_execute_operation(const uint8_t * const buf, size_t bytes_received);
static void	protocol_cmd_get_operation_log(size_t bytes_received);
static void	protocol_cmd_get_application_status(size_t bytes_received);
static void	protocol_cmd_cancel_current_operation(size_t bytes_received);
static void protocol_cmd_deliver_data_to_operation(const uint8_t * const buf, size_t bytes_received);
static void	protocol_cmd_fpga_power_supply_ctrl(const uint8_t * const buf, size_t bytes_received);
static void protocol_cmd_get_application_id(void);
static void protocol_send_reply_with_data(uint8_t ret_code, uint8_t * const data, uint16_t data_size);
static void protocol_send_reply(uint8_t ret_code);

/**
 * @brief Inicializacia aplikacneho protokolu pre programovanie FPGA a vstup do prikazovej slucky pre cakanie na prikazy.
 *
 * Podporovane prikazy:
 *   Spustenie DirectC operacie pre FPGA:
 *      1 B - kod prikazu
 *      1 B - kod DirectC operacie
 *     Odpoved v pripade chyby:
 *      1 B - navratovy kod chyby
 *     Odpoved v pripade dokoncenia operacie:
 *      1 B - navratovy kod aplikacneho protokolu
 *      1 B - navratovy kod DirectC operacie
 *     Odpoved v pripade potreby dalsej stranky z .DAT suboru:
 *      1 B - kod aktualne spustenej DirectC operacie
 *      4 B - offset pozadovanej stranky z .DAT suboru
 *      4 B - pocet bajtov pozadovanej stranky z .DAT suboru
 *
 *
 *   Ziskanie textoveho vypisu o priebehu naposledy spustenej DirectC operacie:
 *      1 B - kod prikazu
 *     Odpoved v pripade chyby:
 *      1 B - navratovy kod chyby
 *     Odpoved v pripade uspechu:
 *      1 B - navratovy kod signalizujuci uspech
 *      ASCII znaky textoveho vypisu o priebehu naposledy spustenej DirectC operacie bez ukoncovacich znakov '\0'.
 *
 *
 *   Ziskanie stavu aplikacie:
 *      1 B - kod prikazu
 *     Odpoved v pripade chyby:
 *      1 B - navratovy kod chyby
 *     Odpoved v pripade, kedy aplikacia caka na kod prikazu DirectC operacie:
 *      1 B - navratovy kod aplikacie signalizujuci uspech
 *     Odpoved v pripade, kedy prebiehajuca DirectC operacia caka na stranku z .DAT suboru:
 *      1 B - kod aktualne prebiehajucej DirectC operacie
 *      4 B - offset pozadovanej stranky z .DAT suboru
 *      4 B - pocet bajtov pozasovanej stranky z daneho offsetu
 *
 *
 *   Zrusenie aktualne prebiehajucej DirectC operacie:
 *      1 B - kod prikazu
 *     Odpoved v pripade chyby:
 *      1 B - navratovy kod chyby
 *     Odpoved v pripade uspechu:
 *      1 B - navratovy kod signalizujuci uspech
 *
 *
 *   Dodanie stranky z .DAT suboru do aktualne prebiehajucej DirectC operacie:
 *      1 B - kod prikazu
 *      N B - data stranky z .DAT suboru
 *     Odpoved v pripade dokoncenia operacie:
 *      1 B - navratovy kod aplikacneho protokolu
 *      1 B - navratovy kod DirectC operacie
 *     Odpoved v pripade potreby dalsej stranky z .DAT suboru:
 *      1 B - kod aktualne spustenej DirectC operacie
 *      4 B - offset pozadovanej stranky z .DAT suboru
 *      4 B - pocet bajtov pozadovanej stranky z .DAT suboru
 *
 *
 *   Zapinanie a vypinanie napajania FPGA cez signal nPRG
 *      1 B - kod prikazu
 *      1 B - 0 vypnut napajanie, != 0 zapnut napajanie
 *     Odpoved:
 *      1 B - navratovy kod
 *
 *
 *   Ziskanie identifikacneho retazca aplikacie:
 *      1 B - kod prikazu
 *     Odpoved:
 *      ASCII znaky ID retazca bez ukoncovacieho znaku '\0'.
 */
void
protocol_loop(void)
{
	// Nie je potrebne rozlisovat, ci sa sem tok programu dostal cez longjmp() alebo nie
	// V kazdom pripade sa inicializuje protokol
	setjmp(setjmp_buffer_cancel_operation);
	// Inicializacia wrapperu pre DirectC (stav JTAG I/O pinov)
	dcw_init();
	// Inicializacia stavu aplikacie
	application_state_init();
	protocol_loop_wait_for_command();		
}

/**
 * @brief Prikazova slucka aplikacneho protokolu pre prijem a spracovavanie prikazov.
 *
 * Kod tejto prikazovej slucky nemoze byt priamo sucastou funkcie protocol_loop(),
 * ale musi to byt samostatna funkcia. Z main() sa k tejto prikazovej slucke musi
 * dostat riadenie s inicializaciou (vo funkcii protocol_loop()). Ked sa ale vracia
 * riadenie do slucky z DirectC wrapperu, inicializacia sa musi preskocit, inak sa
 * 1) ulozi setjmp kontext spolu s rozpracovanym prikazom a 2) prepise sa stav
 * aplikacie na stav po initializacii.
 */
void
protocol_loop_wait_for_command(void)
{
	while (1) {
    	uint8_t buf[PROTOCOL_CMD_MAX_BYTES];
    	size_t bytes_received;
    	
    	// Prijmu sa data prikazu
    	while (data_receive(buf, PROTOCOL_CMD_MAX_BYTES, &bytes_received) != DATA_OK);
    	
    	// Ziska sa kod prikazu
    	const uint8_t cmd_code = GET_UINT8_T_FROM_BYTES(buf, 0);
    	
    	// Dalej sa spracje konkretny prikaz
    	switch (cmd_code) {
        	case PROTOCOL_CMD_EXECUTE_OPERATION:
        	protocol_cmd_execute_operation(buf, bytes_received);
        	break;
        	
        	case PROTOCOL_CMD_GET_OPERATION_LOG:
        	protocol_cmd_get_operation_log(bytes_received);
        	break;
        	
        	case PROTOCOL_CMD_GET_APPLICATION_STATUS:
        	protocol_cmd_get_application_status(bytes_received);
        	break;
        	
        	case PROTOCOL_CMD_CANCEL_CURRENT_OPERATION:
        	protocol_cmd_cancel_current_operation(bytes_received);
        	break;
        	
        	case PROTOCOL_CMD_DELIVER_DATA_TO_OPERATION:
        	protocol_cmd_deliver_data_to_operation(buf, bytes_received);
        	break;
            
            case PROTOCOL_CMD_FPGA_POWER_SUPPLY_CTRL:
            protocol_cmd_fpga_power_supply_ctrl(buf, bytes_received);
            break;
        	
        	case PROTOCOL_CMD_GET_APPLICATION_ID:
        	protocol_cmd_get_application_id();
        	break;

        	default:
        	// Neznamy kod prikazu
        	protocol_send_reply(PROTOCOL_ERROR_UNKNOWN_CMD_CODE);
        	break;
    	}
	}    
}

/**
 * @brief Spracovanie prikazu pre spustenie DirectC operacie pre FPGA.
 *
 * \param buf             Data prijateho prikazu.
 * \param bytes_received  Pocet bajtov bufferu \p buf.
 */
void
protocol_cmd_execute_operation(const uint8_t * const buf, size_t bytes_received)
{
	if (bytes_received != PROTOCOL_CMD_EXECUTE_OPERATION_BYTES) {
		// Neplatna velkost prikazu
		protocol_send_reply(PROTOCOL_ERROR_INCORRECT_CMD_SIZE);
	} else if (application_state.state != APPLICATION_STATE_WAITING_FOR_OPERATION_CODE) {
	    // Nejaka DirectC operacia uz aktualne prebieha
	    protocol_send_reply(PROTOCOL_ERROR_OPERATION_IN_PROGRESS);
	} else {
		// OK, ziska sa kod operacie, overenie jeho spravnosti sa necha na DirectC kod
		const uint8_t directc_operation_code = GET_UINT8_T_FROM_BYTES(buf, 1);
		// Kod spustenej operacie sa ulozi do stavu aplikacie
		application_state.directc_operation_code = directc_operation_code;
		// Navratove hodnoty DirectC operacii sa vojdu do 8 bitov (manualne skontrolovane v DrectC/dpalg.h)
		const uint8_t directc_retcode = dcw_execute_operation((DPUCHAR) directc_operation_code);
		// Operacia dokoncena, zmeni sa stav aplikacie (bude sa cakat na kod dalsej DirectC operacie)
		application_state.state = APPLICATION_STATE_WAITING_FOR_OPERATION_CODE;		
		// Odosle sa vysledok DirectC operacie (1 B pre kod aplikacneho protokolu, 1 B pre navratovy kod DirectC)
		uint8_t data[2];
		data[1] = directc_retcode;
		protocol_send_reply_with_data(PROTOCOL_OK, data, sizeof(data));
	}
}

/**
 * @brief Spracovanie prikazu pre ziskanie textoveho vypisu o priebehu naposledy spustenej DirectC operacie.
 *
 * \param bytes_received  Pocet bajtov prijateho prikazu.
 */
void
protocol_cmd_get_operation_log(size_t bytes_received)
{
	if (bytes_received != PROTOCOL_CMD_GET_OPERATION_LOG_BYTES) {
		// Neplatna velkost prikazu
		protocol_send_reply(PROTOCOL_ERROR_INCORRECT_CMD_SIZE);
	} else {
		// OK, zostavi sa odpoved
		// 1 B vyhradeny pre navratovy kod aplikacie
		uint8_t data[1U + DCW_DISPLAY_BUFFER_SIZE];
		const char * log_data;
		size_t log_size;
		dcw_get_displayed_text(&log_data, &log_size);
		// Prekopirovat data do bufferu s vyhradenym prvym bajtom na zaciatku
		memcpy(data + 1U, log_data, log_size);
		// Odoslanie logu
		protocol_send_reply_with_data(PROTOCOL_OK, data, 1U + log_size);
	}	
}

/**
 * @brief Spracovanie prikazu pre ziskanie stavu aplikacie.
 *
 * \param bytes_received  Pocet bajtov prijateho prikazu.
 */
void
protocol_cmd_get_application_status(size_t bytes_received)
{
	if (bytes_received != PROTOCOL_CMD_GET_APPLICATION_STATUS_BYTES) {
		// Neplatna velkost prikazu
		protocol_send_reply(PROTOCOL_ERROR_INCORRECT_CMD_SIZE);
	} else if (application_state.state == APPLICATION_STATE_WAITING_FOR_OPERATION_CODE) {
		// OK, aplikacia caka na spustenie DirectC operacie
		protocol_send_reply(PROTOCOL_OK);
	} else {
		// OK, prebieha DirectC operacia a caka na stranku z .DAT suboru
		const size_t data_size = sizeof(application_state.directc_operation_code)
								 + sizeof(application_state.directc_paging_image_offset)
								 + sizeof(application_state.directc_paging_bytes_requested);
		uint8_t data[data_size];
		// Kod stavu aplikacie sa bude odosielat vzdy
		SET_BYTES_FROM_UINT8_T(data, 0U, application_state.directc_operation_code);
		SET_BYTES_FROM_UINT32_T(data, 1U, application_state.directc_paging_image_offset);
		SET_BYTES_FROM_UINT32_T(data, 5U, application_state.directc_paging_bytes_requested);
		
		// Odoslat pripravene data odpovede
		protocol_send_reply_with_data(PROTOCOL_OK, data, data_size);
	}
}

/**
 * @brief Spracovanie prikazu pre zrusenie aktualne prebiehajucej DirectC operacie.
 *
 * \param bytes_received  Pocet bajtov prijateho prikazu.
 */
void
protocol_cmd_cancel_current_operation(size_t bytes_received)
{
	if (bytes_received != PROTOCOL_CMD_CANCEL_CURRENT_OPERATION_BYTES) {
		// Neplatna velkost prikazu
		protocol_send_reply(PROTOCOL_ERROR_INCORRECT_CMD_SIZE);
	} else if (application_state.state == APPLICATION_STATE_WAITING_FOR_OPERATION_CODE)  {
		// Ziadna DirectC operacia neprebieha
		protocol_send_reply(PROTOCOL_ERROR_NO_OPERATION_IN_PROGRESS);
	} else {
		// OK, odosle sa odpoved
		protocol_send_reply(PROTOCOL_OK);
		// Skoci sa na zaciatok slucky aplikacneho protokolu
		longjmp(setjmp_buffer_cancel_operation, 1);
	}
}

/**
 * @brief Spracovanie prikazu pre dodanie stranky .DAT suboru do aktualne prebiehajucej DirectC operacie.
 *
 * \param buf             Data prijateho prikazu.
 * \param bytes_received  Pocet bajtov bufferu \p buf.
 */
void
protocol_cmd_deliver_data_to_operation(const uint8_t * const buf, size_t bytes_received)
{
	if (application_state.state == APPLICATION_STATE_WAITING_FOR_OPERATION_CODE) {
        // Ziadna DirectC operacia neprebieha
        protocol_send_reply(PROTOCOL_ERROR_NO_OPERATION_IN_PROGRESS);
    } else if (((bytes_received - 1U) < application_state.directc_paging_bytes_requested)
                || (bytes_received > PROTOCOL_CMD_DELIVER_DATA_TO_OPERATION_MAX_BYTES)) {
        // Neplatna velkost prikazu
        protocol_send_reply(PROTOCOL_ERROR_INCORRECT_CMD_SIZE);
	} else {
		// OK, nakopirovat data do DirectC page bufferu
		memcpy(page_global_buffer, buf + 1U, bytes_received - 1U);
		// Pocet prijatych bajtov .DAT suboru je urcite nenulovy (definovany konstantami pre kontrolu velkosti prijateho prikazu)
		// Do DirectC wrapperu sa pocet bajtov stranky preda cez setjmp()/longjmp()
		longjmp(setjmp_buffer_provide_data_to_operation, (int) (bytes_received - 1U));
	}	
}

/**
 * @brief Spracovanie prikazu pre zapinanie a vypinanie napajania FPGA.
 *
 * \param buf             Data prijateho prikazu.
 * \param bytes_received  Pocet bajtov bufferu \p buf.
 */
void
protocol_cmd_fpga_power_supply_ctrl(const uint8_t * const buf, size_t bytes_received)
{
	if (bytes_received != PROTOCOL_CMD_FPGA_POWER_SUPPLY_CTRL_BYTES) {
    	// Neplatna velkost prikazu
    	protocol_send_reply(PROTOCOL_ERROR_INCORRECT_CMD_SIZE);
	} else {
        // OK, zapne/vypne sa napajanie FPGA
        if (buf[1U] == 0) {
            fpga_power_supply_off();
        } else {
            fpga_power_supply_on();
        }                            	
    	protocol_send_reply(PROTOCOL_OK);
	}    
}

/**
 * @brief Spracovanie prikazu pre ziskanie identifikacneho retazca aplikacie.
 */
void
protocol_cmd_get_application_id(void)
{
	// Prikaz pre ziskanie identifikacneho retazca aplikacie je OK
	// Zostavi sa identifikacny retazec
	char id_string[64];
	int id_string_length = firmware_identification_string(id_string, PROTOCOL_APPLICATION_NAME, PROTOCOL_APPLICATION_VERSION);
	// Odosle sa
	data_send((uint8_t *) id_string, id_string_length);	
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
