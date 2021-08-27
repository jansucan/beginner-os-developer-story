#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdarg.h>
#include <stdint.h>

void terminal_initialize(void);
void terminal_printf(const char *const format, ...);

#endif
