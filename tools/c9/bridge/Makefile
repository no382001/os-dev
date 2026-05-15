CC     = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -I../include
TARGET = c9_bridge

$(TARGET): c9_bridge.c ../proto.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
