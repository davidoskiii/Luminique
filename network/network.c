#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "network.h"
#include "../assert/assert.h"
#include "../collection/collection.h"
#include "../yuarel/yuarel.h"
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

NATIVE_METHOD(URL, pathArray) {
  assertArgCount("URL::pathArray()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* path = AS_STRING(getObjProperty(self, "path"));
  if (path->length == 0) RETURN_NIL;
  else {
    char* paths[UINT4_MAX];
    int length = yuarel_split_path(path->chars, paths, 3);
    if (length == -1) {
      runtimeError("Failed to parse path from URL.");
      RETURN_NIL;
    }

    ObjArray* pathArray = newArray();
    push(OBJ_VAL(pathArray));
    for (int i = 0; i < length; i++) {
      ObjString* subPath = newString(paths[i]);
      writeValueArray(&pathArray->elements, OBJ_VAL(subPath));
    }
    pop();
    RETURN_OBJ(pathArray);
  }
}

NATIVE_METHOD(URL, queryDict) {
  assertArgCount("URL::queryDict()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* query = AS_STRING(getObjProperty(self, "query"));
  if (query->length == 0) RETURN_NIL;
  else {
    struct yuarel_param params[UINT4_MAX];
    int length = yuarel_parse_query(query->chars, '&', params, UINT4_MAX);
    if (length == -1) {
      runtimeError("Failed to parse query parameters from URL.");
      RETURN_NIL;
    }

    ObjDictionary* queryDict = newDictionary();
    push(OBJ_VAL(queryDict));
    for (int i = 0; i < length; i++) {
      ObjString* key = newString(params[i].key);
      ObjString* value = newString(params[i].val);
      dictSet(queryDict, OBJ_VAL(key), OBJ_VAL(value));
    }     
    pop();
    RETURN_OBJ(queryDict);
  }
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


NATIVE_METHOD(URLClass, parse) {
  assertArgCount("URL class::parse(url)", 1, argCount);
  assertArgIsString("URL class::parse(url)", args, 0);
  ObjInstance* instance = newInstance(AS_CLASS(receiver));
  ObjString* url = AS_STRING(args[0]);
  struct yuarel component;
  if (yuarel_parse(&component, url->chars) == -1) {
    runtimeError("Failed to parse url.");
    RETURN_NIL;
  }

  setObjProperty(instance, "scheme", OBJ_VAL(newString(component.scheme != NULL ? component.scheme : "")));
  setObjProperty(instance, "host", OBJ_VAL(newString(component.host != NULL ? component.host : "")));
  setObjProperty(instance, "port", INT_VAL(component.port));
  setObjProperty(instance, "path", OBJ_VAL(newString(component.path != NULL ? component.path : "")));
  setObjProperty(instance, "query", OBJ_VAL(newString(component.query != NULL ? component.query : "")));
  setObjProperty(instance, "fragment", OBJ_VAL(newString(component.fragment != NULL ? component.fragment : "")));
  RETURN_OBJ(instance);
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
  DEF_METHOD(urlClass, URL, pathArray, 0);
  DEF_METHOD(urlClass, URL, queryDict, 0);

  ObjClass* urlMetaclass = urlClass->obj.klass;
  DEF_METHOD(urlMetaclass, URLClass, parse, 1);

  vm.currentNamespace = vm.rootNamespace;
}
