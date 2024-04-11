#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "native.h"
#include "../assert/assert.h"
#include "../memory/memory.h"
#include "../string/string.h"

static unsigned int seed = 0;

void defineNativeFunction(const char* name, int arity, NativeFunction function) {
  ObjString* functionName = copyString(name, (int)strlen(name));
  push(OBJ_VAL(functionName));
  push(OBJ_VAL(newNativeFunction(functionName, arity, function)));
  tableSet(&vm.globalValues, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

ObjClass* defineNativeClass(const char* name) {
  ObjString* className = copyString(name, (int)strlen(name));
  push(OBJ_VAL(className));
  ObjClass* nativeClass = newClass(className);
  nativeClass->isNative = true;
  push(OBJ_VAL(nativeClass));
  tableSet(&vm.globalValues, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
  return nativeClass;
}

void defineNativeMethod(ObjClass* klass, const char* name, int arity, NativeMethod method) {
  ObjString* methodName = copyString(name, (int)strlen(name));
  push(OBJ_VAL(methodName));
  ObjNativeMethod* nativeMethod = newNativeMethod(klass, methodName, arity, method);
  push(OBJ_VAL(nativeMethod));
  tableSet(&klass->methods, methodName, OBJ_VAL(nativeMethod));
  pop();
  pop();
}


ObjClass* getNativeClass(const char* name) {
  Value klass;
  tableGet(&vm.globalValues, newString(name), &klass);
  if (!IS_CLASS(klass)) {
    runtimeError("Native class %s is undefined.", name);
    exit(70);
  }
  return AS_CLASS(klass);
}

ObjNativeFunction* getNativeFunction(const char* name) {
  Value function;
  tableGet(&vm.globalValues, newString(name), &function);
  if (!IS_NATIVE_FUNCTION(function)) {
    runtimeError("Native function %s is undefined.", name);
    exit(70);
  }
  return AS_NATIVE_FUNCTION(function);
}

ObjNativeMethod* getNativeMethod(ObjClass* klass, const char* name) {
  Value method;
  tableGet(&klass->methods, newString(name), &method);
  if (!IS_NATIVE_METHOD(method)) {
    runtimeError("Native method %s::%s is undefined.", klass->name->chars, name);
    exit(70);
  }
  return AS_NATIVE_METHOD(method);
}

ObjClass* defineNativeException(const char* name, ObjClass* superClass) {
  ObjClass* exceptionClass = defineNativeClass(name);
  bindSuperclass(exceptionClass, superClass);
  return exceptionClass;
}

NATIVE_FUNCTION(clock) {
  assertArgCount("clock()", 0, argCount);
  RETURN_NUMBER((double)clock() / CLOCKS_PER_SEC);
}

NATIVE_FUNCTION(print) {
  assertArgCount("print(message)", 1, argCount);

  printValue(args[0]);

  RETURN_NIL;
}

NATIVE_FUNCTION(dateNow) {
  assertArgCount("dateNow()", 0, argCount);
  time_t nowTime;
  time(&nowTime);
  struct tm now;
  localtime_r(&nowTime, &now);
  ObjInstance* date = newInstance(getNativeClass("Date"));
  setObjProperty(date, "year", INT_VAL(1900 + now.tm_year));
  setObjProperty(date, "month", INT_VAL(1 + now.tm_mon));
  setObjProperty(date, "day", INT_VAL(now.tm_mday));
  RETURN_OBJ(date);
}

NATIVE_FUNCTION(dateTimeNow) {
  assertArgCount("dateTimeNow()", 0, argCount);
  time_t nowTime;
  time(&nowTime);
  struct tm now;
  localtime_r(&nowTime, &now);
  ObjInstance* date = newInstance(getNativeClass("DateTime"));
  setObjProperty(date, "year", INT_VAL(1900 + now.tm_year));
  setObjProperty(date, "month", INT_VAL(1 + now.tm_mon));
  setObjProperty(date, "day", INT_VAL(now.tm_mday));
  setObjProperty(date, "hour", INT_VAL(now.tm_hour));
  setObjProperty(date, "minute", INT_VAL(now.tm_min));
  setObjProperty(date, "second", INT_VAL(now.tm_sec));
  RETURN_OBJ(date);
}

NATIVE_FUNCTION(println) {
  assertArgCount("println(message)", 1, argCount);

  printValue(args[0]);
  printf("\n");

  RETURN_NIL;
}

NATIVE_FUNCTION(scanln) {
  assertArgCount("scanln(prompt)", 1, argCount);
  assertArgIsString("scanln(prompt)", args, 0);

  ObjString* prompt = AS_STRING(args[0]);
  printValue(OBJ_VAL(prompt));

  char inputBuffer[2048];
  if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
    size_t length = strlen(inputBuffer);
    if (length > 0 && inputBuffer[length - 1] == '\n') {
      inputBuffer[length - 1] = '\0';
    }

    RETURN_OBJ(copyString(inputBuffer, (int)strlen(inputBuffer)));
  } else {
    runtimeError("Error reading input");
    exit(70);
  }
}

NATIVE_FUNCTION(require) {
  assertArgCount("require(filePath)", 1, argCount);
  assertArgIsString("require(filePath)", args, 0);
  char* filePath = AS_CSTRING(args[0]);

  InterpretResult result = loadSourceFile(filePath);
  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
  RETURN_NIL;
}

InterpretResult loadSourceFile(const char* filePath) {
  char* source = readFile(filePath);
  InterpretResult result = interpret(source);
  free(source);
  return result;
}

void initNatives() {
  DEF_FUNCTION(clock, 0);
  DEF_FUNCTION(dateNow, 0);
  DEF_FUNCTION(dateTimeNow, 0);
  DEF_FUNCTION(print, 1);
  DEF_FUNCTION(println, 1);
  DEF_FUNCTION(scanln, 1);
  DEF_FUNCTION(require, 1);
}

