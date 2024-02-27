#include <stdio.h>
#include <stdlib.h>

#include "std.h"

static Value printlnNativeMethod(Value receiver, int argCount, Value* args) {
	if (argCount != 1) {
		runtimeError("method 'println' expects 1 argument but got %d.", argCount);
		exit(70);
	}

  printValue(receiver);

  return NIL_VAL;
}

static Value toStringNativeMethod(Value receiver, int argCount, Value* args) {
	if (argCount > 0) {
		runtimeError("method 'toString' expects 1 argument but got %d.", argCount);
		exit(70);
	}

	if (IS_BOOL(receiver)) {
    if (AS_BOOL(args[0])) {
		  return OBJ_VAL(copyString("true", 4));
    } else {
		  return OBJ_VAL(copyString("false", 5));
    }
	} else if (IS_NIL(receiver)) {
		return OBJ_VAL(copyString("nil", 3));
	} else if (IS_NUMBER(receiver)) {
		char chars[24];
		int length = snprintf(chars, 24, "%.14g", AS_NUMBER(args[0]));
		return OBJ_VAL(copyString(chars, length));
	} else {
		return OBJ_VAL(copyString("Object", 6));
	}
}

void initStd(){
	ObjClass* objectClass = defineNativeClass("System");
	defineNativeMethod(objectClass, "println", printlnNativeMethod);
	defineNativeMethod(objectClass, "toString", toStringNativeMethod);
}
