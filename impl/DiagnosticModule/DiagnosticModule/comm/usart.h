/* Author: Jan Sucan */

#ifndef USART_H_
#define USART_H_

#include <stdint.h>
#include <limits.h>

#include <clock/wait.h>

/**
 * @brief Navratove kody funkcie pre prijem bajtu cez USART0.
 */
typedef enum {
	USART_RECEIVE_BAD_ARGUMENT = INT_MIN,
	USART_RECEIVE_TIMEOUT,
	USART_RECEIVE_ERROR,
	USART_OK = 0
} usart_retcode_t;

void             usart_init(void);
void             usart_clear_receive_line(void);
usart_retcode_t  usart_receive_byte(uint8_t *byte, const deadline_t * const time_deadline);
void             usart_send_byte(uint8_t byte);

#endif /* USART_H_ */