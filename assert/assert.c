#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "../vm/vm.h"

void assertArgCount(const char* method, int expectedCount, int actualCount){
	if (expectedCount != actualCount) {
		runtimeError("Method %s expects %d argument(s) but got %d instead.", method, expectedCount, actualCount);
		exit(70);
	}
}

void assertArgIsBool(const char* method, Value* args, int index){
	if (!IS_BOOL(args[index])) {
		runtimeError("Method %s expects argument %d to be a boolean value.", method, index + 1);
		exit(70);
	}
}

void assertArgIsClass(const char* method, Value* args, int index){
	if (!IS_CLASS(args[index])) {
		runtimeError("Method %s expects argument %d to be a class.", method, index + 1);
		exit(70);
	}
}

void assertArgIsNumber(const char* method, Value* args, int index){
	if (!IS_NUMBER(args[index])) {
		runtimeError("Method %s expects argument %d to be a number.", method, index + 1);
		exit(70);
	}
}

void assertArgIsString(const char* method, Value* args, int index){
	if (!IS_STRING(args[index])) {
		runtimeError("Method %s expects argument %d to be a string.", method, index + 1);
		exit(70);
	}
}

void assertArgIsArray(const char* method, Value* args, int index){
	if (!IS_ARRAY(args[index])) {
		runtimeError("Method %s expects argument %d to be an array.", method, index + 1);
		exit(70);
	}
}

void assertNonZero(const char* method, double number, int index) {
	if (number == 0) {
		if (index < 0) runtimeError("Method %s expects receiver to be a non-zero number but got %g.", method, number);
		else runtimeError("Method %s expects argument %d to be a non-zero number but got %g.", method, index, number);
		exit(70);
	}
}

void assertPositiveNumber(const char* method, double number, int index) {
	if (number <= 0) {
		if (index < 0) runtimeError("Method %s expects receiver to be a positive number but got %g.", method, number);
		else runtimeError("Method %s expects argument %d to be a positive number but got %g.", method, index, number);
		exit(70);
	}
}


void assertError(const char* message) {
	runtimeError(message);
	exit(70);
}


