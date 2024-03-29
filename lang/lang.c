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

NATIVE_METHOD(Nil, clone) {
	assertArgCount("Nil::clone()", 0, argCount);
	RETURN_NIL;
}

// NIL

NATIVE_METHOD(Nil, __init__) {
	assertError("Cannot instantiate from class Nil.");
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

NATIVE_METHOD(Number, exp) {
	assertArgCount("Number::exp()", 0, argCount);
	RETURN_NUMBER(exp(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, floor) {
	assertArgCount("Number::floor()", 0, argCount);
	RETURN_NUMBER(floor(AS_NUMBER(receiver)));
}

NATIVE_METHOD(Number, toString) {
	assertArgCount("Number::toString()", 0, argCount);
	char chars[24];
	int length = snprintf(chars, 24, "%.14g", AS_NUMBER(receiver));
	RETURN_STRING(chars, length);
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

NATIVE_METHOD(Number, sqrt) {
	assertArgCount("Number::sqrt()", 0, argCount);
	double self = AS_NUMBER(receiver);
	assertPositiveNumber("Number::sqrt()", self, -1);
	RETURN_NUMBER(sqrt(self));
}

// OBJECT

NATIVE_METHOD(Object, clone) {
	assertArgCount("Object::clone()", 0, argCount);
	ObjInstance* thisObject = AS_INSTANCE(receiver);
	ObjInstance* thatObject = newInstance(thisObject->klass);
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
	RETURN_NUMBER(hashValue(receiver));
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
	RETURN_STRING_FMT("[object %s]", AS_INSTANCE(receiver)->klass->name->chars);
}

void registerLangPackage(){
	vm.objectClass = defineNativeClass("Object");
	DEF_METHOD(vm.objectClass, Object, clone);
	DEF_METHOD(vm.objectClass, Object, equals);
	DEF_METHOD(vm.objectClass, Object, getClass);
	DEF_METHOD(vm.objectClass, Object, getClassName);
	DEF_METHOD(vm.objectClass, Object, hasField);
	DEF_METHOD(vm.objectClass, Object, hashCode);
	DEF_METHOD(vm.objectClass, Object, instanceOf);
	DEF_METHOD(vm.objectClass, Object, memberOf);
	DEF_METHOD(vm.objectClass, Object, toString);

	vm.nilClass = defineNativeClass("Nil");
	bindSuperclass(vm.nilClass, vm.objectClass);
	DEF_METHOD(vm.nilClass, Nil, __init__);
	DEF_METHOD(vm.nilClass, Nil, clone);
	DEF_METHOD(vm.nilClass, Nil, toString);

	vm.boolClass = defineNativeClass("Bool");
	bindSuperclass(vm.boolClass, vm.objectClass);
	DEF_METHOD(vm.boolClass, Bool, __init__);
	DEF_METHOD(vm.boolClass, Bool, clone);
	DEF_METHOD(vm.boolClass, Bool, toString);

	vm.numberClass = defineNativeClass("Number");
	bindSuperclass(vm.numberClass, vm.objectClass);
	DEF_METHOD(vm.numberClass, Number, __init__);
	DEF_METHOD(vm.numberClass, Number, abs);
	DEF_METHOD(vm.numberClass, Number, cbrt);
	DEF_METHOD(vm.numberClass, Number, ceil);
	DEF_METHOD(vm.numberClass, Number, clone);
	DEF_METHOD(vm.numberClass, Number, exp);
	DEF_METHOD(vm.numberClass, Number, floor);
	DEF_METHOD(vm.numberClass, Number, log);
	DEF_METHOD(vm.numberClass, Number, log2);
	DEF_METHOD(vm.numberClass, Number, log10);
	DEF_METHOD(vm.numberClass, Number, max);
	DEF_METHOD(vm.numberClass, Number, min);
	DEF_METHOD(vm.numberClass, Number, pow);
	DEF_METHOD(vm.numberClass, Number, round);
	DEF_METHOD(vm.numberClass, Number, sqrt);
	DEF_METHOD(vm.numberClass, Number, toString);
}
