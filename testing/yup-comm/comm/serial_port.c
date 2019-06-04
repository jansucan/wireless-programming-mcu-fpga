/* Author: Jan Sucan */

#include <stdio.h>
#include <string.h>

#include "serial_port.h"

static HANDLE  serial_port_open_port(const char * const portName);
static BOOL    serial_port_set_timeouts(HANDLE serial_port, int ms);

LPSTR
serial_port_get_error(void)
{
    DWORD e = GetLastError();
    if (e == 0) {
      return NULL;
    }

    LPSTR msgBuff = NULL;
    FormatMessageA(
       FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       e,
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
       (LPSTR) &msgBuff,
       0,
       NULL);

    return msgBuff;
}

void
serial_port_free_error(LPSTR msg)
{
  LocalFree(msg);
}

HANDLE
serial_port_open_port(const char * const portName)
{
  HANDLE sp = INVALID_HANDLE_VALUE;
  
  // Construct Windows serial port name
  char sn[16];
  strcpy(sn, "\\\\.\\");
  strcpy(sn + strlen(sn), portName);
  // Open serial port  
  sp = CreateFile(sn,             // Port name
		  GENERIC_READ | GENERIC_WRITE, // Read/Write
		  0,                            // No Sharing
		  NULL,                         // No Security
		  OPEN_EXISTING,// Open existing port only
		  0,            // Non Overlapped I/O
		  NULL);        // Null for Comm Devices

  return sp;
}

HANDLE
serial_port_open(const char * const portName)
{
    HANDLE serial_port = serial_port_open_port(portName);
    if (serial_port == INVALID_HANDLE_VALUE) {
      return INVALID_HANDLE_VALUE;
    }
    
    // Konfiguracia
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
    if (GetCommState(serial_port, &dcbSerialParams) == 0) {
      serial_port_close(serial_port);
      return INVALID_HANDLE_VALUE;
    }
    
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;         // Setting ByteSize = 8
    dcbSerialParams.StopBits = ONESTOPBIT;// Setting StopBits = 1
    dcbSerialParams.Parity   = NOPARITY;  // Setting Parity = None
    
    if ((SetCommState(serial_port, &dcbSerialParams) == 0)
        || (serial_port_set_timeouts(serial_port, 3000U) == 0)) {
      serial_port_close(serial_port);
      return INVALID_HANDLE_VALUE;
    }

    return serial_port;
}

void
serial_port_close(HANDLE serial_port)
{
    if (serial_port != INVALID_HANDLE_VALUE) {
      CloseHandle(serial_port);
      serial_port = INVALID_HANDLE_VALUE;
    }
}

BOOL
serial_port_set_timeouts(HANDLE serial_port, int ms)
{
    COMMTIMEOUTS timeouts;
    
    if (!GetCommTimeouts(serial_port, &timeouts)) {
      return FALSE;
    }
    
    timeouts.ReadIntervalTimeout = ms; 
    timeouts.ReadTotalTimeoutMultiplier = ms;
    timeouts.ReadTotalTimeoutConstant = 0;
    
    timeouts.WriteTotalTimeoutMultiplier = ms;
    timeouts.WriteTotalTimeoutConstant = 0;
    
    if (!SetCommTimeouts(serial_port, &timeouts)) {
      return FALSE;
    }

    return TRUE;
}

int
serial_port_write_byte(HANDLE serial_port, uint8_t * const data, unsigned size)
{   
    DWORD written = 0;

    if (!WriteFile(serial_port, data, size, &written, NULL)) {
      written = 0;
    }

    return (written != size) ? -1 : 0;
}

int
serial_port_read_byte(HANDLE serial_port, uint8_t * const byte)
{
    DWORD read;

    if (!ReadFile(serial_port, byte, sizeof(uint8_t), &read, NULL)) {
      return -1;
    }

    return (read != 0) ? 0 : -2;
}
