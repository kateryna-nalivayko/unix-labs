#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RST  "\033[0m"
#define BOLD "\033[1m"
#define GRN  "\033[32m"
#define RED  "\033[31m"
#define YLW  "\033[33m"

#define DEF_ITERS  100000000LL
#define SYNC_STEPS 1000

static long long ITERS;

static double now_s(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static volatile long long v_unsafe;

static void *th_unsafe(void *arg) {
    (void)arg;
    for (long long i = 0; i < ITERS; ++i)
        v_unsafe = v_unsafe + 1;
    return NULL;
}

static long long       v_mutex;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static void *th_mutex(void *arg) {
    (void)arg;
    for (long long i = 0; i < ITERS; ++i) {
        pthread_mutex_lock(&mtx);
        v_mutex = v_mutex + 1;
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

static _Atomic long long v_atomic;

static void *th_atomic(void *arg) {
    (void)arg;
    for (long long i = 0; i < ITERS; ++i)
        atomic_fetch_add_explicit(&v_atomic, 1, memory_order_relaxed);
    return NULL;
}

static _Atomic long long v_local;

static void *th_local(void *arg) {
    (void)arg;
    volatile long long local = 0;
    for (long long i = 0; i < ITERS; ++i)
        local = local + 1;
    atomic_fetch_add_explicit(&v_local, local, memory_order_relaxed);
    return NULL;
}

typedef struct {
    pthread_mutex_t mu;
    pthread_cond_t  cv;
    int             count;
    int             total;
    unsigned        gen;
} barrier_t;

static void barrier_init(barrier_t *b, int total) {
    pthread_mutex_init(&b->mu, NULL);
    pthread_cond_init(&b->cv, NULL);
    b->count = 0;
    b->total = total;
    b->gen   = 0;
}

static void barrier_wait(barrier_t *b) {
    pthread_mutex_lock(&b->mu);
    unsigned g = b->gen;
    if (++b->count == b->total) {
        b->count = 0;
        b->gen++;
        pthread_cond_broadcast(&b->cv);
    } else {
        while (g == b->gen)
            pthread_cond_wait(&b->cv, &b->mu);
    }
    pthread_mutex_unlock(&b->mu);
}

static barrier_t        sync_bar;
static long long        v_step;
static volatile int     turn;

typedef struct { int id; } step_arg_t;

static void *th_step(void *arg) {
    int id = ((step_arg_t *)arg)->id;
    for (int s = 0; s < SYNC_STEPS; ++s) {
        barrier_wait(&sync_bar);
        if (turn == id)
            v_step = v_step + 1;
        barrier_wait(&sync_bar);
        if (turn == id)
            turn ^= 1;
        barrier_wait(&sync_bar);
    }
    return NULL;
}

static double run_pair(void *(*fn)(void *), void *a0, void *a1) {
    pthread_t t0, t1;
    double start = now_s();
    pthread_create(&t0, NULL, fn, a0);
    pthread_create(&t1, NULL, fn, a1);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    return now_s() - start;
}

static void report(const char *name, long long got, long long want, double secs) {
    int ok = (got == want);
    printf("  %-10s v = %-13lld (очік. %-13lld) %s%s" RST "  час: %.3f с\n",
           name, got, want,
           ok ? GRN : RED, ok ? "коректно" : "НЕКОРЕКТНО", secs);
}

int main(int argc, char **argv) {
    ITERS = (argc > 1) ? atoll(argv[1]) : DEF_ITERS;
    if (ITERS <= 0) { fprintf(stderr, "ITERS має бути додатним\n"); return 1; }

    long long want = 2 * ITERS;

    printf(BOLD "Два потоки, кожен робить %lld інкрементів спільної змінної.\n" RST,
           ITERS);
    printf("Коректне фінальне значення = 2 * %lld = %lld.\n\n", ITERS, want);

    v_unsafe = 0;
    double t1 = run_pair(th_unsafe, NULL, NULL);
    report("unsafe", (long long)v_unsafe, want, t1);

    v_mutex = 0;
    double t2 = run_pair(th_mutex, NULL, NULL);
    report("mutex", v_mutex, want, t2);

    atomic_store(&v_atomic, 0);
    double t3 = run_pair(th_atomic, NULL, NULL);
    report("atomic", atomic_load(&v_atomic), want, t3);

    atomic_store(&v_local, 0);
    double t4 = run_pair(th_local, NULL, NULL);
    report("local-sum", atomic_load(&v_local), want, t4);

    printf("\n" BOLD "Швидкість коректних варіантів:" RST
           " mutex %.3f с  >  atomic %.3f с  >  local-sum %.3f с\n", t2, t3, t4);
    printf("  Найшвидший коректний — local-sum: спільна пам'ять чіпається лише\n"
           "  двічі (по разу на потік), тому немає конкуренції за кеш-лінію.\n");

    printf("\n" BOLD "Повністю синхронний режим (крок-в-крок):" RST "\n");
    barrier_init(&sync_bar, 2);
    v_step = 0;
    turn   = 0;
    step_arg_t s0 = { 0 }, s1 = { 1 };
    double t5 = run_pair(th_step, &s0, &s1);
    int ok = (v_step == SYNC_STEPS);
    printf("  два потоки разом зробили %lld інкрементів за %d кроків: v = %lld %s%s" RST
           "  час: %.4f с\n",
           (long long)SYNC_STEPS, SYNC_STEPS, v_step,
           ok ? GRN : RED, ok ? "(= SYNC_STEPS, а не 2*SYNC_STEPS)" : "(ПОМИЛКА)", t5);
    printf("  На кожному кроці інкремент робить лише один потік (по черзі),\n"
           "  обидва синхронізуються бар'єром — звідси рівно %d, а не %d.\n",
           SYNC_STEPS, 2 * SYNC_STEPS);

    printf("\n" BOLD "Чому unsafe дає некоректний результат (race condition):" RST "\n"
           "  v=v+1 — це три дії: ЧИТАННЯ v, +1, ЗАПИС v. Два потоки виконують їх\n"
           "  упереміш: обидва можуть прочитати одне й те саме значення, додати 1\n"
           "  і записати назад — два інкременти зливаються в один. Тому фінальне\n"
           "  значення в unsafe майже завжди МЕНШЕ за %lld (втрачені оновлення).\n",
           want);

    return 0;
}
