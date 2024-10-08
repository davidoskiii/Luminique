#include <string.h>

#include "hash.h"
#include "../memory/memory.h"

static inline uint32_t hash64To32Bits(uint64_t hash) {
  hash = ~hash + (hash << 18);
  hash = hash ^ (hash >> 31);
  hash = hash * 21;
  hash = hash ^ (hash >> 11);
  hash = hash + (hash << 6);
  hash = hash ^ (hash >> 22);
  return (uint32_t)(hash & 0x3fffffff);
}

uint32_t hashNumber(double num) {
  return hash64To32Bits((uint64_t)num);
}

uint32_t hashString(const char* chars, int length) {
	uint32_t hash = 2166136261u;
	for (int i = 0; i < length; i++) {
		hash ^= (uint8_t)chars[i];
		hash *= 16777619;
	}
	return hash;
}

uint32_t* hashStringArray(const char* chars[], int arity) {
  uint32_t* hash = ALLOCATE(uint32_t, arity);
  for (int i = 0; i < arity; i++) {
    hash[i] = hashString(chars[i], strlen(chars[i]));
  }
  return hash;
}

uint32_t hashObject(Obj* object) {
  switch (object->type) {
    case OBJ_STRING:
      return ((ObjString*)object)->hash;
    case OBJ_CLASS:
      return ((ObjClass*)object)->name->hash;    
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      int hash = 7;
      hash = hash * 31 + hashNumber(function->arity);
      hash = hash * 31 + hashNumber(function->chunk.count);
      return hash;
    }
    case OBJ_ARRAY: { 
      ObjArray* array = (ObjArray*)object;
      int hash = 7;
      for (int i = 0; i < array->elements.count; i++) {
        hash = hash * 31 + hashValue(array->elements.values[i]);
      }
      return hash;
    }
    case OBJ_DICTIONARY: { 
      ObjDictionary* dict = (ObjDictionary*)object;
      int hash = 7;
      for (int i = 0; i < dict->capacity; i++) {
        ObjEntry* entry = &dict->entries[i];
        if (IS_UNDEFINED(entry->key)) continue;
        hash = hash * 31 + hashValue(entry->key);
        hash = hash * 31 + hashValue(entry->value);
      }
      return hash;
    }
    case OBJ_ENTRY: { 
      ObjEntry* entry = (ObjEntry*)object;
      int hash = 7;
      hash = hash * 31 + hashValue(entry->key);
      hash = hash * 31 + hashValue(entry->value);
      return hash;
    }
    case OBJ_INSTANCE: { 
      ObjInstance* instance = (ObjInstance*)object;
      int hash = 7;
      for (int i = 0; i < instance->fields.capacity; i++) {
        Entry* fields = &instance->fields.entries[i];
        if (fields->key == NULL) continue;
        hash = hash * 31 + hashString(fields->key->chars, fields->key->length);
        hash = hash * 31 + hashValue(fields->value);
      }
      return hash;
    }
    default: {
      uint64_t hash = (uint64_t)(&object);
      return hash64To32Bits(hash);
    }
  }
}

uint32_t hashValue(Value value) {
  if (IS_OBJ(value)) return hashObject(AS_OBJ(value));
  return hash64To32Bits(value);
}
