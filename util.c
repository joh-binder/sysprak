#include "util.h"

#define MAX_LEN 1024

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


int ownWrite(int fd, char buffer[MAX_LEN]){
    int n;
    while (strlen(buffer) > 0) {
        do
        {
            n = write(fd, buffer, strlen(buffer));
        } while ((n < 0) && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
        if (n < 0)
            return n;
        buffer += n;
    }
    return 0;
}