/* Author: Jan Sucan */

#include <stdint.h>
#include <clock/wait.h>
#include <utils/system_registers.h>

/**
 * @brief Hodnota systemoveho registra COUNT nacitana za 1 ms.
 *
 * Plati pre pre hlavne hodiny na 64,512 MHz.
 */
#define COUNT_DIFF_FOR_1_MS 64512

/**
 * @brief Zisti, ci bol prekroceny casovy deadline.
 *
 * @warning Predpoklada sa, ze od ziskania casovej znacky vo funkcii wait_get_timestamp()
 * po ziskanie casu v tejto funkcii, neuplynie viac ako priblizne 66 sekund (2^32 - 1 tikov casovaca).
 *
 * @param deadline  Ukazovatel na casovy deadline ziskany funkciou wait_get_timestamp().
 *
 * @return true   Deadline bol prekroceny.
 * @return false  Deadline este nebol prekroceny.
 */
bool
wait_has_deadline_expired(const deadline_t * const deadline)
{	
	// Pocet tikov casovaca pre zadany pocet milisekund v deadline
	uint32_t ticks_ms = deadline->ms * COUNT_DIFF_FOR_1_MS;
		
	uint32_t b = SYSREG_COUNT_GET;
	if (deadline->timestamp <= b) {
		// Casovac od ziskania deadlinu nepretiekol, uplynuty cas sa jednoducho ziska rozdielom casov
		b = b - deadline->timestamp;
	} else {
		// Casovac od ziskania deadlinu pretiekol
		b = ((uint32_t) 0) - deadline->timestamp + b;
	}	

	return (b >= ticks_ms); 
}

/**
 * @brief Pocka zadany pocet milisekund.
 *
 * Funkcia implementuje blokujuce cakanie.
 *
 * @param ms  Pocet milisekund, kolko sa bude cakat.
 */
void
wait_ms(uint16_t ms)
{
	deadline_t d = wait_get_deadline(ms);
	while (!wait_has_deadline_expired(&d))
		;
}

/**
 * @brief Ziska deadline pre cakanie.
 *
 * @param ms  Pocet milisekund, za kolko bude deadline prekroceny od casu zavolania funkcie.
 */
deadline_t
wait_get_deadline(uint16_t ms)
{
	deadline_t d;
	d.timestamp = SYSREG_COUNT_GET;
	d.ms = ms;
	return d;
}
