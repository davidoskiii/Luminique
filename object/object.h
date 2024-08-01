#ifndef cluminique_object_h
#define cluminique_object_h

#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <uv.h>

typedef struct CallFrame CallFrame;

#include "../common.h"
#include "../table/table.h"
#include "../promise/promise.h"
#include "../chunk/chunk.h"
#include "../value/value.h"
#include "../generator/generator.h"
#include "../exception/exception.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define OBJ_KLASS(value) (AS_OBJ(value)->klass)

#define ALLOCATE_OBJ(type, objectType, objectClass) (type*)allocateObject(sizeof(type), objectType, objectClass)
#define ALLOCATE_CLASS(classClass) ALLOCATE_OBJ(ObjClass, OBJ_CLASS, classClass)
#define ALLOCATE_CLOSURE(closureClass) ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE, closureClass)
#define ALLOCATE_NAMESPACE(namespaceClass) ALLOCATE_OBJ(ObjNamespace, OBJ_NAMESPACE, namespaceClass)
#define ALLOCATE_STRING(length, stringClass) (ObjString*)allocateObject(sizeof(ObjString) + length + 1, OBJ_STRING, stringClass)

#define IS_NAMESPACE(value) isObjType(value, OBJ_NAMESPACE)
#define IS_MODULE(value) isObjType(value, OBJ_MODULE)
#define IS_PROMISE(value) isObjType(value, OBJ_PROMISE)
#define IS_ARRAY(value) isObjType(value, OBJ_ARRAY)
#define IS_DICTIONARY(value) isObjType(value, OBJ_DICTIONARY)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_METHOD(value) isObjType(value, OBJ_METHOD)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_FRAME(value) isObjType(value, OBJ_FRAME);
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_GENERATOR(value) isObjType(value, OBJ_GENERATOR)
#define IS_EXCEPTION(value) isObjType(value, OBJ_EXCEPTION)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_FILE(value) isObjType(value, OBJ_FILE)
#define IS_RECORD(value) isObjType(value, OBJ_RECORD)
#define IS_ARRAY(value) isObjType(value, OBJ_ARRAY)
#define IS_RANGE(value) isObjType(value, OBJ_RANGE)
#define IS_NODE(value) isObjType(value, OBJ_NODE)
#define IS_ENUM(value) isObjType(value, OBJ_ENUM)
#define IS_WINDOW(value) isObjType(value, OBJ_WINDOW)
#define IS_TIMER(value) isObjType(value, OBJ_TIMER)
#define IS_NATIVE_FUNCTION(value) isObjType(value, OBJ_NATIVE_FUNCTION)
#define IS_NATIVE_METHOD(value) isObjType(value, OBJ_NATIVE_METHOD)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_NAMESPACE(value) ((ObjNamespace*)AS_OBJ(value))
#define AS_MODULE(value) ((ObjModule*)AS_OBJ(value))
#define AS_PROMISE(value) ((ObjPromise*)AS_OBJ(value))
#define AS_ARRAY(value) ((ObjArray*)AS_OBJ(value))
#define AS_DICTIONARY(value) ((ObjDictionary*)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_METHOD(value) ((ObjMethod*)AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
#define AS_FRAME(value) ((ObjFrame*)AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_GENERATOR(value) ((ObjGenerator*)AS_OBJ(value))
#define AS_EXCEPTION(value) ((ObjException*)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance*)AS_OBJ(value))
#define AS_FILE(value) ((ObjFile*)AS_OBJ(value))
#define AS_RECORD(value) ((ObjRecord*)AS_OBJ(value))
#define AS_ARRAY(value) ((ObjArray*)AS_OBJ(value))
#define AS_RANGE(value) ((ObjRange*)AS_OBJ(value))
#define AS_NODE(value) ((ObjNode*)AS_OBJ(value))
#define AS_ENUM(value) ((ObjEnum*)AS_OBJ(value))
#define AS_WINDOW(value) ((ObjWindow*)AS_OBJ(value))
#define AS_TIMER(value) ((ObjTimer*)AS_OBJ(value))
#define AS_NATIVE_FUNCTION(value) ((ObjNativeFunction*)AS_OBJ(value))
#define AS_NATIVE_METHOD(value) ((ObjNativeMethod*)AS_OBJ(value))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))

#define AS_CARRAY(value) (((ObjArray*)AS_OBJ(value))->elements)
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_NAMESPACE,
  OBJ_MODULE,
  OBJ_PROMISE,
  OBJ_BOUND_METHOD,
  OBJ_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FRAME,
  OBJ_FUNCTION,
  OBJ_GENERATOR,
  OBJ_EXCEPTION,
  OBJ_INSTANCE,
  OBJ_FILE,
  OBJ_RECORD,
  OBJ_ENUM,
  OBJ_ARRAY,
  OBJ_TIMER,
  OBJ_ENTRY,
  OBJ_DICTIONARY,
  OBJ_NATIVE_FUNCTION,
  OBJ_NATIVE_METHOD,
  OBJ_STRING,
  OBJ_RANGE,
  OBJ_NODE,
  OBJ_WINDOW,
  OBJ_UPVALUE
} ObjType;

struct Obj {
  ObjType type;
  ObjClass* klass;
  bool isMarked;
  struct Obj* next;
};

struct ObjFunction {
  Obj obj;
  int arity;
  int upvalueCount;
  bool isGenerator;
  bool isAsync;
  Chunk chunk;
  ObjString* name;
};

typedef Value (*NativeFunction)(int argCount, Value* args);
typedef Value (*NativeMethod)(Value receiver, int argCount, Value* args);

struct ObjNativeFunction {
  Obj obj;
  ObjString* name;
  int arity;
  NativeFunction function;
};

struct ObjNativeMethod {
  Obj obj;
  ObjClass* klass;
  ObjString* name;
  int arity;
  NativeMethod method;
};

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
  ObjClass* behavior;
  ObjClosure* closure;
} ObjMethod;

struct ObjFile {
  Obj obj;
  ObjString* name;
  ObjString* mode;
  bool isOpen;
  size_t offset;
  uv_fs_t* fsStat;
  uv_fs_t* fsOpen;
  uv_fs_t* fsRead;
  uv_fs_t* fsWrite;
};

typedef struct ObjFrame {
  Obj obj;
  ObjClosure* closure;
  uint8_t* ip;
  Value slots[UINT8_MAX];
  uint8_t slotCount;
  uint8_t handlerCount;
  ExceptionHandler handlerStack[UINT4_MAX];
} ObjFrame;

struct ObjArray {
  Obj obj;
  ValueArray elements;
};

typedef struct ObjGenerator {
  Obj obj;
  ObjFrame* frame;
  struct ObjGenerator* outer;
  struct ObjGenerator* inner;
  GeneratorState state;
  Value value;
} ObjGenerator;

struct ObjException {
  Obj obj;
  ObjString* message;
  ObjArray* stacktrace;
};

struct ObjModule {
  Obj obj;
  ObjString* path;
  ObjClosure* closure;
  bool isNative;
  Table values;
  char* source;
};

struct ObjPromise {
  Obj obj;
  int id;
  PromiseState state;
  Value value;
  ObjDictionary* captures;
  ObjException* exception;
  Value executor;
  ValueArray handlers;
  Value onCatch;
  Value onFinally;
};

typedef struct {
  Obj obj;
  uv_timer_t* timer;
  int id;
  bool isRunning;
} ObjTimer;

struct ObjNamespace {
  Obj obj;
  ObjString* shortName;
  ObjString* fullName;
  struct ObjNamespace* enclosing;
  bool isRoot;
  Table values;
  Table compilerValues;
  Table globals;
  Table compilerGlobals;
};

typedef struct ObjRecord {
  Obj obj;
  void* data;
} ObjRecord;

typedef struct ObjEntry {
  Obj obj;
  Value key;
  Value value;
} ObjEntry;

struct ObjClosure {
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalueCount;
};

struct ObjClass {
  Obj obj;
  ObjString* name;

  ObjType classType;

  struct ObjNamespace* namespace_;
  struct ObjClass* superclass;

  uint16_t interceptors;

  Table methods;
  Table fields;
  Table getters;
  Table setters;

  bool isNative;
};

struct ObjEnum {
  Obj obj;
  ObjString* name;
  int nextValue;
  Table values;
};

struct ObjInstance {
  Obj obj;
  Table fields;
};

typedef struct {
  Obj obj;
  Value receiver;
  Value method;
  bool isNative;
} ObjBoundMethod;

struct ObjDictionary {
  Obj obj;
  int capacity;
  int count;
  ObjEntry* entries;
};

typedef struct {
  Obj obj;
  int from;
  int to;
} ObjRange;

struct ObjNode {
  Obj obj;
  Value element;
  struct ObjNode* prev;
  struct ObjNode* next;
};

typedef struct {
  Obj obj;
  SDL_Window* window;
  char* title;
  int width;
  int height;
} ObjWindow;

Obj* allocateObject(size_t size, ObjType type, ObjClass* klass);
ObjBoundMethod* newBoundMethod(Value receiver, Value method);
ObjFrame* newFrame(CallFrame* callFrame);
ObjFile* newFile(ObjString* name);
ObjGenerator* newGenerator(ObjFrame* frame, ObjGenerator* outer);
ObjException* newException(ObjString* message, ObjClass* klass);
ObjRecord* newRecord(void* data);
ObjEntry* newEntry(Value key, Value value);
ObjArray* newArray();
ObjArray* copyArray(ValueArray elements, int fromIndex, int toIndex);
ObjDictionary* newDictionary();
ObjClass* newClass(ObjString* name, ObjType classType);
ObjEnum* newEnum(ObjString* name);
ObjModule* newModule(ObjString* path);
ObjPromise* newPromise(PromiseState state, Value value, Value executor);
ObjTimer* newTimer(ObjClosure* closure, int delay, int interval);
ObjNamespace* newNamespace(ObjString* shortName, ObjNamespace* enclosing);
ObjRange* newRange(int from, int to);
ObjNode* newNode(Value element, ObjNode* prev, ObjNode* next);
ObjWindow* newWindow(const char* title, int width, int height);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjMethod* newMethod(ObjClass* behavior, ObjClosure* closure);
ObjInstance* newInstance(ObjClass* klass);
ObjNativeFunction* newNativeFunction(ObjString* name, int arity, NativeFunction function);
ObjNativeMethod* newNativeMethod(ObjClass* klass, ObjString* name, int arity, NativeMethod method);
ObjUpvalue* newUpvalue(Value* slot);
void printObject(Value value);
bool objMethodExists(Value object, char* name);
ObjClass* getObjClass(Value value);
Value getClassProperty(ObjClass* klass, char* name);
void setClassProperty(ObjClass* klass, char* name, Value value);
Value getObjProperty(ObjInstance* object, char* name);
Value getObjMethod(Value object, char* name);
void setObjProperty(ObjInstance* object, char* name, Value value);
bool isObjInstanceOf(Value value, ObjClass* klass);
bool isClassExtendingSuperclass(ObjClass* klass, ObjClass* superclass);

static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif
