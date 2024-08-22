#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "../native/native.h"
#include "../vm/vm.h"

Value assertArgCount(const char* method, uint16_t expectedCount, uint16_t actualCount) {
  if (expectedCount != actualCount) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects %d argument(s) but got %d instead.", 
      method, expectedCount, actualCount);
  }
  RETURN_NIL;
}

Value assertArgIsBool(const char* method, Value* args, int index) {
  if (!IS_BOOL(args[index])) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a boolean value.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsClass(const char* method, Value* args, int index) {
  if (!IS_CLASS(args[index])) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a class.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsClosure(const char* method, Value* args, int index) {
  if (!IS_CLOSURE(args[index])) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a closure.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsTimer(const char* method, Value* args, int index) {
  if (!IS_TIMER(args[index]) && !isObjInstanceOf(args[index], vm.timerClass)) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a timer.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsDictionary(const char* method, Value* args, int index) {
  if (!IS_DICTIONARY(args[index])) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a dictionary.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsFloat(const char* method, Value* args, int index) {
  if (!IS_FLOAT(args[index])) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a floating point number.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsInt(const char* method, Value* args, int index) {
  if (!IS_INT(args[index])) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be an integer number.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsArray(const char* method, Value* args, int index) {
  if (!IS_ARRAY(args[index])) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be an array.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsNumber(const char* method, Value* args, int index) {
  if (!IS_NUMBER(args[index])) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a number.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsString(const char* method, Value* args, int index) {
  if (!IS_STRING(args[index])) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a string.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsException(const char* method, Value* args, int index) {
  if (!IS_EXCEPTION(args[index]) && !isObjInstanceOf(args[index], vm.exceptionClass)) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be an exception.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsGenerator(const char* method, Value* args, int index) {
  if (!IS_GENERATOR(args[index]) && !isObjInstanceOf(args[index], vm.generatorClass)) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a generator.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsCallable(const char* method, Value* args, int index) {
  char* namespace = "luminique::std::lang";
  if (!isObjInstanceOf(args[index], getNativeClass(namespace, "Function")) && 
      !isObjInstanceOf(args[index], getNativeClass(namespace, "BoundMethod"))) {
      THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
        "Method %s expects argument %d to be an instance of a callable class.",
        method, index + 1); 
  }   
  RETURN_NIL;
}

Value assertArgIsFile(const char* method, Value* args, int index) {
  if (!IS_INSTANCE(args[index]) && !isObjInstanceOf(args[index], vm.fileClass)) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a file.", 
      method, index + 1);
  }
  RETURN_NIL;
}


Value assertArgIsNamespace(const char* method, Value* args, int index) {
  if (!IS_NAMESPACE(args[index]) && !isObjInstanceOf(args[index], vm.namespaceClass)) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a namespace.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsPromise(const char* method, Value* args, int index) {
  if (!IS_PROMISE(args[index]) && !isObjInstanceOf(args[index], vm.promiseClass)) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a promise.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgIsMethod(const char* method, Value* args, int index) { 
  if (!IS_NAMESPACE(args[index]) && !isObjInstanceOf(args[index], vm.methodClass)) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects argument %d to be a method.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertIsNumber(const char* method, Value number) {
  if (!IS_NUMBER(number)){
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Method %s expects value %d to be a number.", 
      method, index + 1);
  }
  RETURN_NIL;
}

Value assertArgInstanceOfEither(const char* method, Value* args, int index, const char* namespaceName, const char* className, const char* namespaceName2, const char* className2) {
  if (!isObjInstanceOf(args[index], getNativeClass(namespaceName, className)) && 
    !isObjInstanceOf(args[index], getNativeClass(namespaceName2, className2))) {
      THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
        "Method %s expects argument %d to be an instance of class %s or %s but got %s.",
        method, index + 1, className, className2, getObjClass(args[index])->name->chars); 
  }   
  RETURN_NIL;
}

Value assertIntWithinRange(const char* method, int value, int min, int max, int index){
  if (value < min || value > max) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
      "Method %s expects argument %d to be an integer within range %d to %d but got %d.", 
      method, index + 1, min, max, value); 
  }
  RETURN_NIL;
}

Value assertNumberNonNegative(const char* method, double number, int index) {
  if (number < 0) {
    if (index < 0) THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
                                      "Method %s expects receiver to be a non negative number but got %g.", method, number);
    else THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
                            "Method %s expects argument %d to be a non negative number but got %g.", method, index + 1, number);
  }
  RETURN_NIL;
}

Value assertNumberNonZero(const char* method, double number, int index) {
  if (number == 0) {
    if (index < 0) THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
                                      "Method %s expects receiver to be a non zero number but got %g.", method, number);
    else THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
                            "Method %s expects argument %d to be a non zero number but got %g.", method, index + 1, number);
  }
  RETURN_NIL;
}

Value assertNumberPositive(const char* method, double number, int index) {
  if (number <= 0) {
    if (index < 0) THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
                                      "Method %s expects receiver to be a positive number but got %g.", method, number);
    else THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
                            "Method %s expects argument %d to be a positive number but got %g.", method, index + 1, number);
  }
  RETURN_NIL;
}

Value assertNumberWithinRange(const char* method, double value, double min, double max, int index) {
  if (value < min || value > max) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
      "Method %s expects argument %d to be a number within range %g to %g but got %g.", method, index + 1, min, max, value); 
  }
  RETURN_NIL;
}

Value assertObjInstanceOfClass(const char* method, Value arg, char* namespaceName, char* className, int index) {
  if (!isObjInstanceOf(arg, getNativeClass(namespaceName, className))) {
    if (index < 0) {
      THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
        "Method %s expects receiver to be an instance of class %s but got %s.", className, getObjClass(arg)->name->chars); 
    } else {
      THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, 
        "Method %s expects argument %d to be an instance of class %s but got %s.", method, index + 1, className, getObjClass(arg)->name->chars); 
    }
  }
  RETURN_NIL;
}

Value assertError(const char* message) {
  THROW_EXCEPTION(luminique::std::lang, AssertException, message);
}
