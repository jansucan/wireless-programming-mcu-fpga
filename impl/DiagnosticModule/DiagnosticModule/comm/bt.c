/* Author: Jan Sucan */

#include <avr32/io.h>
#include <stdio.h>
#include <stdarg.h>

#include <comm/bt.h>
#include <comm/usart.h>
#include <diagnostic_module/parameters.h>


typedef enum {
	COBS_WAITING_FOR_THE_FIRST_DELIMITER,
	COBS_WAITING_FOR_STUFFED_BYTE,
	COBS_WAITING_FOR_NON_STUFFED_BYTE
} cobs_state_t;

static void         bt_module_to_usart_pins(void);
static bt_retcode_t bt_retcode_from_usart_retcode(usart_retcode_t ur);

/**
 * @brief Inicializacia bluetooth modulu na diagnostickom module.
 *
 * Nakonfiguruje sa USART, bluetooth modul sa prepoji na USART0.
 */
void
bt_init(void)
{
	usart_init();
	bt_module_to_usart_pins();
}

/**
 * @brief Prepojenie signalov RxD a TxD bluetooth modulu na USART0 MCU.
 *
 * Vyuziva signal PRP na diagnostickom module pre ovladanie prepinacej logiky.
 */
void
bt_module_to_usart_pins(void)
{
	/*
		(PC21 = PRP = GPIO number 85 = port 2 pin 21) ma byt na nule
		(PD27 = TXD0 = GPIO number 123 = port 3 pin 27) pripojeny na RxL
		(PD28 = RXD0 = GPIO number 124 = port 3 pin 28) pripojeny na TxL
	*/
	
	/* Pre PC21 je bit v GPER na 1 */
	AVR32_GPIO.port[2].gpers = 1 << 21;
	/* PC21 ovladany GPIO modulom, aktivacia driveru pinu */
	AVR32_GPIO.port[2].oders = 1 << 21;
	/* Pomocou clear registra nastavime 0 na PC21 */
	AVR32_GPIO.port[2].ovrc = 1 << 21;
	AVR32_GPIO.port[2].puers = 1 << 21;
	
	/* PD27 a PD28 su ovladane USART, vynuluju sa prislusne bity v GPER */
	AVR32_GPIO.port[3].gperc = 1 << 27;
	AVR32_GPIO.port[3].gperc = 1 << 28;
	
	/* Pre PD27 a PD28 sa vyberie funkcia A (USART0), to je default po resete */
}

/**
 * @brief Odoslanie bajtov cez bluetooth.
 *
 * Odosielane znaky prechadzaju escapovacou vrstvou, aby sa zabranilo odoslanie retazca
 * "$$$" do bluetooth modulu a neziaducej aktivacii prikazoveho rezimu bluetooth modulu.  
 *
 * @param buf  Buffer s bajtmi pre odoslanie.
 * @param n    Pocet bajtov pre odoslanie z bufferu @p buf.
 */
void
bt_send_data(const uint8_t *const buf, size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		usart_send_byte(buf[i]);
	}
}

/**
 * @brief Prijem bajtov cez bluetooth.
 *
 * @param buf           Ukazovatel na buffer pre ulozenie prijatych bajtov.
 * @param n             Pocet bajtov pre prijatie.
 * @param time_deadline Ukazovatel na casovy deadline, dokedy najneskor sa musia vsetky byty prijat.
 *
 * @return 0 v pripade uspechu.
 * @return != 0 pri chybe prijmu znaku, alebo chybe argumentov.
 */
bt_retcode_t
bt_receive_data(uint8_t * const buf, size_t n, const deadline_t * const time_deadline)
{
	bt_retcode_t r = BT_RECEIVE_BAD_ARGUMENT;
	
	// Kontrola argumentov
	if (buf != NULL) {
		for (size_t i = 0; i < n; ++i) {
			// Bajty sa musia prijat v danom casovom okne trvajcom ms milisekund od timestamp
			usart_retcode_t ur = usart_receive_byte (buf + i, time_deadline);
			
			if (ur != 0) {
				r = bt_retcode_from_usart_retcode(ur);
				break;
			}
		}
		// Prijem uspesny
		r = BT_OK;
	}
		
	return r;
}

/**
 * @brief Stavovy automat pre prijem bajtov s COBS.
 *
 * @param b              Bajt prijaty cez USART.
 * @param buf            Ukazovatel na buffer pre ukladanie odstuffovanych bajtov.
 * @param i              Ukazovatel na index do bufferu @p buf.
 * @param cobs_continue  Priznak, ci sa jedna o pokracovanie prijmu ramca, alebo sa bude prijimat novy ramec.
 *
 * @return 0     Ak je vsetko OK.
 * @return != 0  Chyba.
 */
bt_retcode_t
bt_receive_cobs_automaton(uint8_t b, uint8_t * const buf, size_t * const i, bool cobs_continue)
{
	static cobs_state_t cobs_state = COBS_WAITING_FOR_THE_FIRST_DELIMITER;
	static uint8_t next_non_stuffed = 0;
	static bool interrupted = false;
	
	bt_retcode_t r = BT_OK;
	
	if (interrupted) {
		// Delimiter bol prijaty v minulom volani tejto funkcie, 'b' je uz nasledujuci bajt
		interrupted = false;
		// Jeden prazdny priechod nadradenym for cyklom a pokracuje sa v aktualnej funkcii
		++(*i);
	} 
	
	// Frame delimiter resetuje stavovy automat a prijem bajtov
	if (b == COMM_PARAMS_COBS_DELIMITER_CHAR) {
		if (!cobs_continue) {
			cobs_state = COBS_WAITING_FOR_STUFFED_BYTE;
			*i = 0;
		} else {
			interrupted = true;
			r = BT_RECEIVE_COBS_INTERRUPTED;
		}
	} else {
		switch (cobs_state) {
		case COBS_WAITING_FOR_THE_FIRST_DELIMITER:
			// Automat deaktivovany, ztial sa neprijal frame delimiter
			break;
				
		case COBS_WAITING_FOR_STUFFED_BYTE:
			// Prijal sa stuffovany byte, ziskame pocet nasledujuci nestuffovanych bajtov
			next_non_stuffed = (b <= COMM_PARAMS_COBS_DELIMITER_CHAR) ? b : (b - 1);
			// COBS header sa nezapisuje do dat, vsetky dalsie stuffovane ano
			if (!cobs_continue && (*i > 1)) {
				buf[*i - 2] = COMM_PARAMS_COBS_DELIMITER_CHAR;
			} else {
				buf[*i] = COMM_PARAMS_COBS_DELIMITER_CHAR;
			}
			cobs_state = (next_non_stuffed == 0) ? COBS_WAITING_FOR_STUFFED_BYTE : COBS_WAITING_FOR_NON_STUFFED_BYTE;
			break;
				
		case COBS_WAITING_FOR_NON_STUFFED_BYTE:
			// Prijal sa nestuffovany bajt, nic sa s nim nerobi
			--next_non_stuffed;
			// Len sa ulozi
			buf[*i - ((cobs_continue) ? 0 : 2)] = b;
			cobs_state = (next_non_stuffed == 0) ? COBS_WAITING_FOR_STUFFED_BYTE : COBS_WAITING_FOR_NON_STUFFED_BYTE;
			break;
				
		default:
			r = BT_RECEIVE_COBS_UNKNOWN_STATE;
			break;
		}
	}

	return r;	
}

/**
 * @brief Prijem bajtov s COBS.
 *
 * @param buf            Ukazovatel na buffer kam budu ulozene prijate bajty.
 * @param n              Pocet bajtov pre prijatie.
 * @param time_deadline  Deadline, dokedy sa musi prijat @p n bajtov.
 * @param cobs_continue  Priznak, ci sa jedna o pokracovanie prijmu ramca, alebo sa bude prijimat novy ramec.
 *
 * @return 0     Ak je vsetko OK.
 * @return != 0  Chyba.
 */
bt_retcode_t
bt_receive_cobs_data(uint8_t * const buf, size_t n, const deadline_t * const time_deadline, bool cobs_continue)
{	
	bt_retcode_t r = BT_RECEIVE_BAD_ARGUMENT;
	
	// Kontrola argumentov
	if (buf != NULL) {
		size_t i;
		// Na zaciatku dat sa prijma navyse COBS frame delimiter (1B) a COBS header (1B)
		if (!cobs_continue) {
			n += 2;
		}
		
		for (i = 0; i < n; ++i) {
			uint8_t b;
			const usart_retcode_t ur = usart_receive_byte (&b, time_deadline);
		
			if (ur) {
				// Chyba pri prijme bajtu
				r = bt_retcode_from_usart_retcode(ur);
				break;
			}
		
			 const bt_retcode_t ar = bt_receive_cobs_automaton(b, buf, &i, cobs_continue);
			 
			 if (ar != BT_OK) {
				 r = ar;
				 break;
			 }
		}
		
		// Uspesny prijem a odsfuttovanie vsetkych bajtov
		if (i >= n) {
			r = BT_OK;
		}
	}
	
	return r;
}

/**
 * @brief Odoslanie bajtov s COBS.
 *
 * @param buf Ukazovatel na buffer bajtov pre odoslanie.
 * @param n   Pocet bajtov pre odoslanie.
 */
void
bt_send_cobs_data_block(const uint8_t *const buf, size_t n)
{	
	// Kontrola argumentov
	if (buf == NULL) {
		return;
	}
	// POZOR: neosetrujeme velkost dat, moze dojst k preteceniu hodnot na stuffovanych bajtoch
	
	// Odosle sa delimiter
	usart_send_byte(COMM_PARAMS_COBS_DELIMITER_CHAR);
	
	uint8_t next_non_stuffed = 0;
	size_t send_index = 0 - 1;
		
	for (size_t i = 0; i <= n; ++i) {
		if ((i == n) || (buf[i] == COMM_PARAMS_COBS_DELIMITER_CHAR)) {
			usart_send_byte((next_non_stuffed >= COMM_PARAMS_COBS_DELIMITER_CHAR) ? (next_non_stuffed + 1) : next_non_stuffed);
			// Zacne sa odosielat az za virtualnym, alebo realnym stuffovanym bajtom
			++send_index;
			// Odoslu sa napocitane bajty
			while (next_non_stuffed > 0) {
				usart_send_byte(buf[send_index]);
				++send_index;
				--next_non_stuffed;
			}
		} else {
			// Pocitaju sa nestuffovane bajty, zatial sa nic neposiela
			++next_non_stuffed;
		}
	}
}

/**
 * @brief Konverzia navratovej hodnoty z USART vrsty do kodov pre aktualnu vrstvu.
 *
 * Skonvertovany kod je vrateny vyssim vrstvam.
 *
 * @param ur  Navratovy kod z mnoziny usart_retcode_t.
 *
 * @return Navratovy kod z mnoziny bt_retcode_t.
 */
bt_retcode_t
bt_retcode_from_usart_retcode(usart_retcode_t ur)
{
	bt_retcode_t r = BT_RECEIVE_BAD_ARGUMENT;
	
	switch (ur) {
	case USART_RECEIVE_BAD_ARGUMENT:
		r = BT_RECEIVE_BAD_ARGUMENT;
		break;
		
	case USART_RECEIVE_TIMEOUT:
		r = BT_RECEIVE_TIMEOUT;
		break;
			
	case USART_RECEIVE_ERROR:
		r = BT_RECEIVE_ERROR;
		break;
		
	case USART_OK:
		r = BT_OK;
		break;	
	}
	
	return r;
}