#ifndef cluminique_vm_h
#define cluminique_vm_h

#include "../table/table.h"
#include "../value/value.h"
#include "../object/object.h"

#define FRAMES_MAX 512 / 2 
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct CallFrame {
  ObjFunction* function;
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
  uint8_t handlerCount;
  ExceptionHandler handlerStack[UINT4_MAX];
} CallFrame;

struct VM {
  ObjClass* objectClass;
  ObjClass* enumClass;
  ObjClass* classClass;
  ObjClass* functionClass;
  ObjClass* generatorClass;
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
  ObjClass* rangeClass;
  ObjClass* nodeClass;
  ObjClass* windowClass;
  ObjClass* promiseClass;
  ObjClass* timerClass;

  ObjNamespace* rootNamespace;
  ObjNamespace* luminiqueNamespace;
  ObjNamespace* stdNamespace;
  ObjNamespace* langNamespace;
  ObjNamespace* currentNamespace;
  ObjNamespace* previousNamespace;
  bool runModule;

  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Value stack[STACK_MAX];
  Value* stackTop;
  int apiStackDepth;
  ObjGenerator* runningGenerator;
  int argc;
  char** argv;
  bool repl;

  ObjModule* currentModule;

  Table modules;
  Table namespaces;
  Table strings;

  ObjString* initString;
  ObjUpvalue* openUpvalues;
  int promiseCount;

  size_t bytesAllocated;
  size_t nextGC;
  Obj* objects;
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
};

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

InterpretResult run();

void initVM(int argc, char** argv);
void freeVM();

Value callReentrantFunction(Value callee, ...);
Value callReentrantMethod(Value receiver, Value callee, ...);
bool callMethod(Value method, int argCount);
bool isFalsey(Value value);
void bindSuperclass(ObjClass* subclass, ObjClass* superclass);
bool loadGlobal(ObjString* name, Value* value);
InterpretResult interpret(const char* source);
void runtimeError(const char* format, ...);
bool callClosure(ObjClosure* closure, int argCount);
ObjArray* getStackTrace();
Value callGenerator(ObjGenerator* generator);
void push(Value value);
Value pop();
Value peek(int distance);

char* arrowsString(const char* input);
char* returnSpaces(int count);
int digitsInNumber(int number);
char* getLine(const char* source, int argNum);

#endif
