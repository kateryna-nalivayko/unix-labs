#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEF_N 4
#define DEF_M 4
#define DEF_K 4

#define DEF_BENCH_N 200
#define DEF_BENCH_M 200
#define DEF_BENCH_K 200

#define MAX_BENCH_THREADS 1024

#define RST  "\033[0m"
#define BOLD "\033[1m"
#define CYN  "\033[36m"
#define GRN  "\033[32m"
#define YLW  "\033[33m"

static long *A, *B, *C;
static int   N, M, K;

static pthread_mutex_t print_mtx = PTHREAD_MUTEX_INITIALIZER;

static long elem(const long *Mtx, int cols, int r, int c) {
    return Mtx[(long)r * cols + c];
}

static long dot(int i, int j) {
    long acc = 0;
    for (int l = 0; l < M; ++l)
        acc += elem(A, M, i, l) * elem(B, K, l, j);
    return acc;
}

typedef struct { int i, j; } cell_arg_t;

static void *cell_worker(void *p) {
    cell_arg_t *a = (cell_arg_t *)p;
    long v = dot(a->i, a->j);
    C[(long)a->i * K + a->j] = v;

    pthread_mutex_lock(&print_mtx);
    printf("  [%d,%d]=%ld\n", a->i, a->j, v);
    fflush(stdout);
    pthread_mutex_unlock(&print_mtx);
    return NULL;
}

static void run_demo(void) {
    long cells = (long)N * K;
    pthread_t *th = malloc(cells * sizeof *th);
    cell_arg_t *args = malloc(cells * sizeof *args);
    if (!th || !args) { perror("malloc"); exit(1); }

    printf(BOLD "Множення A[%dx%d] * B[%dx%d]: %ld потоків (по одному на комірку)\n" RST,
           N, M, M, K, cells);
    printf("Порядок виведення трійок [рядок,стовпець]=значення — порядок ЗАВЕРШЕННЯ\n"
           "потоків, а не обходу матриці. Він змінюється від запуску до запуску:\n\n");

    long idx = 0;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < K; ++j) {
            args[idx].i = i;
            args[idx].j = j;
            if (pthread_create(&th[idx], NULL, cell_worker, &args[idx]) != 0) {
                perror("pthread_create");
                exit(1);
            }
            ++idx;
        }

    for (long t = 0; t < cells; ++t)
        pthread_join(th[t], NULL);

    printf("\n" BOLD "Результат C[%dx%d] (упорядковано):\n" RST, N, K);
    for (int i = 0; i < N; ++i) {
        printf("  ");
        for (int j = 0; j < K; ++j)
            printf("%6ld ", elem(C, K, i, j));
        printf("\n");
    }

    free(th);
    free(args);
}

typedef struct {
    long start, end;
} range_arg_t;

static void *range_worker(void *p) {
    range_arg_t *a = (range_arg_t *)p;
    for (long flat = a->start; flat < a->end; ++flat) {
        int i = (int)(flat / K);
        int j = (int)(flat % K);
        C[flat] = dot(i, j);
    }
    return NULL;
}

static double multiply_with(int T) {
    long cells = (long)N * K;
    if (T > cells) T = (int)cells;

    pthread_t   *th   = malloc((size_t)T * sizeof *th);
    range_arg_t *args = malloc((size_t)T * sizeof *args);
    if (!th || !args) { perror("malloc"); exit(1); }

    long base = cells / T, rem = cells % T, cur = 0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int t = 0; t < T; ++t) {
        long chunk = base + (t < rem ? 1 : 0);
        args[t].start = cur;
        args[t].end   = cur + chunk;
        cur += chunk;
        if (pthread_create(&th[t], NULL, range_worker, &args[t]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    for (int t = 0; t < T; ++t)
        pthread_join(th[t], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    free(th);
    free(args);
    return (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
}

static long checksum(void) {
    long s = 0;
    long cells = (long)N * K;
    for (long t = 0; t < cells; ++t) s += C[t];
    return s;
}

static void run_bench(void) {
    long  cells = (long)N * K;
    int   ncpu  = (int)sysconf(_SC_NPROCESSORS_ONLN);

    printf(BOLD "Бенчмарк множення A[%dx%d] * B[%dx%d]  (%ld комірок, %d лог. CPU)\n" RST,
           N, M, M, K, cells, ncpu);
    printf("Кожна комірка — скалярний добуток довжини %d.\n\n", M);

    int  list[64];
    int  n = 0;
    for (int t = 1; t <= cells && t <= MAX_BENCH_THREADS; t *= 2)
        list[n++] = t;
    int have_cpu = 0;
    for (int q = 0; q < n; ++q) if (list[q] == ncpu) have_cpu = 1;
    if (!have_cpu && ncpu <= cells) list[n++] = ncpu;

    for (int a = 1; a < n; ++a) {
        int key = list[a], b = a - 1;
        while (b >= 0 && list[b] > key) { list[b + 1] = list[b]; --b; }
        list[b + 1] = key;
    }

    double base = multiply_with(1);
    long   ref  = checksum();

    printf("%8s %12s %10s %10s\n", "потоків", "час, с", "прискор.", "ефект.");
    printf("%8s %12s %10s %10s\n", "-------", "------", "--------", "------");

    for (int q = 0; q < n; ++q) {
        int T = list[q];
        double best = 1e18;
        for (int rep = 0; rep < 3; ++rep) {
            double s = multiply_with(T);
            if (s < best) best = s;
        }
        if (checksum() != ref) {
            fprintf(stderr, "ПОМИЛКА: контрольна сума розійшлась при T=%d\n", T);
            exit(1);
        }
        double speedup = base / best;
        double eff     = speedup / T * 100.0;
        const char *tag = (T == ncpu) ? YLW "  <- CPU" RST : "";
        printf("%8d %12.4f %9.2fx %9.1f%%%s\n", T, best, speedup, eff, tag);
    }

    printf("\nКонтрольна сума C = %ld (однакова для всіх T — результат коректний).\n", ref);
    printf("\nПояснення:\n"
           "  * до ~%d потоків час падає: робота реально розподіляється по ядрах;\n"
           "  * близько кількості ядер (%d) досягається мінімум часу;\n"
           "  * далі прискорення виходить на плато, а потім ПОГІРШУЄТЬСЯ — потоків\n"
           "    більше ніж ядер, з'являються накладні витрати на створення потоків\n"
           "    та перемикання контексту (oversubscription).\n",
           ncpu, ncpu);
}

static void fill_matrices(void) {
    long sa = (long)N * M, sb = (long)M * K, sc = (long)N * K;
    A = malloc(sa * sizeof *A);
    B = malloc(sb * sizeof *B);
    C = calloc(sc, sizeof *C);
    if (!A || !B || !C) { perror("malloc"); exit(1); }

    for (long t = 0; t < sa; ++t) A[t] = (t % 9) + 1;
    for (long t = 0; t < sb; ++t) B[t] = (t % 7) + 1;
}

int main(int argc, char **argv) {
    int bench = 0, arg = 1;
    if (argc > 1 && strcmp(argv[1], "bench") == 0) { bench = 1; arg = 2; }

    if (bench) {
        N = (argc > arg)     ? atoi(argv[arg])     : DEF_BENCH_N;
        M = (argc > arg + 1) ? atoi(argv[arg + 1]) : DEF_BENCH_M;
        K = (argc > arg + 2) ? atoi(argv[arg + 2]) : DEF_BENCH_K;
    } else {
        N = (argc > arg)     ? atoi(argv[arg])     : DEF_N;
        M = (argc > arg + 1) ? atoi(argv[arg + 1]) : DEF_M;
        K = (argc > arg + 2) ? atoi(argv[arg + 2]) : DEF_K;
    }
    if (N <= 0 || M <= 0 || K <= 0) {
        fprintf(stderr, "Розміри мають бути додатними.\n"
                        "Використання: %s [bench] [N [M [K]]]\n", argv[0]);
        return 1;
    }

    fill_matrices();
    if (bench) run_bench();
    else       run_demo();

    free(A); free(B); free(C);
    return 0;
}
