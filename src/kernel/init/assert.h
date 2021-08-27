#ifndef ASSERT_H
#define ASSERT_H

#include "terminal.h"

#undef ASSERT

#define ASSERT(c, m)                                                           \
    if (!(c)) {                                                                \
        terminal_printf("\n");                                                 \
        terminal_printf("ASSERT FAILED: %s: %s\n", __FILE__, m);               \
        for (;;) {                                                             \
            ;                                                                  \
        }                                                                      \
    }

#endif // ASSERT_H
