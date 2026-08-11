#ifndef CACHE_CHECK_H
#define CACHE_CHECK_H

#include <stdint.h>

uint64_t read_tsc(void);
void test_cache_speed(void);

#endif
