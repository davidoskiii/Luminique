#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "network.h"
#include "../assert/assert.h"
#include "../collection/collection.h"
#include "../yuarel/yuarel.h"
#include "../native/native.h"
#include "../value/value.h"
#include "../vm/vm.h"

#define INVALID_SOCKET -1

static struct addrinfo* dnsGetDomainInfo(const char* domainName, int* status) {
  struct addrinfo hints, *result;
  void* ptr = NULL;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;

  *status = getaddrinfo(domainName, NULL, &hints, &result);
  return result;
}

static ObjString* dnsGetDomainFromIPAddress(const char* ipAddress, int* status) {
  struct sockaddr_in socketAddress;
  char domainString[NI_MAXHOST];
  memset(&socketAddress, 0, sizeof socketAddress);
  socketAddress.sin_family = AF_INET;
  inet_pton(AF_INET, ipAddress, &socketAddress.sin_addr);

  *status = getnameinfo((struct sockaddr*)&socketAddress, sizeof(socketAddress), domainString, NI_MAXHOST, NULL, 0, 0);
  return newString(domainString);
}

static ObjArray* dnsGetIPAddressesFromDomain(struct addrinfo* result) {
  char ipString[100];
  void* source = NULL;
  ObjArray* ipAddresses = newArray();
  push(OBJ_VAL(ipAddresses));

  while (result) {
    inet_ntop(result->ai_family, result->ai_addr->sa_data, ipString, 100);
    switch (result->ai_family) {
    case AF_INET:
      source = &((struct sockaddr_in*)result->ai_addr)->sin_addr;
      break;
    case AF_INET6:
      source = &((struct sockaddr_in6*)result->ai_addr)->sin6_addr;
      break;
    }

    if (source != NULL) {
      inet_ntop(result->ai_family, source, ipString, 100);
      writeValueArray(&ipAddresses->elements, OBJ_VAL(newString(ipString)));
    }
    result = result->ai_next;
  }

  pop();
  return ipAddresses;
}

static bool ipIsV4(ObjString* address) {
  unsigned char b1, b2, b3, b4;
  if (sscanf(address->chars, "%hhu.%hhu.%hhu.%hhu", &b1, &b2, &b3, &b4) != 4) return false;
  char buffer[16];
  snprintf(buffer, 16, "%hhu.%hhu.%hhu.%hhu", b1, b2, b3, b4);
  return !strcmp(address->chars, buffer);
}

static bool ipIsV6(ObjString* address) {
  unsigned short b1, b2, b3, b4, b5, b6, b7, b8;
  if (sscanf(address->chars, "%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx", &b1, &b2, &b3, &b4, &b5, &b6, &b7, &b8) != 8) return false;
  char buffer[40];
  snprintf(buffer, 40, "%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx", b1, b2, b3, b4, b5, b6, b7, b8);
  return !strcmp(address->chars, buffer);
}

static int ipParseBlock(ObjString* address, int startIndex, int endIndex, int radix) {
  ObjString* bString = subString(address, startIndex, endIndex);
  return strtol(bString->chars, NULL, radix);
}

static void ipWriteByteArray(ObjArray* array, ObjString* address, int radix) {
  push(OBJ_VAL(array));
  int d = 0;
  for (int i = 0; i < address->length; i++) {
    if (address->chars[i] == '.' || address->chars[i] == ':') {
      writeValueArray(&array->elements, INT_VAL(ipParseBlock(address, d, i, radix)));
      d = i + 1;
    }
  }
  writeValueArray(&array->elements, INT_VAL(ipParseBlock(address, d, address->length - 1, radix)));
  pop();
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

NATIVE_METHOD(Socket, __init__) {
    assertArgCount("Socket::init(addressFamily, socketType, protocolType)", 3, argCount);
    assertArgIsInt("Socket::init(addressFamily, socketType, protocolType)", args, 0);
    assertArgIsInt("Socket::init(addressFamily, socketType, protocolType)", args, 1);
    assertArgIsInt("Socket::init(addressFamily, socketType, protocolType)", args, 2);

    int descriptor = socket(AS_INT(args[0]), AS_INT(args[1]), AS_INT(args[2]));
    if (descriptor == INVALID_SOCKET) {
      runtimeError("Socket creation failed...");
      RETURN_NIL;
    }
    ObjInstance* self = AS_INSTANCE(receiver);
    setObjProperty(self, "addressFamily", args[0]);
    setObjProperty(self, "socketType", args[1]);
    setObjProperty(self, "protocolType", args[2]);
    setObjProperty(self, "descriptor", INT_VAL(descriptor));
    RETURN_OBJ(self);
}

NATIVE_METHOD(Socket, __str__) {
    assertArgCount("Socket::__str__()", 0, argCount);
    ObjInstance* self = AS_INSTANCE(receiver);
    Value addressFamily = getObjProperty(self, "addressFamily");
    Value socketType = getObjProperty(self, "socketType");
    Value protocolType = getObjProperty(self, "protocolType");
    RETURN_STRING_FMT("Socket - AddressFamily: %d, SocketType: %d, ProtocolType: %d", AS_INT(addressFamily), AS_INT(socketType), AS_INT(protocolType));
}

NATIVE_METHOD(Domain, __init__) {
  assertArgCount("Domain::__init__(name)", 1, argCount);
  assertArgIsString("Domain::__init__(name)", args, 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  setObjProperty(self, "name", args[0]);
  RETURN_OBJ(self);
}

NATIVE_METHOD(Domain, ipAddresses) {
  assertArgCount("Domain::ipAddresses()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* name = AS_STRING(getObjProperty(self, "name"));

  int status = -1;
  struct addrinfo* result = dnsGetDomainInfo(name->chars, &status);
  if (status) {
      runtimeError("Failed to get IP address information for domain.");
      RETURN_NIL;
  }

  ObjArray* ipAddresses = dnsGetIPAddressesFromDomain(result);
  freeaddrinfo(result);
  RETURN_OBJ(ipAddresses);
}

NATIVE_METHOD(Domain, __str__) {
  assertArgCount("Domain::__str__(name)", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* name = AS_STRING(getObjProperty(self, "name"));
  RETURN_OBJ(name);
}

NATIVE_METHOD(IPAddress, domain) {
  assertArgCount("IPAddress::domain", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* address = AS_STRING(getObjProperty(self, "address"));

  int status = -1;
  ObjString* domain = dnsGetDomainFromIPAddress(address->chars, &status);
  if (status) {
      runtimeError("Failed to get domain information for IP Address.");
      RETURN_NIL;
  }
  RETURN_OBJ(domain);
}

NATIVE_METHOD(IPAddress, __init__) {
  assertArgCount("IPAddress::__init__(address)", 1, argCount);
  assertArgIsString("IPAddress::__init__(address)", args, 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* address = AS_STRING(args[0]);
  int version = -1;
  if (ipIsV4(address)) version = 4;
  else if (ipIsV6(address)) version = 6;
  else {
    runtimeError("Invalid IP address specified.");
    RETURN_NIL;
  }
  setObjProperty(self, "address", args[0]);
  setObjProperty(self, "version", INT_VAL(version));
  RETURN_OBJ(self);
}


NATIVE_METHOD(IPAddress, isIPV4) {
  assertArgCount("IPAddress::isIPV4()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  int version = AS_INT(getObjProperty(self, "version"));
  RETURN_BOOL(version == 4);
}

NATIVE_METHOD(IPAddress, isIPV6) {
  assertArgCount("IPAddress::isIPV6()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  int version = AS_INT(getObjProperty(self, "version"));
  RETURN_BOOL(version == 6);
}

NATIVE_METHOD(IPAddress, toArray) {
  assertArgCount("IPAddress::toArray()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* address = AS_STRING(getObjProperty(self, "address"));
  int version = AS_INT(getObjProperty(self, "version"));
  ObjArray* array = newArray();
  ipWriteByteArray(array, address, version == 6 ? 16 : 10);
  RETURN_OBJ(array);
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
  DEF_METHOD(ipAddressClass, IPAddress, domain, 0);
  DEF_METHOD(ipAddressClass, IPAddress, isIPV4, 0);
  DEF_METHOD(ipAddressClass, IPAddress, isIPV6, 0);
  DEF_METHOD(ipAddressClass, IPAddress, toArray, 0);
  DEF_METHOD(ipAddressClass, IPAddress, __str__, 0);

  ObjClass* domainClass = defineNativeClass("Domain");
  bindSuperclass(domainClass, vm.objectClass);
  DEF_METHOD(domainClass, Domain, __init__, 1);
  DEF_METHOD(domainClass, Domain, ipAddresses, 0);
  DEF_METHOD(domainClass, Domain, __str__, 0);


  ObjClass* socketClass = defineNativeClass("Socket");
  bindSuperclass(socketClass, vm.objectClass);
  DEF_METHOD(socketClass, Socket, __init__, 3);
  DEF_METHOD(socketClass, Socket, __str__, 0);

  ObjClass* socketMetaclass = socketClass->obj.klass;
  setClassProperty(socketClass, "afUNSPEC", INT_VAL(AF_UNSPEC));
  setClassProperty(socketClass, "afUNIX", INT_VAL(AF_UNIX));
  setClassProperty(socketClass, "afINET", INT_VAL(AF_INET));
  setClassProperty(socketClass, "afIPX", INT_VAL(AF_IPX));
  setClassProperty(socketClass, "afDECnet", INT_VAL(AF_DECnet));
  setClassProperty(socketClass, "afAPPLETALK", INT_VAL(AF_APPLETALK));
  setClassProperty(socketClass, "afINET6", INT_VAL(AF_INET6));
  setClassProperty(socketClass, "sockSTREAM", INT_VAL(SOCK_STREAM));
  setClassProperty(socketClass, "sockDGRAM", INT_VAL(SOCK_DGRAM));
  setClassProperty(socketClass, "sockRAW", INT_VAL(SOCK_RAW));
  setClassProperty(socketClass, "sockRDM", INT_VAL(SOCK_RDM));
  setClassProperty(socketClass, "sockSEQPACKET", INT_VAL(SOCK_SEQPACKET));
  setClassProperty(socketClass, "protoIP", INT_VAL(IPPROTO_IP));
  setClassProperty(socketClass, "protoICMP", INT_VAL(IPPROTO_ICMP));
  setClassProperty(socketClass, "protoTCP", INT_VAL(IPPROTO_TCP));
  setClassProperty(socketClass, "protoUDP", INT_VAL(IPPROTO_UDP));
  setClassProperty(socketClass, "protoICMPV6", INT_VAL(IPPROTO_ICMPV6));
  setClassProperty(socketClass, "protoRAW", INT_VAL(IPPROTO_RAW));

  vm.currentNamespace = vm.rootNamespace;
}
