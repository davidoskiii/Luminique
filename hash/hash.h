#ifndef cluminique_hash_h
#define cluminique_hash_h

#include "../object/object.h"
#include "../common.h"
#include "../value/value.h"

uint32_t hashString(const char* chars, int length);
uint32_t hashObject(Obj* object);
uint32_t hashValue(Value value);

#endif
