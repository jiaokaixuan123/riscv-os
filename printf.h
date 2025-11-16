#ifndef PRINTF_H
#define PRINTF_H

void printf(const char *fmt, ...);
void panic(const char*) __attribute__((noreturn));

#ifdef DEBUG
#define dprint(...) printf(__VA_ARGS__)
#else
#define dprint(...)
#endif

#endif