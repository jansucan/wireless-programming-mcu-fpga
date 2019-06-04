/* Author: Jan Sucan */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>

#include "comm/serial_port.h"
#include "comm/bt.h"
#include "protocol/data.h"
#include "mailslot.h"

#define COM_PORT_OPEN_TRIES  4U
#define MAILSLOT_SERVER_NAME  "\\\\.\\Mailslot\\yup-comm-server"
#define MAILSLOT_CLIENT_NAME  "\\\\.\\Mailslot\\yup-comm-client"
#define MAILSLOT_MAX_MSG_SIZE 400

HANDLE serial_port;
HANDLE mailslot_server = INVALID_HANDLE_VALUE;
HANDLE mailslot_client = INVALID_HANDLE_VALUE;

int
send_byte_to_client(char byte, bool check_errors)
{
  /* Command received, client is alive, open his mailslot */
  if ((mailslot_client = mailslot_connect(MAILSLOT_CLIENT_NAME)) == INVALID_HANDLE_VALUE) {
    return (check_errors) ? -1 : 0;
  }      

  /* Inform possible client about return value */
  if (mailslot_write(mailslot_client, &byte, sizeof(byte)) != 0) {
    return (check_errors) ? -1 : 0;
  }

  CloseHandle(mailslot_client);
  return 0;
}

void
print_msg(FILE * stream, const char * format, ...)
{
  char buf[1024];
  
  va_list args;
  va_start(args, format);
  vsnprintf(buf, 1024, format, args);
  va_end(args);

  fprintf(stream, "    yup-comm-server: %s%s", (stream == stderr) ? "ERROR: " : "" , buf);
  fflush(stream);
}

void
clean_exit(int code)
{
  if (serial_port != INVALID_HANDLE_VALUE) {
    serial_port_close(serial_port);    
  }
  print_msg(stdout, "COM port closed\n");

  send_byte_to_client(0, false);
  
  exit(code);
}

void
usage(void)
{
  printf("Usage: yup-comm-server COMPORT\n");
  printf("  COMPORT  name of the COM port to use (COM0, COM1, ...)\n");
  clean_exit(1);
}

void
execute_yup_cmd(FILE * const in_file, FILE * const out_file)
{
  char command[2048U];
  const size_t command_size = fread(command, sizeof(char), sizeof(command), in_file);

  data_send((uint8_t *) command, command_size);
  
  char reply[2048U];
  size_t reply_size;

  data_receive((uint8_t *) reply, sizeof(reply), &reply_size);
  
  fwrite(reply, sizeof(char), reply_size, out_file);
}

void
process_execute_cmd(const char * const in_path, const char * const out_path)
{
  FILE * const in_file = fopen(in_path, "rb");

  if (in_file == NULL) {
    print_msg(stderr, "Cannot open file with the command data: %s: %s\n", in_path, strerror(errno));
    clean_exit(1);
  }
      
  FILE * const out_file = fopen(out_path, "wb");

  if (out_file == NULL) {
    print_msg(stderr, "Cannot open file for data of reply: %s: %s\n", out_path, strerror(errno));
    clean_exit(1);
  }

  print_msg(stdout, "%s --> ", in_path);

  execute_yup_cmd(in_file, out_file);

  printf("%s\n", out_path);
  fflush(stdout);

  fclose(in_file);
  fclose(out_file);  
}

int
main(int argc, char ** argv)
{
  if (argc != 2) {
    print_msg(stderr, "Missing argument\n");
    usage();
  }

  /* Process commands from mailslot */
  if ((mailslot_server = mailslot_create(MAILSLOT_SERVER_NAME)) == INVALID_HANDLE_VALUE) {
    print_msg(stderr, "Cannot create mailslot for receiving commands\n");
    clean_exit(1);
  }
  
  int retry_delay = 4;
  
  for (int i = 0; i < COM_PORT_OPEN_TRIES; ++i) {
    if (i == 0) {
      print_msg(stdout, "opening port %s\n", argv[1]);
    } else {
      print_msg(stdout, "retrying to open port %s (try %d/%d)\n", argv[1], i + 1, COM_PORT_OPEN_TRIES);
      sleep(retry_delay);
    }
    if ((serial_port = serial_port_open(argv[1])) != INVALID_HANDLE_VALUE) {
      break;
    }
    retry_delay += 2;
  }
  if (serial_port == INVALID_HANDLE_VALUE) {
    print_msg(stderr, "Cannot open %s port\n", argv[1]);
    clean_exit(1);
  } else {
    print_msg(stdout, "%s port opened\n", argv[1]);
  }

  /* Initialize COBS layer over the COM port */
  bt_init(serial_port);
  
  int end = 0;
  
  while (!end) {
    char line[MAILSLOT_MAX_MSG_SIZE];

    /* Receive command from mailslot */
    size_t line_chars = mailslot_read(mailslot_server, line, MAILSLOT_MAX_MSG_SIZE);

    /* Remove line delimiter */
    if (line_chars > 0) {
      line[--line_chars] = '\0';
    }

    if (line_chars == 0) {
      /* The line is empty */
      continue;
    }
    
    char * delim;
    
    if (!strcasecmp(line,"EXIT")) {
      end = 1;
    } else if (!strcasecmp(line,"PING")) {
      ;
    } else if ((delim = strstr(line, ",")) != NULL) {
      /* It is either Sleep or Execute command */
      *delim = '\0';
      process_execute_cmd(line, delim + 1U);
    } else {
      print_msg(stderr, "Unknown command '%s'\n", line);
      clean_exit(1);
    }

    /* Command successful */
    if (send_byte_to_client(0, true) != 0) {
      print_msg(stderr, "Cannot send message to client's mailslot\n");
      clean_exit(1);
    }
  }
  
  clean_exit(0);
}
