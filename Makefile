CC = gcc
CFLAGS = -lm

viewer: src/*.c
	$(CC) $? $(CFLAGS) -o $@

clean:
	rm viewer
