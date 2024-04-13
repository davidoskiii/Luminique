#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "../native/native.h"
#include "../common.h"
#include "../compiler/compiler.h"
#include "../debug/debug.h"
#include "../object/object.h"
#include "../memory/memory.h"
#include "../std/std.h"
#include "../lang/lang.h"
#include "../util/util.h"
#include "../string/string.h"
#include "vm.h"

VM vm;

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
  vm.openUpvalues = NULL;
  resetCallFrames();
}

void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", 
            function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}

void initVM() {
  resetStack();
  vm.objects = NULL;
  vm.bytesAllocated = 0;
  vm.nextGC = (size_t)1024 * 1024;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  initTable(&vm.globalValues);
  initTable(&vm.globals);
  initTable(&vm.strings);

  vm.initString = NULL;
  vm.initString = copyString("__init__", 8);

  registerLangPackage();
  registerUtilPackage();
  initNatives();
  initStd();
}

void freeVM() {
  freeTable(&vm.globalValues);
  freeTable(&vm.globals);
  freeTable(&vm.strings);
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

static bool makeDictionary(uint8_t entryCount) {
  ObjDictionary* dictionary = newDictionary();
  push(OBJ_VAL(dictionary));
  for (int i = 1; i <= entryCount; i++) {
    Value key = peek(2 * i);
    Value value = peek(2 * i - 1);
    if (!IS_STRING(key)) return false;
    tableSet(&dictionary->table, AS_STRING(key), value);
  }
  pop();

  while (entryCount > 0) {
    entryCount--;
    pop();
    pop();
  }
  push(OBJ_VAL(dictionary));
  return true;
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

static bool callMethod(Value method, int argCount) {
  if (IS_NATIVE_METHOD(method)) {
    return callNativeMethod(AS_NATIVE_METHOD(method)->method, argCount);
  }

  else return callClosure(AS_CLOSURE(method), argCount);
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
  runtimeError("Can only call functions and classes.");
  return false;
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
  }

  return invokeFromClass(getObjClass(receiver), name, argCount);
}


static bool bindMethod(ObjClass* klass, ObjString* name) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}

bool loadGlobal(ObjString* name, Value* value) {
    if (tableGet(&vm.globalValues, name, value)) return true;
    return tableGet(&vm.globals, name, value);
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

static void defineMethod(ObjString* name) {
  Value method = peek(0);
  ObjClass* klass = AS_CLASS(peek(1));
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

static bool isFalsey(Value value) {
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

static InterpretResult run() {
  CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        runtimeError("Operands must be numbers."); \
        ObjClass* exceptionClass = getNativeClass("IllegalArgumentException"); \
        return INTERPRET_RUNTIME_ERROR; \
        throwException(exceptionClass, "Operands must be numbers."); \
      } \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
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
        if (!makeDictionary(entryCount)) { 
          runtimeError("Keys for dictionary literal must be strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
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
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }
      case OP_POP: pop(); break;
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
        tableSet(&vm.globals, name, peek(0));
        pop();
        break;
      }
      case OP_DEFINE_CONST: {
        ObjString* name = READ_STRING();
        tableSet(&vm.globalValues, name, peek(0));
        pop();
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (tableSet(&vm.globals, name, peek(0))) {
          tableDelete(&vm.globals, name); 
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;

      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_INT(peek(0)) && IS_INT(peek(1))) {
          int b = AS_INT(pop());
          int a = AS_INT(pop());
          push(INT_VAL(a + b));
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          ObjClass* exceptionClass = getNativeClass("IllegalArgumentException");
          throwException(exceptionClass, "Operands must be two numbers or two strings.");
        }
        break;
      }
      case OP_SUBTRACT: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) {
          int b = AS_INT(pop());
          int a = AS_INT(pop());
          push(INT_VAL(a - b));
        }
        else BINARY_OP(NUMBER_VAL, -);
        break;
      }
      case OP_MULTIPLY: {
        if (IS_INT(peek(0)) && IS_INT(peek(1))) {
          int b = AS_INT(pop());
          int a = AS_INT(pop());
          push(INT_VAL(a * b));
        }
        else BINARY_OP(NUMBER_VAL, *); 
        break;
      }
      case OP_DIVIDE: {
        if (IS_INT(peek(0)) && AS_INT(peek(0)) == 0) {
          ObjClass* exceptionClass = getNativeClass("ArithmeticException");
          throwException(exceptionClass, "Divide by 0 is illegal.");
        } else BINARY_OP(NUMBER_VAL, /); 
        break;
      }
      case OP_MODULO: {
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
          runtimeError("Operands must be numbers.");
          return INTERPRET_RUNTIME_ERROR;
        }
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(fmod(a, b)));
        break;
      }
      case OP_POWER: {
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
          ObjClass* exceptionClass = getNativeClass("IllegalArgumentException"); 
          throwException(exceptionClass, "Operands must be numbers for power operator.");
        }
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(pow(a, b)));
        break;
      }
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          ObjClass* exceptionClass = getNativeClass("IllegalArgumentException"); 
          throwException(exceptionClass, "Operands must be numbers for negate operator.");
        }

        if(IS_INT(peek(0))) push(INT_VAL(-AS_INT(pop())));
        else push(NUMBER_VAL(-AS_NUMBER(pop())));

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
        ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
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
      case OP_CLOSE_UPVALUE:
        closeUpvalues(vm.stackTop - 1);
        pop();
        break;
      case OP_CLASS:
        push(OBJ_VAL(newClass(READ_STRING())));
        break;
      case OP_GET_PROPERTY: {
        if (!IS_INSTANCE(peek(0))) {
          runtimeError("Only instances have properties.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek(0));
        ObjString* name = READ_STRING();

        Value value;
        if (tableGet(&instance->fields, name, &value)) {
          pop(); // Instance.
          push(value);
          break;
        }

        if (!bindMethod(instance->obj.klass, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SET_PROPERTY: {
        if (!IS_INSTANCE(peek(1))) {
          runtimeError("Only instances have fields.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek(1));
        tableSet(&instance->fields, READ_STRING(), peek(0));
        Value value = pop();
        pop();
        push(value);
        break;
      }
      case OP_GET_SUBSCRIPT: {
        if (IS_INT(peek(0))) {
          int index = AS_INT(pop());

          if (IS_STRING(peek(0))) {
            ObjString* string = AS_STRING(pop());
            if (index < 0 || index >= string->length) {
              ObjClass* exceptionClass = getNativeClass("IndexOutOfBoundsException");
              throwException(exceptionClass, "String index is out of bound: %d.", index);
            } else {
              char chars[2] = { string->chars[index], '\0' };
              ObjString* element = copyString(chars, 1);
              push(OBJ_VAL(element));
            }
          } else if (IS_ARRAY(peek(0))) {
            ObjArray* array = AS_ARRAY(pop());

            if (index < 0 || index >= array->elements.count) {
              ObjClass* exceptionClass = getNativeClass("IndexOutOfBoundsException");
              throwException(exceptionClass, "Array index is out of bound: %d.", index);
            } else {
              Value element = array->elements.values[index];
              push(element);
            }
          } else {
            ObjClass* exceptionClass = getNativeClass("IllegalArgumentException");
            throwException(exceptionClass, "Only String or Array can have integer subscripts.");
          }
        } else if (IS_STRING(peek(0))) {
          ObjString* key = AS_STRING(pop());

          if (!IS_DICTIONARY(peek(0))) {
            ObjClass* exceptionClass = getNativeClass("IllegalArgumentException");
            throwException(exceptionClass, "Only Dictionary can have string subscripts");
          } else {
            ObjDictionary* dictionary = AS_DICTIONARY(pop());
            Value value;

            if (tableGet(&dictionary->table, key, &value)) {
              push(value);
            } else {
              ObjClass* exceptionClass = getNativeClass("IndexOutOfBoundsException");
              throwException(exceptionClass, "Key wasn't found in Dictionary");
            }
          }
        } else {
          ObjClass* exceptionClass = getNativeClass("IllegalArgumentException");
          throwException(exceptionClass, "Subscript must be an integer or a string.");
        }
        break;
      }
      case OP_SET_SUBSCRIPT: {
        if (IS_INT(peek(1))) {
          if (!IS_ARRAY(peek(2))) {
            runtimeError("Only Array can have integer subscripts.");
            return INTERPRET_RUNTIME_ERROR;
          } 

          Value element = pop();
          double index = AS_INT(pop());
          ObjArray* array = AS_ARRAY(pop());

          if (index < 0 || index >= array->elements.count) {
            ObjClass* exceptionClass = getNativeClass("IndexOutOfBoundsException");
            throwException(exceptionClass, "Array index is out of bound.");
          } else {
            replaceValueArray(&array->elements, index, element);
            push(OBJ_VAL(array));
          }
        } else if (IS_STRING(peek(1))) {
          if (!IS_DICTIONARY(peek(2))) {
            runtimeError("Only Dictionary can have string subscripts.");
            return INTERPRET_RUNTIME_ERROR;
          }

          Value value = pop();
          ObjString* key = AS_STRING(pop());
          ObjDictionary* dictionary = AS_DICTIONARY(pop());
          tableSet(&dictionary->table, key, value);
          push(OBJ_VAL(dictionary));
        } else {
          ObjClass* exceptionClass = getNativeClass("IllegalArgumentException");
          throwException(exceptionClass, "Subscript must be a string.");
        }
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
        defineMethod(READ_STRING());
        break;
      case OP_INVOKE: {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();
        Value receiver = peek(argCount);

        if (!invoke(method, argCount)) {
          if (IS_NIL(receiver)) runtimeError("Calling undefined method '%s' on nil.", method->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
        
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
      case OP_SUPER_INVOKE: {
        ObjString* method = READ_STRING();
        int argCount = READ_BYTE();
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
        ObjString* exceptionClass = READ_STRING();
        uint16_t handlerAddress = READ_SHORT();
        uint16_t finallyAddress = READ_SHORT();
        Value value;
        if (!loadGlobal(exceptionClass, &value)){
          runtimeError("Undefined class %s specified as exception type.", exceptionClass->chars);
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjClass* klass = AS_CLASS(value);
        if (!isClassExtendingSuperclass(klass, vm.exceptionClass)) {
          runtimeError("Expect subclass of clox.std.lang.Exception, but got Class %s.", exceptionClass->chars);
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
        if (!IS_STRING(filePath)) {
            runtimeError("Required file path must be a string.");
            return INTERPRET_RUNTIME_ERROR;
        }

        char* source = readFile(AS_CSTRING(filePath));
        ObjFunction* function = compile(source);
        free(source);

        if (function == NULL) return INTERPRET_COMPILE_ERROR;
        push(OBJ_VAL(function));

        ObjClosure* closure = newClosure(function);
        pop();

        push(OBJ_VAL(closure));
        callClosure(closure, 0);
        frame = &vm.frames[vm.frameCount - 1];
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
        push(result);
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
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  ObjClosure* closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  callClosure(closure, 0);

  return run();
}

