CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11
LDFLAGS = -lcrypto -lz
TARGET = minigit
SOURCES = src/main.c src/repository.c

.PHONY: build clean

build: $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

clean:
	$(RM) $(TARGET)
