#ifndef UTIL
#define UTIL

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define MAX_LEN 1024

bool startsWith(const char *a, const char *b);

int getSign(int a);

int abs(int a);

int ownWrite(int fd, char buffer[MAX_LEN]);

#endif
