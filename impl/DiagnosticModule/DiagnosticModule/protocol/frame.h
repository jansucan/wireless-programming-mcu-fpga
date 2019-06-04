/* Author: Jan Sucan */

#ifndef FRAME_H_
#define FRAME_H_

#include <clock/wait.h>
#include <protocol/return_codes.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Celkova maximalna velkost ramca v bajtoch.
 *
 * Dva do poctu 256 su pre COBS.
 */
#define FRAME_MAX_BYTES       254

/**
 * @brief Maximalna velkost datoveho nakladu ramca v bajtoch.
 */
#define FRAME_MAX_DATA_BYTES  240

typedef struct {
	uint8_t raw[FRAME_MAX_BYTES];
} frame_t;

void            frame_init   (frame_t * const frame, uint8_t protocol_version);
yup_retcode_t   frame_receive(frame_t * const frame, const deadline_t * const time_deadline, uint8_t protocol_version);
void            frame_send   (frame_t * const frame);

// Protocol version
uint8_t  frame_get_protocol_version(const frame_t * const frame);
void     frame_set_protocol_version(frame_t * const frame, uint8_t v);
// Flags
void     frame_set_flag_ack(frame_t * const frame);
bool     frame_is_flag_ack_set(const frame_t * const frame);
void     frame_set_flag_rej(frame_t * const frame);
bool     frame_is_flag_rej_set(const frame_t * const frame);
void     frame_set_flag_lor(frame_t * const frame);
bool     frame_is_flag_lor_set(const frame_t * const frame);
// Data length
uint8_t  frame_get_data_length(const frame_t * const frame);
void     frame_set_data_length(frame_t * const frame, uint8_t v);
// Relation ID
uint16_t frame_get_relation_id(const frame_t * const frame);
void     frame_set_relation_id(frame_t * const frame, uint16_t v);
// Seq number
uint16_t frame_get_seq_number(const frame_t * const frame);
void     frame_set_seq_number(frame_t * const frame, uint16_t v);
// Header checksum
uint32_t frame_get_header_checksum(const frame_t * const frame);
void     frame_set_header_checksum(frame_t * const frame);
// Data
void     frame_get_data(const frame_t * const frame, uint8_t * const data, uint8_t * const data_length);
void     frame_set_data(frame_t * const frame, const uint8_t * const data, uint8_t num);
// Data checksum
uint32_t frame_get_data_checksum(const frame_t * const frame);
void     frame_set_data_checksum(frame_t * const frame);

// Ostatne
uint8_t  frame_get_length(const frame_t * const frame);

#endif /* FRAME_H_ */