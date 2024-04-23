#ifndef cluminique_vm_h
#define cluminique_vm_h

#include "../object/object.h"
#include "../table/table.h"
#include "../value/value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  uint16_t handlerAddress;
  uint16_t finallyAddress;
  ObjClass* exceptionClass;
} ExceptionHandler;

typedef struct {
  ObjFunction* function;
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
  uint8_t handlerCount;
  ExceptionHandler handlerStack[UINT4_MAX];
} CallFrame;

typedef struct {
  ObjClass* objectClass;
  ObjClass* classClass;
  ObjClass* functionClass;
  ObjClass* namespaceClass;
  ObjClass* methodClass;
  ObjClass* nilClass;
  ObjClass* boolClass;
  ObjClass* intClass;
  ObjClass* numberClass;
  ObjClass* floatClass;
  ObjClass* exceptionClass;
  ObjClass* fileClass;
  ObjClass* stringClass;
  ObjClass* arrayClass;
  ObjClass* dictionaryClass;
  ObjClass* entryClass;

  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Value stack[STACK_MAX];
  Value* stackTop;
  Table globalValues;
  Table globals;
  Table strings;
  ObjString* initString;
  ObjUpvalue* openUpvalues;

  size_t bytesAllocated;
  size_t nextGC;
  Obj* objects;
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

InterpretResult run();

void initVM();
void freeVM();

Value callReentrant(Value receiver, Value callee, ...);
bool isFalsey(Value value);
void bindSuperclass(ObjClass* subclass, ObjClass* superclass);
bool loadGlobal(ObjString* name, Value* value);
InterpretResult interpret(const char* source);
void runtimeError(const char* format, ...);
bool callClosure(ObjClosure* closure, int argCount);
ObjArray* getStackTrace();
ObjInstance* throwException(ObjClass* exceptionClass, const char* format, ...);
void push(Value value);
Value pop();

#endif
