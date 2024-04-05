#ifndef cluminique_object_h
#define cluminique_object_h

#include "../common.h"
#include "../table/table.h"
#include "../chunk/chunk.h"
#include "../value/value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define OBJ_KLASS(value) (AS_OBJ(value)->klass)

#define IS_ARRAY(value) isObjType(value, OBJ_ARRAY)
#define IS_DICTIONARY(value) isObjType(value, OBJ_DICTIONARY)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_ARRAY(value) isObjType(value, OBJ_ARRAY)
#define IS_NATIVE_FUNCTION(value) isObjType(value, OBJ_NATIVE_FUNCTION)
#define IS_NATIVE_METHOD(value) isObjType(value, OBJ_NATIVE_METHOD)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_ARRAY(value) ((ObjArray*)AS_OBJ(value))
#define AS_DICTIONARY(value) ((ObjDictionary*)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))
#define AS_ARRAY(value) ((ObjArray*)AS_OBJ(value))
#define AS_NATIVE_FUNCTION(value) ((ObjNativeFunction*)AS_OBJ(value))
#define AS_NATIVE_METHOD(value) ((ObjNativeMethod*)AS_OBJ(value))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))

#define AS_CARRAY(value) (((ObjArray*)AS_OBJ(value))->elements)
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_ARRAY,
  OBJ_DICTIONARY,
  OBJ_NATIVE_FUNCTION,
  OBJ_NATIVE_METHOD,
  OBJ_STRING,
  OBJ_UPVALUE
} ObjType;

struct Obj {
  ObjType type;
  ObjClass* klass;
  bool isMarked;
  struct Obj* next;
};

typedef struct {
  Obj obj;
  int arity;
  int upvalueCount;
  Chunk chunk;
  ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);
typedef Value (*NativeMethod)(Value receiver, int argCount, Value* args);
typedef Value (*NativeInstance)(Value receiver, int argCount, Value* args);

typedef struct {
  Obj obj;
  ObjString* name;
  int arity;
  NativeFn function;
} ObjNativeFunction;

typedef struct {
  Obj obj;
  ObjClass* klass;
  ObjString* name;
  int arity;
  NativeMethod method;
} ObjNativeMethod;

typedef struct ObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
} ObjString;

typedef struct ObjUpvalue {
  Obj obj;
  Value* location;
  Value closed;
  struct ObjUpvalue* next;
} ObjUpvalue;

typedef struct {
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalueCount;
} ObjClosure;

struct ObjClass {
  Obj obj;
  ObjString* name;
  struct ObjClass* superclass;
  Table methods;
  bool isNative;
};

typedef struct {
  Obj obj;
  Table fields;
} ObjInstance;

typedef struct {
  Obj obj;
  NativeInstance instance;
} ObjNativeInstance;

typedef struct {
  Obj obj;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

typedef struct ObjArray {
  Obj obj;
  ValueArray elements;
} ObjArray;

typedef struct ObjDictionary {
  Obj obj;
  Table table;
} ObjDictionary;

Obj* allocateObject(size_t size, ObjType type, ObjClass* klass);
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);
ObjArray* newArray();
ObjArray* copyArray(ValueArray elements, int fromIndex, int toIndex);
ObjDictionary* newDictionary();
ObjDictionary* copyDictionary(Table table);
ObjClass* newClass(ObjString* name);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjInstance* newInstance(ObjClass* klass);
ObjNativeFunction* newNativeFunction(ObjString* name, int arity, NativeFn function);
ObjNativeMethod* newNativeMethod(ObjClass* klass, ObjString* name, int arity, NativeMethod method);
ObjUpvalue* newUpvalue(Value* slot);
void printObject(Value value);
ObjClass* getObjClass(Value value);
Value getObjProperty(ObjInstance* object, char* name);
void setObjProperty(ObjInstance* object, char* name, Value value);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
