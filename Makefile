# Makefile for Systems Software Continuous Assessment 2
# This file provides automated build and clean operations for the client-server solution

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

# Target executables
SERVER = server
CLIENT = client

# Source files
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Header files
SERVER_HDR = server.h
CLIENT_HDR = client.h

# Default target
all: $(SERVER) $(CLIENT)

# Server compilation
$(SERVER): $(SERVER_SRC) $(SERVER_HDR)
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRC)

# Client compilation
$(CLIENT): $(CLIENT_SRC) $(CLIENT_HDR)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_SRC)

# Clean compiled files
clean:
	rm -f $(SERVER) $(CLIENT)

# Install target - creates necessary directories
install:
	mkdir -p Manufacturing Distribution
	chmod 770 Manufacturing Distribution

# Uninstall target - removes created directories
uninstall:
	rm -rf Manufacturing Distribution

# Run setup script
setup:
	./setup.sh

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build both server and client (default)"
	@echo "  server     - Build only the server"
	@echo "  client     - Build only the client"
	@echo "  clean      - Remove compiled executables"
	@echo "  install    - Create necessary directories"
	@echo "  uninstall  - Remove created directories"
	@echo "  setup      - Run the setup script to create users and groups"
	@echo "  help       - Display this help message"

# Phony targets (targets that don't represent files)
.PHONY: all clean install uninstall setup help