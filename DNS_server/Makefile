CC = gcc
CFLAGS = -Wall -g   # Enable all warnings and debugging symbols
SRC = mainDNS.c trie.c cache.c thread.c logger.c
OBJ = $(SRC:.c=.o)   # Converts .c files to .o object files
OUT = myprogram      # Output binary name

# Default target to build the program
all: $(OUT)

# Rule to link the object files and create the output binary
$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT)

# Rule to compile the .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean target to remove object files and the binary
clean:
	rm -f $(OBJ) $(OUT)

# Phony targets (to avoid conflicts with file names)
.PHONY: all clean