#include "util.h"

#include <stdio.h>
#include <stdlib.h>

_Noreturn void fatal_error(char* error) {
    puts(RED_BACK);
    printf("%s Terminating!", error);
    puts(RESET);
#ifdef DEBUG 
    fflush(stdout);
    abort();
#else
    exit(EXIT_FAILURE);
#endif
}
// EOF