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

// BOOL

NATIVE_METHOD(Bool, __init__) {
	assertError("Cannot instantiate from class Bool.");
	RETURN_NIL;
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

// CLASS

NATIVE_METHOD(Class, __init__) {
  assertArgCount("Class::__init__(name, superclass)", 2, argCount);
  assertArgIsString("Class::__init__(name, superclass)", args, 0);
  assertArgIsClass("Class::__init__(name, superclass)", args, 1);
  ObjClass* klass = newClass(AS_STRING(args[0]));
  bindSuperclass(klass, AS_CLASS(args[1]));
  RETURN_OBJ(klass);
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
  assertError("Cannot instantiate from class Float.");
  RETURN_NIL;
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
  assertError("Cannot instantiate from class Function.");
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
  assertError("Cannot instantiate from class Method.");
  RETURN_NIL;
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
  assertError("Cannot instantiate from class Int.");
  RETURN_NIL;
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
  assertPositiveNumber("Int::factorial()", self, -1);
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

NATIVE_METHOD(Int, toString) {
  assertArgCount("Int::toString()", 0, argCount);
  RETURN_STRING_FMT("%d", AS_INT(receiver));
}

// NIL

NATIVE_METHOD(Nil, __init__) {
	assertError("Cannot instantiate from class Nil.");
	RETURN_NIL;
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
	assertError("Cannot instantiate from class Number.");
	RETURN_NIL;
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
	assertPositiveNumber("Number::log2()", self, -1);
	RETURN_NUMBER(log(self));
}

NATIVE_METHOD(Number, log10) {
	assertArgCount("Number::log10()", 0, argCount);
	double self = AS_NUMBER(receiver);
	assertPositiveNumber("Number::log10()", self, -1);
	RETURN_NUMBER(log10(self));
}

NATIVE_METHOD(Number, log2) {
	assertArgCount("Number::log2()", 0, argCount);
	double self = AS_NUMBER(receiver);
	assertPositiveNumber("Number::log2()", self, -1);
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
	assertPositiveNumber("Number::sqrt()", self, -1);
	RETURN_NUMBER(sqrt(self));
}

NATIVE_METHOD(Number, tan) {
  assertArgCount("Number::tan()", 0, argCount);
  RETURN_NUMBER(tan(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, toInt) {
  assertArgCount("Number::toInt()", 0, argCount);
  RETURN_INT(AS_INT(receiver));
}

NATIVE_METHOD(Number, toString) {
	assertArgCount("Number::toString()", 0, argCount);
	char chars[24];
	int length = snprintf(chars, 24, "%.14g", AS_NUMBER(receiver));
	RETURN_STRING(chars, length);
}

// OBJECT

NATIVE_METHOD(Object, clone) {
	assertArgCount("Object::clone()", 0, argCount);
	ObjInstance* thisObject = AS_INSTANCE(receiver);
  ObjInstance* thatObject = newInstance(OBJ_KLASS(receiver));
	tableAddAll(&thisObject->fields, &thatObject->fields);
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
	ObjClass* thisClass = getObjClass(receiver);
	ObjClass* thatClass = AS_CLASS(args[0]);
	if (thisClass == thatClass) RETURN_TRUE;

	ObjClass* superclass = thisClass->superclass;
	while (superclass != NULL) {
		if (superclass == thatClass) RETURN_TRUE;
		superclass = superclass->superclass;
	}
	RETURN_FALSE;
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

NATIVE_METHOD(String, endsWith) {
  assertArgCount("String::endsWith(chars)", 1, argCount);
  assertArgIsString("String::endsWith(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  if (needle->length > haystack->length) RETURN_FALSE;
  RETURN_BOOL(memcmp(haystack->chars + haystack->length - needle->length, needle->chars, needle->length) == 0);
}

NATIVE_METHOD(String, getChar) {
  assertArgCount("String::getChar(index)", 1, argCount);
  assertArgIsInt("String::getChar(index)", args, 0);

  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  assertIndexWithinRange("String::getChar(index)", index, 0, self->length, 0);

  char chars[2];
  chars[0] = self->chars[index];
  chars[1] = '\n';
  RETURN_STRING(chars, 1);
}

NATIVE_METHOD(String, indexOf) {
  assertArgCount("String::indexOf(chars)", 1, argCount);
  assertArgIsString("String::indexOf(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  RETURN_INT(searchString(haystack, needle, 0));
}

NATIVE_METHOD(String, length) {
  assertArgCount("String::length()", 0, argCount);
  RETURN_INT(AS_STRING(receiver)->length);
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

NATIVE_METHOD(String, startsWith) {
  assertArgCount("String::startsWith(chars)", 1, argCount);
  assertArgIsString("String::startsWith(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  if (needle->length > haystack->length) RETURN_FALSE;
  RETURN_BOOL(memcmp(haystack->chars, needle->chars, needle->length) == 0);
}

NATIVE_METHOD(String, subString) {
  assertArgCount("String::subString(from, to)", 2, argCount);
  assertArgIsInt("String::subString(from, to)", args, 0);
  assertArgIsInt("String::subString(from, to)", args, 1);
  RETURN_OBJ(subString(AS_STRING(receiver), AS_INT(args[0]), AS_INT(args[1])));
}

NATIVE_METHOD(String, toLowercase) {
  assertArgCount("String::toLowercase()", 0, argCount);
  RETURN_OBJ(toLowerString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, toString) {
  assertArgCount("String::toString()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(String, toUppercase) {
  assertArgCount("String::toLowercase()", 0, argCount);
  RETURN_OBJ(toUpperString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, trim) {
  assertArgCount("String::trim()", 0, argCount);
  RETURN_OBJ(trimString(AS_STRING(receiver)));
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
  DEF_METHOD(vm.intClass, Int, toFloat, 0);
  DEF_METHOD(vm.intClass, Int, toString, 0);

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
  DEF_METHOD(vm.stringClass, String, endsWith, 1);
  DEF_METHOD(vm.stringClass, String, getChar, 1);
  DEF_METHOD(vm.stringClass, String, indexOf, 1);
  DEF_METHOD(vm.stringClass, String, length, 0);
  DEF_METHOD(vm.stringClass, String, replace, 2);
  DEF_METHOD(vm.stringClass, String, reverse, 0);
  DEF_METHOD(vm.stringClass, String, startsWith, 1);
  DEF_METHOD(vm.stringClass, String, subString, 2);
  DEF_METHOD(vm.stringClass, String, toLowercase, 0);
  DEF_METHOD(vm.stringClass, String, toString, 0);
  DEF_METHOD(vm.stringClass, String, toUppercase, 0);
  DEF_METHOD(vm.stringClass, String, trim, 0);

  vm.functionClass = defineNativeClass("Function");
  bindSuperclass(vm.functionClass, vm.objectClass);
  DEF_METHOD(vm.functionClass, Function, __init__, 0);
  DEF_METHOD(vm.functionClass, Function, arity, 0);
  DEF_METHOD(vm.functionClass, Function, clone, 0);
  DEF_METHOD(vm.functionClass, Function, name, 0);
  DEF_METHOD(vm.functionClass, Function, toString, 0);
  DEF_METHOD(vm.functionClass, Function, upvalueCount, 0);

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
