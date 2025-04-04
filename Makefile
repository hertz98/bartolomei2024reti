# Makefile

COMPILER = gcc
CFLAGS = -g -Wall

SERVER_FLAGS := 127.0.0.1 1234
CLIENT_FLAGS := 127.0.0.1 1234

DEBUGGER = gdb
DFLAGS := 

SOURCES_DIR := ./src
SERVER_DIR := $(SOURCES_DIR)/server
CLIENT_DIR := $(SOURCES_DIR)/client
SHARED_DIR := $(SOURCES_DIR)/shared
TESTING_DIR := $(SOURCES_DIR)/testing

SERVER_SOURCE := $(SERVER_DIR)/server.c
CLIENT_SOURCE := $(CLIENT_DIR)/client.c
SERVER_SOURCES := $(wildcard $(SERVER_DIR)/*.c) $(wildcard $(SHARED_DIR)/*.c)
CLIENT_SOURCES := $(wildcard $(CLIENT_DIR)/*.c) $(wildcard $(SHARED_DIR)/*.c)

BIN_DIR := ./bin
TARGET_SERVER := $(BIN_DIR)/server
TARGET_CLIENT := $(BIN_DIR)/client

TESTING_SOURCES := $(foreach dir, $(SERVER_DIR) $(SHARED_DIR) $(TESTING_DIR), $(wildcard $(dir)/*.c)) 
TESTING_SOURCES := $(filter-out $(SERVER_SOURCE), $(TESTING_SOURCES))

TARGET_TESTING := $(BIN_DIR)/testing

all: build-server build-client

build-server: $(SERVER_SOURCE) data
	$(COMPILER) $(CFLAGS) $(SERVER_SOURCES) -o $(TARGET_SERVER)

build-client: $(CLIENT_SOURCE) data
	$(COMPILER) $(CFLAGS) $(CLIENT_SOURCES) -o $(TARGET_CLIENT)

clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_TESTING)

server: build-server
	$(TARGET_SERVER) $(SERVER_FLAGS)

client: build-client
	$(TARGET_CLIENT) $(CLIENT_FLAGS)

server-gdb: build-server
	$(DEBUGGER) $(DFLAGS) -x $(SERVER_DIR)/.gdbinit --args $(TARGET_SERVER) $(SERVER_FLAGS)

client-gdb: build-client
	$(DEBUGGER) $(DFLAGS) -x $(CLIENT_DIR)/.gdbinit --args $(TARGET_CLIENT) $(CLIENT_FLAGS)

clean-users:
	rm $(BIN_DIR)/data/users/*.txt

data:
	mkdir -p $(BIN_DIR) $(BIN_DIR)/data $(BIN_DIR)/data/topics $(BIN_DIR)/data/users

testing: data
	$(COMPILER) $(CFLAGS) $(TESTING_SOURCES) -o $(TARGET_TESTING) && $(TARGET_TESTING)
