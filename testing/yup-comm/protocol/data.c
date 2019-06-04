/* Author: Jan Sucan */

#include "data.h"
#include "frame.h"

#include <stdbool.h>

#define DATA_PROTOCOL_VERSION  0x0

#define DATA_ACK_RETRY_COUNT       4


yup_retcode_t data_is_data_frame     (const frame_t * const frame);
yup_retcode_t data_is_starting_frame (const frame_t * const frame);
yup_retcode_t data_is_ack_for_frame  (const frame_t * const ack, const frame_t * const frame);
void          data_send_ack          (uint16_t relation_id, uint16_t seq_number);
void          data_send_rej_for_frame(const frame_t * const frame, uint8_t err_code);


yup_retcode_t
data_is_data_frame(const frame_t * const frame)
{
	yup_retcode_t yr = DATA_OK;
		
	if (frame_get_data_length(frame) == 0) {
		yr = DATA_DATA_HAS_NO_PAYLOAD;
	} else if (frame_is_flag_ack_set(frame)) {
		yr = DATA_DATA_HAS_REJ_FLAG;
	} else if (frame_is_flag_rej_set(frame)) {
		yr = DATA_DATA_HAS_REJ_FLAG;
	}
		
	return yr;
}

yup_retcode_t
data_is_starting_frame(const frame_t * const frame)
{
	yup_retcode_t yr = DATA_OK;
		
	if ((yr = data_is_data_frame(frame)) != DATA_OK) {
		;
	} else if (frame_get_seq_number(frame) != 0) {
		yr = DATA_STARTING_HAS_NONZERO_SEQ_NUMBER;
	}
		
	return yr;
}

yup_retcode_t
data_is_ack_for_frame(const frame_t * const ack, const frame_t * const frame)
{
	yup_retcode_t yr = DATA_OK;
	
	if (frame_get_data_length(ack) != 0) {
		yr = DATA_ACK_HAS_PAYLOAD;
	} else if (!frame_is_flag_ack_set(ack)) {
		yr = DATA_ACK_HAS_NOT_ACK_FLAG;
	} else if (frame_is_flag_lor_set(ack)) {
		yr = DATA_ACK_HAS_LOR_FLAG;
	} else if (frame_is_flag_rej_set(ack)) {
		yr = DATA_ACK_HAS_REJ_FLAG;
	} else if (frame_get_relation_id(ack) != frame_get_relation_id(frame)) {
		yr = DATA_ACK_RELATION_ID_MISMATCH;
	} else if (frame_get_seq_number(ack) != frame_get_seq_number(frame)) {
		yr = DATA_ACK_SEQ_NUMBER_MISMATCH;
	}
	
	return yr;
}

void
data_send_ack(uint16_t relation_id, uint16_t seq_number)
{
	frame_t ack;
	
	frame_init(&ack, DATA_PROTOCOL_VERSION);
	frame_set_flag_ack(&ack);
	frame_set_relation_id(&ack, relation_id);
	frame_set_seq_number(&ack, seq_number);
	
	frame_send(&ack);
}

void
data_send_rej_for_frame(const frame_t * const frame, uint8_t err_code)
{
	frame_t rej;
	
	frame_init(&rej, frame_get_protocol_version(frame));
	frame_set_flag_rej(&rej);
	frame_set_relation_id(&rej, frame_get_relation_id(frame));
	frame_set_seq_number(&rej, frame_get_seq_number(frame));
	frame_set_data(&rej, &err_code, sizeof(err_code));
	
	frame_send(&rej);
}


typedef enum data_receive_automaton_states {
	DATA_R_WAITING_FOR_STARTING_FRAME,
	DATA_R_START_RELATION,
	DATA_R_SAVE,
	DATA_R_ACK_LAST_FRAME,
	DATA_R_WAITING_NEXT_DATA_FRAME,
	DATA_R_END
} data_receive_automaton_state_t;

typedef enum data_send_automaton_states {
	DATA_S_START_RELATION,
	DATA_S_PEPARE_DATA_FRAME,
	DATA_S_SEND_DATA_FRAME,
	DATA_S_WAIT_ACK,
	DATA_S_END
} data_send_automaton_state_t;

yup_retcode_t
data_receive(uint8_t * const buf, size_t bufsize, size_t * const bytes_read)
{
	uint16_t relation_id = 0;
	uint16_t sequence_num = 0;
	size_t data_index = 0;
	data_receive_automaton_state_t state = DATA_R_WAITING_FOR_STARTING_FRAME;
	
	// Kontrola argumentov
	if ((buf == NULL) || (bufsize == 0)) {
		return DATA_ERROR;
	}
	
	frame_t f;
			
	while (state != DATA_R_END) {
			
		yup_retcode_t r;
		uint8_t l;
				
		switch (state) {
		case DATA_R_WAITING_FOR_STARTING_FRAME:
			// Na prijem startovacieho ramca sa bude cakat donekonecna
			r = frame_receive(&f, DATA_PROTOCOL_VERSION);		
			if (r != FRAME_OK) {
				// Chyba pri prijme ramca, posle sa naspat REJ so spravou o chybe
				data_send_rej_for_frame(&f, r);
			} else if ((r = data_is_starting_frame(&f)) == DATA_OK) {
				// Mame platny startovaci ramec, zahajime relaciu
				state = DATA_R_START_RELATION;
			}
			break;
			
		case DATA_R_START_RELATION:
			// Od startovacieho ramca zacne plynut cas pre timeout relacie
			relation_id = frame_get_relation_id(&f);
			sequence_num = frame_get_seq_number(&f);
			data_index = 0;
			state = DATA_R_SAVE;
			break;
			
		case DATA_R_SAVE:
			if ((bufsize - data_index) < frame_get_data_length(&f)) {
				// Data sa nezmestia do bufferu
				// Pocka sa na dalsi paket, co sa bude hodit
				state = DATA_R_WAITING_NEXT_DATA_FRAME;
			} else {
				// Data sa ulozia
				frame_get_data(&f, buf + data_index, &l);
				// Ulozia sa informacie o naposledy prijatom ramci
				sequence_num = frame_get_seq_number(&f);
				data_index += frame_get_data_length(&f);
				// Ramec sa potvrdi	
				state = DATA_R_ACK_LAST_FRAME;
			}
			break;
			
		case DATA_R_ACK_LAST_FRAME:
			// Potvrdime naposledy prijaty ramec 
			data_send_ack(relation_id, sequence_num);
			// Na posledny ramec relacie sa uz neocakava odpoved
			// Je bezpecne pristupovat k ramcu f, ak ma nastaveny LOR nemoze byt prepisany dalsim ramcom
			if (frame_is_flag_lor_set(&f)) {
				state = DATA_R_END;
			} else {
				// Pockame na prijem dalsieho datoveho ramca
				state = DATA_R_WAITING_NEXT_DATA_FRAME;
			}
			break;
			
		case DATA_R_WAITING_NEXT_DATA_FRAME:
			r = frame_receive(&f, DATA_PROTOCOL_VERSION);
			if (r == FRAME_TIMEOUT) {
				// Pri timeoute prijmu prijmu sa nebude opakovane posielat ACK
			        // Timeout relacie vyprsal
			        state = DATA_R_END;
			} else if (r != FRAME_OK) {
				// Chyba pri prijme datoveho ramca
				data_send_rej_for_frame(&f, r);
			} else if ((r = data_is_data_frame(&f)) != DATA_OK) {
				// Datovy ramec prijaty v poriadku, ale s nespavnym obsahom
				data_send_rej_for_frame(&f, r);
			} else if ((frame_get_seq_number(&f) == 0) && (frame_get_relation_id(&f) != relation_id)) {
				// Jedna sa o zaciatok novej relacie
				state = DATA_R_START_RELATION;
			} else if (frame_get_relation_id(&f) != relation_id) {
				// Obycajny datovy ramec, ale v odlisnej relacii
				data_send_rej_for_frame(&f, (uint8_t) DATA_DATA_RELATION_ID_MISMATCH);
			} else if (frame_get_seq_number(&f) != (sequence_num + 1)) {
				// Obycajny datovy ramec v spravnej relacii, ale nenavazujuci do sekvencie
				data_send_rej_for_frame(&f, (uint8_t) DATA_DATA_SEQ_NUMBER_MISMATCH);
			} else {
				// Spravny navazujuci datovy ramac, ulozia sa jeho data
				state = DATA_R_SAVE;
			}
			break;
			
		case DATA_R_END:
			// Sem sa nedostaneme, len sa umlci prekladac, ze stav nie je osetreny vo switch
			break;
		}
		
	}
	
	// Pocet skutocne prijatych bajtov
	*bytes_read = data_index;

	return DATA_OK;
}

yup_retcode_t
data_send(const uint8_t * const buf, size_t bufsize)
{
	static uint16_t relation_id = 0;
	
	uint16_t sequence_num = 0;
	size_t data_index = 0;
	size_t ack_retry_count = DATA_ACK_RETRY_COUNT;
	
	data_send_automaton_state_t state = DATA_S_START_RELATION;
	
	// Kontrola argumentov
	if ((buf == NULL) || (bufsize == 0)) {
		return DATA_ERROR;
	}
	
	frame_t f, a;
	
	while (state != DATA_S_END) {
	
		yup_retcode_t r;
		
		switch (state) {
		case DATA_S_START_RELATION:
			++relation_id;
			sequence_num = 0;
			data_index = 0;
			ack_retry_count = DATA_ACK_RETRY_COUNT;
			state = DATA_S_PEPARE_DATA_FRAME;
			break;
			
		case DATA_S_PEPARE_DATA_FRAME:
			// Pripravi sa ramec s datami
			frame_init(&f, DATA_PROTOCOL_VERSION);
			frame_set_relation_id(&f, relation_id);
			frame_set_seq_number(&f, sequence_num);
			size_t len = ((bufsize - data_index) >= FRAME_MAX_DATA_BYTES) ? FRAME_MAX_DATA_BYTES : (bufsize - data_index);
			frame_set_data(&f, buf + data_index, len);
			data_index += len;
			// Je to posledny ramec?
			if (data_index >= bufsize) {
				frame_set_flag_lor(&f);
			}
			state = DATA_S_SEND_DATA_FRAME;
			break;
			
		case DATA_S_SEND_DATA_FRAME:
			// Odosle sa pripraveny ramec
			frame_send(&f);
			state = DATA_S_WAIT_ACK;
			break;
		
		case DATA_S_WAIT_ACK:
			r = frame_receive(&a, DATA_PROTOCOL_VERSION);
			if (r == FRAME_TIMEOUT) {
				// Neprijal sa ACK, znovu sa odoslu pipravene data
				if (ack_retry_count-- > 0) {
					state = DATA_S_SEND_DATA_FRAME;
				} else {
					// Bol dosiahnuty maximalny pocet pokusov o znozvu odoslanie dat
					return DATA_ERROR;
				}
			} else if (r != FRAME_OK) {
				// Chyba pri prijme ACK ramca
				data_send_rej_for_frame(&a, r);	
			} else if ((r = data_is_ack_for_frame(&a, &f)) != DATA_OK) {
				// ACK ramec prijaty v poriadku, ale s nespravnym obshom
				data_send_rej_for_frame(&a, r);
			} else if (frame_is_flag_lor_set(&f)) {
				// Datovy ramec, ktory som odoslal, bol posledny
				state = DATA_S_END;	
			} else {
				// Naposledy vyslany datovy ramec bol potvrdeny protistranou, odosle sa dalsi
				sequence_num++;
				state = DATA_S_PEPARE_DATA_FRAME;						
			}
			break;
			
		case DATA_S_END:
			// Sem sa nedostaneme, len sa umlci prekladac, ze stav nie je osetreny vo switch
			break;
		}
	}

	return DATA_OK;
}
