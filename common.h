#ifndef cluminique_common_h
#define cluminique_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define NAN_BOXING
#define DEBUG_FORMAT
// #define DEBUG_LOCAL_SLOT
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC

#define DICT_PARAM_VALUE ((uint16_t) 0x8000)
#define ARRAY_PARAM_VALUE ((uint16_t) 0x4000)

#define UINT8_COUNT (UINT8_MAX + 1)
#define UINT4_MAX 15
#define UINT4_COUNT (UINT4_MAX + 1)

typedef struct VM VM;

char* readFile(const char* path);

#endif
