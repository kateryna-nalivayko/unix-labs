#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_N      5
#define THINK_MIN_MS   400
#define THINK_MAX_MS   800
#define EAT_MIN_MS     400
#define EAT_MAX_MS     800

#define RST  "\033[0m"
#define BOLD "\033[1m"
#define BLU  "\033[34m"
#define YLW  "\033[33m"
#define GRN  "\033[32m"

static volatile sig_atomic_t running = 1;

typedef struct {
    pthread_mutex_t mu;
    pthread_cond_t  cv;
    int             value;
} bsem_t;

static void bsem_init(bsem_t *s, int v) {
    pthread_mutex_init(&s->mu, NULL);
    pthread_cond_init(&s->cv, NULL);
    s->value = v;
}

static int bsem_acquire(bsem_t *s) {
    struct timespec ts;
    pthread_mutex_lock(&s->mu);
    while (s->value == 0 && running) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 100000000L;
        if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }
        pthread_cond_timedwait(&s->cv, &s->mu, &ts);
    }
    int ok = (s->value > 0);
    if (ok) s->value = 0;
    pthread_mutex_unlock(&s->mu);
    return ok;
}

static void bsem_release(bsem_t *s) {
    pthread_mutex_lock(&s->mu);
    s->value = 1;
    pthread_cond_signal(&s->cv);
    pthread_mutex_unlock(&s->mu);
}

static void bsem_destroy(bsem_t *s) {
    pthread_mutex_destroy(&s->mu);
    pthread_cond_destroy(&s->cv);
}

typedef enum { THINKING = 0, HUNGRY = 1, EATING = 2 } State;

static int                   N;
static int                   think_max_ms;
static int                   eat_max_ms;
static _Atomic State        *state;
static bsem_t               *fork_sem;
static pthread_mutex_t       critical_mtx;
static atomic_long          *meals;

static void sleep_ms(long ms) {
    if (ms <= 0) return;
    struct timespec rem = { ms / 1000, (ms % 1000) * 1000000L };
    while (nanosleep(&rem, &rem) == -1 && errno == EINTR)
        if (!running) return;
}

static int left_of(int i)  { return (i - 1 + N) % N; }
static int right_of(int i) { return (i + 1)     % N; }

static void test(int i) {
    if (atomic_load(&state[i])           == HUNGRY &&
        atomic_load(&state[left_of(i)])  != EATING &&
        atomic_load(&state[right_of(i)]) != EATING) {
        atomic_store(&state[i], EATING);
        bsem_release(&fork_sem[i]);
    }
}

static int take_forks(int i) {
    pthread_mutex_lock(&critical_mtx);
    atomic_store(&state[i], HUNGRY);
    test(i);
    pthread_mutex_unlock(&critical_mtx);

    if (bsem_acquire(&fork_sem[i]))
        return 1;

    pthread_mutex_lock(&critical_mtx);
    atomic_store(&state[i], THINKING);
    pthread_mutex_unlock(&critical_mtx);
    return 0;
}

static void put_forks(int i) {
    pthread_mutex_lock(&critical_mtx);
    atomic_store(&state[i], THINKING);
    test(left_of(i));
    test(right_of(i));
    pthread_mutex_unlock(&critical_mtx);
}

static void *display_thread(void *arg) {
    (void)arg;
    while (running) {
        printf("\r");
        for (int i = 0; i < N; i++) {
            long m = atomic_load(&meals[i]);
            switch (atomic_load(&state[i])) {
                case THINKING: printf(BLU "P%d" RST ":think %-5ld", i, m); break;
                case HUNGRY:   printf(YLW "P%d" RST ":wait  %-5ld", i, m); break;
                case EATING:   printf(GRN "P%d" RST ":eat   %-5ld", i, m); break;
            }
        }
        fflush(stdout);
        sleep_ms(80);
    }
    return NULL;
}

static void *philosopher(void *arg) {
    int id = *(int *)arg;
    free(arg);

    unsigned int seed = (unsigned int)((long)time(NULL) ^ ((long)id * 2654435761UL));

    while (running) {
        sleep_ms(THINK_MIN_MS + rand_r(&seed) % (think_max_ms - THINK_MIN_MS + 1));
        if (!running) break;

        if (!take_forks(id)) break;

        atomic_fetch_add(&meals[id], 1);
        sleep_ms(EAT_MIN_MS + rand_r(&seed) % (eat_max_ms - EAT_MIN_MS + 1));

        put_forks(id);
    }
    return NULL;
}

static void stop(int sig) { (void)sig; running = 0; }

int main(int argc, char *argv[]) {
    N            = argc > 1 ? atoi(argv[1]) : DEFAULT_N;
    think_max_ms = argc > 2 ? atoi(argv[2]) : THINK_MAX_MS;
    eat_max_ms   = argc > 3 ? atoi(argv[3]) : EAT_MAX_MS;

    if (N < 2 || think_max_ms < THINK_MIN_MS || eat_max_ms < EAT_MIN_MS) {
        fprintf(stderr,
                "usage: %s [N>=2] [think_max_ms>=%d] [eat_max_ms>=%d]\n",
                argv[0], THINK_MIN_MS, EAT_MIN_MS);
        return 1;
    }

    printf("Dining Philosophers (Dijkstra)  "
           "n=%d  think %d-%dms  eat %d-%dms  (Ctrl-C to stop)\n\n",
           N, THINK_MIN_MS, think_max_ms, EAT_MIN_MS, eat_max_ms);

    state    = malloc(N * sizeof(*state));./philosophers 5 800 800
    fork_sem = malloc(N * sizeof(*fork_sem));
    meals    = malloc(N * sizeof(*meals));
    if (!state || !fork_sem || !meals) { perror("malloc"); return 1; }

    pthread_mutex_init(&critical_mtx, NULL);

    for (int i = 0; i < N; i++) {
        atomic_init(&state[i], THINKING);
        bsem_init(&fork_sem[i], 0);
        atomic_init(&meals[i], 0);
    }

    signal(SIGINT,  stop);
    signal(SIGTERM, stop);

    pthread_t *threads = malloc(N * sizeof(*threads));
    pthread_t  disp;
    if (!threads) { perror("malloc"); return 1; }

    pthread_create(&disp, NULL, display_thread, NULL);
    for (int i = 0; i < N; i++) {
        int *id = malloc(sizeof(*id));
        *id = i;
        pthread_create(&threads[i], NULL, philosopher, id);
    }

    for (int i = 0; i < N; i++)
        pthread_join(threads[i], NULL);
    pthread_join(disp, NULL);

    printf("\n\n" BOLD "Meals eaten:" RST "\n");
    long total = 0;
    for (int i = 0; i < N; i++) {
        long m = atomic_load(&meals[i]);
        printf("  P%-2d  %ld\n", i, m);
        total += m;
    }
    printf("  total  %ld\n", total);

    for (int i = 0; i < N; i++) bsem_destroy(&fork_sem[i]);
    pthread_mutex_destroy(&critical_mtx);
    free(state);
    free(fork_sem);
    free(meals);
    free(threads);
    return 0;
}
