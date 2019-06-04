/* Author: Jan Sucan */

#include "mailslot.h"

HANDLE
mailslot_create(const char * const name)
{
  return CreateMailslot(name, 0, MAILSLOT_WAIT_FOREVER, NULL);
}

unsigned
mailslot_read(HANDLE slot, char * const buf, unsigned buf_size)
{
  unsigned bytes_read;
  
  ReadFile(slot, buf, buf_size, &bytes_read, NULL);

  return bytes_read;
}

HANDLE
mailslot_connect(const char * const name)
{
  return CreateFile(name, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

int
mailslot_write(HANDLE slot, char * const buf, unsigned buf_size)
{
  unsigned bytes_written;
  
  if (WriteFile(slot, buf, buf_size, &bytes_written, NULL) == 0) {
    return -1;
  }

  return (bytes_written != buf_size) ? -1 : 0;
}

void
mailslot_close(HANDLE slot)
{
  CloseHandle(slot);
}
