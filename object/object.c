#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "object.h"
#include "../hash/hash.h"
#include "../memory/memory.h"
#include "../table/table.h"
#include "../value/value.h"
#include "../vm/vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->isMarked = false;

  object->next = vm.objects;

// #ifdef DEBUG_LOG_GC
//  printf("%p allocate %zu for %d\n", (void*)object, size, type);
// #endif

  vm.objects = object;
  return object;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

ObjArray* newArray() {
  ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
  initValueArray(&array->elements);
  return array;
}

ObjClass* newClass(ObjString* name) {
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  klass->name = name;
  klass->superclass = NULL;
  klass->isNative = false;
  initTable(&klass->methods);
  return klass;
}

ObjClosure* newClosure(ObjFunction* function) {
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}

ObjInstance* newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->klass = klass;
  initTable(&instance->fields);
  return instance;
}

ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

ObjNativeMethod* newNativeMethod(NativeMethod method) {
  ObjNativeMethod* nativeMethod = ALLOCATE_OBJ(ObjNativeMethod, OBJ_NATIVE_METHOD);
  nativeMethod->method = method;
  return nativeMethod;
}

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  push(OBJ_VAL(string));
  tableSet(&vm.strings, string, NIL_VAL);
  pop();

  return string;
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

ObjString* formattedString(const char* format, ...) {
  char chars[UINT8_MAX];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(chars, UINT8_MAX, format, args);
  va_end(args);
  return copyString(chars, length);
}

ObjString* formattedLongString(const char* format, ...) {
  char chars[UINT16_MAX];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(chars, UINT16_MAX, format, args);
  va_end(args);
  return copyString(chars, length);
}


char* createFormattedString(const char* format, ...) {
  va_list args;
  va_start(args, format);
  size_t len = vsnprintf(NULL, 0, format, args);
  va_end(args);
  char* result = ALLOCATE(char, len + 1);

  va_start(args, format);
  vsnprintf(result, len + 1, format, args);
  va_end(args);

  return result;
}

ObjString* copyFormattedString(const char* format, ...) {
  va_list args;
  va_start(args, format);
  size_t len = vsnprintf(NULL, 0, format, args);
  va_end(args);

  char* heapChars = ALLOCATE(char, len + 1);

  va_start(args, format);
  vsnprintf(heapChars, len + 1, format, args);
  va_end(args);
  uint32_t hash = hashString(heapChars, len);
  ObjString* interned = tableFindString(&vm.strings, heapChars, len, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, heapChars, len + 1);
    return interned;
  }

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

ObjArray* copyArray(ValueArray elements) {
    ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY);
    initValueArray(&array->elements);
    for (int i = 0; i < elements.count; i++) {
        writeValueArray(&array->elements, elements.values[i]);
    }
    return array;
}

ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static void printArray(ObjArray* array) {
  printf("[");
  for (int i = 0; i < array->elements.count; i++) {
    printValue(array->elements.values[i]);
    if (i < array->elements.count - 1) printf(", ");
  }
  printf("]");
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
    case OBJ_ARRAY:
      printArray(AS_ARRAY(value));
      break;
    case OBJ_BOUND_METHOD:
      printFunction(AS_BOUND_METHOD(value)->method->function);
      break;
    case OBJ_CLASS:
      printf("%s class", AS_CLASS(value)->name->chars);
      break;
    case OBJ_CLOSURE:
      printFunction(AS_CLOSURE(value)->function);
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_INSTANCE:
      printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_NATIVE_METHOD:
      printf("<native method>");
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
  }
}

ObjClass* getObjClass(Value value) {
  if (IS_BOOL(value)) return vm.boolClass;
  else if (IS_NIL(value)) return vm.nilClass;
  else if (IS_NUMBER(value)) return vm.numberClass;
  else if (IS_OBJ(value)) return AS_INSTANCE(value)->klass;
  else return NULL;
}
