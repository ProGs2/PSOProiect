.PHONY: all clean

CC = gcc
CFLAGS = -Wall -g   # Enable all warnings and debugging symbols
SRC = dns_packet.c dns_server.c main.c
OBJ = $(SRC:.c=.o)  # Converts .c files to .o object files
OUT = server		# Output binary name

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(OUT)