/* Author: Jan Sucan */

#include "serial_port.h"
#include "bt.h"

#define COMM_PARAMS_COBS_DELIMITER_CHAR '$'

typedef enum {
	COBS_WAITING_FOR_THE_FIRST_DELIMITER,
	COBS_WAITING_FOR_STUFFED_BYTE,
	COBS_WAITING_FOR_NON_STUFFED_BYTE
} cobs_state_t;

static HANDLE bt_serial_port;

static bt_retcode_t bt_retcode_from_usart_retcode(int ur);

void
bt_init(HANDLE serial_port)
{
  bt_serial_port = serial_port;
}

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

bt_retcode_t
bt_receive_cobs_data(uint8_t * const buf, size_t n, bool cobs_continue)
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
			const int ur = serial_port_read_byte(bt_serial_port, &b);
		
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

void
bt_send_cobs_data_block(const uint8_t *const buf, size_t n)
{
        uint8_t comm[2048U];
        unsigned ci = 0;

	// Kontrola argumentov
	if (buf == NULL) {
		return;
	}
	// POZOR: neosetrujeme velkost dat, moze dojst k preteceniu hodnot na stuffovanych bajtoch
	
	// Odosle sa delimiter
	comm[ci++] = COMM_PARAMS_COBS_DELIMITER_CHAR;
	
	uint8_t next_non_stuffed = 0;
	size_t send_index = 0 - 1;
		
	for (size_t i = 0; i <= n; ++i) {
		if ((i == n) || (buf[i] == COMM_PARAMS_COBS_DELIMITER_CHAR)) {
		        comm[ci++] =  (next_non_stuffed >= COMM_PARAMS_COBS_DELIMITER_CHAR) ? (next_non_stuffed + 1) : next_non_stuffed;
			// Zacne sa odosielat az za virtualnym, alebo realnym stuffovanym bajtom
			++send_index;
			// Odoslu sa napocitane bajty
			while (next_non_stuffed > 0) {
				comm[ci++] = buf[send_index];
				++send_index;
				--next_non_stuffed;
			}
		} else {
			// Pocitaju sa nestuffovane bajty, zatial sa nic neposiela
			++next_non_stuffed;
		}
	}

	serial_port_write_byte(bt_serial_port, comm, ci);
}

bt_retcode_t
bt_retcode_from_usart_retcode(int ur)
{
	bt_retcode_t r = BT_RECEIVE_BAD_ARGUMENT;

	if (ur == -2) {
	  r = BT_RECEIVE_TIMEOUT;
	} else if (ur != 0) {
	  r = BT_RECEIVE_ERROR;
	} else {
	  r = BT_OK;
	}
	
	return r;
}
