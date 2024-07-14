#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interceptor.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../vm/vm.h"

void handleInterceptorMethod(ObjClass* klass, ObjString* name) {
  if (name->length <= 2 || name->chars[0] != '_' || name->chars[1] != '_') return;

  if (strcmp(name->chars, "__beforeGetProprety__") == 0) SET_CLASS_INTERCEPTOR(klass, INTERCEPTOR_BEFORE_GET_PROPERTY);
  else if (strcmp(name->chars, "__afterGetProprety__") == 0) SET_CLASS_INTERCEPTOR(klass, INTERCEPTOR_AFTER_GET_PROPERTY);
  else if (strcmp(name->chars, "__undefinedProprety__") == 0) SET_CLASS_INTERCEPTOR(klass, INTERCEPTOR_UNDEFINED_PROPERTY);
  else if (strcmp(name->chars, "__undefinedMethod__") == 0) SET_CLASS_INTERCEPTOR(klass, INTERCEPTOR_UNDEFINED_METHOD);
}

bool interceptBeforeGet(ObjClass* klass, ObjString* name) {
  Value interceptor;
  if (tableGet(&klass->methods, newString("__beforeGetProprety__"), &interceptor)) {
    push(OBJ_VAL(name));
    return callMethod(interceptor, 1);
  }
  return false;
}

bool interceptAfterGet(ObjClass* klass, ObjString* name) {
  Value interceptor;
  if (tableGet(&klass->methods, newString("__afterGetProprety__"), &interceptor)) {
    Value value = peek(0);
    push(OBJ_VAL(name));
    return callMethod(interceptor, 2);
  }
  return false;
}

bool interceptUndefinedProperty(ObjClass* klass, ObjString* name) {
  Value interceptor;
  if (tableGet(&klass->methods, newString("__undefinedProprety__"), &interceptor)) {
    push(OBJ_VAL(name));
    return callMethod(interceptor, 1);
  }
  return false;
}

bool interceptUndefinedMethod(ObjClass* klass, ObjString* name, int argCount) {
  Value interceptor;
  if (tableGet(&klass->methods, newString("__undefinedMethod__"), &interceptor)) {
    ObjArray* args = newArray();
    push(OBJ_VAL(args));
    for (int i = argCount; i > 0; i--) {
      writeValueArray(&args->elements, vm.stackTop[-i - 1]);
    }
    pop();

    vm.stackTop -= argCount;
    push(OBJ_VAL(name));
    push(OBJ_VAL(args));
    return callMethod(interceptor, 2);
  }
  return false;
}
