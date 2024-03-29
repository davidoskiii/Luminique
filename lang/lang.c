#include <stdio.h>
#include <stdlib.h>

#include "lang.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../vm/vm.h"

NATIVE_METHOD(Object, equals) {
	assertArgCount("Object::equals(value)", 1, argCount);
	RETURN_BOOL(valuesEqual(receiver, args[0]));
}

NATIVE_METHOD(Object, getClass) {
	assertArgCount("Object::getClass()", 0, argCount);
	RETURN_OBJ(AS_INSTANCE(receiver)->klass);
}

NATIVE_METHOD(Object, getClassName) {
	assertArgCount("Object::getClassName()", 0, argCount);
	RETURN_OBJ(AS_INSTANCE(receiver)->klass->name);
}

NATIVE_METHOD(Object, hasField) {
	assertArgCount("Object::hasField(field)", 1, argCount);
	assertArgIsString("Object::hasField(field)", args, 0);
	if (!IS_INSTANCE(receiver)) RETURN_BOOL(false);
	ObjInstance* instance = AS_INSTANCE(receiver);
	Value value;
	RETURN_BOOL(tableGet(&instance->fields, AS_STRING(args[0]), &value));
}

NATIVE_METHOD(Object, instanceOf) {
	assertArgCount("Object::instanceOf(class)", 1, argCount);
	if (!IS_CLASS(args[0])) RETURN_FALSE;
	ObjClass* thisClass = AS_INSTANCE(receiver)->klass;
	ObjClass* thatClass = AS_CLASS(args[0]);
	RETURN_BOOL(thisClass == thatClass);
}

NATIVE_METHOD(Object, memberOf) {
	assertArgCount("Object::memberOf(class)", 1, argCount);
	if (!IS_CLASS(args[0])) RETURN_FALSE;
	ObjClass* thisClass = AS_INSTANCE(receiver)->klass;
	ObjClass* thatClass = AS_CLASS(args[0]);
	RETURN_BOOL(thisClass == thatClass);
}

NATIVE_METHOD(Object, toString) {
	assertArgCount("Object::toString()", 0, argCount);

	if (IS_BOOL(receiver)) {
		if (AS_BOOL(receiver)) RETURN_STRING("true", 4);
		else RETURN_STRING("false", 5);
	}
	else if (IS_NIL(receiver)) {
		RETURN_STRING("nil", 3);
	}
	else if (IS_NUMBER(receiver)) {
		char chars[24];
		int length = snprintf(chars, 24, "%.14g", AS_NUMBER(args[0]));
		RETURN_STRING(chars, length);
	}
	else {
		RETURN_STRING("object", 6);
	}
}

void registerLangPackage(){
	ObjClass* objectClass = defineNativeClass("Object");
	DEF_METHOD(objectClass, Object, equals);
	DEF_METHOD(objectClass, Object, getClass);
	DEF_METHOD(objectClass, Object, getClassName);
	DEF_METHOD(objectClass, Object, hasField);
	DEF_METHOD(objectClass, Object, instanceOf);
	DEF_METHOD(objectClass, Object, memberOf);
	DEF_METHOD(objectClass, Object, toString);
	vm.objectClass = objectClass;
}
