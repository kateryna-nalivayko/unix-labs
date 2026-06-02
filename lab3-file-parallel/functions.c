#include "functions.h"

int f(int x) {
    return x;
}

int g(int x) {
    (void)x;
    while (1);
    return 0;
}
