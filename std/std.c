#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "std.h"

static Value printlnNativeMethod(Value receiver, int argCount, Value* args) {
	if (argCount != 1) {
		runtimeError("Method 'println' expects 1 argument but got %d.", argCount);
		exit(70);
	}

  printValue(args[0]);
  printf("\n");

  return NIL_VAL;
}

static Value toStringNativeMethod(Value receiver, int argCount, Value* args) {
	if (argCount != 1) {
		runtimeError("Method 'toString' expects 1 argument but got %d.", argCount);
		exit(70);
	}

	if (IS_BOOL(args[0])) {
    if (AS_BOOL(args[0])) {
		  return OBJ_VAL(copyString("true", 4));
    } else {
		  return OBJ_VAL(copyString("false", 5));
    }
	} else if (IS_NIL(args[0])) {
		return OBJ_VAL(copyString("nil", 3));
	} else if (IS_NUMBER(args[0])) {
		char chars[24];
		int length = snprintf(chars, 24, "%.14g", AS_NUMBER(args[0]));
		return OBJ_VAL(copyString(chars, length));
	} else {
		return OBJ_VAL(copyString("Object", 6));
	}
}

static Value scanlnNativeMethod(Value reciver, int argCount, Value* args) {
  if (argCount != 1) {
		runtimeError("Method 'scanln' expects 1 argument but got %d.", argCount);
		exit(70);
  }

  if (!IS_STRING(args[0])) {
    runtimeError("The argument must be of type string");
    exit(70);
  }

  ObjString* prompt = AS_STRING(args[0]);
  printValue(OBJ_VAL(prompt));

  char inputBuffer[2048];
  if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
    size_t length = strlen(inputBuffer);
    if (length > 0 && inputBuffer[length - 1] == '\n') {
      inputBuffer[length - 1] = '\0';
    }

    return OBJ_VAL(copyString(inputBuffer, (int)strlen(inputBuffer)));
  } else {
    runtimeError("Error reading input");
    exit(70);
  }
}

void initStd(){
	ObjClass* systemClass = defineNativeClass("System");
	ObjClass* mathClass = defineNativeClass("Math");
	defineNativeMethod(systemClass, "println", printlnNativeMethod);
	defineNativeMethod(systemClass, "toString", toStringNativeMethod);
  defineNativeMethod(systemClass, "scanln", scanlnNativeMethod);
}
