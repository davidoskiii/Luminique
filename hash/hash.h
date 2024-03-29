#ifndef cluminique_hash_h
#define cluminique_hash_h

#include "../object/object.h"
#include "../common.h"
#include "../value/value.h"

uint32_t hashString(const char* chars, int length);
uint32_t hashObject(Obj* object);
uint32_t hashValue(Value value);

static inline uint32_t hash64To32Bits(uint64_t hash) {
  hash = ~hash + (hash << 18);
  hash = hash ^ (hash >> 31);
  hash = hash * 21;
  hash = hash ^ (hash >> 11);
  hash = hash + (hash << 6);
  hash = hash ^ (hash >> 22);
  return (uint32_t)(hash & 0x3fffffff);
}

static inline uint32_t hashNumber(double num) {
  return hash64To32Bits((uint64_t)num);
}

#endif
