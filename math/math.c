#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "math.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../hash/hash.h"
#include "../value/value.h"
#include "../vm/vm.h"

NATIVE_FUNCTION(sum) {
  assertArgCount("sum(n1, n2)", 2, argCount);
  assertArgIsNumber("sum(n1, n2)", args, 0);
  assertArgIsNumber("sum(n1, n2)", args, 1);

  RETURN_NUMBER(AS_NUMBER(args[0]) + AS_NUMBER(args[1]));
}

void registerMathPackage() {
  ObjNamespace* mathNamespace = defineNativeNamespace("Math", vm.stdNamespace);
  vm.currentNamespace = mathNamespace;

  DEF_FUNCTION(sum, 2);

  vm.currentNamespace = vm.rootNamespace;
}
