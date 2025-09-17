CC = $(CROSS_COMPILE)gcc
CFLAGS = -static

all: tx

clean:
	-rm tx

tx: tx.c
	$(CC) $(CFLAGS) -o tx tx.c
