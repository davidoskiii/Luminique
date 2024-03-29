#include <string.h>

#include "hash.h"

uint32_t hashString(const char* chars, int length) {
	uint32_t hash = 2166136261u;
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)chars[i];
		hash *= 16777619;
	}
	return hash;
}

uint32_t hashObject(Obj* object) {
	switch (object->type) {
	    case OBJ_STRING:
			return ((ObjString*)object)->hash;
		default: {
			uint64_t hash = (uint64_t)(&object);
			return hash64To32Bits(hash);
		}
	}
}

uint32_t hashValue(Value value) {
  if (IS_BOOL(value)) {
    return hash64To32Bits((uint64_t)value.as.boolean);
  } else if (IS_OBJ(value)) {
    return hashObject(AS_OBJ(value));
  } else {
    double num = value.as.number;
    return hash64To32Bits((uint64_t)num);
  }
}
