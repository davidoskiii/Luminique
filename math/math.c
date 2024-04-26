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

NATIVE_FUNCTION(sin) {
  assertArgCount("sin(radiants)", 1, argCount);
  assertArgIsNumber("sin(radiants)", args, 0);
  double result = sin(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(cos) {
  assertArgCount("cos(radians)", 1, argCount);
  assertArgIsNumber("cos(radians)", args, 0);
  double result = cos(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(tan) {
  assertArgCount("tan(radians)", 1, argCount);
  assertArgIsNumber("tan(radians)", args, 0);
  double result = tan(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(asin) {
  assertArgCount("asin(value)", 1, argCount);
  assertArgIsNumber("asin(value)", args, 0);
  double result = asin(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(acos) {
  assertArgCount("acos(value)", 1, argCount);
  assertArgIsNumber("acos(value)", args, 0);
  double result = acos(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(atan) {
  assertArgCount("atan(value)", 1, argCount);
  assertArgIsNumber("atan(value)", args, 0);
  double result = atan(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(exp) {
  assertArgCount("exp(value)", 1, argCount);
  assertArgIsNumber("exp(value)", args, 0);
  double result = exp(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(ln) {
  assertArgCount("ln(value)", 1, argCount);
  assertArgIsNumber("ln(value)", args, 0);
  double result = log(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(log10) {
  assertArgCount("log10(value)", 1, argCount);
  assertArgIsNumber("log10(value)", args, 0);
  double result = log10(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(log2) {
  assertArgCount("log2(value)", 1, argCount);
  assertArgIsNumber("log2(value)", args, 0);
  double result = log2(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(pow) {
  assertArgCount("pow(base, exponent)", 2, argCount);
  assertArgIsNumber("pow(base, exponent)", args, 0);
  assertArgIsNumber("pow(base, exponent)", args, 1);
  double result = pow(AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(sqrt) {
  assertArgCount("sqrt(value)", 1, argCount);
  assertArgIsNumber("sqrt(value)", args, 0);
  double result = sqrt(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(logb) {
  assertArgCount("logb(base, value)", 2, argCount);
  assertArgIsNumber("logb(base, value)", args, 0);
  assertArgIsNumber("logb(base, value)", args, 1);
  double result = log(AS_NUMBER(args[1])) / log(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(sinh) {
  assertArgCount("sinh(radiants)", 1, argCount);
  assertArgIsNumber("sinh(radiants)", args, 0);
  double result = sinh(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(cosh) {
  assertArgCount("cosh(radiants)", 1, argCount);
  assertArgIsNumber("cosh(radiants)", args, 0);
  double result = cosh(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(tanh) {
  assertArgCount("tanh(radiants)", 1, argCount);
  assertArgIsNumber("tanh(radiants)", args, 0);
  double result = tanh(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(asinh) {
  assertArgCount("asinh(radiants)", 1, argCount);
  assertArgIsNumber("asinh(radiants)", args, 0);
  double result = asinh(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(acosh) {
  assertArgCount("acosh(radiants)", 1, argCount);
  assertArgIsNumber("acosh(radiants)", args, 0);
  double result = acosh(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(atanh) {
  assertArgCount("atanh(radiants)", 1, argCount);
  assertArgIsNumber("atanh(radiants)", args, 0);
  double result = atanh(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}
NATIVE_FUNCTION(ceil) {
  assertArgCount("ceil(value)", 1, argCount);
  assertArgIsNumber("ceil(value)", args, 0);
  double result = ceil(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(floor) {
  assertArgCount("floor(value)", 1, argCount);
  assertArgIsNumber("floor(value)", args, 0);
  double result = floor(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(round) {
  assertArgCount("round(value)", 1, argCount);
  assertArgIsNumber("round(value)", args, 0);
  double result = round(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(fabs) {
  assertArgCount("fabs(value)", 1, argCount);
  assertArgIsNumber("fabs(value)", args, 0);
  double result = fabs(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(fmod) {
  assertArgCount("fmod(x, y)", 2, argCount);
  assertArgIsNumber("fmod(x, y)", args, 0);
  assertArgIsNumber("fmod(x, y)", args, 1);
  double result = fmod(AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(remainder) {
  assertArgCount("remainder(x, y)", 2, argCount);
  assertArgIsNumber("remainder(x, y)", args, 0);
  assertArgIsNumber("remainder(x, y)", args, 1);
  double result = remainder(AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(fmax) {
  assertArgCount("fmax(x, y)", 2, argCount);
  assertArgIsNumber("fmax(x, y)", args, 0);
  assertArgIsNumber("fmax(x, y)", args, 1);
  double result = fmax(AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(fmin) {
  assertArgCount("fmin(x, y)", 2, argCount);
  assertArgIsNumber("fmin(x, y)", args, 0);
  assertArgIsNumber("fmin(x, y)", args, 1);
  double result = fmin(AS_NUMBER(args[0]), AS_NUMBER(args[1]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

void registerMathPackage() {
  ObjNamespace* mathNamespace = defineNativeNamespace("Math", vm.stdNamespace);
  vm.currentNamespace = mathNamespace;

  defineNativeConstant("pi", NUMBER_VAL(M_PI));
  defineNativeConstant("inf", NUMBER_VAL(INFINITY));
  defineNativeConstant("e", NUMBER_VAL(M_E));

  // Trigonometric functions
  DEF_FUNCTION(sin, 1);
  DEF_FUNCTION(cos, 1);
  DEF_FUNCTION(tan, 1);
  DEF_FUNCTION(asin, 1);
  DEF_FUNCTION(acos, 1);
  DEF_FUNCTION(atan, 1);

  // Hyperbolic functions
  DEF_FUNCTION(sinh, 1);
  DEF_FUNCTION(cosh, 1);
  DEF_FUNCTION(tanh, 1);
  DEF_FUNCTION(asinh, 1);
  DEF_FUNCTION(acosh, 1);
  DEF_FUNCTION(atanh, 1);

  // Exponential and logarithmic functions
  DEF_FUNCTION(exp, 1);
  DEF_FUNCTION(ln, 1);
  DEF_FUNCTION(log10, 1);
  DEF_FUNCTION(log2, 1);
  DEF_FUNCTION(logb, 2);
  DEF_FUNCTION(pow, 2);
  DEF_FUNCTION(sqrt, 1);

  // Rounding and absolute functions
  DEF_FUNCTION(ceil, 1);
  DEF_FUNCTION(floor, 1);
  DEF_FUNCTION(round, 1);
  DEF_FUNCTION(fabs, 1);

  // Additional arithmetic operations
  DEF_FUNCTION(fmod, 2);
  DEF_FUNCTION(remainder, 2);
  DEF_FUNCTION(fmax, 2);
  DEF_FUNCTION(fmin, 2);

  vm.currentNamespace = vm.rootNamespace;
}
