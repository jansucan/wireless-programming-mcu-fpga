/* Author: Jan Sucan */

#include <directc_wrapper.h>

#include <diagnostic_module/diagnostic_module.h>
#include <protocol.h>
#include <protocol_return_codes.h>
#include <application_state.h>
#include <setjmp_buffers.h>

#include <DirectC/dpalg.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief Pocet tikov systemoveho COUNT registra pre odmeranie 1 mikrosekundy.
 */
#define DCW_COUNT_DIFF_FOR_1_US 65

/*
 *   TDI  = PD00, gpio  96, port 3
 *   TDO  = PD01, gpio  97, port 3
 *   TCK  = PD02, gpio  98, port 3
 *   TMS  = PD03, gpio  99, port 3
 *   TRST = PD13, gpio 109, port 3
 */
#define DCW_TDI    96
#define DCW_TDO    97
#define DCW_TCK    98
#define DCW_TMS    99
#define DCW_TRST  109

/**
 * @brief Buffer pre ulozenie textovych vypisov z DirectC operacii.
 *
 * 1 bajt navyse je vyhradeny pre ukoncovaci nulovy bajt znakoveho retazca.
 */
char dcw_display_buffer[DCW_DISPLAY_BUFFER_SIZE + 1U];

/**
 * @brief Index do bufferu pre ulozenie textovych vypisov z DirectC operacii.
 *
 * Ukazuje na dalsi volny znak.
 */
unsigned dcw_display_buffer_index = 0U;

static void dcw_display_buffer_clear(void);
static void dcw_print(const char * format, ...);
static void dcw_configure_io(void);

/**
 * @brief Inicializacia wrapperu pre DirectC.
 *
 * Nastavia sa I/O piny pre emulovany JTAG.
 */
void
dcw_init(void)
{
	dcw_configure_io();
}

/**
 * @brief Spustenie DirectC operacie.
 *
 * @param opcode Operacny kod DirectC operacie (treba najst v dokumentacii k DirectC).
 */
int
dcw_execute_operation(DPUCHAR opcode)
{
	// Zmazat vypis z predchadzajucej operacie
	dcw_display_buffer_clear();
	// Urcit spustanu DirectC akciu
	Action_code = opcode;
	return dp_top();
}

/**
 * @brief Ziskanie textoveho vypisu DirectC operacie.
 *
 * @param text Ukazovatel na ukazovatel na zaciatok bufferu s textovym vypisom DirectC operacie.
 * @param chars Ukazovatel na premennu pre ulozenie poctu znakov textoveho vypisu.
 */
void
dcw_get_displayed_text(const char ** const text, size_t * const chars)
{
	*text = dcw_display_buffer;
	*chars = dcw_display_buffer_index;
}

/**
 * @brief Blokujuce cakanie pre potreby DirectC kodu.
 *
 * @param microseconds Pocet mikrosekund, kolko sa ma cakat (moze sa aj viac).
 */
void
dcw_delay(DPULONG microseconds)
{
	uint32_t a = 1; // a < b, vynuti sa opakovanie merania aktualnej mikrosekundy
	uint32_t b = 0;
	
	while (microseconds > 0) {
		if (b < a) {
			// Doslo k preteceniu casovaca, meranie aktualnej mikrosekundy sa spusti znovu
			a = SYSREG_COUNT_GET;
		} else if ((b - a) >= DCW_COUNT_DIFF_FOR_1_US) {
			// Uplynula minimalne 1 mikrosekunda, zacne sa s meranim dalsej
			--microseconds;
			a = SYSREG_COUNT_GET;
		}
		b = SYSREG_COUNT_GET;
	}
}

/**
 * @brief Ziskanie hodnoty vstupneho JTAG pinu.
 *
 * Funkcia zapise logicku hodnotu vstupu do datoveho registra, ktory pouziva DirectC.
 */
DPUCHAR
dcw_jtag_inp(void)
{
	const unsigned port = DCW_TDO / 32U;
	const unsigned pin = DCW_TDO % 32U;
	const uint32_t m = (1 << pin);
	
	return ((DPUCHAR) ((AVR32_GPIO.port[port].pvr & m) >> pin)) << 7;
}

/**
 * @brief Nastavenie hodnot vystupnych JTAG pinov.
 *
 * Funkcia zapise logicke hodnoty vystupov z formatu, ktory pouziva DirectC, do GPIO registrov MCU.
 *
 * @param outdata  Slovo s logickymi hodnotami vystupnych JTAG pinov.
 */
void
dcw_jtag_outp(DPUCHAR outdata)
{
	const unsigned directc_output_masks[] = {TDI, TCK, TMS, TRST};
	const unsigned gpio_outputs[] = {DCW_TDI, DCW_TCK, DCW_TMS, DCW_TRST};
		
	const unsigned output_count = sizeof(gpio_outputs) / sizeof(gpio_outputs[0]); 
		
	for (unsigned i = 0; i < output_count; ++i) {
		const unsigned port = gpio_outputs[i] / 32U;
		const unsigned pin = gpio_outputs[i] % 32U;
		const uint32_t m = (1 << pin);
		const unsigned value = outdata & directc_output_masks[i];
		
		if (value == 0) {
			AVR32_GPIO.port[port].ovrc = m;
		} else {
			AVR32_GPIO.port[port].ovrs = m;
		}
	}		
}

/**
 * @brief Pridanie textu do textoveho bufferu vypisov DirectC operacie.
 *
 * @param text  Ukazatel na znakovy retazec pre zapis do bufferu pre DirectC vystup.
 */
void
dcw_dp_display_text(const char *text)
{
	dcw_print(text);
}

/**
 * @brief Pridanie textu, reprezentujuceho ciselnu hodnotu, do textoveho bufferu vypisov DirectC operacie.
 *
 * Predvolena volba pri neznamej hodnote @p descriptive je desiatkovy format.
 *
 * @param value        Ciselna hodnota.
 * @param descriptive  DirectC kod urcujuci, v akom formate bude hodnota vypisana.
 */
void
dcw_dp_display_value(DPULONG value, DPUINT descriptive)
{
	switch (descriptive)
	{
	case HEX:
		dcw_print("%X", value);
		break;
	case CHR:
		dcw_print("%c", value);
		break;
	default: // DEC, DPULONG
	    dcw_print("%lu", value);
		break;
	}
}

/**
 * @brief Pridanie textu, reprezentujuceho pole ciselnych hodnot, do textoveho bufferu vypisov DirectC operacie.
 *
 * Predvolena volba pri neznamej hodnote @p descriptive je desiatkovy format.
 *
 * @param value        Ukazatel na pole ciselnych hodnot.
 * @param bytes        Pocet prvkov pola @p value.
 * @param descriptive  DirectC kod urcujuci, v akom formate bude hodnota vypisana.
 */
void
dcw_dp_display_array(DPUCHAR *value, DPUINT bytes, DPUINT descriptive)
{
	if (bytes >= 1) {
		dcw_dp_display_value(value[0], descriptive);
	}
	for (DPUINT i = 1; i < bytes; ++i) {
		dcw_print(", ");
		dcw_dp_display_value(value[i], descriptive);
	}
}

/**
 * @brief Pridanie textu, reprezentujuceho postup DirectC operacie, do textoveho bufferu vypisov DirectC operacie.
 *
 * Ciselna hodnota postupu P sa vypise ako retazec "Progress: P".
 *
 * @param value  Ciselna hodnota udavajuca postup DirectC operacie.
 */
void
dcw_dp_report_progress(DPUCHAR value)
{
	dcw_print("Progress: %d", value);
}

/**
 * @brief Ziskanie stranky z .DAT suboru.
 *
 * Odosle poziadavku na stranku.
 *
 * Funkcia je volana DirectC kodom, ked je potrebne ziskat dalsiu stranku z .DAT suboru. V tomto pripade nedojde k navratu z funkcie, ale
 * je zavolana hlavna slucka aplikacneho protkolu a caka sa na prikaz od PC, ktory doda prislusnu stranku z .DAT suboru.
 * 
 * Do funkcie sa tiez moze skocit zvonku pomocou longjmp() vtedy, ked PC poslal stranku z .DAT suboru. Az potom dojde k navratu z funkcie
 * a k pokracovaniu DirectC kodu, ktory tuto funkciu zavolal.
 *
 * @param image_requested_address  Offset dat stranky v .DAT subore.
 * @param return_bytes             Ukazatel na premennu obsahujucu pocet bajtov stranky. V nej bude po skonceni funkcie ulozeny pocet skutocne nacitanych bajtov alebo 0 pri chybe.
 * @param start_page_address       Ukazatel na premennu pre ulozenie offsetu prveho bajtu stranky v .DAT subore.
 * @param end_page_address         Ukazatel na premennu pre ulozenie offsetu posledneho bajtu stranky v .DAT subore.
 */
void
dcw_get_page_data_from_external_storage(DPULONG image_requested_address, DPULONG * const return_bytes,
										DPULONG * const start_page_address, DPULONG * const end_page_address)
{
	// Odoslat poziadavku na usek z .DAT suboru
	uint8_t request_data[sizeof(uint8_t) + (2U * sizeof(uint32_t))];
	SET_BYTES_FROM_UINT8_T(request_data, 0U, application_state.directc_operation_code);
	SET_BYTES_FROM_UINT32_T(request_data, 1U, (uint32_t) image_requested_address);
	SET_BYTES_FROM_UINT32_T(request_data, 5U, (uint32_t) *return_bytes);
	data_send(request_data, sizeof(request_data));
	
	// Nastavit stav aplikacie signalizujuci, ze DirectC operacia caka na stranku z .DAT suboru
	application_state.state = APPLICATION_STATE_OPERATION_WAITING_FOR_DATA;
	application_state.directc_paging_image_offset = (uint32_t) image_requested_address;
	application_state.directc_paging_bytes_requested = (uint32_t) *return_bytes;
	
	// Navratovy kod setjmp() je bude 0 alebo pocet prijatych bajtov stranky .DAT suboru
	const int setjmp_ret = setjmp(setjmp_buffer_provide_data_to_operation);
	
	if (!setjmp_ret) {
		// Su potrebne data pre DirectC operaciu
		// Stav je ulozeny, znovu vstupit do prikazovej slucky aplikacneho protokolu pre prijem prikazov
	    protocol_loop_wait_for_command();
	} else {
		// Bolo zavolane longjmp(), tzn. ze boli dodane nejake data pre DirectC operaciu, ktore uz su naopirovane v DirectC page_global_buffer
		*return_bytes = (DPULONG) setjmp_ret;
		*start_page_address = image_requested_address;
		*end_page_address = image_requested_address + *return_bytes - 1;
	}
	// Pokracuje sa v spracovavani DirectC operacie s dodanymi datami z .DAT suboru
}

/* Static functions */

/**
 * @brief Vycistenie bufferu pre vypisy DirectC operacie.
 */
void
dcw_display_buffer_clear(void)
{
	dcw_display_buffer_index = 0;
}

void
dcw_print(const char * format, ...) {
	const size_t free_chars = DCW_DISPLAY_BUFFER_SIZE - dcw_display_buffer_index;

	if (free_chars > 0) {
		va_list args;
		va_start(args, format);
		char * const buf = dcw_display_buffer + dcw_display_buffer_index;
		const size_t chars_written = vsnprintf(buf, free_chars + 1, format, args);
		va_end(args);

		if ((chars_written > 0) && (chars_written <= free_chars)) {
			// Write was successful, update index for the next write
			dcw_display_buffer_index += chars_written;
		}
	}
}

void
dcw_configure_io(void)
{
	const unsigned gpio_inputs[] = {DCW_TDO};
	const unsigned gpio_outputs[] = {DCW_TDI, DCW_TCK, DCW_TMS, DCW_TRST};
	
	const unsigned gpio_input_count = sizeof(gpio_inputs) / sizeof(gpio_inputs[0]); 
	const unsigned gpio_output_count = sizeof(gpio_outputs) / sizeof(gpio_outputs[0]); 
	
	for (unsigned i = 0; i < gpio_input_count; ++i) {
		const unsigned port = gpio_inputs[i] / 32U;
		const unsigned pin = gpio_inputs[i] % 32U;
		const uint32_t m = (1 << pin);
		// Odpoji sa pull-up a pull-down rezistor
		AVR32_GPIO.port[port].puerc = m;
		AVR32_GPIO.port[port].pderc = m;
		// Nastavi sa ako vstup, deaktivuje sa output driver
		AVR32_GPIO.port[port].oderc = m;
		// Bude ovladany GPIO
		AVR32_GPIO.port[port].gpers = m;
	}
	
	for (unsigned i = 0; i < gpio_output_count; ++i) {
		const unsigned port = gpio_outputs[i] / 32U;
		const unsigned pin = gpio_outputs[i] % 32U;
		const uint32_t m = (1 << pin);
		// Odpoji sa pull-up a pull-down rezistor
		AVR32_GPIO.port[port].puerc = m;
		AVR32_GPIO.port[port].pderc = m;
		// Nastavi sa ako vystup, aktivuje sa output driver
		AVR32_GPIO.port[port].oders = m;
		// Bude ovladany GPIO
		AVR32_GPIO.port[port].gpers = m;
	}
}