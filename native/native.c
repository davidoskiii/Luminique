#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "native.h"
#include "../assert/assert.h"
#include "../interceptor/interceptor.h"
#include "../memory/memory.h"
#include "../string/string.h"
#include "../vm/vm.h"

static unsigned int seed = 0;

void defineNativeFunction(const char* name, int arity, bool isAsync, NativeFunction function) {
  ObjString* functionName = copyString(name, (int)strlen(name));
  push(OBJ_VAL(functionName));
  push(OBJ_VAL(newNativeFunction(functionName, arity, isAsync, function)));
  tableSet(&vm.currentNamespace->values, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void defineNativeInterceptor(ObjClass* klass, InterceptorType type, int arity, NativeMethod method) {
  switch (type) {
    case INTERCEPTOR_INIT:
      defineNativeMethod(klass, "__init__", arity, false, method);
      break;
    case INTERCEPTOR_NEW:
      defineNativeMethod(klass, "__new__", arity, false, method);
      break;
    case INTERCEPTOR_BEFORE_GET_PROPERTY:
      defineNativeMethod(klass, "__beforeGetProperty__", 1, false, method);
      break;
    case INTERCEPTOR_AFTER_GET_PROPERTY:
      defineNativeMethod(klass, "__afterGetProperty__", 2, false, method);
      break;
    case INTERCEPTOR_BEFORE_SET_PROPERTY:
      defineNativeMethod(klass, "__beforeSetProperty__", 2, false, method);
      break;
    case INTERCEPTOR_AFTER_SET_PROPERTY: 
      defineNativeMethod(klass, "__afterSetProperty__", 2, false, method);
      break;
    case INTERCEPTOR_BEFORE_INVOKE_METHOD:
      defineNativeMethod(klass, "__beforeInvokeMethod__", 2, false, method);
      break;
    case INTERCEPTOR_AFTER_INVOKE_METHOD:
      defineNativeMethod(klass, "__afterInvokeMethod__", 3, false, method);
      break;
    case INTERCEPTOR_UNDEFINED_PROPERTY:
      defineNativeMethod(klass, "__undefinedProperty__", 1, false, method);
      break;
    case INTERCEPTOR_UNDEFINED_METHOD:
      defineNativeMethod(klass, "__undefinedMethod__", 2, false, method);
      break;
    case INTERCEPTOR_BEFORE_THROW:
      defineNativeMethod(klass, "__beforeThrow__", 2, false, method);
      break;
    case INTERCEPTOR_AFTER_THROW:
      defineNativeMethod(klass, "__afterThrow__", 2, false, method);
      break;
    default: 
      runtimeError("Unknown interceptor type %d.", type);
      exit(70);
  }
  SET_CLASS_INTERCEPTOR(klass, type);
}


ObjClass* defineNativeClass(const char* name, bool isAbstract) {
  ObjString* className = copyString(name, (int)strlen(name));
  push(OBJ_VAL(className));
  ObjClass* nativeClass = newClass(className, OBJ_INSTANCE, false);
  nativeClass->isNative = true;
  nativeClass->isAbstract = isAbstract;
  push(OBJ_VAL(nativeClass));
  tableSet(&vm.currentNamespace->values, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
  return nativeClass;
}

ObjEnum* defineNativeEnum(const char* name) {
  ObjString* enumName = copyString(name, (int)strlen(name));
  push(OBJ_VAL(enumName));
  ObjEnum* nativeEnum = newEnum(enumName);
  nativeEnum->nextValue = 0;
  push(OBJ_VAL(nativeEnum));
  tableSet(&vm.currentNamespace->values, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
  return nativeEnum;
}

void defineNativeArtificialEnumElement(ObjEnum* enum_, const char* name, Value value) {
  ObjString* elementName = copyString(name, (int)strlen(name));
  push(OBJ_VAL(elementName));
  int elementValue = enum_->nextValue++;
  tableSet(&enum_->values, elementName, value);
  pop();
}

void defineNativeEnumElement(ObjEnum* enum_, const char* name) {
  ObjString* elementName = copyString(name, (int)strlen(name));
  push(OBJ_VAL(elementName));
  int elementValue = enum_->nextValue++;
  tableSet(&enum_->values, elementName, INT_VAL(elementValue));
  pop();
}

void defineNativeMethod(ObjClass* klass, const char* name, int arity, bool isAsync, NativeMethod method) {
  ObjString* methodName = copyString(name, (int)strlen(name));
  push(OBJ_VAL(methodName));
  ObjNativeMethod* nativeMethod = newNativeMethod(klass, methodName, arity, isAsync, false, method);
  push(OBJ_VAL(nativeMethod));
  tableSet(&klass->methods, methodName, OBJ_VAL(nativeMethod));
  pop();
  pop();
}

void defineNativeAbstractMethod(ObjClass* klass, const char* name, int arity, uint32_t* paramHashes, NativeMethod method) {
  ObjString* methodName = copyString(name, (int)strlen(name));
  writeValueArray(&klass->abstractMethodNames, OBJ_VAL(methodName));
  push(OBJ_VAL(methodName));
  ObjNativeMethod* nativeMethod = newNativeMethod(klass, methodName, arity, false, true, method);
  for (int i = 0; i < arity; i++) {
    nativeMethod->paramHashes[i] = paramHashes[i];
  }
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
  ObjClass* exceptionClass = defineNativeClass(name, false);
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
  if (!IS_STRING(args[0]) && !IS_NUMBER(args[0]) && !IS_BOOL(args[0])) {
    runtimeError("Method num(value) expects argument 1 to be a string, a number or a boolean.", 1);
    exit(70);
  }

  if (IS_NUMBER(args[0])) {
    return args[0];
  }

  if (IS_BOOL(args[0])) {
    RETURN_NUMBER(AS_BOOL(args[0]));
  }

  ObjString* stringValue = AS_STRING(args[0]);

  if (stringValue->length == 0) {
    RETURN_INT(0);
  }

  const char* str = stringValue->chars;

  char* endptr;
  double value = strtod(str, &endptr);

  if (endptr == str) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Can't convert '%s' to a number.", endptr);
  } else {
    RETURN_NUMBER(value);
  }
}

NATIVE_FUNCTION(int) {
  assertArgCount("int(value)", 1, argCount);
  if (!IS_STRING(args[0]) && !IS_NUMBER(args[0]) && !IS_BOOL(args[0])) {
    runtimeError("Method int(value) expects argument 1 to be a string, a number or a boolean.", 1);
    exit(70);
  }

  if (IS_NUMBER(args[0])) {
    RETURN_INT(AS_NUMBER(args[0]));
  }

  if (IS_BOOL(args[0])) {
    RETURN_INT(AS_BOOL(args[0]));
  }

  ObjString* stringValue = AS_STRING(args[0]);

  if (stringValue->length == 0) {
    RETURN_INT(0);
  }

  const char* str = stringValue->chars;

  char* endptr;
  double value = strtod(str, &endptr);

  if (endptr == str) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Can't convert '%s' to an integer.", endptr);
  } else {
    RETURN_INT(value);
  }
}

NATIVE_FUNCTION(float) {
  assertArgCount("float(value)", 1, argCount);
  if (!IS_STRING(args[0]) && !IS_NUMBER(args[0]) && !IS_BOOL(args[0])) {
    runtimeError("Method float(value) expects argument 1 to be a string, a number or a boolean.", 1);
    exit(70);
  }

  if (IS_NUMBER(args[0])) {
    RETURN_FLOAT(AS_NUMBER(args[0]));
  }

  if (IS_BOOL(args[0])) {
    RETURN_FLOAT(AS_BOOL(args[0]));
  }

  ObjString* stringValue = AS_STRING(args[0]);

  if (stringValue->length == 0) {
    RETURN_INT(0);
  }

  const char* str = stringValue->chars;

  char* endptr;
  double value = strtod(str, &endptr);

  if (endptr == str) {
    THROW_EXCEPTION_FMT(luminique::std::lang, IllegalArgumentException, "Can't convert '%s' to a floating point number.", endptr);
  } else {
    RETURN_FLOAT(value);
  }
}

NATIVE_FUNCTION(str) {
  assertArgCount("str(value)", 1, argCount);
  Value str = args[0];
  do {
    Value toStringMethod = getObjMethod(str, "__str__");
    str = callReentrantMethod(str, toStringMethod);
  } while (!IS_STRING(str));

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
