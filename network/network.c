#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "network.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../value/value.h"
#include "../vm/vm.h"

NATIVE_METHOD(URI, __init__) {
  assertArgCount("URI::__init__(scheme, host, port, path, query, fragment)", 6, argCount);
  assertArgIsString("URI::__init__(scheme, host, port, path, query, fragment)", args, 0);
  assertArgIsString("URI::__init__(scheme, host, port, path, query, fragment)", args, 1);
  assertArgIsInt("URI::__init__(scheme, host, port, path, query, fragment)", args, 2);
  assertArgIsString("URI::__init__(scheme, host, port, path, query, fragment)", args, 3);
  assertArgIsString("URI::__init__(scheme, host, port, path, query, fragment)", args, 4);
  assertArgIsString("URI::__init__(scheme, host, port, path, query, fragment)", args, 5);

  ObjInstance* self = AS_INSTANCE(receiver);
  setObjProperty(self, "scheme", args[0]);
  setObjProperty(self, "host", args[1]);
  setObjProperty(self, "port", args[2]);
  setObjProperty(self, "path", args[3]);
  setObjProperty(self, "query", args[4]);
  setObjProperty(self, "fragment", args[5]);
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(URI, __str__) {
  assertArgCount("URI::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* scheme = AS_STRING(getObjProperty(self, "scheme"));
  ObjString* host = AS_STRING(getObjProperty(self, "host"));
  int port = AS_INT(getObjProperty(self, "port"));
  ObjString* path = AS_STRING(getObjProperty(self, "path"));
  ObjString* query = AS_STRING(getObjProperty(self, "query"));
  ObjString* fragment = AS_STRING(getObjProperty(self, "fragment"));

  ObjString* uriString = formattedString("%s://%s", scheme->chars, host->chars);
  if (port > 0 && port < 65536) uriString = formattedString("%s:%d", uriString->chars, port);
  if (path->length > 0) uriString = formattedString("%s/%s", uriString->chars, path->chars);
  if (query->length > 0) uriString = formattedString("%s&%s", uriString->chars, query->chars);
  if (fragment->length > 0) uriString = formattedString("%s#%s", uriString->chars, fragment->chars);
  RETURN_OBJ(uriString);
}

void registerNetworkPackage() {
  ObjNamespace* networkNamespace = defineNativeNamespace("network", vm.stdNamespace);
  vm.currentNamespace = networkNamespace;

  ObjClass* uriClass = defineNativeClass("URI");
  bindSuperclass(uriClass, vm.objectClass);
  DEF_METHOD(uriClass, URI, __init__, 6);
  DEF_METHOD(uriClass, URI, __str__, 0);

  vm.currentNamespace = vm.rootNamespace;
}
