/* Author: Jan Sucan */

#include <windows.h>

HANDLE   mailslot_create(const char * const name);
unsigned mailslot_read(HANDLE slot, char * const buf, unsigned buf_size);
HANDLE   mailslot_connect(const char * const name);
int      mailslot_write(HANDLE slot, char * const buf, unsigned buf_size);
void     mailslot_close(HANDLE slot);
