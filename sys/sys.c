#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "sys.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../value/value.h"
#include "../vm/vm.h"

static void argvArray(ObjArray* array) {
  for (int i = 0; i < vm.argc; i++) {
    writeValueArray(&array->elements, OBJ_VAL(newString(vm.argv[i])));
  }
}

NATIVE_FUNCTION(exit) {
  if (argCount == 0) {
    exit(0);
  } else if (argCount == 1) {
    assertArgIsInt("exit(status)", args, 0);
    exit(AS_INT(args[0]));
  } else {
    runtimeError("Method exit(status) expects 1 or 0 argument but got %d instead.", argCount);
    exit(70);
  }
}

NATIVE_FUNCTION(shell) {
  assertArgCount("shell(command)", 1, argCount);
  
  ObjString* command = AS_STRING(args[0]);
  system(command->chars);
  RETURN_NIL;
}

NATIVE_FUNCTION(getcwd) {
  assertArgCount("getcwd()", 0, argCount);

  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    RETURN_OBJ(copyString(cwd, strlen(cwd)));
  } else {
    runtimeError("Failed to get current working directory.");
    RETURN_NIL;
  }
}

NATIVE_FUNCTION(setcwd) {
  assertArgCount("setcwd(directory)", 1, argCount);
  assertArgIsString("setcwd(directory)", args, 0);

  ObjString* path = AS_STRING(args[0]);
  if (chdir(path->chars) == 0) {
    RETURN_OBJ(copyString(path->chars, path->length));
  } else {
    runtimeError("Failed to set current working directory to '%s'.", path->chars);
    RETURN_NIL;
  }
}

NATIVE_FUNCTION(getenv) {
  assertArgCount("getenv(name)", 1, argCount);
  assertArgIsString("getenv(name)", args, 0);

  ObjString* name = AS_STRING(args[0]);
  char* value = getenv(name->chars);
  if (value != NULL) {
    RETURN_OBJ(copyString(value, strlen(value)));
  } else {
    RETURN_NIL;
  }
}

NATIVE_FUNCTION(setenv) {
  assertArgCount("setenv(name, value)", 2, argCount);
  assertArgIsString("setenv(name, value)", args, 0);
  assertArgIsString("setenv(name, value)", args, 1);

  ObjString* name = AS_STRING(args[0]);
  ObjString* value = AS_STRING(args[1]);
  if (setenv(name->chars, value->chars, 1) == 0) {
    RETURN_TRUE;
  } else {
    runtimeError("Failed to set environment variable '%s' to '%s'.", name->chars, value->chars);
    RETURN_NIL;
  }
}

NATIVE_FUNCTION(unsetenv) {
  assertArgCount("unsetenv(name)", 1, argCount);
  assertArgIsString("unsetenv(name)", args, 0);

  ObjString* name = AS_STRING(args[0]);
  if (unsetenv(name->chars) == 0) {
    RETURN_TRUE;
  } else {
    runtimeError("Failed to unset environment variable '%s'.", name->chars);
    RETURN_NIL;
  }
}

NATIVE_FUNCTION(platform) {
  assertArgCount("platform()", 0, argCount);

  struct utsname info;
  if (uname(&info) != -1) {
    RETURN_OBJ(copyString(info.sysname, strlen(info.sysname)));
  } else {
    runtimeError("Failed to retrieve platform information.");
    RETURN_NIL;
  }
}

void registerSysPackage() {
  ObjNamespace* sysNamespace = defineNativeNamespace("sys", vm.stdNamespace);

  defineNativeConstant(sysNamespace, "argc", INT_VAL(vm.argc));
  ObjArray* array = newArray();
  argvArray(array);
  defineNativeConstant(sysNamespace, "argv", OBJ_VAL(array));
  vm.currentNamespace = sysNamespace;

  DEF_FUNCTION(exit, -1);
  DEF_FUNCTION(shell, 1);
  DEF_FUNCTION(getcwd, 0);
  DEF_FUNCTION(setcwd, 1);
  DEF_FUNCTION(getenv, 1);
  DEF_FUNCTION(setenv, 2);
  DEF_FUNCTION(unsetenv, 1);
  DEF_FUNCTION(platform, 0);

  vm.currentNamespace = vm.rootNamespace;
}
