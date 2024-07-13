#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

#include "../native/native.h"
#include "../dsa/dsa.h"
#include "../common.h"
#include "../compiler/compiler.h"
#include "../object/object.h"
#include "../memory/memory.h"
#include "../lang/lang.h"
#include "../util/util.h"
#include "../string/string.h"
#include "../io/io.h"
#include "../collection/collection.h"
#include "../math/math.h"
#include "../chrono/chrono.h"
#include "../sys/sys.h"
#include "../json/json.h"
#include "../random/random.h"
#include "../graphics/graphics.h"
#include "../network/network.h"
#include "../statistics/statistics.h"
#include "../debug/debug.h"
#include "vm.h"

VM vm;
static Stack namespaceStack;

static void resetCallFrames() {
  for (int i = 0; i < FRAMES_MAX; i++) {
    CallFrame* frame = &vm.frames[i];
    frame->closure = NULL;
    frame->ip = NULL;
    frame->slots = NULL;
    frame->handlerCount = 0;
  }
}

static void resetStack() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.apiStackDepth = 0;
  vm.openUpvalues = NULL;
  resetCallFrames();
}

char* getLine(const char* source, int argNum) {
  int lineNumber = argNum + 1;

  if (!source || lineNumber <= 0) {
    return NULL;
  }

  const char* start = source;
  const char* end = source;

  for (int i = 1; i < lineNumber; i++) {
    start = end;
    end = strchr(start, '\n');

    if (!end) {
      return NULL;
    }

    end++;
  }

  const char* lineEnd = strchr(start, '\n');
  if (!lineEnd) {
    lineEnd = start + strlen(start);
  }

  size_t lineLength = lineEnd - start;

  char* line = (char*)malloc(lineLength + 1);
  if (!line) {
    return NULL;
  }

  strncpy(line, start, lineLength);
  line[lineLength] = '\0';

  return line;
}

int digitsInNumber(int number) {
  int digits = 0;
  while (number != 0) {
    number /= 10;
    digits++;
  }
  return digits;
}

char* returnSpaces(int count) {
  if (count <= 0) {
    return NULL;
  }

  char* spaceString = (char*)malloc(count + 1);
  if (!spaceString) {
    return NULL;
  }

  for (int i = 0; i < count; i++) {
    spaceString[i] = ' ';
  }

  spaceString[count] = '\0';

  return spaceString;
}

char* arrowsString(const char* input) {
  if (input == NULL) {
    return NULL;
  }

  size_t len = strlen(input);
  char* caretStr = (char*)malloc(len + 2);
  if (!caretStr) return NULL;

  caretStr[0] = '^';

  for (size_t i = 1; i <= len; i++) {
    caretStr[i] = '~';
  }

  caretStr[len + 1] = '\0';
  return caretStr;
}

void runtimeError(const char* format, ...) {
  va_list args;
  
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    int lineNumber = function->chunk.lines[instruction];
    fprintf(stderr, "\n\033[1m%s:%d in ", vm.currentModule->path->chars, lineNumber);
    if (function->name == NULL) {
      fprintf(stderr, "script: \033[0m");
    } else {
      fprintf(stderr, "%s(): \033[0m", function->name->chars);
    }

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    char* line;
    if (vm.repl) {
      line = vm.currentModule->source;
    } else {
      line = getLine(vm.currentModule->source, lineNumber);
    }

    char* spaces = returnSpaces(digitsInNumber(lineNumber));
    char* arrows = arrowsString(line);
    fprintf(stderr, "   %d |    %s\n   %s |    \033[31;1m%s\033[0m\n   %s |\n", lineNumber, line, spaces, arrows, spaces);
    if (vm.repl) free(line);
    free(spaces);
    free(arrows);
  }

  resetStack();
}


static bool loadModule(ObjString* filePath) {
  ObjModule* lastModule = vm.currentModule;
  vm.currentModule = newModule(filePath);

  char* source = readFile(filePath->chars);
  vm.currentModule->source = source;
  ObjFunction* function = compile(source);

  vm.currentNamespace = vm.rootNamespace;
  vm.previousNamespace = vm.rootNamespace;

  free(source);

  if (function == NULL) return false;
  push(OBJ_VAL(function));

  ObjClosure* closure = newClosure(function);
  pop();

  push(OBJ_VAL(closure));
  callClosure(closure, 0);

  vm.currentModule = lastModule;

  vm.runModule = true;
  return true;
}

void initVM(int argc, char** argv) {
  resetStack();
  vm.currentModule = NULL;
  vm.objects = NULL;
  vm.bytesAllocated = 0;
  vm.nextGC = (size_t)1024 * 1024;

  vm.argc = argc;
  vm.argv = argv;
  vm.repl = false;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  initTable(&vm.namespaces);
  initTable(&vm.strings);

  initStack(&namespaceStack);

  vm.initString = NULL;
  vm.initString = copyString("__init__", 8);

  registerLangPackage();
  registerSysPackage();
  registerRandomPackage();
  registerGraphicsPackage();
  registerIOPackage();
  registerCollectionPackage();
  registerUtilPackage();
  registerMathPackage();
  registerTimePackage();
  registerJsonPackage();
  registerStatisticsPackage();
  registerNetworkPackage();
  initNatives();

  vm.currentNamespace = vm.rootNamespace;
  vm.previousNamespace = vm.rootNamespace;
}

void freeVM() {
  freeTable(&vm.namespaces);
  freeTable(&vm.modules);
  freeTable(&vm.strings);

  freeStack(&namespaceStack);
  vm.initString = NULL;

  freeObjects();
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}

static void makeArray(uint8_t elementCount) {
  ObjArray* array = newArray();
  push(OBJ_VAL(array));
  for (int i = elementCount; i > 0; i--) {
    writeValueArray(&array->elements, peek(i));
  }
  pop();

  while (elementCount > 0) {
    elementCount--;
    pop();
  }
  push(OBJ_VAL(array));
}

static void makeDictionary(uint8_t entryCount) {
  ObjDictionary* dictionary = newDictionary();
  push(OBJ_VAL(dictionary));

  for (int i = 1; i <= entryCount; i++) {
    Value key = peek(2 * i);
    Value value = peek(2 * i - 1);
    dictSet(dictionary, key, value);
  }

  pop();

  while (entryCount > 0) {
    entryCount--;
    pop();
    pop();
  }

  push(OBJ_VAL(dictionary));
}

static ObjNamespace* declareNamespace(uint8_t namespaceDepth) {
  ObjNamespace* enclosingNamespace = vm.rootNamespace;
  for (int i = namespaceDepth - 1; i >= 0; i--) {
    ObjString* name = AS_STRING(peek(i));
    Value value;
    if (!tableGet(&enclosingNamespace->values, name, &value)) {
      enclosingNamespace = defineNativeNamespace(name->chars, enclosingNamespace);
    }
    else enclosingNamespace = AS_NAMESPACE(value);
  }

  while (namespaceDepth > 0) {
    pop();
    namespaceDepth--;
  }
  return enclosingNamespace;
}


static ObjString* resolveNamespacedFile(ObjNamespace* enclosingNamespace, ObjString* shortName) {
  int fullNameLength = enclosingNamespace->fullName->length;
  int shortNameLength = shortName->length;

  bool containsColons = false;
  for (int i = 0; i < fullNameLength - 1; i++) {
    if (enclosingNamespace->fullName->chars[i] == ':' && enclosingNamespace->fullName->chars[i + 1] == ':') {
      containsColons = true;
      break;
    }
  }

  int length = containsColons ? (fullNameLength + shortNameLength + 2) : (shortNameLength + 1);
  length += 4;
  char* heapChars = ALLOCATE(char, length + 1);
  int offset = 0;

  if (containsColons) {
    for (int i = 0; i < fullNameLength; i++) {
      char currentChar = enclosingNamespace->fullName->chars[i];
      if (currentChar == ':' && i + 1 < fullNameLength && enclosingNamespace->fullName->chars[i + 1] == ':') {
        heapChars[offset++] = '/';
        i++;
      } else {
        heapChars[offset++] = currentChar;
      }
    }
    heapChars[offset++] = '/';
  }

  for (int i = 0; i < shortNameLength; i++) {
    heapChars[offset++] = shortName->chars[i];
  }

  heapChars[offset++] = '.';
  heapChars[offset++] = 'l';
  heapChars[offset++] = 'm';
  heapChars[offset++] = 'q';
  heapChars[offset] = '\0';

  ObjString* result = newString(heapChars);
  FREE_ARRAY(char, heapChars, length + 1);
  return result;
}

static Value usingNamespace(uint8_t namespaceDepth) {
  ObjNamespace* enclosingNamespace = vm.rootNamespace;
  Value value;
  for (int i = namespaceDepth - 1; i >= 1; i--) {
    ObjString* name = AS_STRING(peek(i));
    if (!tableGet(&enclosingNamespace->values, name, &value)) {
      enclosingNamespace = defineNativeNamespace(name->chars, enclosingNamespace);
    }
    else enclosingNamespace = AS_NAMESPACE(value);
  }

  ObjString* shortName = AS_STRING(peek(0));
  bool valueExists = tableGet(&enclosingNamespace->values, shortName, &value);

  while (namespaceDepth > 0) {
    pop();
    namespaceDepth--;
  }

  push(OBJ_VAL(shortName));
  push(OBJ_VAL(enclosingNamespace));
  return valueExists ? value : NIL_VAL;
}

bool callClosure(ObjClosure* closure, int argCount) {
  if (closure->function->arity > 0 && argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.",
        closure->function->arity, argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  if (closure->function->arity == -1) {
    makeArray(argCount);
    argCount = 1;
  }

  CallFrame* frame = &vm.frames[vm.frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stackTop - argCount - 1;
  return true;
}

static bool callNativeFunction(NativeFunction function, int argCount) {
  Value result = function(argCount, vm.stackTop - argCount);
  vm.stackTop -= (size_t)argCount + 1;
  push(result);
  return true;
}

static bool callNativeMethod(NativeMethod method, int argCount) {
  Value result = method(vm.stackTop[-argCount - 1], argCount, vm.stackTop - argCount);
  vm.stackTop -= argCount + 1;
  push(result);
  return true;
}

bool callMethod(Value method, int argCount) {
  if (IS_NATIVE_METHOD(method)) {
    return callNativeMethod(AS_NATIVE_METHOD(method)->method, argCount);
  }

  else return callClosure(AS_CLOSURE(method), argCount);
}

Value callReentrant(Value receiver, Value callee, ...) {
    push(receiver);
    int argCount = IS_NATIVE_METHOD(callee) ? AS_NATIVE_METHOD(callee)->arity : AS_CLOSURE(callee)->function->arity;
    va_list args;
    va_start(args, callee);
    for (int i = 0; i < argCount; i++) {
      push(va_arg(args, Value));
    }
    va_end(args);

    if (IS_CLOSURE(callee)) {
      vm.apiStackDepth++;
      callClosure(AS_CLOSURE(callee), argCount);
      InterpretResult result = run();
      if (result == INTERPRET_RUNTIME_ERROR) exit(70);
      vm.apiStackDepth--;
    } else {
      callNativeMethod(AS_NATIVE_METHOD(callee)->method, argCount);
    }
    return pop();
}

static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
        vm.stackTop[-argCount - 1] = bound->receiver;
        return callMethod(OBJ_VAL(bound->method), argCount);
      }
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);
        vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
        Value initializer;
        if (tableGet(&klass->methods, vm.initString, &initializer)) {
          return callMethod(initializer, argCount);
        } else if (argCount != 0) {
          runtimeError("Expected 0 arguments but got %d.", argCount);
          return false;
        }
        return true;
      }
      case OBJ_CLOSURE:
        return callClosure(AS_CLOSURE(callee), argCount);
      case OBJ_NATIVE_FUNCTION: 
        return callNativeFunction(AS_NATIVE_FUNCTION(callee)->function, argCount);
      case OBJ_NATIVE_METHOD: 
        return callNativeMethod(AS_NATIVE_METHOD(callee)->method, argCount);
      default:
        break; // Non-callable object type.
    }
  }

  ObjClass* klass = getObjClass(callee);
  ObjString* name = copyString("()", 2);
  Value method;
  if (!tableGet(&klass->methods, name, &method)) { 
    ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "CallException");
    throwException(exceptionClass, "Undefined operator method '%s' on class %s.", name->chars, klass->name->chars);
    return false;
  }
  int arity = IS_NATIVE_METHOD(method) ? AS_NATIVE_METHOD(method)->arity : AS_CLOSURE(method)->function->arity;
  return callMethod(method, arity);
}



static bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }
  return callMethod(method, argCount);
}

static bool invoke(ObjString* name, int argCount) {
  Value receiver = peek(argCount);
  if (!IS_OBJ(receiver)) {
    return invokeFromClass(getObjClass(receiver), name, argCount);
  }

  if (IS_INSTANCE(receiver)) {
    ObjInstance* instance = AS_INSTANCE(receiver);
    Value value;
    if (tableGet(&instance->fields, name, &value)) {
      vm.stackTop[-argCount - 1] = value;
      return callValue(value, argCount);
    }
  } else if (IS_NAMESPACE(receiver)) { 
    ObjNamespace* namespace = AS_NAMESPACE(receiver);
    Value value;
    if (tableGet(&namespace->values, name, &value)) { 
      return callValue(value, argCount);
    }
  }

  return invokeFromClass(getObjClass(receiver), name, argCount);
}


static bool invokeOperator(ObjString* op, int arity) {
  Value receiver = peek(arity);
  ObjClass* klass = getObjClass(receiver);
  Value method;

  if (!tableGet(&klass->methods, op, &method)) {
    ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "CallException");
    throwException(exceptionClass, "Undefined operator method '%s' on class %s.", op->chars, klass->name->chars);
    return false;
  }
  return invoke(op, arity);
}

static bool bindGetter(ObjClass* klass, ObjInstance* instance, ObjString* name) {
  Value method;
  if (!tableGet(&klass->getters, name, &method)) {
    return false;
  }

  Value getter = callReentrant(peek(0), method, NIL_VAL); 

  tableSet(&instance->fields, name, getter);

  pop();
  push(getter);
  return true;
}

static bool bindMethod(ObjClass* klass, ObjString* name) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    return false;
  }

  ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}

bool loadGlobal(ObjString* name, Value* value) {
  if (tableGet(&vm.rootNamespace->values, name, value)) return true;
  else if (tableGet(&vm.rootNamespace->globals, name, value)) return true;
  else if (tableGet(&vm.currentNamespace->values, name, value)) return true;
  else if (tableGet(&vm.currentNamespace->globals, name, value)) return true;
  else return (tableGet(&vm.currentModule->values, name, value));
} 

static ObjUpvalue* captureUpvalue(Value* local) {
  ObjUpvalue* prevUpvalue = NULL;
  ObjUpvalue* upvalue = vm.openUpvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  ObjUpvalue* createdUpvalue = newUpvalue(local);

  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    vm.openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }

  return createdUpvalue;
}

static void closeUpvalues(Value* last) {
  while (vm.openUpvalues != NULL &&
         vm.openUpvalues->location >= last) {
    ObjUpvalue* upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}

static void defineGetter(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));

  tableSet(&klass->getters, name, method);

  pop();
}

static void defineSetter(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));

  tableSet(&klass->setters, name, method);

  pop();
}

static void defineMethod(ObjString* name, bool isMethodStatic) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));

  if (isMethodStatic) {
    klass = klass->obj.klass;
  }

  tableSet(&klass->methods, name, method);
  pop();
}

void bindSuperclass(ObjClass* subclass, ObjClass* superclass) {
  if (superclass == NULL) {
    runtimeError("Superclass cannot be null for class %s", subclass->name);
    return;
  }
  subclass->superclass = superclass;
  tableAddAll(&superclass->methods, &subclass->methods);
}

bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString* b = AS_STRING(peek(0));
  ObjString* a = AS_STRING(peek(1));

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}

ObjArray* getStackTrace() {
  ObjArray* stackTrace = newArray();
  push(OBJ_VAL(stackTrace));
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    char stackTraceBuffer[UINT8_MAX];
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    uint32_t line = function->chunk.lines[instruction];

    uint8_t length = snprintf(stackTraceBuffer, UINT8_MAX, "[line %d] in %s()", line, function->name == NULL ? "script" : function->name->chars);
    ObjString* stackElement = copyString(stackTraceBuffer, length);
    writeValueArray(&stackTrace->elements, OBJ_VAL(stackElement));
  }
  pop();
  return stackTrace;
}

static bool propagateException() {
  ObjInstance* exception = AS_INSTANCE(peek(0));

  while (vm.frameCount > 0) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    for (int i = frame->handlerCount; i > 0; i--) {
      ExceptionHandler handler = frame->handlerStack[i - 1];
      if (isObjInstanceOf(OBJ_VAL(exception), handler.exceptionClass)) {
        frame->ip = &frame->closure->function->chunk.code[handler.handlerAddress];
        return true;
      } else if (handler.finallyAddress != UINT16_MAX){
        push(TRUE_VAL);
        frame->ip = &frame->closure->function->chunk.code[handler.finallyAddress];
        return true;
      }
    }
    vm.frameCount--;
  }

  ObjString* message = AS_STRING(getObjProperty(exception, "message"));
  fprintf(stderr, "Unhandled %s: %s\n", exception->obj.klass->name->chars, message->chars);
  ObjArray* stackTrace = AS_ARRAY(getObjProperty(exception, "stacktrace"));
  for (int i = 0; i < stackTrace->elements.count; i++) {
    Value item = stackTrace->elements.values[i];
    fprintf(stderr, "%s.\n", AS_CSTRING(item));
  }
  fflush(stderr);
  return false;
}

static void pushExceptionHandler(ObjClass* exceptionClass, uint16_t handlerAddress, uint16_t finallyAddress) {
  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  if (frame->handlerCount >= UINT4_MAX) {
    runtimeError("Too many nested exception handlers.");
    exit(70);
  }
  frame->handlerStack[frame->handlerCount].handlerAddress = handlerAddress;
  frame->handlerStack[frame->handlerCount].exceptionClass = exceptionClass;
  frame->handlerStack[frame->handlerCount].finallyAddress = finallyAddress;
  frame->handlerCount++;
}

ObjInstance* throwException(ObjClass* exceptionClass, const char* format, ...) {
  char chars[UINT8_MAX];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(chars, UINT8_MAX, format, args);
  va_end(args);
  ObjString* message = copyString(chars, length);

  ObjArray* stacktrace = getStackTrace();
  ObjInstance* exception = newInstance(exceptionClass);
  push(OBJ_VAL(exception));
  setObjProperty(exception, "message", OBJ_VAL(message));
  setObjProperty(exception, "stacktrace", OBJ_VAL(stacktrace));
  if (!propagateException()) exit(70);
  else return exception;
}

InterpretResult run() {
  CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[(uint8_t)READ_BYTE()])
#define READ_CONSTANT_16() (frame->closure->function->chunk.constants.values[(uint16_t)READ_SHORT()])

#define READ_STRING() AS_STRING(READ_CONSTANT_16())

#define BINARY_INT_OP(valueType, op) \
    do {\
      int b = AS_INT(pop()); \
      int a = AS_INT(pop()); \
      push(INT_VAL(a op b)); \
    } while (false)

#define BINARY_NUMBER_OP(valueType, op) \
    do { \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)


#define OVERLOAD_OP(op, arity) \
    do { \
      ObjString* opName = newString(#op); \
      if (!invokeOperator(opName, arity)) { \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      frame = &vm.frames[vm.frameCount - 1]; \
    } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
   printf("          ");
   for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
     printf("[ ");
     printValue(*slot);
     printf(" ]");
   }
   printf("\n");
   disassembleInstruction(&frame->closure->function->chunk,
       (int)(frame->ip - frame->closure->function->chunk.code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_CONSTANT_16: {
        Value constant = READ_CONSTANT_16();
        push(constant);
        break;
      }
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
      case OP_ARRAY: {
        uint8_t elementCount = READ_BYTE();
        makeArray(elementCount);
        break;
      }
      case OP_DICTIONARY: {
        int entryCount = READ_BYTE();
        makeDictionary(entryCount); 
        break;
      }
      case OP_RANGE: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) {
          int b = AS_INT(pop());
          int a = AS_INT(pop());
          push(OBJ_VAL(newRange(a, b)));
        } else {
          ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "IllegalArgumentException");
          throwException(exceptionClass, "Operand must be two integers.");
        }
        break;
      }
      case OP_BEGIN_NAMESPACE: {
        ObjString* name = READ_STRING();
        ObjNamespace* namespaceObj = defineNativeNamespace(name->chars, vm.currentNamespace);
        stackPush(&namespaceStack, vm.currentNamespace);
        vm.previousNamespace = vm.currentNamespace;
        vm.currentNamespace = namespaceObj;
        push(OBJ_VAL(namespaceObj));
        tableSet(&vm.previousNamespace->values, name, peek(0));
        break;
      }
      case OP_END_NAMESPACE: {
        vm.currentNamespace = stackPop(&namespaceStack);
        break;
      }
      case OP_SUBNAMESPACE: { 
        Value value = pop();

        if (IS_NIL(value)) {
          runtimeError("Undefined class/function/namespace.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjString* alias = READ_STRING(); 
        if (alias->length > 0) {
          tableSet(&vm.currentModule->values, alias, value);
        } else if (IS_CLASS(value)) {
          ObjClass* klass = AS_CLASS(value);
          tableSet(&vm.rootNamespace->values, klass->name, value);
        } else if (IS_FUNCTION(value)) {
          ObjFunction* function = AS_FUNCTION(value);
          tableSet(&vm.rootNamespace->values, function->name, value);
        } else if (IS_NATIVE_FUNCTION(value)) {
          ObjNativeFunction* function = AS_NATIVE_FUNCTION(value);
          tableSet(&vm.rootNamespace->values, function->name, value);
        } else if (IS_ENUM(value)) {
          ObjEnum* enum_ = AS_ENUM(value);
          tableSet(&vm.rootNamespace->values, enum_->name, value);
        } else if (IS_NAMESPACE(value)) {
          ObjNamespace* namespace = AS_NAMESPACE(value);
          tableSet(&vm.rootNamespace->values, namespace->shortName, value);
        } else {
          runtimeError("Only classes, functions, enums and namespaces may be imported.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_USING: {
        uint8_t namespaceDepth = READ_BYTE();
        Value value = usingNamespace(namespaceDepth);
        ObjNamespace* enclosingNamespace = AS_NAMESPACE(pop());
        ObjString* shortName = AS_STRING(pop());
        pop();
        push(OBJ_VAL(enclosingNamespace));

        if (IS_NIL(value)) {
          ObjString* filePath = resolveNamespacedFile(enclosingNamespace, shortName);
          struct stat fileStat;
          if (stat(filePath->chars, &fileStat) == -1) {
            runtimeError("Failed to load source file %s.", filePath->chars);
            return INTERPRET_RUNTIME_ERROR;
          }
          loadModule(filePath);
          frame = &vm.frames[vm.frameCount - 1];
        }
        push(value);
        break;
      }
      case OP_GET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        push(*frame->closure->upvalues[slot]->location);
        break;
      }
      case OP_SET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        *frame->closure->upvalues[slot]->location = peek(0);
        break;
      }
      case OP_EQUAL: {
        if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) BINARY_NUMBER_OP(BOOL_VAL, ==);
        else { 
          ObjString* operator = copyString("==", 2);
          if (!invokeOperator(operator, true)) {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(a == b));
          }
          else frame = &vm.frames[vm.frameCount - 1];
        }
        break;
      }
      case OP_POP: pop(); break;
      case OP_INCREMENT_GLOBAL:
      case OP_DECREMENT_GLOBAL: {
        ObjString* name = READ_STRING();
        bool isPrefix = READ_BYTE();

        Value value;
        if (!loadGlobal(name, &value)) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }

        if (!IS_NUMBER(value)) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }

        bool isInt = IS_INT(value);

        double delta = (instruction == OP_INCREMENT_GLOBAL) ? 1.0 : -1.0;
        if (tableSet(&vm.currentNamespace->globals, name, isInt ? INT_VAL(AS_INT(value) + delta) : NUMBER_VAL(AS_NUMBER(value) + delta))) {
          tableDelete(&vm.currentNamespace->globals, name); 
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        
        if (isPrefix) {
          pop();
          push(isInt ? INT_VAL(AS_INT(value) + delta) : NUMBER_VAL(AS_NUMBER(value) + delta));
        }
        break;
      }
      case OP_INCREMENT_LOCAL:
      case OP_DECREMENT_LOCAL: {
        uint8_t slot = READ_BYTE();
        bool isPrefix = READ_BYTE();

        if (!IS_NUMBER(frame->slots[slot])) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }

        bool isInt = IS_INT(frame->slots[slot]);

        double delta = (instruction == OP_INCREMENT_LOCAL) ? 1.0 : -1.0;
        frame->slots[slot] = isInt ? INT_VAL(AS_INT(frame->slots[slot]) + delta) : NUMBER_VAL(AS_NUMBER(frame->slots[slot]) + delta);

        if (isPrefix) {
          pop();
          push(isInt ? INT_VAL(AS_INT(frame->slots[slot]) + delta) : NUMBER_VAL(AS_NUMBER(frame->slots[slot]) + delta));
        }
        break;
      }
      case OP_INCREMENT_UPVALUE:
      case OP_DECREMENT_UPVALUE: {
        uint8_t slot = READ_BYTE();
        bool isPrefix = READ_BYTE();
        if (!IS_NUMBER(*frame->closure->upvalues[slot]->location)) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }

        bool isInt = IS_INT(frame->slots[slot]);

        double delta = (instruction == OP_INCREMENT_UPVALUE) ? 1.0 : -1.0;
        *frame->closure->upvalues[slot]->location = isInt ? INT_VAL(AS_INT(*frame->closure->upvalues[slot]->location) + delta) :
          NUMBER_VAL(AS_NUMBER(*frame->closure->upvalues[slot]->location) + delta);

        if (isPrefix) {
          pop();
          push(isInt ? INT_VAL(AS_INT(*frame->closure->upvalues[slot]->location) + delta) :
            NUMBER_VAL(AS_NUMBER(*frame->closure->upvalues[slot]->location) + delta));
        }
        break;
      }
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name = READ_STRING();
        Value value;
        if (!loadGlobal(name, &value)) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_STRING();
        tableSet(&vm.currentNamespace->globals, name, peek(0));
        pop();
        break;
      }
      case OP_DEFINE_CONST: {
        ObjString* name = READ_STRING();
        tableSet(&vm.currentNamespace->values, name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (tableSet(&vm.currentNamespace->globals, name, peek(0))) {
          tableDelete(&vm.currentNamespace->globals, name); 
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_GREATER: 
        if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) BINARY_NUMBER_OP(BOOL_VAL, >);
        else OVERLOAD_OP(>, 1);
        break;
      case OP_LESS: 
        if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) BINARY_NUMBER_OP(BOOL_VAL, <);
        else OVERLOAD_OP(<, 1);
        break;
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        }
        else if (IS_INT(peek(0)) && IS_INT(peek(1))) BINARY_INT_OP(INT_VAL, +);
        else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) BINARY_NUMBER_OP(NUMBER_VAL, +);
        else OVERLOAD_OP(+, 1);
        break;
      }
      case OP_SUBTRACT: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) BINARY_INT_OP(INT_VAL, -);
        else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) BINARY_NUMBER_OP(NUMBER_VAL, -);
        else OVERLOAD_OP(-, 1);
        break;
      }
      case OP_MULTIPLY: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) BINARY_INT_OP(INT_VAL, *);
        else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) BINARY_NUMBER_OP(NUMBER_VAL, *);
        else OVERLOAD_OP(*, 1);
        break;
      }
      case OP_DIVIDE: {
        if (IS_INT(peek(0)) && AS_INT(peek(0)) == 0) {
          ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "ArithmeticException");
          throwException(exceptionClass, "Divide by 0 is illegal.");
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          BINARY_NUMBER_OP(NUMBER_VAL, /);
        } else OVERLOAD_OP(/, 1); 
        break;
      }
      case OP_MODULO: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) BINARY_INT_OP(INT_VAL, %);
        else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(fmod(a, b)));
        }
        else OVERLOAD_OP(%, 1);
        break;
      }
      case OP_POWER: {
        if (IS_NUMBER(peek(0)) || IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(pow(a, b)));
        } else {
          ObjString* op = newString("**");
          if (!invokeOperator(op, 1)) {
            return INTERPRET_RUNTIME_ERROR;
          } 
          frame = &vm.frames[vm.frameCount - 1];
        }
        break;
      }
      case OP_BITOR: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) BINARY_INT_OP(INT_VAL, |);
        else OVERLOAD_OP(|, 1);
        break;
      }
      case OP_BITXOR: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) BINARY_INT_OP(INT_VAL, ^);
        else OVERLOAD_OP(^, 1);
        break;
      }
      case OP_BITAND: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) BINARY_INT_OP(INT_VAL, &);
        else OVERLOAD_OP(&, 1);
        break;
      }
      case OP_SHOWEL_L: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) BINARY_INT_OP(INT_VAL, <<);
        else OVERLOAD_OP(<<, 1);
        break;
      }
      case OP_SHOWEL_R: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) BINARY_INT_OP(INT_VAL, >>);
        else OVERLOAD_OP(>>, 1);
        break;
      }
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "IllegalArgumentException"); 
          throwException(exceptionClass, "Operands must be numbers for negate operator.");
        }

        if(IS_INT(peek(0))) push(INT_VAL(-AS_INT(pop())));
        else push(NUMBER_VAL(-AS_NUMBER(pop())));

        break;
      case OP_AFFERM:
        if (!IS_NUMBER(peek(0))) {
          ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "IllegalArgumentException"); 
          throwException(exceptionClass, "Operands must be numbers for negate operator.");
        }

        if (IS_INT(peek(0))) push(INT_VAL(+AS_INT(pop())));
        else push(NUMBER_VAL(+AS_NUMBER(pop())));

        break;
      case OP_BITNOT:
        if (!IS_INT(peek(0))) {
          ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "IllegalArgumentException");
          throwException(exceptionClass, "Operand must be an integer for bitwise NOT operator.");
        }

        push(INT_VAL(~AS_INT(pop())));
        break;
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        frame->ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0))) frame->ip += offset;
        break;
      }
      case OP_JUMP_IF_EMPTY: {
        uint16_t offset = READ_SHORT();
        if (IS_NIL(peek(0)) || IS_UNDEFINED(peek(0))) frame->ip += offset;
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        frame->ip -= offset;
        break;
      }
      case OP_CALL: {
        int argCount = READ_BYTE();
        if (!callValue(peek(argCount), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_CLOSURE: {
        ObjFunction* function = AS_FUNCTION(READ_CONSTANT_16());
        ObjClosure* closure = newClosure(function);
        push(OBJ_VAL(closure));
        for (int i = 0; i < closure->upvalueCount; i++) {
          uint8_t isLocal = READ_BYTE();
          uint8_t index = READ_BYTE();
          if (isLocal) {
            closure->upvalues[i] =
                captureUpvalue(frame->slots + index);
          } else {
            closure->upvalues[i] = frame->closure->upvalues[index];
          }
        }
        break;
      }
      case OP_DUP: push(peek(0)); break;
      case OP_CLOSE_UPVALUE: {
        closeUpvalues(vm.stackTop - 1);
        pop();
        break;
      }
      case OP_ENUM: {
        ObjString* name = READ_STRING();
        ObjEnum* enumObj = newEnum(name);
        enumObj->nextValue = 0;
        push(OBJ_VAL(enumObj));
        tableSet(&vm.currentNamespace->values, name, peek(0));
        break;
      }
      case OP_ENUM_ELEMENT: {
        ObjString* elementName = READ_STRING();
        if (!IS_ENUM(peek(0))) {
          runtimeError("Expected enum object.");
          break;
        }
        ObjEnum* enumObj = AS_ENUM(peek(0));
        int elementValue = enumObj->nextValue++;
        tableSet(&enumObj->values, elementName, INT_VAL(elementValue));
        break;
      }
      case OP_CLASS: {
        ObjString* name = READ_STRING();
        push(OBJ_VAL(newClass(name)));
        tableSet(&vm.currentNamespace->values, name, peek(0));
        break;
      }
      case OP_GET_COLON_PROPERTY: {
        ObjString* name = READ_STRING();
        Value object = peek(0);

        if (IS_NAMESPACE(object)) {
          ObjNamespace* namespace_ = AS_NAMESPACE(object);
          Value value;

          if (tableGet(&namespace_->values, name, &value)) {
            pop();
            push(value);
          } else if (tableGet(&namespace_->globals, name, &value)) {
            pop();
            push(value);
          } else {
            runtimeError("Undefined subnamespace or property '%s'.", name->chars);
            return INTERPRET_RUNTIME_ERROR;
          }
        } else if (IS_ENUM(object)) {
          ObjEnum* enumObj = AS_ENUM(object);
          Value elementValue;

          if (tableGet(&enumObj->values, name, &elementValue)) {
            pop();
            push(elementValue);
          } else {
            runtimeError("Undefined enum element '%s'.", name->chars);
            return INTERPRET_RUNTIME_ERROR;
          }
        } else {
          runtimeError("Only namespaces or enums can access properties using '::'.");
          return INTERPRET_RUNTIME_ERROR;
        }

        break;
      }
      case OP_GET_PROPERTY: {
        Value receiver = peek(0);
        if (IS_INSTANCE(receiver)) {
          ObjInstance* instance = AS_INSTANCE(receiver);
          ObjString* name = READ_STRING();
          Value value;
  
          bool getterResult = !bindGetter(instance->obj.klass, instance, name);

          if (tableGet(&instance->fields, name, &value)) {
            pop();
            push(value);
            break;
          }

          if (getterResult && !bindMethod(instance->obj.klass, name)) {
            runtimeError("Undefined property '%s'.", name->chars);
            return INTERPRET_RUNTIME_ERROR;
          }
        } else if (IS_CLASS(receiver)) {
          ObjClass* klass = AS_CLASS(receiver);
          ObjString* name = READ_STRING();
          Value value;

          if (tableGet(&klass->fields, name, &value)) {
            pop();
            push(value);
            break;
          }

          if (!bindMethod(klass->obj.klass, name)) {
            runtimeError("Undefined property '%s'.", name->chars);
            return INTERPRET_RUNTIME_ERROR;
          }
        } else {
          runtimeError("Only instances and classes can access properties with '.'.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SET_PROPERTY: {
        Value receiver = peek(1);
        if (IS_INSTANCE(receiver)) {
          ObjInstance* instance = AS_INSTANCE(receiver);
          ObjString* name = READ_STRING();

          Value value = pop();

          Value setter;
          bool isSetter = tableGet(&instance->obj.klass->setters, name, &setter);
          if (isSetter) {
            callReentrant(OBJ_VAL(instance), setter, value);
            pop();
            push(value);
          }

          Value getter;
          bool isGetter = tableGet(&instance->obj.klass->getters, name, &getter);
          if (isSetter && isGetter) {
            // Do nothing
          } else if (isGetter) {
            runtimeError("Cannot modify a getter.");
            return INTERPRET_RUNTIME_ERROR;
          }

          if (!isSetter) {
            tableSet(&instance->fields, name, value);
            pop();
            push(value);
          }
        } else if (IS_CLASS(receiver)) {
          ObjClass* klass = AS_CLASS(receiver);
          tableSet(&klass->fields, READ_STRING(), peek(0));

          Value value = pop();
          pop();
          push(value);
        } else {
          runtimeError("Only instances and classes can set properties with '.'.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_GET_SUBSCRIPT: {
        if (IS_INT(peek(0))) {
          int index = AS_INT(peek(0));
          if (IS_STRING(peek(0))) {
            pop();
            ObjString* string = AS_STRING(pop());
            if (index < 0 || index >= string->length) {
              ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "IndexOutOfBoundsException");
              throwException(exceptionClass, "String index is out of bound: %d.", index);
            } else {
              char chars[2] = { string->chars[index], '\0' };
              ObjString* element = copyString(chars, 1);
              push(OBJ_VAL(element));
            }
          } else if (IS_ARRAY(peek(0))) {
            pop();
            ObjArray* array = AS_ARRAY(pop());

            if (index < 0 || index >= array->elements.count) {
              ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "IndexOutOfBoundsException");
              throwException(exceptionClass, "Array index is out of bound: %d.", index);
            } else {
              Value element = array->elements.values[index];
              push(element);
            }
          } else OVERLOAD_OP([], 1);
        } else if (IS_DICTIONARY(peek(1))) {
            Value key = pop();
            ObjDictionary* dictionary = AS_DICTIONARY(pop());
            Value value;
            if (dictGet(dictionary, key, &value)) push(value);
            else push(NIL_VAL);
        } else OVERLOAD_OP([], 1);
        break;
      }
      case OP_SET_SUBSCRIPT: {
        if (IS_INT(peek(1)) && IS_ARRAY(peek(2))) {
          Value element = pop();
          int index = AS_INT(pop());
          ObjArray* array = AS_ARRAY(pop());

          if (index < 0 || index >= array->elements.count) {
            ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "IndexOutOfBoundsException");
            throwException(exceptionClass, "Array index is out of bound.");
          } else {
            replaceValueArray(&array->elements, index, element);
            push(OBJ_VAL(array));
          }
        } else if (IS_DICTIONARY(peek(2))) {
          Value value = pop();
          Value key = pop();
          ObjDictionary* dictionary = AS_DICTIONARY(pop());
          dictSet(dictionary, key, value);
          push(OBJ_VAL(dictionary));
        } else OVERLOAD_OP([]=, 2);
        break;
      }
      case OP_GET_SUPER: {
        ObjString* name = READ_STRING();
        ObjClass* superclass = AS_CLASS(pop());

        if (!bindMethod(superclass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_METHOD:
        defineMethod(READ_STRING(), false);
        break;
      case OP_STATIC_METHOD:
        defineMethod(READ_STRING(), true);
        break;
      case OP_GETTER:
        defineGetter(READ_STRING());
        break;
      case OP_SETTER:
        defineSetter(READ_STRING());
        break;
      case OP_INVOKE: {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();

        if (!invoke(method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_SUPER_INVOKE: {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();

        if (!IS_CLASS(peek(0))) {
          runtimeError("Superclass must be a class. (This message is needed for debug purposes)");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjClass* superclass = AS_CLASS(pop());
        if (!invokeFromClass(superclass, method, argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_INHERIT: {
        Value superclass = peek(1);

        if (!IS_CLASS(superclass)) {
          runtimeError("Superclass must be a class.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjClass* subclass = AS_CLASS(peek(0));
        bindSuperclass(subclass, AS_CLASS(superclass));
        pop(); // Subclass.
        break;
      }
      case OP_ASSERT: {
        Value message = pop();
        Value condition = pop();
        
        if (isFalsey(condition)) {
          ObjClass* exceptionClass = getNativeClass("luminique::std::lang", "AssertException"); 
          if (IS_NIL(message)) { 
            throwException(exceptionClass, "<assert statement>");
          } else if (IS_STRING(message)) {
            throwException(exceptionClass, AS_CSTRING(message));
          } else {
            runtimeError("Message must be a string.");
            return INTERPRET_RUNTIME_ERROR;
          }
        }
        break;
      }
      case OP_THROW: {
        ObjArray* stackTrace = getStackTrace();
        Value exception = peek(0);
        if (!isObjInstanceOf(exception, vm.exceptionClass)) {
          runtimeError("Only instances of class Exception may be thrown.");
          return INTERPRET_RUNTIME_ERROR;
        }
        setObjProperty(AS_INSTANCE(exception), "stacktrace", OBJ_VAL(stackTrace));
        if (propagateException()) {
          frame = &vm.frames[vm.frameCount - 1];
          break;
        }
        return INTERPRET_RUNTIME_ERROR;
      }
      case OP_TRY: {
        ObjString* exceptionClass = AS_STRING(READ_CONSTANT());
        uint16_t handlerAddress = READ_SHORT();
        uint16_t finallyAddress = READ_SHORT();
        Value value;
        if (!loadGlobal(exceptionClass, &value)){
          runtimeError("Undefined class %s specified as exception type.", exceptionClass->chars);
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjClass* klass = AS_CLASS(value);
        if (!isClassExtendingSuperclass(klass, vm.exceptionClass)) {
          runtimeError("Expect subclass of Exception, but got Class %s.", exceptionClass->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        pushExceptionHandler(klass, handlerAddress, finallyAddress);
        break;
      }
      case OP_CATCH:
        frame->handlerCount--;
        break;
      case OP_FINALLY: { 
        frame->handlerCount--;
        if (propagateException()) {
          frame = &vm.frames[vm.frameCount - 1];
          break;
        }
        return INTERPRET_RUNTIME_ERROR;
      }
      case OP_REQUIRE: {
        Value filePath = pop();
        Value value;
        if (!IS_STRING(filePath)) {
          runtimeError("Required file path must be a string.");
          return INTERPRET_RUNTIME_ERROR;
        } else if (tableGet(&vm.modules, AS_STRING(filePath), &value)) {
          break;
        }
        loadModule(AS_STRING(filePath));
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_TYPEOF: {
        Value value = pop();

        if (IS_CLASS(value)) {
          push(value);
        } else {
          push(OBJ_VAL(getObjClass(value)));
        }
        break;
      }
      case OP_RETURN: {
        Value result = pop();
        closeUpvalues(frame->slots);
        vm.frameCount--;
        if (vm.frameCount == 0) {
          pop();
          return INTERPRET_OK;
        }

        vm.stackTop = frame->slots;
        if (vm.runModule) {
          vm.runModule = false;
        } else {
          vm.runModule = false;
          push(result);
        }
        if (vm.apiStackDepth > 0) return INTERPRET_OK;
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_RETURN_NONLOCAL: {
        Value result = pop();
        int depth = READ_BYTE();
        closeUpvalues(frame->slots);
        vm.frameCount -= depth + 1;
        if (vm.frameCount == 0) {
          pop();
          return INTERPRET_OK;
        }

        vm.stackTop = frame->slots;
        push(result);
        if (vm.apiStackDepth > 0) return INTERPRET_OK;
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_CONSTANT_16
#undef READ_STRING
#undef BINARY_INT_OP
#undef BINARY_NUMBER_OP
#undef OVERLOAD_OP
}

InterpretResult interpret(const char* source) {
  ObjFunction* function = compile(source);

  vm.currentNamespace = vm.rootNamespace;
  vm.previousNamespace = vm.rootNamespace;

  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  ObjClosure* closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  callClosure(closure, 0);

  return run();
}

