#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "native.h"
#include "../arrays/arrays.h"
#include "../assert/assert.h"
#include "../memory/memory.h"

static unsigned int seed = 0;

void defineNativeFunction(const char* name, NativeFn function) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
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

void defineNativeMethod(ObjClass* klass, const char* name, NativeMethod method) {
	ObjNativeMethod* nativeMethod = newNativeMethod(method);
	push(OBJ_VAL(nativeMethod));
	ObjString* methodName = copyString(name, (int)strlen(name));
	push(OBJ_VAL(methodName));
	tableSet(&klass->methods, methodName, OBJ_VAL(nativeMethod));
	pop();
	pop();
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

NATIVE_FUNCTION(append) {
  assertArgCount("append(array, element)", 2, argCount);
  assertArgIsArray("append(array, element)", args, 0);
  writeValueArray(&AS_ARRAY(args[0])->elements, args[1]);
  RETURN_NIL;
}

NATIVE_FUNCTION(clear) {
  assertArgCount("clear(array)", 1, argCount);
  assertArgIsArray("clear(array)", args, 0);
  freeValueArray(&AS_ARRAY(args[0])->elements);
  RETURN_NIL;
}

NATIVE_FUNCTION(clone) {
  assertArgCount("clone(array)", 1, argCount);
  assertArgIsArray("clone(array)", args, 0);
  RETURN_OBJ(copyArray(AS_ARRAY(args[0])->elements));
}

NATIVE_FUNCTION(indexOf) {
  assertArgCount("indexOf(array, value)", 2, argCount);
  assertArgIsArray("indexOf(array, value)", args, 0);

  ObjArray* array = AS_ARRAY(args[0]);
  if (array->elements.count == 0) {
    RETURN_NIL;
  }

  RETURN_NUMBER(arrayIndexOf(array, args[1]));
}

NATIVE_FUNCTION(length) {
  assertArgCount("length(array)", 1, argCount);
  assertArgIsArray("length(array)", args, 0);
	RETURN_NUMBER(AS_ARRAY(args[0])->elements.count);
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
  DEF_FUNCTION(clock);
  DEF_FUNCTION(print);
  DEF_FUNCTION(println);
  DEF_FUNCTION(scanln);
  DEF_FUNCTION(append);
  DEF_FUNCTION(clear);
  DEF_FUNCTION(clone);
  DEF_FUNCTION(indexOf);
  DEF_FUNCTION(length);
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

