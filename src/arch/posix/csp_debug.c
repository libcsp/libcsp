#include <stdio.h>
#include <stdarg.h>
#include <csp/csp.h>

inline void csp_debug(const char * format, ...) {
#ifdef CSP_DEBUG
    va_list args;
    printf("CSP: ");
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#endif
}
