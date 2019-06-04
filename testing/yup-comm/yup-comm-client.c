/* Author: Jan Sucan */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "mailslot.h"

#define MAILSLOT_SERVER_NAME  "\\\\.\\Mailslot\\yup-comm-server"
#define MAILSLOT_CLIENT_NAME  "\\\\.\\Mailslot\\yup-comm-client"
#define MAILSLOT_MAX_MSG_SIZE 400

HANDLE mailslot_server = INVALID_HANDLE_VALUE;
HANDLE mailslot_client = INVALID_HANDLE_VALUE;

void
print_msg(FILE * stream, const char * format, ...)
{
  char buf[1024];
  
  va_list args;
  va_start(args, format);
  vsnprintf(buf, 1024, format, args);
  va_end(args);

  fprintf(stream, "    yup-comm-client: %s%s", (stream == stderr) ? "ERROR: " : "" , buf);
  fflush(stream);
}

void
clean_exit(int code)
{
  if (mailslot_server != INVALID_HANDLE_VALUE) {
    CloseHandle(mailslot_server);
  }
  
  exit(code);
}

void
usage(void)
{
  printf("Usage: yup-comm-client  CMD\n");
  printf("  CMD  command for yup-comm-server\n");
  clean_exit(1);
}

int
main(int argc, char ** argv)
{
  if (argc != 2) {
    print_msg(stderr, "Missing argument\n");
    usage();
  } if (strlen(argv[1]) > MAILSLOT_MAX_MSG_SIZE) {
    print_msg(stderr, "The command is too long (> 400 characters)\n");
    clean_exit(1);
  }  

  if ((mailslot_client = mailslot_create(MAILSLOT_CLIENT_NAME)) == INVALID_HANDLE_VALUE) {
    print_msg(stderr, "Cannot create mailslot for receiving reply from the server\n");
    clean_exit(1);
  }
  
  if ((mailslot_server = mailslot_connect(MAILSLOT_SERVER_NAME)) == INVALID_HANDLE_VALUE) {
    print_msg(stderr, "Cannot open server's mailslot\n");
    clean_exit(1);
  }

  if (mailslot_write(mailslot_server, argv[1], strlen(argv[1]) + 1U) != 0) {
    print_msg(stderr, "Cannot send command to the server\n");
    clean_exit(1);    
  }
  
  print_msg(stdout, "Waiting for reply from the server ...\n");
  
  char c;
  mailslot_read(mailslot_client, &c, sizeof(c));

  clean_exit(0);
}
