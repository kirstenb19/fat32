CC=gcc
CFLAGS=-Wall -Iinclude
SRCS=$(wildcard src/*.c)
OBJS=$(patsubst src/%.c,bin/%.o,$(SRCS))
TARGET=bin/filesys

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

bin/%.o: src/%.c
	mkdir -p bin
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin

.PHONY: all clean