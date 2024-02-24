#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../memory/memory.h"
#include "object.h"
#include "../table/table.h"
#include "../value/value.h"
#include "../vm/vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;

  object->next = vm.objects;
  vm.objects = object;
  return object;
}

ObjClosure* newClosure(ObjFunction* function) {
  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  return closure;
}

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}

ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;
  tableSet(&vm.strings, string, NIL_VAL);
  return string;
}

static uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString* takeString(char* chars, int length) {
  uint32_t hash = hashString(chars, length);

  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return allocateString(chars, length, hash);
}

char* createFormattedString(const char* format, ...) {
  va_list args;
  va_start(args, format);
  // Determine the length of the formatted string
  size_t len = vsnprintf(NULL, 0, format, args);
  va_end(args);

  // Allocate memory for the formatted string
  char* result = ALLOCATE(char, len + 1);

  va_start(args, format);
  // Create the formatted string
  vsnprintf(result, len + 1, format, args);
  va_end(args);

  return result;
}

// Updated copyString function to take a formatted string
ObjString* copyFormattedString(const char* format, ...) {
  va_list args;
  va_start(args, format);
  // Determine the length of the formatted string
  size_t len = vsnprintf(NULL, 0, format, args);
  va_end(args);

  // Allocate memory for the formatted string
  char* heapChars = ALLOCATE(char, len + 1);

  va_start(args, format);
  // Create the formatted string
  vsnprintf(heapChars, len + 1, format, args);
  va_end(args);

  // Calculate the hash of the formatted string
  uint32_t hash = hashString(heapChars, len);

  // Check if the string is already interned
  ObjString* interned = tableFindString(&vm.strings, heapChars, len, hash);
  if (interned != NULL) {
    // String is already interned, free the memory allocated for the formatted string
    FREE_ARRAY(char, heapChars, len + 1);
    return interned;
  }

  // String is not interned, create a new string
  return allocateString(heapChars, len, hash);
}

ObjString* copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);


  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;

  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length, hash);
}

static void printFunction(ObjFunction* function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_CLOSURE:
      printFunction(AS_CLOSURE(value)->function);
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}
