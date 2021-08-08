#ifndef ASSERT_H
#define ASSERT_H

#include "terminal.h"

#undef ASSERT

#define ASSERT(c, m)                                                           \
    if (!(c)) {                                                                \
        terminal_write_string("\n");                                           \
        terminal_write_string("ASSERT FAILED: ");                              \
        terminal_write_string(__FILE__);                                       \
        terminal_write_string(": ");                                           \
        terminal_write_string(m);                                              \
        terminal_write_string("\n");                                           \
        for (;;) {                                                             \
            ;                                                                  \
        }                                                                      \
    }

#endif // ASSERT_H
