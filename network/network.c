#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "network.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../value/value.h"
#include "../vm/vm.h"

NATIVE_METHOD(URL, __init__) {
  assertArgCount("URL::__init__(scheme, host, port, path, query, fragment)", 6, argCount);
  assertArgIsString("URL::__init__(scheme, host, port, path, query, fragment)", args, 0);
  assertArgIsString("URL::__init__(scheme, host, port, path, query, fragment)", args, 1);
  assertArgIsInt("URL::__init__(scheme, host, port, path, query, fragment)", args, 2);
  assertArgIsString("URL::__init__(scheme, host, port, path, query, fragment)", args, 3);
  assertArgIsString("URL::__init__(scheme, host, port, path, query, fragment)", args, 4);
  assertArgIsString("URL::__init__(scheme, host, port, path, query, fragment)", args, 5);

  ObjInstance* self = AS_INSTANCE(receiver);
  setObjProperty(self, "scheme", args[0]);
  setObjProperty(self, "host", args[1]);
  setObjProperty(self, "port", args[2]);
  setObjProperty(self, "path", args[3]);
  setObjProperty(self, "query", args[4]);
  setObjProperty(self, "fragment", args[5]);
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(URL, isAbsolute) {
  assertArgCount("URL::isAbsolute()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* host = AS_STRING(getObjProperty(self, "host"));
  RETURN_BOOL(host->length > 0);
}

NATIVE_METHOD(URL, isRelative) {
  assertArgCount("URL::isRelative()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* host = AS_STRING(getObjProperty(self, "host"));
  RETURN_BOOL(host->length == 0);
}

NATIVE_METHOD(URL, __str__) {
  assertArgCount("URL::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* scheme = AS_STRING(getObjProperty(self, "scheme"));
  ObjString* host = AS_STRING(getObjProperty(self, "host"));
  int port = AS_INT(getObjProperty(self, "port"));
  ObjString* path = AS_STRING(getObjProperty(self, "path"));
  ObjString* query = AS_STRING(getObjProperty(self, "query"));
  ObjString* fragment = AS_STRING(getObjProperty(self, "fragment"));

  ObjString* uriString = newString("");
  if (host->length > 0) {
    uriString = (scheme->length > 0) ? formattedString("%s://%s", scheme->chars, host->chars) : host;
    if (port > 0 && port < 65536) uriString = formattedString("%s:%d", uriString->chars, port);
  }
  if (path->length > 0) uriString = formattedString("%s/%s", uriString->chars, path->chars);
  if (query->length > 0) uriString = formattedString("%s&%s", uriString->chars, query->chars);
  if (fragment->length > 0) uriString = formattedString("%s#%s", uriString->chars, fragment->chars);
  RETURN_OBJ(uriString);
}

void registerNetworkPackage() {
  ObjNamespace* networkNamespace = defineNativeNamespace("network", vm.stdNamespace);
  vm.currentNamespace = networkNamespace;

  ObjClass* urlClass = defineNativeClass("URL");
  bindSuperclass(urlClass, vm.objectClass);
  DEF_METHOD(urlClass, URL, __init__, 6);
  DEF_METHOD(urlClass, URL, __str__, 0);
  DEF_METHOD(urlClass, URL, isAbsolute, 0);
  DEF_METHOD(urlClass, URL, isRelative, 0);

  vm.currentNamespace = vm.rootNamespace;
}
