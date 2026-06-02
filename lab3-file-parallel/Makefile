CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET  = parallel

.PHONY: all run clean

all: $(TARGET)

$(TARGET): main.c functions.c functions.h
	$(CC) $(CFLAGS) -o $@ main.c functions.c

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
