/* Author: Jan Sucan */

#ifndef WAIT_H_
#define WAIT_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Slucka pre aktivne cakanie.
 *
 * Telo obsahuje NOP ako 'asm volatile', aby sa zaranilo odstraneniu slucky
 * optimalizujucim prekladacom.
 *
 * @param iterations  Pocet iteracii.
 */
#define WAIT_ACTIVE_LOOP(iterations) \
	for (uint32_t d = iterations; d > 0; --d) { \
		asm volatile ("nop"); \
	}

typedef struct deadline_s {
	uint32_t timestamp; /**< Casova znacka kedy bol deadline ziskany */
	uint16_t ms;        /**< Pocet milisekund od deadline_s#timestamp, za kolko deadline nastane. */
} deadline_t;

void       wait_ms(uint16_t ms);
deadline_t wait_get_deadline(uint16_t ms);
bool       wait_has_deadline_expired(const deadline_t * const deadline);

#endif /* WAIT_H_ */