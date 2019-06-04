CC = gcc
CFLAGS = -Wall

SERVER_SRCS = yup-comm-server.c comm/serial_port.c comm/bt.c protocol/data.c protocol/frame.c
SERVER_SRCS += utils/crc32q.c mailslot.c

SERVER_HDRS = comm/bt.h comm/serial_port.h protocol/data.h protocol/frame.h
SERVER_HDRS += protocol/return_codes.h utils/byte_buffer.h utils/crc32q.h
SERVER_HDRS += mailslot.h

CLIENT_SRCS = yup-comm-client.c mailslot.c

CLIENT_HDRS = mailslot.h


all: yup-comm-server yup-comm-client

yup-comm-server: $(SERVER_SRCS) $(SERVER_HDRS)
	$(CC) $(CFLAGS) $(SERVER_SRCS) -o yup-comm-server

yup-comm-client: $(CLIENT_SRCS) $(CLIENT_HDRS)
	$(CC) $(CFLAGS) $(CLIENT_SRCS) -o yup-comm-client

.PHONY: clean

clean:
	rm -f *.o *~ */*.o */*~ yup-comm-server.exe yup-comm-client.exe
