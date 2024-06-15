#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "object.h"
#include "../native/native.h"
#include "../memory/memory.h"
#include "../table/table.h"
#include "../value/value.h"
#include "../string/string.h"
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


ObjFile* newFile(ObjString* name) {
  ObjFile* file = ALLOCATE_OBJ(ObjFile, OBJ_FILE, vm.fileClass);
  file->name = name;
  file->mode = emptyString();
  file->isOpen = false;
  return file;
}

ObjRecord* newRecord(void* data) {
  ObjRecord* record = ALLOCATE_OBJ(ObjRecord, OBJ_RECORD, NULL);
  record->data = data;
  return record;
}

ObjEntry* newEntry(Value key, Value value) {
  ObjEntry* entry = ALLOCATE_OBJ(ObjEntry, OBJ_ENTRY, vm.entryClass);
  entry->key = key;
  entry->value = value;
  return entry;
}

ObjArray* newArray() {
  ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY, vm.arrayClass);
  initValueArray(&array->elements);
  return array;
}

ObjRange* newRange(int from, int to) {
  ObjRange* range = ALLOCATE_OBJ(ObjRange, OBJ_RANGE, vm.rangeClass);
  range->from = from;
  range->to = to;
  return range;
}

ObjWindow* newWindow(const char* title, int width, int height) {
  ObjWindow* window = ALLOCATE_OBJ(ObjWindow, OBJ_WINDOW, vm.windowClass);
  window->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_HIDDEN);
  window->title = strdup(title);
  window->width = width;
  window->height = height;
  return window;
}

ObjArray* copyArray(ValueArray elements, int fromIndex, int toIndex) {
  ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY, vm.arrayClass);
  initValueArray(&array->elements);
  for (int i = fromIndex; i < toIndex; i++) {
    writeValueArray(&array->elements, elements.values[i]);
  }
  return array;
}

ObjDictionary* newDictionary() {
  ObjDictionary* dict = ALLOCATE_OBJ(ObjDictionary, OBJ_DICTIONARY, vm.dictionaryClass);
  dict->count = 0;
  dict->capacity = 0;
  dict->entries = NULL;
  return dict;
}

ObjClass* newClass(ObjString* name) {
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS, vm.classClass);
  klass->name = name;
  klass->namespace_ = vm.currentNamespace;
  klass->superclass = NULL;
  klass->isNative = false;
  initTable(&klass->methods);
  initTable(&klass->fields);
  return klass;
}

ObjEnum* newEnum(ObjString* name) {
  ObjEnum* enum_ = ALLOCATE_OBJ(ObjEnum, OBJ_ENUM, vm.enumClass);
  enum_->name = name;
  enum_->nextValue = 0;
  initTable(&enum_->values);
  return enum_;
}

ObjModule* newModule(ObjString* path) {
  ObjModule* module = ALLOCATE_OBJ(ObjModule, OBJ_MODULE, NULL);
  module->path = path;
  module->isNative = false;
  module->source = NULL;
  initTable(&module->values);
  tableAddAll(&vm.langNamespace->values, &module->values);
  tableSet(&vm.modules, path, NIL_VAL);
  return module;
}

ObjNamespace* newNamespace(ObjString* shortName, ObjNamespace* enclosing) {
  ObjNamespace* namespace = ALLOCATE_OBJ(ObjNamespace, OBJ_NAMESPACE, vm.namespaceClass);
  namespace->shortName = shortName;
  namespace->enclosing = enclosing;
  namespace->isRoot = false;

  if (namespace->enclosing != NULL && !namespace->enclosing->isRoot) {
    char chars[UINT8_MAX];
    int length = snprintf(chars, UINT8_MAX, "%s::%s", namespace->enclosing->fullName->chars, shortName->chars);
    namespace->fullName = copyString(chars, length);
  }
  else namespace->fullName = namespace->shortName;

  initTable(&namespace->values);
  return namespace;
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

ObjNativeFunction* newNativeFunction(ObjString* name, int arity, NativeFunction function) {
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
    if (IS_STRING(array->elements.values[i])) printf("\"");
    printValue(array->elements.values[i]);
    if (IS_STRING(array->elements.values[i])) printf("\"");
    if (i < array->elements.count - 1) printf(", ");
  }
  printf("]");
}

Value getObjProperty(ObjInstance* object, char* name) {
  Value value;
  tableGet(&object->fields, copyString(name, (int)strlen(name)), &value);
  return value;
}

Value getObjMethod(Value object, char* name) {
  ObjClass* klass = getObjClass(object);
  Value method;
  if (!tableGet(&klass->methods, newString(name), &method)) {
    runtimeError("Method %s::%s does not exist.", klass->name->chars, name);
  }
  return method;
}

bool objMethodExists(Value object, char* name) {
  ObjClass* klass = getObjClass(object);
  Value method;
  if (!tableGet(&klass->methods, newString(name), &method)) {
    return false;
  }
  return true;
}

void setObjProperty(ObjInstance* object, char* name, Value value) {
  tableSet(&object->fields, copyString(name, (int)strlen(name)), value);
}

Value getClassProperty(ObjClass* klass, char* name) {
  Value value;
  tableGet(&klass->fields, newString(name), &value);
  return value;
}

void setClassProperty(ObjClass* klass, char* name, Value value) {
  ObjString* key = newString(name);
  push(OBJ_VAL(key));
  tableSet(&klass->fields, key, ((void*)value == NULL) ? NIL_VAL : value);
  pop();
}

bool isClassExtendingSuperclass(ObjClass* klass, ObjClass* superclass) {
  if (klass == superclass) return true;

  ObjClass* currentClass = klass->superclass;
  while (currentClass != NULL) {
    if (currentClass == superclass) return true;
    currentClass = currentClass->superclass;
  }
  return false;
}

bool isObjInstanceOf(Value value, ObjClass* klass) {
  ObjClass* currentClass = getObjClass(value);
  if (currentClass == klass) return true;
  if (isClassExtendingSuperclass(currentClass->superclass, klass)) return true;
  return false;
}

static void printDictionary(ObjDictionary* dictionary) {
  printf("{");
  int startIndex = 0;
  for (int i = 0; i < dictionary->capacity; i++) {
    ObjEntry* entry = &dictionary->entries[i];
    if (IS_UNDEFINED(entry->key)) continue;
    if (IS_STRING(entry->key)) printf("\"");
    printValue(entry->key);
    if (IS_STRING(entry->key)) printf("\"");
    printf(": ");
    if (IS_STRING(entry->value)) printf("\"");
    printValue(entry->value);
    if (IS_STRING(entry->value)) printf("\"");
    startIndex = i + 1;
    break;
  }

  for (int i = startIndex; i < dictionary->capacity; i++) {
    ObjEntry* entry = &dictionary->entries[i];
    if (IS_UNDEFINED(entry->key)) continue;
    printf(", ");
    if (IS_STRING(entry->key)) printf("\"");
    printValue(entry->key);
    if (IS_STRING(entry->key)) printf("\"");
    printf(": ");
    if (IS_STRING(entry->value)) printf("\"");
    printValue(entry->value);
    if (IS_STRING(entry->value)) printf("\"");
  }
  printf("}");
}

static void printFunction(ObjFunction* function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  } else if (function->name->length == 0) {
    printf("<function>");
  } else printf("<function %s>", function->name->chars);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_ARRAY:
      printArray(AS_ARRAY(value));
      break;
    case OBJ_NAMESPACE:
      printf("<namespace %s>", AS_NAMESPACE(value)->fullName->chars);
      break;
    case OBJ_MODULE:
      printf("<module %s>", AS_MODULE(value)->path->chars);
      break;
    case OBJ_RANGE:
      printf("%d...%d", AS_RANGE(value)->from, AS_RANGE(value)->to);
      break;
    case OBJ_WINDOW:
      printf("<%s window>", AS_WINDOW(value)->title);
      break;
    case OBJ_FILE:
      printf("<file \"%s\">", AS_FILE(value)->name->chars);
      break;
    case OBJ_RECORD:
      printf("record");
      break;
    case OBJ_DICTIONARY: 
      printDictionary(AS_DICTIONARY(value));
      break;
    case OBJ_BOUND_METHOD:
      printFunction(AS_BOUND_METHOD(value)->method->function);
      break;
    case OBJ_CLASS: {
      ObjClass* klass = AS_CLASS(value);
      if (klass->namespace_->isRoot) printf("<class %s>", klass->name->chars);
      else printf("<class %s::%s>", klass->namespace_->fullName->chars, klass->name->chars);
      break;
    }
    case OBJ_CLOSURE:
      printFunction(AS_CLOSURE(value)->function);
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_ENUM:
      printf("<enum %s>", AS_ENUM(value)->name->chars);
      break;
    case OBJ_INSTANCE:
      #ifdef DEBUG_FORMAT
      if (objMethodExists(value, "__format__")) {
        Value method = getObjMethod(value, "__format__");
        Value str = callReentrant(value, method);
        do {
          Value toStringMethod = getObjMethod(str, "__str__");
          str = callReentrant(str, toStringMethod);
        } while (!IS_STRING(str));
        printf("%s", AS_CSTRING(str));
      } else 
      #endif
      printf("<object %s>", AS_OBJ(value)->klass->name->chars);
      break;
    case OBJ_NATIVE_FUNCTION:
      printf("<native function %s>", AS_NATIVE_FUNCTION(value)->name->chars);
      break;
    case OBJ_NATIVE_METHOD:
      printf("<native method %s::%s>", AS_NATIVE_METHOD(value)->klass->name->chars, AS_NATIVE_METHOD(value)->name->chars);
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
