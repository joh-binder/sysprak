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