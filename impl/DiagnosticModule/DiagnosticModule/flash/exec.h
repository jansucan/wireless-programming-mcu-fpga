/* Author: Jan Sucan */

#ifndef EXEC_H_
#define EXEC_H_

#include <stdint.h>
#include <avr32/io.h>

void exec_application_firmware_at_virtual_page(uint16_t page_num);

#endif /* EXEC_H_ */