#  ЛАБОРАТОРНА №5 — System Call API (Linux).
#  Повноцінно збирається й запускається ТІЛЬКИ на Linux (потрібні /proc та /dev/input).
#  На не-Linux (macOS) keyhook та inject пропускаються з повідомленням — це очікувано.

CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic
# writer збираємо без оптимізацій, щоб змінна гарантовано лежала в пам'яті
CFLAGS_WRITER = -std=c11 -Wall -Wextra -Wpedantic -O0

UNAME_S := $(shell uname -s)

TARGETS = writer reader keyhook inject

.PHONY: all task1 task2 clean

all: $(TARGETS)

task1: writer reader
task2: keyhook inject

writer: writer.c
	$(CC) $(CFLAGS_WRITER) -o $@ $<

reader: reader.c
	$(CC) $(CFLAGS) -O2 -o $@ $<

# keyhook та inject потребують <linux/input.h> / <linux/uinput.h>:
# на Linux компілюються, на інших ОС — пропускаються без помилки.
keyhook: keyhook.c
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		echo "$(CC) $(CFLAGS) -O2 -o $@ $<"; \
		$(CC) $(CFLAGS) -O2 -o $@ $<; \
	else \
		echo ">> keyhook пропущено: лише Linux (немає <linux/input.h> на $(UNAME_S))."; \
	fi

inject: inject.c
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		echo "$(CC) $(CFLAGS) -O2 -o $@ $<"; \
		$(CC) $(CFLAGS) -O2 -o $@ $<; \
	else \
		echo ">> inject пропущено: лише Linux (немає <linux/uinput.h> на $(UNAME_S))."; \
	fi

clean:
	rm -f $(TARGETS)
