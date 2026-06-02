#include "functions.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define TIMEOUT_SEC 10

static volatile pid_t pid_f = -1;
static volatile pid_t pid_g = -1;

static void kill_workers(void) {
    if (pid_f > 0) kill(pid_f, SIGKILL);
    if (pid_g > 0) kill(pid_g, SIGKILL);
}

static void handle_sigint(int sig) {
    (void)sig;
    kill_workers();
    write(STDOUT_FILENO, "\nInterrupted.\n", 14);
    _exit(1);
}

static void run_worker(int sock, int (*fn)(int)) {
    signal(SIGINT,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    int x;
    if (recv(sock, &x, sizeof x, MSG_WAITALL) <= 0) _exit(1);
    int r = fn(x);
    send(sock, &r, sizeof r, 0);
    close(sock);
    _exit(0);
}

int main(void) {
    printf("f(x) || g(x)  [Kleene three-valued logic]\n");
    printf("Enter x: ");
    fflush(stdout);

    int x;
    if (scanf("%d", &x) != 1) {
        fputs("Invalid input.\n", stderr);
        return 1;
    }

    signal(SIGINT, handle_sigint);

    int sv_f[2], sv_g[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv_f) < 0) {
        perror("socketpair");
        return 1;
    }
    pid_f = fork();
    if (pid_f < 0) { perror("fork"); return 1; }
    if (pid_f == 0) {
        close(sv_f[0]);
        run_worker(sv_f[1], f);
    }
    close(sv_f[1]);

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv_g) < 0) {
        perror("socketpair");
        kill(pid_f, SIGKILL);
        waitpid(pid_f, NULL, 0);
        return 1;
    }
    pid_g = fork();
    if (pid_g < 0) { perror("fork"); return 1; }
    if (pid_g == 0) {
        close(sv_g[0]);
        run_worker(sv_g[1], g);
    }
    close(sv_g[1]);

    send(sv_f[0], &x, sizeof x, 0);
    send(sv_g[0], &x, sizeof x, 0);

    printf("Computing f(%d) and g(%d) in parallel...\n\n", x, x);

    int f_done = 0, g_done = 0;
    int f_val  = 0, g_val  = 0;
    int no_ask = 0;
    const char *result = NULL;

    while (!result) {
        fd_set rfds;
        FD_ZERO(&rfds);
        if (!f_done) FD_SET(sv_f[0], &rfds);
        if (!g_done) FD_SET(sv_g[0], &rfds);
        int nfds = (sv_f[0] > sv_g[0] ? sv_f[0] : sv_g[0]) + 1;

        struct timeval tv  = {TIMEOUT_SEC, 0};
        int sel = select(nfds, &rfds, NULL, NULL, no_ask ? NULL : &tv);

        if (sel < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        if (sel == 0) {
            printf("A function has been computing for %d+ seconds.\n", TIMEOUT_SEC);
            printf("  1) Continue waiting\n");
            printf("  2) Stop and output result now\n");
            printf("  3) Continue without asking again\n");
            printf("Choice: ");
            fflush(stdout);
            int ch = 0;
            scanf("%d", &ch);
            if (ch == 2) {
                kill_workers();
                result = ((f_done && f_val) || (g_done && g_val)) ? "true" : "undefined";
            } else if (ch == 3) {
                no_ask = 1;
            }
            continue;
        }

        if (!f_done && FD_ISSET(sv_f[0], &rfds)) {
            if (recv(sv_f[0], &f_val, sizeof f_val, MSG_WAITALL) > 0) {
                f_done = 1;
                printf("  f(%d) = %d\n", x, f_val);
                if (f_val) {
                    kill(pid_g, SIGKILL);
                    g_done = 1;
                    result = "true";
                } else if (g_done) {
                    result = g_val ? "true" : "false";
                }
            }
        }

        if (!result && !g_done && FD_ISSET(sv_g[0], &rfds)) {
            if (recv(sv_g[0], &g_val, sizeof g_val, MSG_WAITALL) > 0) {
                g_done = 1;
                printf("  g(%d) = %d\n", x, g_val);
                if (g_val) {
                    kill(pid_f, SIGKILL);
                    f_done = 1;
                    result = "true";
                } else if (f_done) {
                    result = f_val ? "true" : "false";
                }
            }
        }
    }

    printf("\nf(%d) || g(%d)  =  %s\n", x, x, result ? result : "error");

    kill_workers();
    waitpid(pid_f, NULL, 0);
    waitpid(pid_g, NULL, 0);
    close(sv_f[0]);
    close(sv_g[0]);
    return 0;
}
