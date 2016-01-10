#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

//#define DEBUG

#ifdef DEBUG
#define LOG(format, ...)  fprintf(stderr, ">> "format"\n", ##__VA_ARGS__)
#else
#define LOG(format, ...)
#endif

#endif /* __LOG_H__ */
