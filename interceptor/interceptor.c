#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interceptor.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../vm/vm.h"

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
