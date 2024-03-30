#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "std.h"
#include "../assert/assert.h"
#include "../string/string.h"

NATIVE_METHOD(System, println) {
  assertArgCount("println(message)", 1, argCount);

  printValue(args[0]);
  printf("\n");

  RETURN_NIL;
}

NATIVE_METHOD(System, scanln) {
  assertArgCount("scanln(prompt)", 1, argCount);
  assertArgIsString("scanln(prompt)", args, 0);

  ObjString* prompt = AS_STRING(args[0]);
  printValue(OBJ_VAL(prompt));

  char inputBuffer[2048];
  if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
    size_t length = strlen(inputBuffer);
    if (length > 0 && inputBuffer[length - 1] == '\n') {
      inputBuffer[length - 1] = '\0';
    }

    RETURN_OBJ(copyString(inputBuffer, (int)strlen(inputBuffer)));
  } else {
    runtimeError("Error reading input");
    exit(70);
  }
}

void initStd(){
	ObjClass* systemClass = defineNativeClass("System");
  DEF_METHOD(systemClass, System, println, 1);
  DEF_METHOD(systemClass, System, scanln, 1);
}
