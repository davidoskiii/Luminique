#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "math.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../value/value.h"
#include "../vm/vm.h"

static int factorial(int self) {
  int result = 1;
  for (int i = 1; i <= self; i++) {
    result *= i;
  }
  return result;
}

static double clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}

static int gcd(int self, int other) {
  while (self != other) {
    if (self > other) self -= other;
    else other -= self;
  }
  return self;
}

static int lcm(int self, int other) {
  return (self * other) / gcd(self, other);
}

char* intToBinary(int num, char* binaryString) {
  int index = 0;
  while (num > 0) {
    binaryString[index++] = (num % 2) + '0';
    num /= 2;
  }
  
  binaryString[index] = '\0';
  
  int i, j;
  for (i = 0, j = index - 1; i < j; i++, j--) {
    char temp = binaryString[i];
    binaryString[i] = binaryString[j];
    binaryString[j] = temp;
  }
  
  return binaryString;
}

NATIVE_METHOD(Complex, __init__) {
  assertArgCount("Complex::__init__(real, imaginary)", 2, argCount);
  assertArgIsNumber("Complex::__init__(real, imaginary)", args, 0);
  assertArgIsNumber("Complex::__init__(real, imaginary)", args, 1);

  ObjInstance* self = AS_INSTANCE(receiver);
  setObjProperty(self, "real", args[0]);
  setObjProperty(self, "imaginary", args[1]);
  return receiver;
}

NATIVE_METHOD(Complex, __equal__) {
  assertArgCount("Complex::==(complex)", 1, argCount);
  assertObjInstanceOfClass("Complex::==(complex)", args[0], "luminique::std::math", "Complex", 0);

  ObjInstance* self = AS_INSTANCE(receiver);
  Value real1 = getObjProperty(self, "real");
  Value imag1 = getObjProperty(self, "imaginary");
  ObjInstance* other = AS_INSTANCE(args[0]);
  Value real2 = getObjProperty(other, "real");
  Value imag2 = getObjProperty(other, "imaginary");

  RETURN_BOOL(AS_NUMBER(real1) == AS_NUMBER(real2) && AS_NUMBER(imag1) == AS_NUMBER(imag2));
}

NATIVE_METHOD(Complex, __add__) {
  assertArgCount("Complex::+(complex)", 1, argCount);
  assertObjInstanceOfClass("Complex::+(complex)", args[0], "luminique::std::math", "Complex", 0);

  ObjInstance* self = AS_INSTANCE(receiver);
  Value real1 = getObjProperty(self, "real");
  Value imag1 = getObjProperty(self, "imaginary");
  ObjInstance* other = AS_INSTANCE(args[0]);
  Value real2 = getObjProperty(other, "real");
  Value imag2 = getObjProperty(other, "imaginary");

  double resultReal = AS_NUMBER(real1) + AS_NUMBER(real2);
  double resultImaginary = AS_NUMBER(imag1) + AS_NUMBER(imag2);

  ObjInstance* result = newInstance(getNativeClass("luminique::std::math", "Complex"));
  push(OBJ_VAL(result));
  setObjProperty(result, "real", NUMBER_VAL(resultReal));
  setObjProperty(result, "imaginary", NUMBER_VAL(resultImaginary));
  pop();
  
  RETURN_OBJ(result);
}

NATIVE_METHOD(Complex, __subtract__) {
  assertArgCount("Complex::-(complex)", 1, argCount);
  assertObjInstanceOfClass("Complex::-(complex)", args[0], "luminique::std::math", "Complex", 0);

  ObjInstance* self = AS_INSTANCE(receiver);
  Value real1 = getObjProperty(self, "real");
  Value imag1 = getObjProperty(self, "imaginary");
  ObjInstance* other = AS_INSTANCE(args[0]);
  Value real2 = getObjProperty(other, "real");
  Value imag2 = getObjProperty(other, "imaginary");

  double resultReal = AS_NUMBER(real1) - AS_NUMBER(real2);
  double resultImaginary = AS_NUMBER(imag1) - AS_NUMBER(imag2);

  ObjInstance* result = newInstance(getNativeClass("luminique::std::math", "Complex"));
  push(OBJ_VAL(result));
  setObjProperty(result, "real", NUMBER_VAL(resultReal));
  setObjProperty(result, "imaginary", NUMBER_VAL(resultImaginary));
  pop();
  
  RETURN_OBJ(result);
}

NATIVE_METHOD(Complex, __multiply__) {
  assertArgCount("Complex::*(complex)", 1, argCount);
  assertObjInstanceOfClass("Complex::*(complex)", args[0], "luminique::std::math", "Complex", 0);

  ObjInstance* self = AS_INSTANCE(receiver);
  Value real1 = getObjProperty(self, "real");
  Value imag1 = getObjProperty(self, "imaginary");
  ObjInstance* other = AS_INSTANCE(args[0]);
  Value real2 = getObjProperty(other, "real");
  Value imag2 = getObjProperty(other, "imaginary");

  double resultReal = AS_NUMBER(real1) * AS_NUMBER(real2) - AS_NUMBER(imag1) * AS_NUMBER(imag2);
  double resultImaginary = AS_NUMBER(real1) * AS_NUMBER(imag2) + AS_NUMBER(imag1) * AS_NUMBER(real2);

  ObjInstance* result = newInstance(getNativeClass("luminique::std::math", "Complex"));
  push(OBJ_VAL(result));
  setObjProperty(result, "real", NUMBER_VAL(resultReal));
  setObjProperty(result, "imaginary", NUMBER_VAL(resultImaginary));
  pop();
  
  RETURN_OBJ(result);
}

NATIVE_METHOD(Complex, __divide__) {
  assertArgCount("Complex::/(complex)", 1, argCount);
  assertObjInstanceOfClass("Complex::/(complex)", args[0], "luminique::std::math", "Complex", 0);

  ObjInstance* self = AS_INSTANCE(receiver);
  Value real1 = getObjProperty(self, "real");
  Value imag1 = getObjProperty(self, "imaginary");
  ObjInstance* other = AS_INSTANCE(args[0]);
  Value real2 = getObjProperty(other, "real");
  Value imag2 = getObjProperty(other, "imaginary");

  double denom = AS_NUMBER(real2) * AS_NUMBER(real2) + AS_NUMBER(imag2) * AS_NUMBER(imag2);
  double resultReal = (AS_NUMBER(real1) * AS_NUMBER(real2) + AS_NUMBER(imag1) * AS_NUMBER(imag2)) / denom;
  double resultImaginary = (AS_NUMBER(imag1) * AS_NUMBER(real2) - AS_NUMBER(real1) * AS_NUMBER(imag2)) / denom;

  ObjInstance* result = newInstance(getNativeClass("luminique::std::math", "Complex"));
  push(OBJ_VAL(result));
  setObjProperty(result, "real", NUMBER_VAL(resultReal));
  setObjProperty(result, "imaginary", NUMBER_VAL(resultImaginary));
  pop();
  
  RETURN_OBJ(result);
}

NATIVE_METHOD(Complex, __less__) {
  assertArgCount("Complex::<(complex)", 1, argCount);
  assertObjInstanceOfClass("Complex::<(complex)", args[0], "luminique::std::math", "Complex", 0);

  ObjInstance* self = AS_INSTANCE(receiver);
  double magnitude1 = sqrt(AS_NUMBER(getObjProperty(self, "real")) * AS_NUMBER(getObjProperty(self, "real")) +
                          AS_NUMBER(getObjProperty(self, "imaginary")) * AS_NUMBER(getObjProperty(self, "imaginary")));
  ObjInstance* other = AS_INSTANCE(args[0]);
  double magnitude2 = sqrt(AS_NUMBER(getObjProperty(other, "real")) * AS_NUMBER(getObjProperty(other, "real")) +
                          AS_NUMBER(getObjProperty(other, "imaginary")) * AS_NUMBER(getObjProperty(other, "imaginary")));

  RETURN_BOOL(magnitude1 < magnitude2);
}

NATIVE_METHOD(Complex, __greater__) {
  assertArgCount("Complex::>(complex)", 1, argCount);
  assertObjInstanceOfClass("Complex::>(complex)", args[0], "luminique::std::math", "Complex", 0);

  ObjInstance* self = AS_INSTANCE(receiver);
  double magnitude1 = sqrt(AS_NUMBER(getObjProperty(self, "real")) * AS_NUMBER(getObjProperty(self, "real")) +
                          AS_NUMBER(getObjProperty(self, "imaginary")) * AS_NUMBER(getObjProperty(self, "imaginary")));
  ObjInstance* other = AS_INSTANCE(args[0]);
  double magnitude2 = sqrt(AS_NUMBER(getObjProperty(other, "real")) * AS_NUMBER(getObjProperty(other, "real")) +
                          AS_NUMBER(getObjProperty(other, "imaginary")) * AS_NUMBER(getObjProperty(other, "imaginary")));

  RETURN_BOOL(magnitude1 > magnitude2);
}

NATIVE_METHOD(Complex, __lessEqual__) {
  assertArgCount("Complex::<=(complex)", 1, argCount);
  assertObjInstanceOfClass("Complex::<=(complex)", args[0], "luminique::std::math", "Complex", 0);

  ObjInstance* self = AS_INSTANCE(receiver);
  double magnitude1 = sqrt(AS_NUMBER(getObjProperty(self, "real")) * AS_NUMBER(getObjProperty(self, "real")) +
                          AS_NUMBER(getObjProperty(self, "imaginary")) * AS_NUMBER(getObjProperty(self, "imaginary")));
  ObjInstance* other = AS_INSTANCE(args[0]);
  double magnitude2 = sqrt(AS_NUMBER(getObjProperty(other, "real")) * AS_NUMBER(getObjProperty(other, "real")) +
                          AS_NUMBER(getObjProperty(other, "imaginary")) * AS_NUMBER(getObjProperty(other, "imaginary")));

  RETURN_BOOL(magnitude1 <= magnitude2);
}

NATIVE_METHOD(Complex, __greaterEqual__) {
  assertArgCount("Complex::>=(complex)", 1, argCount);
  assertObjInstanceOfClass("Complex::>=(complex)", args[0], "luminique::std::math", "Complex", 0);

  ObjInstance* self = AS_INSTANCE(receiver);
  double magnitude1 = sqrt(AS_NUMBER(getObjProperty(self, "real")) * AS_NUMBER(getObjProperty(self, "real")) +
                          AS_NUMBER(getObjProperty(self, "imaginary")) * AS_NUMBER(getObjProperty(self, "imaginary")));
  ObjInstance* other = AS_INSTANCE(args[0]);
  double magnitude2 = sqrt(AS_NUMBER(getObjProperty(other, "real")) * AS_NUMBER(getObjProperty(other, "real")) +
                          AS_NUMBER(getObjProperty(other, "imaginary")) * AS_NUMBER(getObjProperty(other, "imaginary")));

  RETURN_BOOL(magnitude1 >= magnitude2);
}

NATIVE_METHOD(Complex, __str__) {
  assertArgCount("Complex::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  Value real = getObjProperty(self, "real");
  Value imaginary = getObjProperty(self, "imaginary");

  double realPart = AS_NUMBER(real);
  double imaginaryPart = AS_NUMBER(imaginary);
  char buffer[255];

  if (realPart == 0 && imaginaryPart == 0) {
    snprintf(buffer, sizeof(buffer), "0");
  } else if (realPart == 0) {
    if (imaginaryPart == 1) {
      snprintf(buffer, sizeof(buffer), "i");
    } else if (imaginaryPart == -1) {
      snprintf(buffer, sizeof(buffer), "-i");
    } else {
      snprintf(buffer, sizeof(buffer), "%gi", imaginaryPart);
    }
  } else if (imaginaryPart == 0) {
      snprintf(buffer, sizeof(buffer), "%g", realPart);
  } else {
    if (imaginaryPart == 1) {
      snprintf(buffer, sizeof(buffer), "%g + i", realPart);
    } else if (imaginaryPart == -1) {
      snprintf(buffer, sizeof(buffer), "%g - i", realPart);
    } else if (imaginaryPart > 0) {
      snprintf(buffer, sizeof(buffer), "%g + %gi", realPart, imaginaryPart);
    } else {
      snprintf(buffer, sizeof(buffer), "%g - %gi", realPart, -imaginaryPart);
    }
  }

  RETURN_STRING(buffer, strlen(buffer));
}

NATIVE_METHOD(Complex, __format__) {
  assertArgCount("Complex::__format__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  Value real = getObjProperty(self, "real");
  Value imaginary = getObjProperty(self, "imaginary");

  double realPart = AS_NUMBER(real);
  double imaginaryPart = AS_NUMBER(imaginary);
  char buffer[255];

  if (realPart == 0 && imaginaryPart == 0) {
    snprintf(buffer, sizeof(buffer), "0");
  } else if (realPart == 0) {
    if (imaginaryPart == 1) {
      snprintf(buffer, sizeof(buffer), "i");
    } else if (imaginaryPart == -1) {
      snprintf(buffer, sizeof(buffer), "-i");
    } else {
      snprintf(buffer, sizeof(buffer), "%gi", imaginaryPart);
    }
  } else if (imaginaryPart == 0) {
      snprintf(buffer, sizeof(buffer), "%g", realPart);
  } else {
    if (imaginaryPart == 1) {
      snprintf(buffer, sizeof(buffer), "%g + i", realPart);
    } else if (imaginaryPart == -1) {
      snprintf(buffer, sizeof(buffer), "%g - i", realPart);
    } else if (imaginaryPart > 0) {
      snprintf(buffer, sizeof(buffer), "%g + %gi", realPart, imaginaryPart);
    } else {
      snprintf(buffer, sizeof(buffer), "%g - %gi", realPart, -imaginaryPart);
    }
  }

  RETURN_STRING(buffer, strlen(buffer));
}

NATIVE_METHOD(Complex, magnitude) {
  assertArgCount("Complex::magnitude()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  Value real = getObjProperty(self, "real");
  Value imaginary = getObjProperty(self, "imaginary");
  double magnitude = sqrt(AS_NUMBER(real) * AS_NUMBER(real) + AS_NUMBER(imaginary) * AS_NUMBER(imaginary));
  RETURN_NUMBER(magnitude);
}

NATIVE_METHOD(Complex, phase) {
  assertArgCount("Complex::phase()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  Value real = getObjProperty(self, "real");
  Value imaginary = getObjProperty(self, "imaginary");
  double phase = atan2(AS_NUMBER(imaginary), AS_NUMBER(real));
  RETURN_NUMBER(phase);
}

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

NATIVE_FUNCTION(hypot) {
  assertArgCount("hypot(a, b)", 2, argCount);
  assertArgIsNumber("hypot(a, b)", args, 0);
  assertArgIsNumber("hypot(a, b)", args, 1);
  double result = hypot(AS_NUMBER(args[0]), AS_NUMBER(args[1]));
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

NATIVE_FUNCTION(cbrt) {
  assertArgCount("cbrt(value)", 1, argCount);
  assertArgIsNumber("cbrt(value)", args, 0);
  double result = cbrt(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(nthrt) {
  assertArgCount("nthrt(value, n)", 2, argCount);
  assertArgIsNumber("nthrt(value, n)", args, 0);
  assertArgIsNumber("nthrt(value, n)", args, 1);
  
  double value = AS_NUMBER(args[0]);
  double n = AS_NUMBER(args[1]);
  
  if (n == 0.0) {
    runtimeError("Invalid root degree: n cannot be zero");
  }
  
  RETURN_NUMBER(pow(value, 1.0 / n));
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
  RETURN_INT(result);
}

NATIVE_FUNCTION(floor) {
  assertArgCount("floor(value)", 1, argCount);
  assertArgIsNumber("floor(value)", args, 0);
  double result = floor(AS_NUMBER(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_INT(result);
}

NATIVE_FUNCTION(clamp) {
  assertArgCount("clamp(value, min, max)", 3, argCount);
  assertArgIsNumber("clamp(value, min, max)", args, 0);
  assertArgIsNumber("clamp(value, min, max)", args, 1);
  assertArgIsNumber("clamp(value, min, max)", args, 2);
  double result = clamp(AS_NUMBER(args[0]), AS_NUMBER(args[1]), AS_NUMBER(args[2]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_INT(result);
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

NATIVE_FUNCTION(abs) {
  assertArgCount("abs(value)", 1, argCount);
  assertArgIsInt("abs(value)", args, 0);
  double result = abs(AS_INT(args[0]));
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_INT(result);
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

NATIVE_FUNCTION(rad) {
  assertArgCount("rad(degrees)", 1, argCount);
  assertArgIsNumber("rad(degrees)", args, 0);
  double result = AS_NUMBER(args[0]) * (M_PI / 180.0);
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(deg) {
  assertArgCount("deg(radiants)", 1, argCount);
  assertArgIsNumber("deg(radiants)", args, 0);
  double result = AS_NUMBER(args[0]) * (180.0 / M_PI);
  if (isnan(result)) {
    RETURN_NIL;
  }
  RETURN_NUMBER(result);
}

NATIVE_FUNCTION(bin) {
  assertArgCount("bin(value)", 1, argCount);
  assertArgIsInt("bin(value)", args, 0);
  char buffer[32];
  intToBinary(AS_INT(args[0]), buffer);
  RETURN_STRING(buffer, strlen(buffer));
}

NATIVE_FUNCTION(hex) {
  assertArgCount("hex(value)", 1, argCount);
  assertArgIsInt("hex(value)", args, 0);
  char buffer[32];
  sprintf(buffer, "%x", AS_INT(args[0]));
  RETURN_STRING(buffer, strlen(buffer));
}

NATIVE_FUNCTION(factorial) {
  assertArgCount("factorial(value)", 1, argCount);
  assertArgIsInt("factorial(value)", args, 0);
  int self = AS_INT(args[0]);
  assertNumberNonNegative("factorial(value)", self, -1);
  RETURN_INT(factorial(self));
}

NATIVE_FUNCTION(gcd) {
  assertArgCount("gcd(value1, value2)", 2, argCount);
  assertArgIsInt("gcd(value1, value2)", args, 0);
  assertArgIsInt("gcd(value1, value2)", args, 1);
  RETURN_INT(gcd(abs(AS_INT(args[0])), abs(AS_INT(args[1]))));
}

NATIVE_FUNCTION(lcm) {
  assertArgCount("lcm(value1, value2)", 2, argCount);
  assertArgIsInt("lcm(value1, value2)", args, 0);
  assertArgIsInt("lcm(value1, value2)", args, 1);
  RETURN_INT(lcm(abs(AS_INT(args[0])), abs(AS_INT(args[1]))));
}

NATIVE_FUNCTION(even) {
  assertArgCount("even(value)", 1, argCount);
  assertArgIsInt("even(value)", args, 0);
  RETURN_BOOL(AS_INT(args[0]) % 2 == 0);
}

NATIVE_FUNCTION(odd) {
  assertArgCount("odd(value)", 1, argCount);
  assertArgIsInt("odd(value)", args, 0);
  RETURN_BOOL(AS_INT(args[0]) % 2 != 0);
}

void registerMathPackage() {
  ObjNamespace* mathNamespace = defineNativeNamespace("math", vm.stdNamespace);

  defineNativeConstant(mathNamespace, "pi", NUMBER_VAL(M_PI));
  defineNativeConstant(mathNamespace, "inf", NUMBER_VAL(INFINITY));
  defineNativeConstant(mathNamespace, "e", NUMBER_VAL(M_E));

  vm.currentNamespace = mathNamespace;

	ObjClass* dateClass = defineNativeClass("Complex", false);
	bindSuperclass(dateClass, vm.objectClass);
	DEF_METHOD(dateClass, Complex, __init__, 3);
	DEF_METHOD(dateClass, Complex, magnitude, 0);
	DEF_METHOD(dateClass, Complex, phase, 0);
	DEF_METHOD(dateClass, Complex, __str__, 0);
	DEF_METHOD(dateClass, Complex, __format__, 0);
  DEF_OPERATOR(dateClass, Complex, ==, __equal__, 1);
  DEF_OPERATOR(dateClass, Complex, <, __less__, 1);
  DEF_OPERATOR(dateClass, Complex, >, __greater__, 1);
  DEF_OPERATOR(dateClass, Complex, <=, __lessEqual__, 1);
  DEF_OPERATOR(dateClass, Complex, >=, __greaterEqual__, 1);
  DEF_OPERATOR(dateClass, Complex, +, __add__, 1);
  DEF_OPERATOR(dateClass, Complex, -, __subtract__, 1);
  DEF_OPERATOR(dateClass, Complex, *, __multiply__, 1);
  DEF_OPERATOR(dateClass, Complex, /, __divide__, 1);

  // Trigonometric functions
  DEF_FUNCTION(sin, 1);
  DEF_FUNCTION(cos, 1);
  DEF_FUNCTION(tan, 1);
  DEF_FUNCTION(asin, 1);
  DEF_FUNCTION(acos, 1);
  DEF_FUNCTION(atan, 1);

  DEF_FUNCTION(hypot, 2);

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
  DEF_FUNCTION(cbrt, 1);
  DEF_FUNCTION(nthrt, 2);

  // Rounding and absolute functions
  DEF_FUNCTION(ceil, 1);
  DEF_FUNCTION(floor, 1);
  DEF_FUNCTION(round, 1);
  DEF_FUNCTION(abs, 1);
  DEF_FUNCTION(fabs, 1);

  // Additional arithmetic operations
  DEF_FUNCTION(fmod, 2);
  DEF_FUNCTION(remainder, 2);
  DEF_FUNCTION(fmax, 2);
  DEF_FUNCTION(fmin, 2);

  // Conversion
  DEF_FUNCTION(rad, 1);
  DEF_FUNCTION(deg, 1);
  DEF_FUNCTION(bin, 1);
  DEF_FUNCTION(hex, 1);

  // Others
  DEF_FUNCTION(factorial, 1);
  DEF_FUNCTION(gcd, 1);
  DEF_FUNCTION(lcm, 1);
  DEF_FUNCTION(even, 1);
  DEF_FUNCTION(odd, 1);
  DEF_FUNCTION(clamp, 3);

  vm.currentNamespace = vm.rootNamespace;
}
