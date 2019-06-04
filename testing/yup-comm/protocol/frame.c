/* Author: Jan Sucan */

#include "frame.h"
#include "../comm/bt.h"
#include "../utils/byte_buffer.h"
#include "../utils/crc32q.h"

#include <string.h>

// Bitove masky priznakov ramca
#define FRAME_FLAG_ACK  0x1  // Prijatie paketu
#define FRAME_FLAG_REJ  0x2  // Odmietnutie paketou
#define FRAME_FLAG_LOR  0x8  // Oznacenie posledneho paketu relacie

// Offsety hodnot v surovych datach ramca
#define FRAME_OFFSET_PROTOCOL_VERSION  0x00
#define FRAME_OFFSET_FLAGS             0x00
#define FRAME_OFFSET_DATA_LENGTH       0x00
#define FRAME_OFFSET_RELATION_ID       0x02
#define FRAME_OFFSET_SEQ_NUMBER        0x04
#define FRAME_OFFSET_HEADER_CHECKSUM   0x06
#define FRAME_OFFSET_DATA              0x0A

// Bitove masky pre 16-bitovu hodnotu obsahujucu verziu protokolu YUP, priznaky ramca a dlzku pripojenych dat
#define FRAME_MASK_PROTOCOL_VERSION  0xF000
#define FRAME_MASK_FLAGS             0x0F00
#define FRAME_MASK_DATA_LENGTH       0x00FF

// Bitove offsety v 16-bitovej hodnote, v ktorej je obsiahnuta verzia YUP a priznaky
#define FRAME_BITOFF_PROTOCOL_VERSION  12
#define FRAME_BITOFF_FLAGS              8

// Velkost hlavicky v bajtoch
#define FRAME_HEADER_BYTES    FRAME_OFFSET_DATA

// Velkost pola kontrolneho suctu
#define FRAME_CHECKSUM_BYTES  4

// Interne pouzivany navratovy kod
#define	FRAME_PVT_ERR_DATA_INTERRUPTED  1


static yup_retcode_t frame_receive_header         (frame_t * const frame);
static yup_retcode_t frame_verify_header_checksum (const frame_t * const frame);
static yup_retcode_t frame_receive_data           (frame_t * const frame);
static yup_retcode_t frame_verify_data_checksum   (const frame_t * const frame);
static uint8_t       frame_get_flags              (const frame_t * const frame);
static void          frame_set_flags              (frame_t * const frame, uint8_t f);
static yup_retcode_t yup_retcode_from_bt_retcode  (bt_retcode_t btr);

void
frame_init(frame_t * const frame, uint8_t protocol_version)
{	
	frame_set_protocol_version(frame, protocol_version);
	frame_set_flags(frame, 0);
	frame_set_data(frame, NULL, 0);
	frame_set_relation_id(frame, 0);
	frame_set_seq_number(frame, 0);	
}

void
frame_send(frame_t * const frame)
{
	// Vyplni sa kontrolny sucet hlavicky
	frame_set_header_checksum(frame);
	
	// Vyplni sa kontrolny sucet dat, ak nejake su
	frame_set_data_checksum(frame);
	
	// Odosle sa ramec
	bt_send_cobs_data_block(frame->raw, frame_get_length(frame));
}

yup_retcode_t
frame_receive(frame_t * const frame, uint8_t protocol_version)
{	
	yup_retcode_t retcode = FRAME_OK;
	
	while (1) {
		// Hlavicka sa prijme a skontroluje
		if ((retcode = frame_receive_header(frame))) {
			break;
		}
		if ((retcode = frame_verify_header_checksum(frame))) {
			break;
		}
		// Hlavicka ramca je OK, odteraz sa moze getterom pristupovat k informacii o verzii protokolu a dlzke dat
		// Skontroluje sa pozadovana verzia protokolu
		if (frame_get_protocol_version(frame) != protocol_version) {
			retcode = FRAME_PROTOCOL_VERSION_MISMATCH;
			break;
		}	
		// Prijmu sa data
		yup_retcode_t r = frame_receive_data(frame);

		if (r == FRAME_OK) {
			break;
		} else if (r == FRAME_PVT_ERR_DATA_INTERRUPTED) {
			// Prijal sa delimiter, novy ramec zacal skor nez skoncil aktualny, zacne sa s prijmom noveho ramca
			continue;
		} else {
			retcode = r;
			break;
		} 
	}
	
	if (retcode == FRAME_OK) {
		retcode = frame_verify_data_checksum(frame);
	}
	
	return retcode;
}

yup_retcode_t
frame_receive_header(frame_t * const frame)
{
  return yup_retcode_from_bt_retcode(bt_receive_cobs_data(frame->raw, FRAME_HEADER_BYTES, BT_RECEIVE_COBS_START));
}

yup_retcode_t
frame_verify_header_checksum (const frame_t * const frame)
{
	yup_retcode_t retcode = FRAME_OK;
	
	// Vypocet kontrolneho suctu prijatych dat
	// Preskoci sa COBS delimiter (1B) a COBS header (1B)
	// Do vypoctu nebudu zahrnute 4 bajty kontrolneho suctu
	const uint32_t sum = crc32q(frame->raw, FRAME_HEADER_BYTES - FRAME_CHECKSUM_BYTES);
	
	// Vypocitany kontrolny sucet sa porovna so suctom v ramci
	if (sum != frame_get_header_checksum(frame)) {
		retcode = FRAME_BAD_HEADER_CHECKSUM;
	}
	
	return retcode;
}

yup_retcode_t
frame_receive_data (frame_t * const frame)
{
	const uint8_t data_length = frame_get_data_length(frame);
	yup_retcode_t retcode = FRAME_OK;
	
	// Pri nulovych data sa nic neprijme
	if (data_length > 0) {
		// Treba prijat naviac 4B kontrolneho suctu
		retcode = yup_retcode_from_bt_retcode(bt_receive_cobs_data(frame->raw + FRAME_HEADER_BYTES,
									   data_length + FRAME_CHECKSUM_BYTES,
									   BT_RECEIVE_COBS_CONTINUE));
	}
	
	return retcode;
}

yup_retcode_t
frame_verify_data_checksum (const frame_t * const frame)
{
	yup_retcode_t retcode = FRAME_OK;
	const uint8_t data_length = frame_get_data_length(frame);
	
	// Ked sme sa dostali az sem, kontrolny sucet hlavicky je OK
	// Ak nemame ziadne data, nic sa uz nemusi kontrolovat
	if (data_length > 0) {		
		// Kontrola suctu dat
		// Do vypoctu nebudu zahrnute 4 bajty kontrolneho suctu
		const uint32_t sum = crc32q(frame->raw + FRAME_HEADER_BYTES, data_length);
		
		// Vypocitany kontrolny sucet sa porovna so suctom v ramci
		if (sum != frame_get_data_checksum(frame)) {
			retcode = FRAME_BAD_DATA_CHECKSUM;
		}
	}
	
	return retcode;
}

yup_retcode_t
yup_retcode_from_bt_retcode(bt_retcode_t btr)
{
	yup_retcode_t r = FRAME_RECEIVE_ERROR;
	
	switch (btr) {
	case BT_RECEIVE_TIMEOUT:
		r = FRAME_TIMEOUT;
		break;
		
	case BT_RECEIVE_COBS_INTERRUPTED:
		r = FRAME_PVT_ERR_DATA_INTERRUPTED;
		break;
		
	case BT_RECEIVE_BAD_ARGUMENT:
	case BT_RECEIVE_ERROR:
	case BT_RECEIVE_COBS_UNKNOWN_STATE:
		r = FRAME_RECEIVE_ERROR;
		break;
			
	case BT_OK:
		r = FRAME_OK;
		break;
	}
	
	return r;
}


// Getter/Setter funkcie
uint8_t
frame_get_protocol_version(const frame_t * const frame)
{
	const uint16_t x = GET_UINT16_T_FROM_BYTES(frame->raw, FRAME_OFFSET_PROTOCOL_VERSION);
	return (x & FRAME_MASK_PROTOCOL_VERSION) >> FRAME_BITOFF_PROTOCOL_VERSION;
}

void
frame_set_protocol_version(frame_t * const frame, uint8_t v)
{
	uint16_t x = GET_UINT16_T_FROM_BYTES(frame->raw, FRAME_OFFSET_PROTOCOL_VERSION);
	x &= ~FRAME_MASK_PROTOCOL_VERSION;
	x |= (v & 0x0F) << FRAME_BITOFF_PROTOCOL_VERSION;
	SET_BYTES_FROM_UINT16_T(frame->raw, FRAME_OFFSET_PROTOCOL_VERSION, x);
}

void
frame_set_flag_ack(frame_t * const frame)
{
	frame_set_flags(frame, frame_get_flags(frame) | FRAME_FLAG_ACK);
}

bool
frame_is_flag_ack_set(const frame_t * const frame)
{
	return frame_get_flags(frame) & FRAME_FLAG_ACK;
}

void
frame_set_flag_rej(frame_t * const frame)
{
	frame_set_flags(frame, frame_get_flags(frame) | FRAME_FLAG_REJ);
}

bool
frame_is_flag_rej_set(const frame_t * const frame)
{
	return frame_get_flags(frame) & FRAME_FLAG_REJ;
}

void
frame_set_flag_lor(frame_t * const frame)
{
	frame_set_flags(frame, frame_get_flags(frame) | FRAME_FLAG_LOR);
}

bool
frame_is_flag_lor_set(const frame_t * const frame)
{
	return frame_get_flags(frame) & FRAME_FLAG_LOR;
}

uint8_t
frame_get_flags(const frame_t * const frame)
{
	const uint16_t x = GET_UINT16_T_FROM_BYTES(frame->raw, FRAME_OFFSET_FLAGS);
	return (x & FRAME_MASK_FLAGS) >> FRAME_BITOFF_FLAGS;
}

void
frame_set_flags(frame_t * const frame, uint8_t f)
{
	uint16_t x = GET_UINT16_T_FROM_BYTES(frame->raw, FRAME_OFFSET_FLAGS);
	x &= ~FRAME_MASK_FLAGS;
	x |= (f & 0x0F) << FRAME_BITOFF_FLAGS;
	SET_BYTES_FROM_UINT16_T(frame->raw, FRAME_OFFSET_FLAGS, x);
}

uint8_t
frame_get_data_length(const frame_t * const frame)
{
	const uint16_t x = GET_UINT16_T_FROM_BYTES(frame->raw, FRAME_OFFSET_DATA_LENGTH);
	return (x & FRAME_MASK_DATA_LENGTH);
}

void
frame_set_data_length(frame_t * const frame, uint8_t v)
{
	uint16_t x = GET_UINT16_T_FROM_BYTES(frame->raw, FRAME_OFFSET_DATA_LENGTH);
	x &= ~FRAME_MASK_DATA_LENGTH;
	x |= v;
	SET_BYTES_FROM_UINT16_T(frame->raw, FRAME_OFFSET_DATA_LENGTH, x);
}

uint16_t
frame_get_relation_id(const frame_t * const frame)
{
	return GET_UINT16_T_FROM_BYTES(frame->raw, FRAME_OFFSET_RELATION_ID);
}

void
frame_set_relation_id(frame_t * const frame, uint16_t v)
{
	SET_BYTES_FROM_UINT16_T(frame->raw, FRAME_OFFSET_RELATION_ID, v);	
}

uint16_t
frame_get_seq_number(const frame_t * const frame)
{
	return GET_UINT16_T_FROM_BYTES(frame->raw, FRAME_OFFSET_SEQ_NUMBER);
}

void
frame_set_seq_number(frame_t * const frame, uint16_t v)
{
	SET_BYTES_FROM_UINT16_T(frame->raw, FRAME_OFFSET_SEQ_NUMBER, v);		
}

uint32_t
frame_get_header_checksum(const frame_t * const frame)
{
	return GET_UINT32_T_FROM_BYTES(frame->raw, FRAME_OFFSET_HEADER_CHECKSUM);
}

void
frame_set_header_checksum(frame_t * const frame)
{
	// Nezapocitavaju sa prve dva bajty COBSu a posledne 4 bajty pre ulozenie kontrolneho suctu
	const uint32_t cs = crc32q(frame->raw, FRAME_HEADER_BYTES - FRAME_CHECKSUM_BYTES);
	SET_BYTES_FROM_UINT32_T(frame->raw, FRAME_OFFSET_HEADER_CHECKSUM, cs);	
}

void
frame_get_data(const frame_t * const frame, uint8_t * const data, uint8_t * const data_length)
{
	*data_length = frame_get_data_length(frame);
	memcpy(data, frame->raw + FRAME_OFFSET_DATA, *data_length);
}

void
frame_set_data(frame_t * const frame, const uint8_t * const data, uint8_t num)
{
	uint8_t n = (num > FRAME_MAX_DATA_BYTES) ? FRAME_MAX_DATA_BYTES : num;
	
	frame_set_data_length(frame, n);
	
	if ((data != NULL) && (n > 0)) {
		memcpy(frame->raw + FRAME_OFFSET_DATA, data, n);	
	}
}

uint32_t
frame_get_data_checksum(const frame_t * const frame)
{
	uint8_t data_length = frame_get_data_length(frame);
	uint32_t x = 0;
	// Pole checksum je pripojene len vtedy, ked su pripojene nejake data
	if (data_length > 0) {
		x = GET_UINT32_T_FROM_BYTES(frame->raw, FRAME_OFFSET_DATA + data_length);
	}
	return x;
}

void
frame_set_data_checksum(frame_t * const frame)
{
	const uint8_t dl = frame_get_data_length(frame);
	if (dl == 0) {
		// Zadne data, nie je k comu pocitat kontrolny sucet
		return;
	}
	
	const uint32_t cs = crc32q(frame->raw + FRAME_OFFSET_DATA, dl);
	SET_BYTES_FROM_UINT32_T(frame->raw, FRAME_OFFSET_DATA + dl, cs);	
}

uint8_t
frame_get_length(const frame_t * const frame)
{
	const uint8_t dl = frame_get_data_length(frame);
	return FRAME_OFFSET_DATA + dl + ((dl > 0) ? FRAME_CHECKSUM_BYTES : 0);
}
