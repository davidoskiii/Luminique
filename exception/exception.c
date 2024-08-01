#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../exception/exception.h"
#include "../native/native.h"
#include "../string/string.h"
#include "../vm/vm.h"

bool propagateException() {
  ObjException* exception = AS_EXCEPTION(peek(0));
  while (vm.frameCount > 0) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    for (int i = frame->handlerCount; i > 0; i--) {
      ExceptionHandler handler = frame->handlerStack[i - 1];
      if (isObjInstanceOf(OBJ_VAL(exception), handler.exceptionClass)) {
        frame->ip = &frame->closure->function->chunk.code[handler.handlerAddress];
        return true;
      } else if (handler.finallyAddress != UINT16_MAX) {
        push(TRUE_VAL);
        frame->ip = &frame->closure->function->chunk.code[handler.finallyAddress];
        return true;
      }
    }
    vm.frameCount--;
  }

  fprintf(stderr, "Unhandled %s::%s: %s\n", exception->obj.klass->namespace_->fullName->chars, exception->obj.klass->name->chars, exception->message->chars);
  ObjArray* stackTrace = exception->stacktrace;
  for (int i = 0; i < stackTrace->elements.count; i++) {
    Value item = stackTrace->elements.values[i];
    fprintf(stderr, "    %s.\n", AS_CSTRING(item));
  }
  fflush(stderr);
  return false;
}

void pushExceptionHandler(ObjClass* exceptionClass, uint16_t handlerAddress, uint16_t finallyAddress) {
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

ObjArray* getStackTrace() {
  ObjArray* stackTrace = newArray();
  push(OBJ_VAL(stackTrace));
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    char stackTraceBuffer[UINT8_MAX];
    CallFrame* frame = &vm.frames[i];
    ObjModule* module = vm.currentModule;
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    uint32_t line = function->chunk.lines[instruction];

    uint8_t length = snprintf(stackTraceBuffer, UINT8_MAX, "in %s() from %s at line %d",
        function->name == NULL ? "script" : function->name->chars, module->path->chars, line);
    ObjString* stackElement = copyString(stackTraceBuffer, length);
    writeValueArray(&stackTrace->elements, OBJ_VAL(stackElement));
  }
  pop();
  return stackTrace;
}

ObjException* throwException(ObjClass* exceptionClass, const char* format, ...) {
  char chars[UINT8_MAX];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(chars, UINT8_MAX, format, args);
  va_end(args);
  ObjString* message = copyString(chars, length);
  ObjArray* stacktrace = getStackTrace();

  ObjException* exception = newException(message, exceptionClass);
  exception->stacktrace = stacktrace;
  push(OBJ_VAL(exception));
  if (!propagateException()) exit(70);
  else return exception;
}

ObjException* throwNativeException(const char* namespace_, const char* exceptionClassName, const char* format, ...) { 
  ObjClass* exceptionClass = getNativeClass(namespace_, exceptionClassName);
  char chars[UINT8_MAX];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(chars, UINT8_MAX, format, args);
  va_end(args);
  ObjString* message = copyString(chars, length);
  ObjArray* stacktrace = getStackTrace();

  ObjException* exception = newException(message, exceptionClass);
  exception->stacktrace = stacktrace;
  push(OBJ_VAL(exception));
  if (!propagateException()) {
    exit(70); return exception;
  }
  else return exception;
}

ObjException* createException(ObjClass* exceptionClass, const char* format, ...) {
  char chars[UINT8_MAX];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(chars, UINT8_MAX, format, args);
  va_end(args);
  ObjString* message = copyString(chars, length);
  ObjArray* stacktrace = getStackTrace();

  ObjException* exception = newException(message, exceptionClass);
  exception->stacktrace = stacktrace;
  return exception;
}
