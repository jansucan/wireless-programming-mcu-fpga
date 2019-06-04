/* Author: Jan Sucan */

#include "crc32q.h"

#include <stdbool.h>

#define CRC32_TABLE_SIZE  256

#define CRC32_GENERATING_POLYNOME  0x814141ab

static void crc32q_create_table(uint32_t * const tab, uint32_t gen_polynome);

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

static uint32_t crc32_tab[CRC32_TABLE_SIZE];

static bool crc32q_table_created = false;

void
crc32q_init(void)
{
	crc32q_create_table(crc32_tab, CRC32_GENERATING_POLYNOME);
	crc32q_table_created = true;
}

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
