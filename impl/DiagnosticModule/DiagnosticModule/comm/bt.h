/* Author: Jan Sucan */

#ifndef BT_H_
#define BT_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include <clock/wait.h>

/**
 * @brief Konstanta pre funkciu bt_receive_cobs_data().
 *
 * Signalizuje, ze funkcia caka na COBS delimiter a zacne spracovavat novy ramec.
 */
#define BT_RECEIVE_COBS_START  false

/**
 * @brief Konstanta pre funkciu bt_receive_cobs_data().
 *
 * Signalizuje, ze volanie funkcie bude pokracovat v spracovavani zacateho ramca
 * a prijem frame delimiteru bude povazovany za chybu.
 */
#define BT_RECEIVE_COBS_CONTINUE  true

typedef enum {
	BT_RECEIVE_BAD_ARGUMENT = INT_MIN,
	BT_RECEIVE_TIMEOUT,
	BT_RECEIVE_ERROR,
	BT_RECEIVE_COBS_INTERRUPTED,
	BT_RECEIVE_COBS_UNKNOWN_STATE,
	BT_OK = 0
} bt_retcode_t;

void         bt_init(void);
bt_retcode_t bt_receive_data(uint8_t * const buf, size_t n, const deadline_t *const time_deadline);
bt_retcode_t bt_receive_cobs_data(uint8_t * const buf, size_t n, const deadline_t * const time_deadline, bool cobs_continue);
void         bt_send_data(const uint8_t *const buf, size_t n);
void         bt_send_cobs_data_block(const uint8_t *const buf, size_t n);

#endif /* BT_H_ */