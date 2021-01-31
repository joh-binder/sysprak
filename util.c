#include "util.h"

bool startsWith(const char *a, const char *b) {
    if (strncmp(a, b, strlen(b)) == 0)
        return 1;
    return 0;
}

int getSign(int a) {
    if (a == 0) return 0;
    else if (a > 0) return 1;
    else return -1;
}

int abs(int a) {
    if (a >= 0) return a;
    else return -a;
}
