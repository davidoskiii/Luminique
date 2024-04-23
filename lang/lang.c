#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lang.h"
#include "../hash/hash.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../vm/vm.h"

#define MAX_BUFFER_SIZE 32

static int factorial(int self) {
  int result = 1;
  for (int i = 1; i <= self; i++) {
    result *= i;
  }
  return result;
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

// BOOL

NATIVE_METHOD(Bool, __init__) {
  THROW_EXCEPTION(InstantiationException, "Cannot instantiate from class Bool.");
}

NATIVE_METHOD(Bool, clone) {
	assertArgCount("Bool::clone()", 0, argCount);
	return receiver;
}

NATIVE_METHOD(Bool, toString) {
	assertArgCount("Bool::toString()", 0, argCount);
	if (AS_BOOL(receiver)) RETURN_STRING("true", 4);
	else RETURN_STRING("false", 5);
}

// EXCEPTION

NATIVE_METHOD(Exception, __init__) {
  assertArgCount("Exception::__init__(message)", 1, argCount);
  assertArgIsString("Exception::__init__(message)", args, 0);
  ObjInstance* exception = AS_INSTANCE(receiver);
  setObjProperty(exception, "message", args[0]);
  setObjProperty(exception, "stacktrace", NIL_VAL);
  RETURN_OBJ(exception);
}

NATIVE_METHOD(Exception, toString) {
  assertArgCount("Exception::toString()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  Value message = getObjProperty(self, "message");
  RETURN_STRING_FMT("<Exception %s - %s>", self->obj.klass->name->chars, AS_CSTRING(message));
}

// CLASS

NATIVE_METHOD(Class, __init__) {
  assertArgCount("Class::__init__(name, superclass)", 2, argCount);
  assertArgIsString("Class::__init__(name, superclass)", args, 0);
  assertArgIsClass("Class::__init__(name, superclass)", args, 1);
  ObjClass* klass = newClass(AS_STRING(args[0]));
  bindSuperclass(klass, AS_CLASS(args[1]));
  RETURN_OBJ(klass);
}

NATIVE_METHOD(Class, __invoke__) { 
  ObjClass* self = AS_CLASS(receiver);
  ObjInstance* instance = newInstance(self);
  push(OBJ_VAL(instance));
  Value initMethod;

  if (tableGet(&self->methods, vm.initString, &initMethod)) {
    callReentrant(receiver, initMethod, args);
  }

  pop();
  RETURN_OBJ(instance);
}

NATIVE_METHOD(Class, clone) {
  assertArgCount("Class::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Class, getClass) {
  assertArgCount("Class::getClass()", 0, argCount);
  RETURN_OBJ(vm.classClass);
}

NATIVE_METHOD(Class, getClassName) {
  assertArgCount("Class::getClassName()", 0, argCount);
  RETURN_OBJ(vm.classClass->name);
}

NATIVE_METHOD(Class, instanceOf) {
  assertArgCount("Class::instanceOf(class)", 1, argCount);
  if (!IS_CLASS(args[0])) RETURN_FALSE;
  ObjClass* klass = AS_CLASS(args[0]);
  if (klass == vm.classClass) RETURN_TRUE;
  else RETURN_FALSE;
}

NATIVE_METHOD(Class, memberOf) {
  assertArgCount("Class::memberOf(class)", 1, argCount);
  if (!IS_CLASS(args[0])) RETURN_FALSE;
  ObjClass* klass = AS_CLASS(args[0]);
  if (klass == vm.classClass) RETURN_TRUE;
  else RETURN_FALSE;
}

NATIVE_METHOD(Class, name) {
  assertArgCount("Class::name()", 0, argCount);
  RETURN_OBJ(AS_CLASS(receiver)->name);
}

NATIVE_METHOD(Class, superclass) {
  assertArgCount("Class::superclass()", 0, argCount);
  ObjClass* klass = AS_CLASS(receiver);
  if (klass->superclass == NULL) RETURN_NIL;
  RETURN_OBJ(klass->superclass);
}

NATIVE_METHOD(Class, toString) {
  assertArgCount("Class::toString()", 0, argCount);
  RETURN_STRING_FMT("<class %s>", AS_CLASS(receiver)->name->chars);
}

// FLOAT

NATIVE_METHOD(Float, __init__) {
  THROW_EXCEPTION(InstantiationException, "Cannot instantiate from class Float.");
}

NATIVE_METHOD(Float, clone) {
  assertArgCount("Float::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Float, toString) {
  assertArgCount("Float::toString()", 0, argCount);
  RETURN_STRING_FMT("%g", AS_FLOAT(receiver));
}

// FUNCTION

NATIVE_METHOD(Function, __init__) {
  THROW_EXCEPTION(InstantiationException, "Cannot instantiate from class Function.");
}

NATIVE_METHOD(Function, __invoke__) {
  ObjClosure* self = AS_CLOSURE(receiver);
  if (callClosure(self, argCount)) {
    int i = 0;
    while (i < argCount) {
      push(args[i]);
      i++;
    }
    RETURN_VAL(args[argCount - 1]);
  }
  RETURN_NIL;
}


NATIVE_METHOD(Function, arity) {
  assertArgCount("Function::arity()", 0, argCount);

  if (IS_NATIVE_FUNCTION(receiver)) {
     RETURN_INT(AS_NATIVE_FUNCTION(receiver)->arity);
  }

  RETURN_INT(AS_CLOSURE(receiver)->function->arity);
}

NATIVE_METHOD(Function, clone) {
  assertArgCount("Function::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Function, isNative) {
  assertArgCount("Function::isNative()", 0, argCount);
  RETURN_BOOL(IS_NATIVE_FUNCTION(receiver));
}

NATIVE_METHOD(Function, name) {
  assertArgCount("Function::name()", 0, argCount);
  if (IS_NATIVE_FUNCTION(receiver)) {
    RETURN_OBJ(AS_NATIVE_FUNCTION(receiver)->name);
  }
  RETURN_OBJ(AS_CLOSURE(receiver)->function->name);
}

NATIVE_METHOD(Function, toString) {
  assertArgCount("Function::toString()", 0, argCount);

  if (IS_NATIVE_FUNCTION(receiver)) {
    RETURN_STRING_FMT("<native fn %s>", AS_NATIVE_FUNCTION(receiver)->name->chars);
  }

  RETURN_STRING_FMT("<fn %s>", AS_CLOSURE(receiver)->function->name->chars);
}

NATIVE_METHOD(Function, upvalueCount) {
  assertArgCount("Function::upvalueCount()", 0, argCount);
  RETURN_INT(AS_CLOSURE(receiver)->upvalueCount);
}

// METHOD


NATIVE_METHOD(Method, __init__) {
  THROW_EXCEPTION(InstantiationException, "Cannot instantiate from class Method.");
}

NATIVE_METHOD(Method, arity) {
  assertArgCount("Method::arity()", 0, argCount);
  RETURN_INT(AS_BOUND_METHOD(receiver)->method->function->arity);
}

NATIVE_METHOD(Method, clone) {
  assertArgCount("Method::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Method, name) {
  assertArgCount("Method::name()", 0, argCount);
  ObjBoundMethod* bound = AS_BOUND_METHOD(receiver);
  RETURN_STRING_FMT("%s::%s", getObjClass(bound->receiver)->name->chars, bound->method->function->name->chars);
}

NATIVE_METHOD(Method, receiver) {
  assertArgCount("Method::receiver()", 0, argCount);
  RETURN_VAL(AS_BOUND_METHOD(receiver)->receiver);
}

NATIVE_METHOD(Method, toString) {
  assertArgCount("Method::toString()", 0, argCount);
  ObjBoundMethod* bound = AS_BOUND_METHOD(receiver);
  RETURN_STRING_FMT("<method %s::%s>", getObjClass(bound->receiver)->name->chars, bound->method->function->name->chars);
}

NATIVE_METHOD(Method, upvalueCount) {
  assertArgCount("Function::upvalueCount()", 0, argCount);
  RETURN_INT(AS_BOUND_METHOD(receiver)->method->upvalueCount);
}

// INT

NATIVE_METHOD(Int, __init__) {
  THROW_EXCEPTION(InstantiationException, "Cannot instantiate from class Int.");
}

NATIVE_METHOD(Int, __add__) {
  assertArgCount("Int::+(other)", 1, argCount);
  assertArgIsNumber("Int::+(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) + AS_INT(args[0])));
  else RETURN_NUMBER((AS_NUMBER(receiver) + AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, __subtract__) {
  assertArgCount("Int::-(other)", 1, argCount);
  assertArgIsNumber("Int::-(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) - AS_INT(args[0])));
  else RETURN_NUMBER((AS_NUMBER(receiver) - AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, __multiply__) {
  assertArgCount("Int::*(other)", 1, argCount);
  assertArgIsNumber("Int::*(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) * AS_INT(args[0])));
  else RETURN_NUMBER((AS_NUMBER(receiver) * AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, __modulo__) {
  assertArgCount("Int::%(other)", 1, argCount);
  assertArgIsNumber("Int::%(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) % AS_INT(args[0])));
  else RETURN_NUMBER(fmod(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, abs) {
  assertArgCount("Int::abs()", 0, argCount);
  RETURN_INT(abs(AS_INT(receiver)));
}

NATIVE_METHOD(Int, clone) {
  assertArgCount("Int::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Int, factorial) {
  assertArgCount("Int::factorial()", 0, argCount);
  int self = AS_INT(receiver);
  assertNumberNonNegative("Int::factorial()", self, -1);
  RETURN_INT(factorial(self));
}

NATIVE_METHOD(Int, gcd) {
  assertArgCount("Int::gcd(other)", 1, argCount);
  assertArgIsInt("Int::gcd(other)", args, 0);
  RETURN_INT(gcd(abs(AS_INT(receiver)), abs(AS_INT(args[0]))));
}

NATIVE_METHOD(Int, isEven) {
  assertArgCount("Int::isEven()", 0, argCount);
  RETURN_BOOL(AS_INT(receiver) % 2 == 0);
}

NATIVE_METHOD(Int, isOdd) {
  assertArgCount("Int::isOdd()", 0, argCount);
  RETURN_BOOL(AS_INT(receiver) % 2 != 0);
}

NATIVE_METHOD(Int, lcm) {
  assertArgCount("Int::lcm(other)", 1, argCount);
  assertArgIsInt("Int::lcm(other)", args, 0);
  RETURN_INT(lcm(abs(AS_INT(receiver)), abs(AS_INT(args[0]))));
}

NATIVE_METHOD(Int, toFloat) {
  assertArgCount("Int::toFloat()", 0, argCount);
  RETURN_NUMBER((double)AS_INT(receiver));
}

NATIVE_METHOD(Int, toBinary) {
  assertArgCount("Int::toBinary()", 0, argCount);
  char buffer[MAX_BUFFER_SIZE];
  intToBinary(AS_INT(receiver), buffer);
  RETURN_STRING(buffer, strlen(buffer));
}

NATIVE_METHOD(Int, toHexadecimal) {
  assertArgCount("Int::toHexadecimal()", 0, argCount);
  char buffer[MAX_BUFFER_SIZE];
  sprintf(buffer, "%x", AS_INT(receiver));
  RETURN_STRING(buffer, strlen(buffer));
}

NATIVE_METHOD(Int, toString) {
  assertArgCount("Int::toString()", 0, argCount);
  RETURN_STRING_FMT("%d", AS_INT(receiver));
}

// NIL

NATIVE_METHOD(Nil, __init__) {
	THROW_EXCEPTION(InstantiationException, "Cannot instantiate from class Nil.");
}

NATIVE_METHOD(Nil, clone) {
	assertArgCount("Nil::clone()", 0, argCount);
	RETURN_NIL;
}

NATIVE_METHOD(Nil, toString) {
	assertArgCount("Nil::toString()", 0, argCount);
	RETURN_STRING("nil", 3);
}

// NUMBER

NATIVE_METHOD(Number, __init__) {
  THROW_EXCEPTION(InstantiationException, "Cannot instantiate from class Number.");
}

NATIVE_METHOD(Number, __equal__) {
  assertArgCount("Number::==(other)", 1, argCount);
  assertArgIsNumber("Number::==(other)", args, 0);
  RETURN_BOOL((AS_NUMBER(receiver) == AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __greater__) {
  assertArgCount("Number::>(other)", 1, argCount);
  assertArgIsNumber("Number::>(other)", args, 0);
  RETURN_BOOL((AS_NUMBER(receiver) > AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __less__) {
  assertArgCount("Number::<(other)", 1, argCount);
  assertArgIsNumber("Number::<(other)", args, 0);
  RETURN_BOOL((AS_NUMBER(receiver) < AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __add__) { 
  assertArgCount("Number::+(other)", 1, argCount);
  assertArgIsNumber("Number::+(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) + AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __subtract__) {
  assertArgCount("Number::-(other)", 1, argCount);
  assertArgIsNumber("Number::-(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) - AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __multiply__) {
  assertArgCount("Number::*(other)", 1, argCount);
  assertArgIsNumber("Number::*(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) * AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __divide__) { 
  assertArgCount("Number::/(other)", 1, argCount);
  assertArgIsNumber("Number::/(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) / AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __modulo__) {
  assertArgCount("Number::%(other)", 1, argCount);
  assertArgIsNumber("Number::%(other)", args, 0);
  RETURN_NUMBER(fmod(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __power__) {
  assertArgCount("Number::**(other)", 1, argCount);
  assertArgIsNumber("Number::**(other)", args, 0);
  RETURN_NUMBER(pow(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, abs) {
	assertArgCount("Number::abs()", 0, argCount);
	RETURN_NUMBER(fabs(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, arccos) {
  assertArgCount("Number::acos()", 0, argCount);
  RETURN_NUMBER(acos(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, arcsin) {
  assertArgCount("Number::asin()", 0, argCount);
  RETURN_NUMBER(asin(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, arctan) {
  assertArgCount("Number::atan()", 0, argCount);
  RETURN_NUMBER(atan(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, cbrt) {
	assertArgCount("Number::cbrt()", 0, argCount);
	RETURN_NUMBER(cbrt(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, ceil) {
	assertArgCount("Number::ceil()", 0, argCount);
	RETURN_NUMBER(ceil(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, clone) {
	assertArgCount("Number::clone()", 0, argCount);
	return receiver;
}

NATIVE_METHOD(Number, cos) {
  assertArgCount("Number::cos()", 0, argCount);
  RETURN_NUMBER(cos(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, exp) {
	assertArgCount("Number::exp()", 0, argCount);
	RETURN_NUMBER(exp(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, floor) {
	assertArgCount("Number::floor()", 0, argCount);
	RETURN_NUMBER(floor(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, hypot) {
  assertArgCount("Number::hypot(other)", 1, argCount);
  assertArgIsNumber("Number::hypot(other)", args, 0);
  RETURN_NUMBER(hypot(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, log) {
	assertArgCount("Number::log()", 0, argCount);
	double self = AS_NUMBER(receiver);
	assertNumberPositive("Number::log2()", self, -1);
	RETURN_NUMBER(log(self));
}

NATIVE_METHOD(Number, log10) {
	assertArgCount("Number::log10()", 0, argCount);
	double self = AS_NUMBER(receiver);
	assertNumberPositive("Number::log10()", self, -1);
	RETURN_NUMBER(log10(self));
}

NATIVE_METHOD(Number, log2) {
	assertArgCount("Number::log2()", 0, argCount);
	double self = AS_NUMBER(receiver);
	assertNumberPositive("Number::log2()", self, -1);
	RETURN_NUMBER(log2(self));
}

NATIVE_METHOD(Number, max) {
	assertArgCount("Number::max(other)", 1, argCount);
	assertArgIsNumber("Number::max(other)", args, 0);
	RETURN_NUMBER(fmax(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, min) {
	assertArgCount("Number::min(other)", 1, argCount);
	assertArgIsNumber("Number::min(other)", args, 0);
	RETURN_NUMBER(fmin(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, pow) {
	assertArgCount("Number::pow(exponent)", 1, argCount);
	assertArgIsNumber("Number::pow(exponent)", args, 0);
	RETURN_NUMBER(pow(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, round) {
	assertArgCount("Number::round()", 0, argCount);
	RETURN_NUMBER(round(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, sin) {
  assertArgCount("Number::sin()", 0, argCount);
  RETURN_NUMBER(sin(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, sqrt) {
	assertArgCount("Number::sqrt()", 0, argCount);
	double self = AS_NUMBER(receiver);
	assertNumberPositive("Number::sqrt()", self, -1);
	RETURN_NUMBER(sqrt(self));
}

NATIVE_METHOD(Number, tan) {
  assertArgCount("Number::tan()", 0, argCount);
  RETURN_NUMBER(tan(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, toInt) {
  assertArgCount("Number::toInt()", 0, argCount);
  RETURN_INT(AS_NUMBER(receiver));
}

NATIVE_METHOD(Number, toString) {
	assertArgCount("Number::toString()", 0, argCount);
	char chars[24];
	int length = snprintf(chars, 24, "%.14g", AS_NUMBER(receiver));
	RETURN_STRING(chars, length);
}

// OBJECT


NATIVE_METHOD(Object, __equal__) {
  assertArgCount("Object::==(other)", 1, argCount);
  RETURN_BOOL(receiver == args[0]);
}

NATIVE_METHOD(Object, clone) {
	assertArgCount("Object::clone()", 0, argCount);
	ObjInstance* thisObject = AS_INSTANCE(receiver);
  ObjInstance* thatObject = newInstance(OBJ_KLASS(receiver));
  push(OBJ_VAL(thatObject));
	tableAddAll(&thisObject->fields, &thatObject->fields);
  pop();
	RETURN_OBJ(thatObject);
}

NATIVE_METHOD(Object, equals) {
	assertArgCount("Object::equals(value)", 1, argCount);
	RETURN_BOOL(valuesEqual(receiver, args[0]));
}

NATIVE_METHOD(Object, getClass) {
	assertArgCount("Object::getClass()", 0, argCount);
	RETURN_OBJ(getObjClass(receiver));
}

NATIVE_METHOD(Object, getClassName) {
	assertArgCount("Object::getClassName()", 0, argCount);
	RETURN_OBJ(getObjClass(receiver)->name);
}

NATIVE_METHOD(Object, hasField) {
	assertArgCount("Object::hasField(field)", 1, argCount);
	assertArgIsString("Object::hasField(field)", args, 0);
	if (!IS_INSTANCE(receiver)) RETURN_BOOL(false);
	ObjInstance* instance = AS_INSTANCE(receiver);
	Value value;
	RETURN_BOOL(tableGet(&instance->fields, AS_STRING(args[0]), &value));
}

NATIVE_METHOD(Object, hashCode) {
	assertArgCount("Object::hashCode()", 0, argCount);
	RETURN_INT(hashValue(receiver));
}

NATIVE_METHOD(Object, instanceOf) {
	assertArgCount("Object::instanceOf(class)", 1, argCount);
	if (!IS_CLASS(args[0])) RETURN_FALSE;
  RETURN_BOOL(isObjInstanceOf(receiver, AS_CLASS(args[0])));
}

NATIVE_METHOD(Object, memberOf) {
	assertArgCount("Object::memberOf(class)", 1, argCount);
	if (!IS_CLASS(args[0])) RETURN_FALSE;
	ObjClass* thisClass = getObjClass(receiver);
	ObjClass* thatClass = AS_CLASS(args[0]);
	RETURN_BOOL(thisClass == thatClass);
}

NATIVE_METHOD(Object, toString) {
	assertArgCount("Object::toString()", 0, argCount);
  RETURN_STRING_FMT("<object %s>", AS_OBJ(receiver)->klass->name->chars);
}

// STRING

NATIVE_METHOD(String, __init__) {
  assertArgCount("String::__init__(chars)", 1, argCount);
  assertArgIsString("String::__init__(chars)", args, 0);
  return args[0];
}


NATIVE_METHOD(String, __add__) {
  assertArgCount("String::+(other)", 1, argCount);
  assertArgIsString("String::+(other)", args, 0);
  RETURN_STRING_FMT("%s%s", AS_CSTRING(receiver), AS_CSTRING(args[0]));
}

NATIVE_METHOD(String, __getSubscript__) {
  assertArgCount("String::[](index)", 1, argCount);
  assertArgIsInt("String::[getChar]](index)", args, 0);

  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  assertIntWithinRange("String::[](index)", index, 0, self->length, 0);

  char chars[2] = { self->chars[index], '\0' };
  RETURN_STRING(chars, 1);
}

NATIVE_METHOD(String, capitalize) {
  assertArgCount("String::capitalize()", 0, argCount);
  RETURN_OBJ(capitalizeString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, clone) {
  assertArgCount("String::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(String, contains) {
  assertArgCount("String::contains(chars)", 1, argCount);
  assertArgIsString("String::contains(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  RETURN_BOOL(searchString(haystack, needle, 0) != -1);
}

NATIVE_METHOD(String, decapitalize) {
  assertArgCount("String::decapitalize()", 0, argCount);
  RETURN_OBJ(decapitalizeString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, ends) {
  assertArgCount("String::ends(chars)", 1, argCount);
  assertArgIsString("String::ends(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  if (needle->length > haystack->length) RETURN_FALSE;
  RETURN_BOOL(memcmp(haystack->chars + haystack->length - needle->length, needle->chars, needle->length) == 0);
}

NATIVE_METHOD(String, subscript) {
  assertArgCount("String::subscript(index)", 1, argCount);
  assertArgIsInt("String::subscript(index)", args, 0);

  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  assertNumberWithinRange("String::subscript(index)", index, 0, self->length, 0);

  char chars[2] = { self->chars[index], '\0' };
  RETURN_STRING(chars, 1);
}

NATIVE_METHOD(String, search) {
  assertArgCount("String::search(chars)", 1, argCount);
  assertArgIsString("String::search(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  RETURN_INT(searchString(haystack, needle, 0));
}

NATIVE_METHOD(String, length) {
  assertArgCount("String::length()", 0, argCount);
  RETURN_INT(AS_STRING(receiver)->length);
}

NATIVE_METHOD(String, next) {
  assertArgCount("String::next(index)", 1, argCount);
  ObjString* self = AS_STRING(receiver);
  if (IS_NIL(args[0])) {
    if (self->length == 0) RETURN_FALSE;
    RETURN_INT(0);
  }

  assertArgIsInt("Stirng::next(index)", args, 0);
  int index = AS_INT(args[0]);
  if (index < 0 || index < self->length - 1) RETURN_INT(index + 1);
  RETURN_NIL;
}

NATIVE_METHOD(String, nextValue) {
  assertArgCount("String::nextValue(index)", 1, argCount);
  assertArgIsInt("String::nextValue(index)", args, 0);
  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  if (index > -1 && index < self->length) {
    char chars[2] = { self->chars[index], '\0' };
    RETURN_STRING(chars, 1);
  }
  RETURN_NIL;
}

NATIVE_METHOD(String, replace) {
  assertArgCount("String::replace(target, replacement)", 2, argCount);
  assertArgIsString("String::replace(target, replacement)", args, 0);
  assertArgIsString("String::replace(target, replacement)", args, 1);
  RETURN_OBJ(replaceString(AS_STRING(receiver), AS_STRING(args[0]), AS_STRING(args[1])));
}

NATIVE_METHOD(String, reverse) {
  assertArgCount("String::reverse()", 0, argCount);
  ObjString* self = AS_STRING(receiver);
  if (self->length <= 1) return receiver;
  return OBJ_VAL(reverseStringBasedOnMemory(self));
}

NATIVE_METHOD(String, split) {
  assertArgCount("String::split(delimiter)", 1, argCount);
  assertArgIsString("String::split(delimiter)", args, 0);
  ObjString* self = AS_STRING(receiver);
  ObjString* delimiter = AS_STRING(args[0]);

  ObjArray* array = newArray();
  push(OBJ_VAL(array));
  char* string = strdup(self->chars);
  char* next = NULL;
  char* token = strtok_r(string, delimiter->chars, &next);
  while (token != NULL) {
    writeValueArray(&array->elements, OBJ_VAL(copyString(token, (int)strlen(token))));
    token = strtok_r(NULL, delimiter->chars, &next);
  }
  free(string);
  pop();
  RETURN_OBJ(array);
}

NATIVE_METHOD(String, starts) {
  assertArgCount("String::starts(chars)", 1, argCount);
  assertArgIsString("String::starts(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  if (needle->length > haystack->length) RETURN_FALSE;
  RETURN_BOOL(memcmp(haystack->chars, needle->chars, needle->length) == 0);
}

NATIVE_METHOD(String, cut) {
  assertArgCount("String::cut(from, to)", 2, argCount);
  assertArgIsInt("String::cut(from, to)", args, 0);
  assertArgIsInt("String::cut(from, to)", args, 1);
  RETURN_OBJ(subString(AS_STRING(receiver), AS_INT(args[0]), AS_INT(args[1])));
}

NATIVE_METHOD(String, lower) {
  assertArgCount("String::lower()", 0, argCount);
  RETURN_OBJ(toLowerString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, toString) {
  assertArgCount("String::toString()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(String, upper) {
  assertArgCount("String::upper()", 0, argCount);
  RETURN_OBJ(toUpperString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, trim) {
  assertArgCount("String::trim()", 0, argCount);
  RETURN_OBJ(trimString(AS_STRING(receiver)));
}



NATIVE_METHOD(Namespace, init) {
  THROW_EXCEPTION(InstantiationException, "Cannot instantiate from class Namespace.");
  RETURN_NIL;
}

NATIVE_METHOD(Namespace, clone) {
  assertArgCount("Namespace::clone()", 0, argCount);
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(Namespace, toString) {
  assertArgCount("Namespace::toString()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  RETURN_STRING_FMT("<namespace %s.%s>", self->path->chars, self->name->chars);
}


void registerLangPackage(){
	vm.objectClass = defineNativeClass("Object");
  DEF_METHOD(vm.objectClass, Object, clone, 0);
  DEF_METHOD(vm.objectClass, Object, equals, 1);
  DEF_METHOD(vm.objectClass, Object, getClass, 0);
  DEF_METHOD(vm.objectClass, Object, getClassName, 0);
  DEF_METHOD(vm.objectClass, Object, hasField, 1);
  DEF_METHOD(vm.objectClass, Object, hashCode, 0);
  DEF_METHOD(vm.objectClass, Object, instanceOf, 1);
  DEF_METHOD(vm.objectClass, Object, memberOf, 1);
  DEF_METHOD(vm.objectClass, Object, toString, 0);
  DEF_OPERATOR(vm.objectClass, Object, ==, __equal__, 1);

  vm.classClass = defineNativeClass("Class");
  bindSuperclass(vm.classClass, vm.objectClass);
  DEF_METHOD(vm.classClass, Class, __init__, 2);
  DEF_METHOD(vm.classClass, Class, clone, 0);
  DEF_METHOD(vm.classClass, Class, getClass, 0);
  DEF_METHOD(vm.classClass, Class, getClassName, 0);
  DEF_METHOD(vm.classClass, Class, instanceOf, 1);
  DEF_METHOD(vm.classClass, Class, memberOf, 1);
  DEF_METHOD(vm.classClass, Class, name, 0);
  DEF_METHOD(vm.classClass, Class, superclass, 0);
  DEF_METHOD(vm.classClass, Class, toString, 0);
  DEF_OPERATOR(vm.classClass, Class, (), __invoke__, -1);
  vm.objectClass->obj.klass = vm.classClass;


  vm.namespaceClass = defineNativeClass("Namespace");
  bindSuperclass(vm.namespaceClass, vm.objectClass);
  DEF_METHOD(vm.namespaceClass, Namespace, clone, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, init, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, toString, 0);

  vm.exceptionClass = defineNativeClass("Exception");
  bindSuperclass(vm.exceptionClass, vm.objectClass);
  DEF_METHOD(vm.exceptionClass, Exception, __init__, 1);
  DEF_METHOD(vm.exceptionClass, Exception, toString, 0);


  ObjClass* runtimeExceptionClass = defineNativeException("RuntimeException", vm.exceptionClass);
  defineNativeException("ArithmeticException", runtimeExceptionClass);
  defineNativeException("IllegalArgumentException", runtimeExceptionClass);
  defineNativeException("IndexOutOfBoundsException", runtimeExceptionClass);
  defineNativeException("UnsupportedOperationException", runtimeExceptionClass);
  defineNativeException("InstantiationException", runtimeExceptionClass);
  defineNativeException("CallException", runtimeExceptionClass);

	vm.nilClass = defineNativeClass("Nil");
	bindSuperclass(vm.nilClass, vm.objectClass);
	DEF_METHOD(vm.nilClass, Nil, __init__, 0);
	DEF_METHOD(vm.nilClass, Nil, clone, 0);
	DEF_METHOD(vm.nilClass, Nil, toString, 0);

	vm.boolClass = defineNativeClass("Bool");
	bindSuperclass(vm.boolClass, vm.objectClass);
	DEF_METHOD(vm.boolClass, Bool, __init__, 0);
	DEF_METHOD(vm.boolClass, Bool, clone, 0);
	DEF_METHOD(vm.boolClass, Bool, toString, 0);

	vm.numberClass = defineNativeClass("Number");
	bindSuperclass(vm.numberClass, vm.objectClass);
  DEF_METHOD(vm.numberClass, Number, __init__, 0);
  DEF_METHOD(vm.numberClass, Number, abs, 0);
  DEF_METHOD(vm.numberClass, Number, arccos, 0);
  DEF_METHOD(vm.numberClass, Number, arcsin, 0);
  DEF_METHOD(vm.numberClass, Number, arctan, 0);
  DEF_METHOD(vm.numberClass, Number, cbrt, 0);
  DEF_METHOD(vm.numberClass, Number, ceil, 0);
  DEF_METHOD(vm.numberClass, Number, clone, 0);
  DEF_METHOD(vm.numberClass, Number, cos, 0);
  DEF_METHOD(vm.numberClass, Number, exp, 1);
  DEF_METHOD(vm.numberClass, Number, floor, 0);
  DEF_METHOD(vm.numberClass, Number, hypot, 1);
  DEF_METHOD(vm.numberClass, Number, log, 0);
  DEF_METHOD(vm.numberClass, Number, log2, 0);
  DEF_METHOD(vm.numberClass, Number, log10, 0);
  DEF_METHOD(vm.numberClass, Number, max, 1);
  DEF_METHOD(vm.numberClass, Number, min, 1);
  DEF_METHOD(vm.numberClass, Number, pow, 1);
  DEF_METHOD(vm.numberClass, Number, round, 0);
  DEF_METHOD(vm.numberClass, Number, sin, 0);
  DEF_METHOD(vm.numberClass, Number, sqrt, 0);
  DEF_METHOD(vm.numberClass, Number, tan, 0);
  DEF_METHOD(vm.numberClass, Number, toInt, 0);
  DEF_METHOD(vm.numberClass, Number, toString, 0);
  DEF_OPERATOR(vm.numberClass, Number, ==, __equal__, 1);
  DEF_OPERATOR(vm.numberClass, Number, >, __greater__, 1);
  DEF_OPERATOR(vm.numberClass, Number, <, __less__, 1);
  DEF_OPERATOR(vm.numberClass, Number, +, __add__, 1);
  DEF_OPERATOR(vm.numberClass, Number, -, __subtract__, 1);
  DEF_OPERATOR(vm.numberClass, Number, *, __multiply__, 1);
  DEF_OPERATOR(vm.numberClass, Number, /, __divide__, 1);
  DEF_OPERATOR(vm.numberClass, Number, %, __modulo__, 1);
  DEF_OPERATOR(vm.numberClass, Number, **, __power__, 1);

  vm.intClass = defineNativeClass("Int");
  bindSuperclass(vm.intClass, vm.numberClass);
  DEF_METHOD(vm.intClass, Int, __init__, 0);
  DEF_METHOD(vm.intClass, Int, abs, 0);
  DEF_METHOD(vm.intClass, Int, clone, 0);
  DEF_METHOD(vm.intClass, Int, factorial, 0);
  DEF_METHOD(vm.intClass, Int, gcd, 1);
  DEF_METHOD(vm.intClass, Int, isEven, 0);
  DEF_METHOD(vm.intClass, Int, isOdd, 0);
  DEF_METHOD(vm.intClass, Int, lcm, 1);
  DEF_METHOD(vm.intClass, Int, toBinary, 0);
  DEF_METHOD(vm.intClass, Int, toFloat, 0);
  DEF_METHOD(vm.intClass, Int, toHexadecimal, 0);
  DEF_METHOD(vm.intClass, Int, toString, 0);
  DEF_OPERATOR(vm.intClass, Int, +, __add__, 1);
  DEF_OPERATOR(vm.intClass, Int, -, __subtract__, 1);
  DEF_OPERATOR(vm.intClass, Int, *, __multiply__, 1);
  DEF_OPERATOR(vm.intClass, Int, %, __modulo__, 1);

  vm.floatClass = defineNativeClass("Float");
  bindSuperclass(vm.floatClass, vm.numberClass);
  DEF_METHOD(vm.floatClass, Float, __init__, 0);
  DEF_METHOD(vm.floatClass, Float, clone, 0);
  DEF_METHOD(vm.floatClass, Float, toString, 0);


  vm.stringClass = defineNativeClass("String");
  bindSuperclass(vm.stringClass, vm.objectClass);
  DEF_METHOD(vm.stringClass, String, __init__, 1);
  DEF_METHOD(vm.stringClass, String, capitalize, 0);
  DEF_METHOD(vm.stringClass, String, clone, 0);
  DEF_METHOD(vm.stringClass, String, contains, 1);
  DEF_METHOD(vm.stringClass, String, decapitalize, 0);
  DEF_METHOD(vm.stringClass, String, ends, 1);
  DEF_METHOD(vm.stringClass, String, subscript, 1);
  DEF_METHOD(vm.stringClass, String, search, 1);
  DEF_METHOD(vm.stringClass, String, length, 0);
  DEF_METHOD(vm.stringClass, String, next, 1);
  DEF_METHOD(vm.stringClass, String, nextValue, 1);
  DEF_METHOD(vm.stringClass, String, replace, 2);
  DEF_METHOD(vm.stringClass, String, reverse, 0);
  DEF_METHOD(vm.stringClass, String, split, 1);
  DEF_METHOD(vm.stringClass, String, starts, 1);
  DEF_METHOD(vm.stringClass, String, cut, 2);
  DEF_METHOD(vm.stringClass, String, lower, 0);
  DEF_METHOD(vm.stringClass, String, toString, 0);
  DEF_METHOD(vm.stringClass, String, upper, 0);
  DEF_METHOD(vm.stringClass, String, trim, 0);
  DEF_OPERATOR(vm.stringClass, String, +, __add__, 1);
  DEF_OPERATOR(vm.stringClass, String, [], __getSubscript__, 1);

  for (int i = 0; i < vm.strings.capacity; i++) {
    Entry* entry = &vm.strings.entries[i];
    if (entry->key == NULL) continue;
    entry->key->obj.klass = vm.stringClass;
  }

  vm.functionClass = defineNativeClass("Function");
  bindSuperclass(vm.functionClass, vm.objectClass);
  DEF_METHOD(vm.functionClass, Function, __init__, 0);
  DEF_METHOD(vm.functionClass, Function, arity, 0);
  DEF_METHOD(vm.functionClass, Function, clone, 0);
  DEF_METHOD(vm.functionClass, Function, isNative, 0);
  DEF_METHOD(vm.functionClass, Function, name, 0);
  DEF_METHOD(vm.functionClass, Function, toString, 0);
  DEF_METHOD(vm.functionClass, Function, upvalueCount, 0);
  DEF_OPERATOR(vm.functionClass, Function, (), __invoke__, -1);

  vm.methodClass = defineNativeClass("Method");
  bindSuperclass(vm.methodClass, vm.objectClass);
  DEF_METHOD(vm.methodClass, Method, __init__, 0);
  DEF_METHOD(vm.methodClass, Method, arity, 0);
  DEF_METHOD(vm.methodClass, Method, clone, 0);
  DEF_METHOD(vm.methodClass, Method, name, 0);
  DEF_METHOD(vm.methodClass, Method, receiver, 0);
  DEF_METHOD(vm.methodClass, Method, toString, 0);
  DEF_METHOD(vm.methodClass, Method, upvalueCount, 0);
}
