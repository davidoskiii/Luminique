#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../value/value.h"
#include "../vm/vm.h"

static int arrayIndexOf(ObjArray* array, Value element) {
	for (int i = 0; i < array->elements.count; i++) {
		if (valuesEqual(array->elements.values[i], element)) {
			return i;
		}
	}
	return -1;
}

static bool arrayEqual(ObjArray* array1, ObjArray* array2) {
	if (array1->elements.count != array2->elements.count) return false;

	for (int i = 0; i < array1->elements.count; i++) {
		if (array1->elements.values[i].type != array2->elements.values[i].type) return false;
	}

	return true;
}

static void arrayInsertAt(ObjArray* array, int index, Value element) {
	if (IS_OBJ(element)) push(element);
	writeValueArray(&array->elements, NIL_VAL);
	if (IS_OBJ(element)) pop(vm);

	for (int i = array->elements.count - 1; i > index; i--) {
		array->elements.values[i] = array->elements.values[i - 1];
	}
	array->elements.values[index] = element;
}

static int arrayLastIndexOf(ObjArray* array, Value element) {
	for (int i = array->elements.count - 1; i >= 0; i--) {
		if (valuesEqual(array->elements.values[i], element)) {
			return i;
		}
	}
	return -1;
}

static Value arrayRemoveAt(ObjArray* array, int index) {
	Value element = array->elements.values[index];
	if (IS_OBJ(element)) push(element);

	for (int i = index; i < array->elements.count - 1; i++) {
		array->elements.values[i] = array->elements.values[i + 1];
	}
	array->elements.count--;

	if (IS_OBJ(element)) pop(vm);
	return element;
}

static ObjString* arrayToString(ObjArray* array) {
	if (array->elements.count == 0) return copyString("[]", 2);
	else {
		char string[UINT8_MAX] = "";
		string[0] = '[';
		size_t offset = 1;
		for (int i = 0; i < array->elements.count; i++) {
			char* chars = valueToString(array->elements.values[i]);
			size_t length = strlen(chars);
			memcpy(string + offset, chars, length);
			if (i == array->elements.count - 1) {
				offset += length;
			}
			else{
				memcpy(string + offset + length, ", ", 2);
				offset += length + 2;
			}
		}
		string[offset] = ']';
		string[offset + 1] = '\0';
		return copyString(string, (int)offset + 1);
	}
}

NATIVE_METHOD(Array, __init__) {
	assertArgCount("Array::__init__()", 0, argCount);
	RETURN_OBJ(newArray());
}

NATIVE_METHOD(Array, append) {
	assertArgCount("Array::append(element)", 1, argCount);
	writeValueArray(&AS_ARRAY(receiver)->elements, args[0]);
	RETURN_OBJ(&receiver);
}

NATIVE_METHOD(Array, clear) {
	assertArgCount("Array::clear()", 0, argCount);
	freeValueArray(&AS_ARRAY(receiver)->elements);
	RETURN_OBJ(&receiver);
}

NATIVE_METHOD(Array, clone) {
	assertArgCount("Array::clone()", 0, argCount);
	ObjArray* self = AS_ARRAY(receiver);
	RETURN_OBJ(copyArray(self->elements, 0, self->elements.count));
}

NATIVE_METHOD(Array, contains) {
	assertArgCount("Array::contains(element)", 1, argCount);
	RETURN_BOOL(arrayIndexOf(AS_ARRAY(receiver), args[0]) != -1);
}

NATIVE_METHOD(Array, equals) {
	assertArgCount("Array::equals(other)", 1, argCount);
	if (!IS_ARRAY(args[0])) RETURN_FALSE;
	RETURN_BOOL(arrayEqual(AS_ARRAY(receiver), AS_ARRAY(args[0])));
}

NATIVE_METHOD(Array, getAt) {
	assertArgCount("Array::getAt(index)", 1, argCount);
	assertArgIsInt("Array::getAt(index)", args, 0);
	ObjArray* self = AS_ARRAY(receiver);
	int index = AS_INT(args[0]);
	assertIndexWithinRange("Array::getAt(index)", index, 0, self->elements.count - 1, 0);
	RETURN_VAL(self->elements.values[index]);
}

NATIVE_METHOD(Array, isEmpty) {
	assertArgCount("Array::isEmpty()", 0, argCount);
	RETURN_BOOL(AS_ARRAY(receiver)->elements.count == 0);
}

NATIVE_METHOD(Array, indexOf) {
	assertArgCount("Array::indexOf(element)", 1, argCount);
	ObjArray* self = AS_ARRAY(receiver);
	if (self->elements.count == 0) RETURN_NIL;
	RETURN_INT(arrayIndexOf(self, args[0]));
}

NATIVE_METHOD(Array, insertAt) {
	assertArgCount("Array::insertAt(index, element)", 2, argCount);
	assertArgIsInt("Array::insertAt(index, element)", args, 0);
	ObjArray* self = AS_ARRAY(receiver);
	int index = AS_INT(args[0]);
	assertIndexWithinRange("Array::insertAt(index)", index, 0, self->elements.count, 0);
	arrayInsertAt(self, index, args[1]);
	RETURN_VAL(args[1]);
}

NATIVE_METHOD(Array, lastIndexOf) {
	assertArgCount("Array::indexOf(element)", 1, argCount);
	ObjArray* self = AS_ARRAY(receiver);
	if (self->elements.count == 0) RETURN_NIL;
	RETURN_INT(arrayLastIndexOf(self, args[0]));
}

NATIVE_METHOD(Array, length) {
	assertArgCount("Array::length()", 0, argCount);
	RETURN_INT(AS_ARRAY(receiver)->elements.count);
}

NATIVE_METHOD(Array, remove) {
	assertArgCount("Array::remove(element)", 1, argCount);
	ObjArray* self = AS_ARRAY(receiver);
	int index = arrayIndexOf(self, args[0]);
	if (index == -1) RETURN_FALSE;
	arrayRemoveAt(self, index);
	RETURN_TRUE;
}

NATIVE_METHOD(Array, removeAt) {
	assertArgCount("Array::removeAt(index)", 1, argCount);
	assertArgIsInt("Array::removeAt(index)", args, 0);
	ObjArray* self = AS_ARRAY(receiver);
	int index = AS_INT(args[0]);
	assertIndexWithinRange("Array::removeAt(index)", AS_INT(args[0]), 0, self->elements.count - 1, 0);
	Value element = arrayRemoveAt(self, index);
	RETURN_VAL(element);
}

NATIVE_METHOD(Array, setAt) {
	assertArgCount("Array::setAt(index, element)", 2, argCount);
	assertArgIsInt("Array::setAt(index, element)", args, 0);
	ObjArray* self = AS_ARRAY(receiver);
	int index = AS_INT(args[0]);
	assertIndexWithinRange("Array::insertAt(index)", index, 0, self->elements.count, 0);
	self->elements.values[index] = args[1];
	if (index == self->elements.count) self->elements.count++;
	return receiver;
}


NATIVE_METHOD(Array, subArray) {
	assertArgCount("Array::subArray(from, to)", 2, argCount);
	assertArgIsInt("Array::subArray(from, to)", args, 0);
	assertArgIsInt("Array::subArray(from, to)", args, 1);
	ObjArray* self = AS_ARRAY(receiver);
	int fromIndex = AS_INT(args[0]);
	int toIndex = AS_INT(args[1]);

	assertIndexWithinRange("Array::subArray(from, to)", fromIndex, 0, self->elements.count, 0);
	assertIndexWithinRange("Array::subArray(from, to", toIndex, fromIndex, self->elements.count, 1);
	RETURN_OBJ(copyArray(self->elements, fromIndex, toIndex));
}

NATIVE_METHOD(Array, toString) {
	assertArgCount("Array::toString()", 0, argCount);
	RETURN_OBJ(arrayToString(AS_ARRAY(receiver)));
}

void registerUtilPackage() {
	vm.arrayClass = defineNativeClass("Array");
	bindSuperclass(vm.arrayClass, vm.objectClass);
	DEF_METHOD(vm.arrayClass, Array, __init__, 0);
	DEF_METHOD(vm.arrayClass, Array, append, 1);
	DEF_METHOD(vm.arrayClass, Array, clear, 0);
	DEF_METHOD(vm.arrayClass, Array, clone, 0);
	DEF_METHOD(vm.arrayClass, Array, contains, 1);
	DEF_METHOD(vm.arrayClass, Array, equals, 1);
	DEF_METHOD(vm.arrayClass, Array, getAt, 1);
	DEF_METHOD(vm.arrayClass, Array, indexOf, 1);
	DEF_METHOD(vm.arrayClass, Array, insertAt, 2);
  DEF_METHOD(vm.arrayClass, Array, isEmpty, 0);
	DEF_METHOD(vm.arrayClass, Array, lastIndexOf, 1);
	DEF_METHOD(vm.arrayClass, Array, length, 0);
	DEF_METHOD(vm.arrayClass, Array, remove, 1);
	DEF_METHOD(vm.arrayClass, Array, removeAt, 1);
  DEF_METHOD(vm.arrayClass, Array, subArray, 2);
	DEF_METHOD(vm.arrayClass, Array, toString, 0);
}
