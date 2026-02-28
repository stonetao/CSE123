CC      = gcc
CFLAGS  = -Wall -Wextra -g

TARGETS = server_test test_echo_client tftp_server tftp_client

all: $(TARGETS)

server_test: server_test.c tftp.h
	$(CC) $(CFLAGS) -o $@ $<

test_echo_client: test_echo_client.c tftp.h
	$(CC) $(CFLAGS) -o $@ $<

tftp_server: tftp_server.c tftp.h
	$(CC) $(CFLAGS) -o $@ $<

tftp_client: tftp_client.c tftp.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
