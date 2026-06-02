CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -pthread
TARGETS = matrix shared race_demo

# розміри для `make matrix` (демо-режим) та бенчмарку
N ?= 4
M ?= 4
K ?= 4
ITERS ?= 100000000

.PHONY: all matrix-demo run1-bench run1-bench-small run2 outputs clean

all: $(TARGETS)

matrix: matrix.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

shared: shared.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

race_demo: race_demo.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# демонстрація паралелізму: один потік на комірку, друк трійок [i,j]=val
matrix-demo: matrix
	./matrix $(N) $(M) $(K)

# бенчмарк швидкодії vs кількість потоків (за замовчуванням 200x200x200)
run1-bench: matrix
	./matrix bench

run1-bench-small: matrix
	./matrix bench 10 10 10

# спільна змінна: race condition vs синхронізація
run2: shared
	./shared $(ITERS)

# зберегти вивід усіх програм у outputs/*.txt окремими файлами по пунктах умови
outputs: $(TARGETS)
	./gen_outputs.sh

clean:
	rm -f $(TARGETS)
