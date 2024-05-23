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


static bool ipIsV4(ObjString* address) {
  unsigned char b1, b2, b3, b4;
  if (4 != sscanf(address->chars, "%hhu.%hhu.%hhu.%hhu", &b1, &b2, &b3, &b4)) return false;
  char buf[16];
  snprintf(buf, 16, "%hhu.%hhu.%hhu.%hhu", b1, b2, b3, b4);
  return !strcmp(address->chars, buf);
}

static bool urlIsAbsolute(ObjInstance* url) {
  ObjString* host = AS_STRING(getObjProperty(url, "host"));
  return host->length > 0;
}

static ObjString* urlToString(ObjInstance* url) {
  ObjString* scheme = AS_STRING(getObjProperty(url, "scheme"));
  ObjString* host = AS_STRING(getObjProperty(url, "host"));
  int port = AS_INT(getObjProperty(url, "port"));
  ObjString* path = AS_STRING(getObjProperty(url, "path"));
  ObjString* query = AS_STRING(getObjProperty(url, "query"));
  ObjString* fragment = AS_STRING(getObjProperty(url, "fragment"));

  ObjString* urlString = newString("");
  if (host->length > 0) {
    urlString = (scheme->length > 0) ? formattedString("%s://%s", scheme->chars, host->chars) : host;
    if (port > 0 && port < 65536) urlString = formattedString("%s:%d", urlString->chars, port);
  }
  if (path->length > 0) urlString = formattedString("%s/%s", urlString->chars, path->chars);
  if (query->length > 0) urlString = formattedString("%s&%s", urlString->chars, query->chars);
  if (fragment->length > 0) urlString = formattedString("%s#%s", urlString->chars, fragment->chars);
  return urlString;
}

NATIVE_METHOD(IPAddress, __init__) {
  assertArgCount("IPAddress::__init__(address)", 1, argCount);
  assertArgIsString("IPAddress::__init__(address)", args, 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  if (!ipIsV4(AS_STRING(args[0]))) { 
    runtimeError("Invalid IPv4 address specified.");
    RETURN_NIL;
  }
  setObjProperty(self, "address", args[0]); 
  RETURN_OBJ(self);
}

NATIVE_METHOD(IPAddress, isIPV4) {
  assertArgCount("IPAddress::isIPV4()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  Value address = getObjProperty(self, "address");
  RETURN_BOOL(ipIsV4(AS_STRING(address)));
}

NATIVE_METHOD(IPAddress, __str__) {
  assertArgCount("IPAddress::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  Value address = getObjProperty(self, "address");
  RETURN_OBJ(AS_STRING(address));
}

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
  setObjProperty(self, "raw", OBJ_VAL(urlToString(self)));
  RETURN_OBJ(self);
}

NATIVE_METHOD(URL, isAbsolute) {
  assertArgCount("URL::isAbsolute()", 0, argCount);
  RETURN_BOOL(urlIsAbsolute(AS_INSTANCE(receiver)));
}

NATIVE_METHOD(URL, isRelative) {
  assertArgCount("URL::isRelative()", 0, argCount);
  RETURN_BOOL(!urlIsAbsolute(AS_INSTANCE(receiver)));
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

NATIVE_METHOD(URL, relativize) {
  assertArgCount("URL::relativize(url)", 1, argCount);
  assertObjInstanceOfClass("URL::relativize(url)", args[0], "luminique::std::network", "URL", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjInstance* url = AS_INSTANCE(args[0]);
  if (urlIsAbsolute(self) || urlIsAbsolute(url)) RETURN_OBJ(url);

  ObjString* urlString = AS_STRING(getObjProperty(self, "raw"));
  ObjString* urlString2 = urlToString(url);
  int index = searchString(urlString, urlString2, 0);
  if (index == 0) {
    ObjInstance* relativized = newInstance(self->obj.klass);
    ObjString* relativizedURL = subString(urlString, urlString2->length, urlString->length); 
    struct yuarel component;
    char fullURL[UINT8_MAX];
    int length = snprintf(fullURL, UINT8_MAX, "%s%s", "https://example.com/", relativizedURL->chars);
    yuarel_parse(&component, fullURL);

    setObjProperty(relativized, "scheme", OBJ_VAL(emptyString()));
    setObjProperty(relativized, "host", OBJ_VAL(emptyString()));
    setObjProperty(relativized, "port", INT_VAL(0));
    setObjProperty(relativized, "path", OBJ_VAL(newString(component.path != NULL ? component.path : "")));
    setObjProperty(relativized, "query", OBJ_VAL(newString(component.query != NULL ? component.query : "")));
    setObjProperty(relativized, "fragment", OBJ_VAL(newString(component.fragment != NULL ? component.fragment : "")));
    setObjProperty(relativized, "raw", OBJ_VAL(relativizedURL));
    RETURN_OBJ(relativized);
  }
  RETURN_OBJ(url);
}

NATIVE_METHOD(URL, __str__) {
  assertArgCount("URL::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* raw = AS_STRING(getObjProperty(self, "raw"));
  RETURN_OBJ(raw);
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
  setObjProperty(instance, "raw", OBJ_VAL(urlToString(instance)));
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
  DEF_METHOD(urlClass, URL, relativize, 1);

  ObjClass* urlMetaclass = urlClass->obj.klass;
  DEF_METHOD(urlMetaclass, URLClass, parse, 1);


  ObjClass* ipAddressClass = defineNativeClass("IPAddress");
  bindSuperclass(ipAddressClass, vm.objectClass);
  DEF_METHOD(ipAddressClass, IPAddress, __init__, 1);
  DEF_METHOD(ipAddressClass, IPAddress, isIPV4, 0);
  DEF_METHOD(ipAddressClass, IPAddress, __str__, 0);

  vm.currentNamespace = vm.rootNamespace;
}
