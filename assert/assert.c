#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "../native/native.h"

void assertArgCount(const char* method, uint16_t expectedCount, uint16_t actualCount) {
  if (expectedCount != actualCount) {
    runtimeError("Method %s expects %d argument(s) but got %d instead.", method, expectedCount, actualCount);
    exit(70);
  }
}

void assertArgIsBool(const char* method, Value* args, int index) {
  if (!IS_BOOL(args[index])) {
    runtimeError("Method %s expects argument %d to be a boolean value.", method, index + 1);
    exit(70);
  }
}

void assertArgIsClass(const char* method, Value* args, int index) {
  if (!IS_CLASS(args[index])) {
    runtimeError("Method %s expects argument %d to be a class.", method, index + 1);
    exit(70);
  }
}

void assertArgIsClosure(const char* method, Value* args, int index) {
  if (!IS_CLOSURE(args[index])) {
    runtimeError("Method %s expects argument %d to be a closure.", method, index + 1);
    exit(70);
  }
}

void assertArgIsDictionary(const char* method, Value* args, int index) {
  if (!IS_DICTIONARY(args[index])) {
    runtimeError("Method %s expects argument %d to be a dictionary.", method, index + 1);
    exit(70);
  }
}

void assertArgIsFloat(const char* method, Value* args, int index) {
  if (!IS_FLOAT(args[index])) {
    runtimeError("Method %s expects argument %d to be a floating point number.", method, index + 1);
    exit(70);
  }
}

void assertArgIsInt(const char* method, Value* args, int index) {
  if (!IS_INT(args[index])) {
    runtimeError("Method %s expects argument %d to be an integer number.", method, index + 1);
    exit(70);
  }
}

void assertArgIsArray(const char* method, Value* args, int index) {
  if (!IS_ARRAY(args[index])) {
    runtimeError("Method %s expects argument %d to be a list.", method, index + 1);
    exit(70);
  }
}

void assertArgIsNumber(const char* method, Value* args, int index) {
  if (!IS_NUMBER(args[index])) {
    runtimeError("Method %s expects argument %d to be a number.", method, index + 1);
    exit(70);
  }
}

void assertArgIsString(const char* method, Value* args, int index) {
  if (!IS_STRING(args[index])) {
    runtimeError("Method %s expects argument %d to be a string.", method, index + 1);
    exit(70);
  }
}

void assertArgIsException(const char* method, Value* args, int index) {
  if (!IS_INSTANCE(args[index]) && !isObjInstanceOf(args[index], vm.exceptionClass)) {
    runtimeError("Method %s expects argument %d to be an exception.", method, index + 1);
    exit(70);
  }
}

void assertArgIsFile(const char* method, Value* args, int index) {
  if (!IS_INSTANCE(args[index]) && !isObjInstanceOf(args[index], vm.fileClass)) {
    runtimeError("Method %s expects argument %d to be a file.", method, index + 1);
    exit(70);
  }
}


void assertArgIsNamespace(const char* method, Value* args, int index) {
  if (!IS_NAMESPACE(args[index]) && !isObjInstanceOf(args[index], vm.namespaceClass)) {
    runtimeError("method %s expects argument %d to be a namespace.", method, index + 1);
    exit(70);
  }
}

void assertArgIsMethod(const char* method, Value* args, int index) { 
  if (!IS_NAMESPACE(args[index]) && !isObjInstanceOf(args[index], vm.methodClass)) {
    runtimeError("method %s expects argument %d to be a method.", method, index + 1);
    exit(70);
  }
}

void assertArgInstanceOfEither(const char* method, Value* args, int index, const char* namespaceName, const char* className, const char* namespaceName2, const char* className2) {
  if (!isObjInstanceOf(args[index], getNativeClass(namespaceName, className)) && !isObjInstanceOf(args[index], getNativeClass(namespaceName2, className2))) {
    runtimeError("method %s expects argument %d to be an instance of class %s or %s but got %s.", method, index + 1, className, className2, getObjClass(args[index])->name->chars);
    exit(70);
  }   
}

void assertIntWithinRange(const char* method, int value, int min, int max, int index){
  if (value < min || value > max) {
    runtimeError("Method %s expects argument %d to be an integer within range %d to %d but got %d.", method, index + 1, min, max, value);
    exit(70);
  }
}

void assertNumberNonNegative(const char* method, double number, int index) {
  if (number < 0) {
    if (index < 0) runtimeError("Method %s expects receiver to be a non negative number but got %g.", method, number);
    else runtimeError("Method %s expects argument %d to be a non negative number but got %g.", method, index + 1, number);
    exit(70);
  }
}

void assertNumberNonZero(const char* method, double number, int index) {
  if (number == 0) {
    if (index < 0) runtimeError("Method %s expects receiver to be a non-zero number but got %g.", method, number);
    else runtimeError("Method %s expects argument %d to be a non-zero number but got %g.", method, index + 1, number);
    exit(70);
  }
}

void assertNumberPositive(const char* method, double number, int index) {
  if (number <= 0) {
    if (index < 0) runtimeError("Method %s expects receiver to be a positive number but got %g.", method, number);
    else runtimeError("Method %s expects argument %d to be a positive number but got %g.", method, index + 1, number);
    exit(70);
  }
}

void assertNumberWithinRange(const char* method, double value, double min, double max, int index) {
  if (value < min || value > max) {
    runtimeError("Method %s expects argument %d to be a number within range %g to %g but got %g.", method, index + 1, min, max, value);
    exit(70);
  }
}

void assertObjInstanceOfClass(const char* method, Value arg, char* namespaceName, char* className, int index) {
  if (!isObjInstanceOf(arg, getNativeClass(namespaceName, className))) {
    if (index < 0) {
      runtimeError("Method %s expects receiver to be an instance of class %s but got %s.", className, getObjClass(arg)->name->chars);
    } else {
      runtimeError("Method %s expects argument %d to be an instance of class %s but got %s.", method, index + 1, className, getObjClass(arg)->name->chars);
    }
    exit(70);
  }
}

void assertError(const char* message) {
  runtimeError(message);
  exit(70);
}
