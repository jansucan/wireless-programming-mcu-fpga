/* Author: Jan Sucan */

#ifndef CRC32Q_H_
#define CRC32Q_H_

#include <stdint.h>
#include <stdlib.h>

void     crc32q_init(void);
uint32_t crc32q(const void * const buf, size_t size);

#endif /* CRC32Q_H_ */
