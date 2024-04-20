#ifndef cluminique_common_h
#define cluminique_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// #define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_EXECUTION

// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

#define UINT8_COUNT (UINT8_MAX + 1)
#define UINT4_MAX 15
#define UINT4_COUNT (UINT4_MAX + 1)

char* readFile(const char* path);

#endif
