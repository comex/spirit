#include <stdio.h>
#include <stdarg.h>
int asl_log(void *a, void *b, int c, char *d, char *e) {
    printf("! %s", e);
}
int mobdevlog(int level, char *from, char *args, ...) {
    va_list ap;
    va_start(ap, args);
    printf("[%d] %s: ", level, from);
    vprintf(args, ap);
    va_end(ap);
}
