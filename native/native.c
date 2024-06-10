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
  tableSet(&vm.currentNamespace->values, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

ObjClass* defineNativeClass(const char* name) {
  ObjString* className = copyString(name, (int)strlen(name));
  push(OBJ_VAL(className));
  ObjClass* nativeClass = newClass(className);
  nativeClass->isNative = true;
  push(OBJ_VAL(nativeClass));
  tableSet(&vm.currentNamespace->values, AS_STRING(vm.stack[0]), vm.stack[1]);
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

void defineNativeConstant(ObjNamespace* namespace_, const char* name, Value value) {
  ObjString* variableName = copyString(name, (int)strlen(name));
  push(OBJ_VAL(variableName));
  push(value);
  tableSet(&namespace_->values, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

ObjNamespace* defineNativeNamespace(const char* name, ObjNamespace* enclosing) {
  ObjString* shortName = newString(name);
  push(OBJ_VAL(shortName));
  ObjNamespace* nativeNamespace = newNamespace(shortName, enclosing);
  push(OBJ_VAL(nativeNamespace));
  tableSet(&vm.namespaces, nativeNamespace->fullName, OBJ_VAL(nativeNamespace));
  tableSet(&enclosing->values, nativeNamespace->shortName, OBJ_VAL(nativeNamespace));
  pop();
  pop();
  return nativeNamespace;
}

ObjClass* getNativeClass(const char* namespaceName, const char* className) {
  ObjNamespace* namespace = getNativeNamespace(namespaceName);
  Value klass;
  tableGet(&namespace->values, newString(className), &klass);
  if (!IS_CLASS(klass)) {
    runtimeError("Class %s.%s is undefined.", namespaceName, className);
    exit(70);
  }
  return AS_CLASS(klass);
}

ObjNativeFunction* getNativeFunction(const char* name) {
  Value function;
  tableGet(&vm.rootNamespace->values, newString(name), &function);
  if (!IS_NATIVE_FUNCTION(function)) {
    runtimeError("Native function %s is undefined.", name);
    exit(70);
  }
  return AS_NATIVE_FUNCTION(function);
}

ObjNamespace* getNativeNamespace(const char* name) {
  Value namespace;
  tableGet(&vm.namespaces, newString(name), &namespace);
  if (!IS_NAMESPACE(namespace)) {
    runtimeError("Namespace %s is undefined.", name);
    exit(70);
  }
  return AS_NAMESPACE(namespace);
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


void initNativePackage(const char* filePath) {
  char* source = readFile(filePath);
  interpret(source);
  free(source);
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

NATIVE_FUNCTION(num) {
  assertArgCount("num(value)", 1, argCount);
  if (!IS_STRING(args[0]) && !IS_NUMBER(args[0])) {
    runtimeError("Method num(value) expects argument 1 to be a string or a number.", 1);
    exit(70);
  }

  if (IS_NUMBER(args[0])) {
    return args[0];
  }

  ObjString* stringValue = AS_STRING(args[0]);

  if (stringValue->length == 0) {
    RETURN_INT(0);
  }

  const char* str = stringValue->chars;

  char* endptr;
  double value = strtod(str, &endptr);

  if (endptr == str) {
    RETURN_NIL;
  } else {
    RETURN_NUMBER(value);
  }
}

NATIVE_FUNCTION(int) {
  assertArgCount("int(value)", 1, argCount);
  if (!IS_STRING(args[0]) && !IS_NUMBER(args[0])) {
    runtimeError("Method int(value) expects argument 1 to be a string or a number.", 1);
    exit(70);
  }

  if (IS_NUMBER(args[0])) {
    RETURN_INT(AS_NUMBER(args[0]));
  }

  ObjString* stringValue = AS_STRING(args[0]);

  if (stringValue->length == 0) {
    RETURN_INT(0);
  }

  const char* str = stringValue->chars;

  char* endptr;
  double value = strtod(str, &endptr);

  if (endptr == str) {
    RETURN_NIL;
  } else {
    RETURN_INT(value);
  }
}

NATIVE_FUNCTION(float) {
  assertArgCount("float(value)", 1, argCount);
  if (!IS_STRING(args[0]) && !IS_NUMBER(args[0])) {
    runtimeError("Method float(value) expects argument 1 to be a string or a number.", 1);
    exit(70);
  }

  if (IS_NUMBER(args[0])) {
    RETURN_FLOAT(AS_NUMBER(args[0]));
  }

  ObjString* stringValue = AS_STRING(args[0]);

  if (stringValue->length == 0) {
    RETURN_INT(0);
  }

  const char* str = stringValue->chars;

  char* endptr;
  double value = strtod(str, &endptr);

  if (endptr == str) {
    RETURN_NIL;
  } else {
    RETURN_FLOAT(value);
  }
}

NATIVE_FUNCTION(str) {
  assertArgCount("str(value)", 1, argCount);
  Value toStringMethod = getObjMethod(args[0], "__str__");

  Value str = callReentrant(args[0], toStringMethod);
  RETURN_STRING_FMTL(AS_CSTRING(str));
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

void loadSourceFile(const char* filePath) {
  char* source = readFile(filePath);
  interpret(source);
  free(source);
}

void initNatives() {
  vm.currentNamespace = vm.rootNamespace;

  DEF_FUNCTION(print, 1);
  DEF_FUNCTION(println, 1);
  DEF_FUNCTION(scanln, 1);
  DEF_FUNCTION(num, 1);
  DEF_FUNCTION(int, 1);
  DEF_FUNCTION(float, 1);
  DEF_FUNCTION(str, 1);
}
