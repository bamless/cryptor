#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>

//prints a message to standard output
#define logs(msg) printf("%s\n", msg);
#define logsf(format, ...) printf(format, __VA_ARGS__)

//prints a message to standard error
#define elog(msg) fprintf(stderr, "%s\n", msg)
#define elogf(format, ...) fprintf(stderr, format, __VA_ARGS__)

//prints a message to standard output only if DEBUG is defined
#ifdef DEBUG
#define dlog(msg) logs(msg)
#define dlogf(format, ...) logf(format, __VA_ARGS__)
#else
#define dlog(msg)
#define dlogf(format, ...)
#endif

#endif
