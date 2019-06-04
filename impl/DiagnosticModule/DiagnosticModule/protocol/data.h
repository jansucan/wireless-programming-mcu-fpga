/* Author: Jan Sucan */

#ifndef DATA_H_
#define DATA_H_

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include <protocol/return_codes.h>

yup_retcode_t data_receive(uint8_t * const buf, size_t bufsize, size_t * const bytes_read);
yup_retcode_t data_send   (const uint8_t * const buf, size_t bufsize);

#endif /* DATA_H_ */

