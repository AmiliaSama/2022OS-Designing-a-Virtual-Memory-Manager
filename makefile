CC = gcc
CFLAGS = -Wall -g
sources = $(wildcard *.c)
target = $(patsubst %.c,%,$(sources))
all:$(target)

$(target):%:%.c
	$(CC) $< -o $@

clean:
	rm $(target)