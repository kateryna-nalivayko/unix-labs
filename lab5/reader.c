#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Використання: %s <pid> <hex-адреса>\n", argv[0]);
        fprintf(stderr, "Приклад:      sudo %s 12345 0x5601234abc\n", argv[0]);
        return 1;
    }

    int           pid  = atoi(argv[1]);
    unsigned long addr = strtoul(argv[2], NULL, 0);

    char path[64];
    snprintf(path, sizeof path, "/proc/%d/mem", pid);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open(/proc/<pid>/mem)");
        fprintf(stderr, "Підказка: запусти через sudo або встанови ptrace_scope=0.\n");
        return 1;
    }

    printf("=== Спостерігач пам'яті (reader) ===\n");
    printf("Стежу за %s, адреса 0x%lx ...\n", path, addr);
    printf("(зміни значення у writer — вони з'являться нижче)\n\n");

    long prev = 0, cur = 0;
    int  first = 1;
    while (1) {
        ssize_t n = pread(fd, &cur, sizeof cur, (off_t)addr);
        if (n != (ssize_t)sizeof cur) {
            if (n < 0) perror("pread");
            else       fprintf(stderr, "Процес %d, схоже, завершився.\n", pid);
            break;
        }
        if (first || cur != prev) {
            printf("[зміна] значення = %ld  (0x%lx)\n", cur, (unsigned long)cur);
            fflush(stdout);
            prev  = cur;
            first = 0;
        }
        usleep(100000);
    }

    close(fd);
    return 0;
}
