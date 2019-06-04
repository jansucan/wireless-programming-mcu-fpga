/* Author: Jan Sucan */

#ifndef BT_H_
#define BT_H_

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#define BT_RECEIVE_COBS_START  false

#define BT_RECEIVE_COBS_CONTINUE  true

typedef enum {
	BT_RECEIVE_BAD_ARGUMENT = INT_MIN,
	BT_RECEIVE_TIMEOUT,
	BT_RECEIVE_ERROR,
	BT_RECEIVE_COBS_INTERRUPTED,
	BT_RECEIVE_COBS_UNKNOWN_STATE,
	BT_OK = 0
} bt_retcode_t;

void         bt_init(HANDLE serial_port);
void         bt_close(void);
bt_retcode_t bt_receive_cobs_data(uint8_t * const buf, size_t n, bool cobs_continue);
void         bt_send_cobs_data_block(const uint8_t *const buf, size_t n);

#endif /* BT_H_ */
