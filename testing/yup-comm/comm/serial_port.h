/* Author: Jan Sucan */

#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H 1

#include <windows.h>
#include <stdint.h>

HANDLE  serial_port_open(const char * const portName);
void    serial_port_close(HANDLE serial_port);
int     serial_port_write_byte(HANDLE serial_port, uint8_t * const data, unsigned size);
int     serial_port_read_byte(HANDLE serial_port, uint8_t * const byte);
LPSTR   serial_port_get_error(void);
void    serial_port_free_error(LPSTR msg);

#endif
