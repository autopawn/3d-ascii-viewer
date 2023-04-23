CC = gcc
CFLAGS = -lm -lncurses
SRC_DIR = src

SRCS := $(shell find $(SRC_DIR) -name '*.c')

viewer: $(SRCS)
	$(CC) $? $(CFLAGS) -o $@

clean:
	rm viewer
