#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile long shared_value = 0;

int main(void) {
    printf("=== Процес-власник пам'яті (writer) ===\n");
    printf("PID                = %d\n", getpid());
    printf("Адреса shared_value = %p\n", (void *)&shared_value);
    printf("Розмір             = %zu байт\n\n", sizeof(shared_value));
    FILE *tf = fopen("/tmp/lab5-target", "w");
    if (tf) { fprintf(tf, "%d %p\n", getpid(), (void *)&shared_value); fclose(tf); }

    printf("Стеж за цією змінною з ДРУГОГО термінала:\n");
    printf("    sudo ./reader %d %p\n", getpid(), (void *)&shared_value);
    printf("  або простіше: sudo ./reader $(cat /tmp/lab5-target)\n\n");

    printf("Вводь нові значення (цілі числа), Ctrl+D — завершити:\n");
    long v;
    while (1) {
        printf("value> ");
        fflush(stdout);
        if (scanf("%ld", &v) != 1)
            break;
        shared_value = v;
        printf("  -> у пам'ять за адресою %p записано %ld\n",
               (void *)&shared_value, shared_value);
    }
    printf("\nЗавершення writer.\n");
    return 0;
}
