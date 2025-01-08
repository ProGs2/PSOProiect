CC = gcc
CFLAGS = -Wall -g   # Enable all warnings and debugging symbols
SRC = mainDNS.c trie.c cache.c thread.c logger.c dns_packet.c dns_server.c
OBJ = $(SRC:.c=.o)   # Converts .c files to .o object files
OUT = myprogram      # Output binary name

# Default target to build the program
all: $(OUT) dns_client

# Rule to link the object files and create the output binary
$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT)

# Rule to compile the .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

dns_client: dns_client.c
	gcc -Wall -g dns_client.c -o dns_client

# Clean target to remove object files and the binary
clean:
	rm -f $(OBJ) $(OUT) dns_client

# Phony targets (to avoid conflicts with file names)
.PHONY: all clean