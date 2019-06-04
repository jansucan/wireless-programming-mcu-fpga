/* Author: Jan Sucan */

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

/**
 * @brief Velkost FLASH v strankach vyhradena pre bootloader od stranky 0 (vratane).
 */
#define DM_BOOTLOADER_SIZE_IN_PAGES  32

/**
 * @brief Najnizsia stranka vo FLASH, na ktorej mozu byt zapisane aplikacne data.
 */
#define DM_APPLICATION_REAL_MIN_PAGE  DM_BOOTLOADER_SIZE_IN_PAGES

/**
 * @brief Najvyssia stranka vo FLASH, na ktorej mozu byt zapisane aplikacne data.
 */
#define DM_APPLICATION_REAL_MAX_PAGE  ((AVR32_FLASH_SIZE / AVR32_FLASH_PAGE_SIZE) - 1)

/**
 * @brief Najnizsia stranka vo virtualnej FLASH, na ktorej mozu byt zapisane aplikacne data.
 *
 * Tym, ze Bootloader obsadi cast FLASH, zostane menej miesta na aplikacne data. FLASH sa bude
 * tvarit pre klienta, ze ma menej stranok ako v skutocnosti. Virtualne adresy su relativne k
 * strankovemu priestoru pre aplikacne data.
 */
#define DM_APPLICATION_VIRT_MIN_PAGE  0

/**
 * @brief najvyssia stranka vo virtualnej FLASH, na ktorej mozu byt zapisane aplikacne data.
 */
#define DM_APPLICATION_VIRT_MAX_PAGE  (DM_APPLICATION_REAL_MAX_PAGE - DM_BOOTLOADER_SIZE_IN_PAGES)

/**
 * @brief Znak, ktory sa nahradzuje metodou COBS a ktory preto zaroven sluzi ako oddelovac COBS blokov dat
 */
#define COMM_PARAMS_COBS_DELIMITER_CHAR '$'

#endif /* PARAMETERS_H_ */