#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define START 100

static long v = START;

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cv = PTHREAD_COND_INITIALIZER;
static int phase = 0;

static void wait_phase(int want) {
    pthread_mutex_lock(&mu);
    while (phase != want)
        pthread_cond_wait(&cv, &mu);
    pthread_mutex_unlock(&mu);
}

static void set_phase(int next) {
    pthread_mutex_lock(&mu);
    phase = next;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mu);
}

static void *thread_A(void *arg) {
    (void)arg;
    wait_phase(0);
    long tmp = v;
    printf("  потік A: READ  v = %ld\n", tmp);
    set_phase(1);

    wait_phase(2);
    tmp = tmp + 1;
    v = tmp;
    printf("  потік A: +1 -> WRITE v = %ld\n", v);
    set_phase(3);
    return NULL;
}

static void *thread_B(void *arg) {
    (void)arg;
    wait_phase(1);
    long tmp = v;
    printf("  потік B: READ  v = %ld        <-- прочитав ТЕ САМЕ значення\n", tmp);
    set_phase(2);

    wait_phase(3);
    tmp = tmp + 1;
    v = tmp;
    printf("  потік B: +1 -> WRITE v = %ld        <-- ЗАТЕР запис потоку A\n", v);
    set_phase(4);
    return NULL;
}

int main(void) {
    printf("Наочна демонстрація race condition (lost update).\n");
    printf("Два потоки роблять по одному інкременту v (старт v = %d).\n", START);
    printf("Якби все було коректно, у кінці було б v = %d.\n\n", START + 2);

    pthread_t a, b;
    pthread_create(&a, NULL, thread_A, NULL);
    pthread_create(&b, NULL, thread_B, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);

    printf("\nФінальне v = %ld, а мало бути %d.\n", v, START + 2);
    printf("Два інкременти злилися в один: обидва потоки прочитали %d,\n", START);
    printf("обидва записали %d — одне оновлення ВТРАЧЕНО. Це і є race condition.\n",
           START + 1);
    return 0;
}
