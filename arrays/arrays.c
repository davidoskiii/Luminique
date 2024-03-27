#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arrays.h"

int arrayIndexOf(ObjArray* array, Value element) {
	for (int i = 0; i < array->elements.count; i++) {
		if (valuesEqual(array->elements.values[i], element)) {
			return i;
		}
	}
	return -1;
}

void arrayInsertAt(ObjArray* array, int index, Value element) {
	if (IS_OBJ(element)) push(element);
	writeValueArray(&array->elements, NIL_VAL);
	if (IS_OBJ(element)) pop();

	for (int i = array->elements.count - 1; i > index; i--) {
		array->elements.values[i] = array->elements.values[i - 1];
	}
	array->elements.values[index] = element;
}

int arrayLastIndexOf(ObjArray* array, Value element) {
	for (int i = array->elements.count - 1; i >= 0; i--) {
		if (valuesEqual(array->elements.values[i], element)) {
			return i;
		}
	}
	return -1;
}

Value arrayRemoveAt(ObjArray* array, int index) {
	Value element = array->elements.values[index];
	if (IS_OBJ(element)) push(element);

	for (int i = index; i < array->elements.count - 1; i++) {
		array->elements.values[i] = array->elements.values[i + 1];
	}
	array->elements.count--;

	if (IS_OBJ(element)) pop();
	return element;
}

ObjString* arrayToString(ObjArray* array) {
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
		if (offset < 1) return copyString("[]", 2);
		string[offset] = ']';
		string[offset + 1] = '\0';
		return copyString(string, (int)offset + 1);
	}
}
