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

void defineNativeFunction(const char* name, int arity, NativeFn function) {
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
  struct tm *now = localtime(&nowTime);
  ObjInstance* date = newInstance(getNativeClass("Date"));
  setObjProperty(date, "year", INT_VAL(1900 + now->tm_year));
  setObjProperty(date, "month", INT_VAL(1 + now->tm_mon));
  setObjProperty(date, "day", INT_VAL(now->tm_mday));
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

/* static bool randomNative(int argCount, Value* args) {
  if (seed == 0) {
    seed = (unsigned int)time(NULL);
    srand(seed);
  }
  
  if (argCount == 2) {
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
      args[-1] = OBJ_VAL(copyFormattedString("Arguments must be numbers"));
      return false;
    }

    int min = AS_NUMBER(args[0]);
    int max = AS_NUMBER(args[1]);

    if (min >= max) {
      args[-1] = OBJ_VAL(copyFormattedString("Min value must be less than max value"));
      return false;
    }

    int randomValue = (rand() % (max - min + 1)) + min;
    args[-1] = NUMBER_VAL(randomValue);
    return true;
  } else {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 2 arguments but got %d", argCount));
    return false;
  }
}

static bool currentTimeNative(int argCount, Value* args) {
  time_t currentTime = time(NULL);
  args[-1] = NUMBER_VAL((double)currentTime);
  return true;
}

static bool sqrtNative(int argCount, Value* args) {
  if (argCount != 1) {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
    return false;
  }

  if (!IS_NUMBER(args[0])) {
    args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
    return false;
  }

  double value = AS_NUMBER(args[0]);
  args[-1] = NUMBER_VAL(sqrt(value));
  return true;
}

static bool absNative(int argCount, Value* args) {
  if (argCount != 1) {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
    return false;
  }

  if (!IS_NUMBER(args[0])) {
    args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
    return false;
  }

  double value = AS_NUMBER(args[0]);
  args[-1] = NUMBER_VAL(fabs(value));
  return true;
}

static bool ceilNative(int argCount, Value* args) {
    if (argCount != 1) {
        args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
        return false;
    }

    if (!IS_NUMBER(args[0])) {
        args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
        return false;
    }

    double value = AS_NUMBER(args[0]);
    args[-1] = NUMBER_VAL(ceil(value));
    return true;
}

static bool fabsNative(int argCount, Value* args) {
    if (argCount != 1) {
        args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
        return false;
    }

    if (!IS_NUMBER(args[0])) {
        args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
        return false;
    }

    double value = AS_NUMBER(args[0]);
    args[-1] = NUMBER_VAL(fabs(value));
    return true;
}

static bool factorialNative(int argCount, Value* args) {
    if (argCount != 1) {
        args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
        return false;
    }

    if (!IS_NUMBER(args[0])) {
        args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
        return false;
    }

    int n = AS_NUMBER(args[0]);
    if (n < 0 || n != (int)n) {
        args[-1] = OBJ_VAL(copyFormattedString("Argument must be a non-negative integer"));
        return false;
    }

    int result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }

    args[-1] = NUMBER_VAL((double)result);
    return true;
}

static bool fmodNative(int argCount, Value* args) {
    if (argCount != 2) {
        args[-1] = OBJ_VAL(copyFormattedString("Expected 2 arguments but got %d", argCount));
        return false;
    }

    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        args[-1] = OBJ_VAL(copyFormattedString("Arguments must be numbers"));
        return false;
    }

    double x = AS_NUMBER(args[0]);
    double y = AS_NUMBER(args[1]);

    double result = fmod(x, y);
    args[-1] = NUMBER_VAL(result);
    return true;
}

static bool inputNative(int argCount, Value* args) {
  if (argCount != 1) {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
    return false;
  }

  if (!IS_STRING(args[0])) {
    args[-1] = OBJ_VAL(copyFormattedString("Argument must be a string"));
    return false;
  }

  ObjString* prompt = AS_STRING(args[0]);
  printValue(OBJ_VAL(prompt));

  char inputBuffer[1024];
  if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
    size_t length = strlen(inputBuffer);
    if (length > 0 && inputBuffer[length - 1] == '\n') {
      inputBuffer[length - 1] = '\0';
    }

    args[-1] = OBJ_VAL(copyString(inputBuffer, (int)strlen(inputBuffer)));
    return true;
  } else {
    args[-1] = OBJ_VAL(copyFormattedString("Error reading input"));
    return false;
  }
}

static bool numNative(int argCount, Value* args) {
  if (argCount != 1) {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
    return false;
  }

  if (IS_NUMBER(args[0])) {
    // If it's already a number, return it as is
    args[-1] = args[0];
    return true;
  }

  if (!IS_STRING(args[0])) {
    args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number or string"));
    return false;
  }

  ObjString* str = AS_STRING(args[0]);
  char* endptr;
  double num = strtod(str->chars, &endptr);

  if (*endptr != '\0') {
    args[-1] = OBJ_VAL(copyFormattedString("Invalid number format"));
    return false;
  }

  args[-1] = NUMBER_VAL(num);
  return true;
} */

void initNatives() {
  DEF_FUNCTION(clock, 0);
  DEF_FUNCTION(dateNow, 0);
  DEF_FUNCTION(print, 1);
  DEF_FUNCTION(println, 1);
  DEF_FUNCTION(scanln, 1);
//  defineNativeFunction("randint", randomNative);
//  defineNativeFunction("currentTime", currentTimeNative);
//  defineNativeFunction("sqrt", sqrtNative);
//  defineNativeFunction("abs", absNative);
//  defineNativeFunction("ceil", ceilNative);
//  defineNativeFunction("fabs", fabsNative);
//  defineNativeFunction("factorial", factorialNative);
//  defineNativeFunction("fmod", fmodNative);
//  defineNativeFunction("scanf", inputNative);
//  defineNativeFunction("num", numNative);
}

