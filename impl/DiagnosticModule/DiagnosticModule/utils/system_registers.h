/* Author: Jan Sucan */

#ifndef SYSTEM_REGISTERS_H_
#define SYSTEM_REGISTERS_H_

/**
 * @brief Makro pre nastavenie hodnoty systemoveho registra.
 */
#define SYSREG_SET(r, v) __builtin_mtsr(r, v)

/**
 * @brief Makro pre ziskanie hodnoty systemoveho registra.
 */
#define SYSREG_GET(r) __builtin_mfsr(r)

/**
 * @brief Adresa systemoveho registra COUNT, ktory pocita hodinove cykly CPU.
 */
#define SYSREG_COUNT_ADDRESS 264U

/**
 * @brief Makro pre nastavenie hodnoty systemoveho registra COUNT.
 */
#define SYSREG_COUNT_SET(v) SYSREG_SET(SYSREG_COUNT_ADDRESS, v)

/**
 * @brief Makro pre ziskanie hodnoty systemoveho registra COUNT.
 */
#define SYSREG_COUNT_GET SYSREG_GET(SYSREG_COUNT_ADDRESS)

/**
 * @brief Adresa systemoveho registra CPUCR, ktory nastavuje vlastnosti CPU.
 */
#define SYSREG_CPUCR_ADDRESS 12U

/**
 * @brief Makro pre nastavenie hodnoty systemoveho registra CPUSR.
 */
#define SYSREG_CPUCR_SET(v) SYSREG_SET(SYSREG_CPUCR_ADDRESS, v)

/**
 * @brief Makro pre ziskanie hodnoty systemoveho registra CPUSR.
 */
#define SYSREG_CPUCR_GET SYSREG_GET(SYSREG_CPUCR_ADDRESS)

#endif /* SYSTEM_REGISTERS_H_ */