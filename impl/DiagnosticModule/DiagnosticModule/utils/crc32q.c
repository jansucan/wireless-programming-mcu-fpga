/* Author: Jan Sucan */

#include <utils/crc32q.h>

#include <stdbool.h>

/**
 * @brief Velkost tabulky hodnot pre vypocet CRC32-Q v bajtoch.
 */
#define CRC32_TABLE_SIZE  256

/**
 * @brief Velkost tabulky hodnot pre vypocet CRC32-Q v bajtoch.
 */
#define CRC32_GENERATING_POLYNOME  0x814141ab

static void crc32q_create_table(uint32_t * const tab, uint32_t gen_polynome);

/**
 * @brief Vypocet hodnot tabulky pre CRC32-Q.
 *
 * @param tab           Tabulka 32-bitovych hodnot.
 * @param gen_polynome  Generujuci polynom z ktoreho sa budu hodnoty pocitat.
 */
void
crc32q_create_table(uint32_t * const tab, uint32_t gen_polynome)
{	
	int i, j;
	uint32_t c;
	uint32_t msbMask = 1U << ((sizeof(c) * 8) - 1);
	
	for (i = 0; i < CRC32_TABLE_SIZE; i++) {
		c = i << 24;
		for (j = 0; j < 8; j++) {
			if (c & msbMask) {
				c = (c << 1) ^ gen_polynome;
			} else {
				c = c << 1;
			}
		}
		tab[i] = c;
	}
}

/**
 * @brief Tabulka hodnot pre vypocet CRC32-Q.
 */
static uint32_t crc32_tab[CRC32_TABLE_SIZE];

/**
 * @brief Priznak, ci je tabulka hodnot pre vypocet CRC32-Q inicializovana.
 */
static bool crc32q_table_created = false;

/**
 * @brief Inicializacia datovych struktur pre vypocet CRC32-Q.
 *
 * Vypocita tabulku hodnot pre CRC.
 */
void
crc32q_init(void)
{
	crc32q_create_table(crc32_tab, CRC32_GENERATING_POLYNOME);
	crc32q_table_created = true;
}

/**
 * @brief Vypocet kontrolneho suctu CRC32-Q.
 *
 * Tento kod je prevzaty z
 *
 *   https://svnweb.freebsd.org/base/stable/9/sys/libkern/crc32.c?revision=225736&view=co
 *
 * a upraveny pre CRC32-Q.
 *
 * @param buf   Ukazovatel na data, z ktorych sa bude sucet pocitat.
 * @param size  Pocet bajtov dat pre vypocet CRC.
 *
 * @return CRC32-Q kontrolny sucet.
 */
uint32_t
crc32q(const void * const buf, size_t size)
{	
	// Vytvorenie tabulky pred prvym pouzitim
	if (!crc32q_table_created) {
		crc32q_create_table(crc32_tab, CRC32_GENERATING_POLYNOME);
		crc32q_table_created = true;
	}

	// Vypocet CRC-32
	const uint8_t *p;

	p = buf;
	uint32_t crc = 0U; // Inicializacna CRC hodnota

	while (size--) {
		crc = (crc << 8) ^ crc32_tab[(crc >> 24) ^ *p++];
	}

	return crc ^ 0U;
}
