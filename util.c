#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


bool startsWith(const char *a, const char *b){
  if (strncmp(a, b, strlen(b)) == 0)
    return 1;
  return 0;
}

int minInt(int a, int b) {
    if (b < a) return b;
    else return a;
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
