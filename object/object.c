#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "object.h"
#include "../hash/hash.h"
#include "../memory/memory.h"
#include "../table/table.h"
#include "../value/value.h"
#include "../vm/vm.h"

#define ALLOCATE_OBJ(type, objectType, objectClass) (type*)allocateObject(sizeof(type), objectType, objectClass)

Obj* allocateObject(size_t size, ObjType type, ObjClass* klass) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->klass = klass;
  object->isMarked = false;

  object->next = vm.objects;

// #ifdef DEBUG_LOG_GC
//  printf("%p allocate %zu for %d\n", (void*)object, size, type);
// #endif

  vm.objects = object;
  return object;
}

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD, vm.methodClass);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

ObjArray* newArray() {
  ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY, vm.arrayClass);
  initValueArray(&array->elements);
  return array;
}

ObjClass* newClass(ObjString* name) {
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS, vm.classClass);
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

  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE, vm.functionClass);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION, NULL);
  function->arity = 0;
  function->upvalueCount = 0;
  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}

ObjInstance* newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE, klass);
  initTable(&instance->fields);
  return instance;
}

ObjNativeFunction* newNativeFunction(ObjString* name, int arity, NativeFn function) {
  ObjNativeFunction* nativeFunction = ALLOCATE_OBJ(ObjNativeFunction, OBJ_NATIVE_FUNCTION, vm.functionClass);
  nativeFunction->name = name;
  nativeFunction->arity = arity;
  nativeFunction->function = function;
  return nativeFunction;
}

ObjNativeMethod* newNativeMethod(ObjClass* klass, ObjString* name, int arity, NativeMethod method) {
  ObjNativeMethod* nativeMethod = ALLOCATE_OBJ(ObjNativeMethod, OBJ_NATIVE_METHOD, NULL);
  nativeMethod->klass = klass;
  nativeMethod->name = name;
  nativeMethod->arity = arity;
  nativeMethod->method = method;
  return nativeMethod;
}

ObjArray* copyArray(ValueArray elements, int fromIndex, int toIndex) {
  ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY, vm.arrayClass);
  initValueArray(&array->elements);
  for (int i = fromIndex; i < toIndex; i++) {
    writeValueArray(&array->elements, elements.values[i]);
  }
  return array;
}

ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE, NULL);
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
      printf("<object %s>", AS_OBJ(value)->klass->name->chars);
      break;
    case OBJ_NATIVE_FUNCTION:
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
  else if (IS_INT(value)) return vm.intClass;
  else if (IS_FLOAT(value)) return vm.numberClass;
  else if (IS_OBJ(value)) return AS_OBJ(value)->klass;
  else return NULL;
}
