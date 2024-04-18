CC = gcc
CFLAGS = -Wall -Wextra

all: rate edf

rate: real.c
	$(CC) $(CFLAGS) -o rate real.c -DRATE
	
edf: real.c
	$(CC) $(CFLAGS) -o edf real.c -DEDF

clean:
	rm -f rate edf
    
.PHONY: clean