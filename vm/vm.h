#ifndef cluminique_vm_h
#define cluminique_vm_h

#include <uv.h>

#include "../table/table.h"
#include "../value/value.h"
#include "../exception/exception.h"

#define FRAMES_MAX 512 / 2 
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

#define ABORT_IFNULL(pointer, message, ...) \
  do {\
    if (pointer == NULL) { \
      fprintf(stderr, message, ##__VA_ARGS__); \
      exit(74); \
    } \
  } while (false)

#define ABORT_IFTRUE(condition, message, ...) \
  do {\
    if (condition) { \
      fprintf(stderr, message, ##__VA_ARGS__); \
      exit(74); \
    } \
  } while (false)


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
  ObjClass* eventClass;
  ObjClass* promiseClass;
  ObjClass* timerClass;
  ObjClass* boundMethodClass;

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
  uv_loop_t* eventLoop;
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
InterpretResult runModule(ObjModule* module, bool isRootModule);
InterpretResult interpret(const char* source);

void runtimeError(const char* format, ...);
bool callClosure(ObjClosure* closure, int argCount);
ObjArray* getStackTrace();
Value callGenerator(ObjGenerator* generator);
Value createObject(ObjClass* klass, int argCount);
void push(Value value);
Value pop();
Value peek(int distance);

char* arrowsString(const char* input);
char* returnSpaces(int count);
int digitsInNumber(int number);
char* getLine(const char* source, int argNum);

#endif
