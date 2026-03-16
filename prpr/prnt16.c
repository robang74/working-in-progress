/* 
 * (c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, MIT
 *
 */
#include <stdio.h>
#include <unistd.h>

#define ALPH16  "0123456789abcdef"
#define LEN 1024

int main(void)
{
    register const char *p = ALPH16;
    int i = 0;

    setvbuf(stdout, NULL, _IONBF, 0);   // disable buffering → each write is visible immediately

    for (size_t i = 0; i < LEN; i++)
    {
        if(put) {
            putchar(p[i & 0x0f]);
            fflush(stdout);
        } else {
            write(1, p + (i & 15), 1);
            fsync(1);
        }
        if(dly) {
            usleep(dly);
        }
    }

    return 0;
}
