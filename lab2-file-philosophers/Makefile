CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -pthread
TARGET  = philosophers

# Default number of philosophers for `make run`
N            ?= 5
THINK_MS     ?= 800
EAT_MS       ?= 800

.PHONY: all run clean

all: $(TARGET)

$(TARGET): philosophers.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

run: all
	./$(TARGET) $(N) $(THINK_MS) $(EAT_MS)

clean:
	rm -f $(TARGET)
