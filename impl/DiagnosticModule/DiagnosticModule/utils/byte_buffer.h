/* Author: Jan Sucan */

#ifndef BYTE_BUFFER_H_
#define BYTE_BUFFER_H_

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Ziskanie uint8_t hodnoty z pola bajtov.
 *
 * @param b  Pole bajtov.
 * @param i  Offset hodnoty v poli @p b.
 */
#define GET_UINT8_T_FROM_BYTES(b, i)   ((uint8_t) b[i])

/**
 * @brief Ziskanie uint16_t hodnoty z pola bajtov vo formate little-endian.
 *
 * @param b  Pole bajtov.
 * @param i  Offset hodnoty v poli @p b.
 */
#define GET_UINT16_T_FROM_BYTES(b, i)  ((uint16_t) ((b[i + 1] << 8) | b[i]))

/**
 * @brief Ziskanie uint32_t hodnoty z pola bajtov vo formate little-endian.
 *
 * @param b  Pole bajtov.
 * @param i  Offset hodnoty v poli @p b.
 */
#define GET_UINT32_T_FROM_BYTES(b, i)  ((uint32_t) ((b[i + 3] << 24) | (b[i + 2] << 16) | (b[i + 1] << 8) | b[i]))

/**
 * @brief Ulozenie uint8_t hodnoty do pola bajtov.
 *
 * @param b  Pole bajtov.
 * @param i  Offset hodnoty v poli @p b.
 * @param v  Hodnota pre ulozenie.
 */
#define SET_BYTES_FROM_UINT8_T(b, i, v)  {\
	b[i + 0] = v;\
}

/**
 * @brief Ulozenie uint16_t hodnoty do pola bajtov vo formate little-endian.
 *
 * @param b  Pole bajtov.
 * @param i  Offset hodnoty v poli @p b.
 * @param v  Hodnota pre ulozenie.
 */
#define SET_BYTES_FROM_UINT16_T(b, i, v)  {\
	 b[i + 1] = (v & 0xFF00) >> 8;\
	 b[i + 0] = (v & 0x00FF);\
}

/**
 * @brief Ulozenie uint32_t hodnoty do pola bajtov vo formate little-endian.
 *
 * @param b  Pole bajtov.
 * @param i  Offset hodnoty v poli @p b.
 * @param v  Hodnota pre ulozenie.
 */
#define SET_BYTES_FROM_UINT32_T(b, i, v)  {\
	 b[i + 3] = (v & 0xFF000000) >> 24;\
	 b[i + 2] = (v & 0x00FF0000) >> 16;\
	 b[i + 1] = (v & 0x0000FF00) >> 8;\
	 b[i + 0] = (v & 0x000000FF);\
}

#endif /* BYTE_BUFFER_H_ */