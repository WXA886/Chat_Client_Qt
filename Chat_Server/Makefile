CC = gcc

CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS = -pthread
LDLIBS = -lcjson

SERVER = chat_server
CLIENT = chat_client

SERVER_SRC = \
	src/main.c \
	src/server.c \
	src/client.c \
	src/broadcast.c \
	src/protocol.c

CLIENT_SRC = \
	client/chat_client.c \
	src/protocol.c

.PHONY: all clean

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $@ $(LDFLAGS) $(LDLIBS)

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(SERVER) $(CLIENT)