#include <stdlib.h>
#include <stdio.h>

#include "../../assert/assert.h"
#include "../native/native.h"
#include "../../object/object.h"
#include "../../vm/vm.h"

NATIVE_METHOD(Object, equals) {
	assertArgCount("Object::equals(value)", 1, argCount);
	RETURN_BOOL(valuesEqual(receiver, args[0]));
}

NATIVE_METHOD(Object, toString) {
	assertArgCount("Object::toString()", 0, argCount);

	if (IS_BOOL(args[0])) {
    if (AS_BOOL(args[0])) {
		  return OBJ_VAL(copyString("true", 4));
    } else {
		  return OBJ_VAL(copyString("false", 5));
    }
	} else if (IS_NIL(args[0])) {
		return OBJ_VAL(copyString("nil", 3));
	} else if (IS_NUMBER(args[0])) {
		char chars[24];
		int length = snprintf(chars, 24, "%.14g", AS_NUMBER(args[0]));
		return OBJ_VAL(copyString(chars, length));
	} else {
		return OBJ_VAL(copyString("Object", 6));
	}
}

void initObject(){
	ObjClass* objectClass = defineNativeClass("Object");
	vm.objectClass = objectClass;
	DEF_METHOD(objectClass, Object, equals);
	DEF_METHOD(objectClass, Object, toString);
}
