#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <curl/curl.h>

#include "network.h"
#include "../cnet/cnet.h"
#include "../os/os.h"
#include "../assert/assert.h"
#include "../collection/collection.h"
#include "../yuarel/yuarel.h"
#include "../native/native.h"
#include "../value/value.h"
#include "../vm/vm.h"

NATIVE_METHOD(Socket, __init__) {
  assertArgCount("Socket::__init__(addressFamily, socketType, protocolType)", 3, argCount);
  assertArgIsInt("Socket::__init__(addressFamily, socketType, protocolType)", args, 0);
  assertArgIsInt("Socket::__init__(addressFamily, socketType, protocolType)", args, 1);
  assertArgIsInt("Socket::__init__(addressFamily, socketType, protocolType)", args, 2);

  socklen_t descriptor = socket(AS_INT(args[0]), AS_INT(args[1]), AS_INT(args[2]));
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

NATIVE_METHOD(Socket, close) {
  assertArgCount("Socket::close()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  int descriptor = AS_INT(getObjProperty(self, "descriptor"));
  close(descriptor);
  RETURN_NIL;
}

NATIVE_METHOD(Socket, connect) { 
  assertArgCount("Socket::connect(ipAddress)", 1, argCount);
  assertObjInstanceOfClass("Socket::connect(IPAddress)", args[0], "luminique::std::network", "IPAddress", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjInstance* ipAddress = AS_INSTANCE(args[0]);

  struct sockaddr_in socketAddress = { 0 };
  ObjString* ipString = AS_STRING(getObjProperty(ipAddress, "address"));
  socketAddress.sin_family = AF_INET;
  socketAddress.sin_port = htons(AS_INT(getObjProperty(ipAddress, "port")));

  if (inet_pton(AF_INET, ipString->chars, &socketAddress.sin_addr) <= 0) {
    runtimeError("Invalid socket address provided.");
    RETURN_NIL;
  }

  int descriptor = AS_INT(getObjProperty(self, "descriptor")); 
  if (connect(descriptor, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) < 0) {
    runtimeError("Socket connection failed.");
  }
  RETURN_NIL;
}

NATIVE_METHOD(Socket, receive) {
  assertArgCount("Socket::receive()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  int descriptor = AS_INT(getObjProperty(self, "descriptor"));
  char message[UINT8_MAX] = "";
  if (recv(descriptor, message, UINT8_MAX, 0) < 0) {
    runtimeError("Failed to receive message from socket.");
    RETURN_NIL;
  }
  RETURN_STRING(message, (int)strlen(message));
}

NATIVE_METHOD(Socket, send) {
  assertArgCount("Socket::send(message)", 1, argCount);
  assertArgIsString("Socket::send(message)", args, 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* message = AS_STRING(args[0]);
  int descriptor = AS_INT(getObjProperty(self, "descriptor"));
  if (send(descriptor, message->chars, message->length, 0) < 0) runtimeError("Failed to send message to socket.");
  RETURN_NIL;
}

NATIVE_METHOD(Socket, __str__) {
  assertArgCount("Socket::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  Value addressFamily = getObjProperty(self, "addressFamily");
  Value socketType = getObjProperty(self, "socketType");
  Value protocolType = getObjProperty(self, "protocolType");
  RETURN_STRING_FMT("Socket - AddressFamily: %d, SocketType: %d, ProtocolType: %d", AS_INT(addressFamily), AS_INT(socketType), AS_INT(protocolType));
}

NATIVE_METHOD(Socket, __format__) {
  assertArgCount("Socket::__format__()", 0, argCount);
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

NATIVE_METHOD(Domain, getIpAddresses) {
  assertArgCount("Domain::getIpAddresses()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* name = AS_STRING(getObjProperty(self, "name"));

  int status = -1;
  struct addrinfo* result = dnsGetDomainInfo(name->chars, &status);
  if (result == NULL) printf("unable to get domain info.");
  if (status) {
      runtimeError("Failed to get IP address information for domain.");
      RETURN_NIL;
  }

  ObjArray* ipAddresses = dnsGetIPAddressesFromDomain(result);
  uv_freeaddrinfo(result);
  RETURN_OBJ(ipAddresses);
}

NATIVE_METHOD(Domain, getIpAddressesAsync) {
  assertArgCount("Domain::getIpAddressesAsync()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjPromise* promise = dnsGetDomainInfoAsync(self, dnsOnGetAddrInfo);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::network, DomainHostException, "Failed to get IP Addresses from Domain.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(Domain, __str__) {
  assertArgCount("Domain::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* name = AS_STRING(getObjProperty(self, "name"));
  RETURN_OBJ(name);
}

NATIVE_METHOD(Domain, __format__) {
  assertArgCount("Domain::__format__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* name = AS_STRING(getObjProperty(self, "name"));
  RETURN_OBJ(name);
}

NATIVE_METHOD(HTTPClient, close) {
  assertArgCount("HTTPClient::close()", 0, argCount);
  curl_global_cleanup();
  RETURN_NIL;
}

NATIVE_METHOD(HTTPClient, delete) {
  assertArgCount("HTTPClient::delete(url)", 1, argCount);
  assertArgInstanceOfEither("HTTPClient::delete(url)", args, 0, "luminique::std::lang", "String", "luminique::std::network", "URL");
  ObjString* url = httpRawURL(args[0]);

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    runtimeError("Failed to initiate a DELETE request using CURL.");
    RETURN_NIL;
  }

  CURLResponse curlResponse;
  CURLcode curlCode = httpSendRequest(url, HTTP_DELETE, NULL, curl, &curlResponse);
  if (curlCode != CURLE_OK) {
    runtimeError("Failed to complete a DELETE request from URL.");
    curl_easy_cleanup(curl);
    RETURN_NIL;
  }

  ObjInstance* httpResponse = httpCreateResponse(url, curl, curlResponse);
  curl_easy_cleanup(curl);
  RETURN_OBJ(httpResponse);
}

NATIVE_METHOD(HTTPClient, __init__) {
  assertArgCount("HTTPClient::__init__()", 0, argCount);
  curl_global_init(CURL_GLOBAL_ALL);
  RETURN_VAL(receiver);
}

NATIVE_METHOD(HTTPClient, get) {
  assertArgCount("HTTPClient::get(url)", 1, argCount);
  assertArgInstanceOfEither("HTTPClient::get(url)", args, 0, "luminique::std::lang", "String", "luminique::std::network", "URL");
  ObjString* url = httpRawURL(args[0]);

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    runtimeError("Failed to initiate a GET request using CURL.");
    RETURN_NIL;
  }

  CURLResponse curlResponse;
  CURLcode curlCode = httpSendRequest(url, HTTP_GET, NULL, curl, &curlResponse);
  if (curlCode != CURLE_OK) {
    runtimeError("Failed to complete a GET request from URL.");
    curl_easy_cleanup(curl);
    RETURN_NIL;
  }

  ObjInstance* httpResponse = httpCreateResponse(url, curl, curlResponse);
  curl_easy_cleanup(curl);
  RETURN_OBJ(httpResponse);
}

NATIVE_METHOD(HTTPClient, head) {
  assertArgCount("HTTPClient::head(url)", 1, argCount);
  assertArgInstanceOfEither("HTTPClient::head(url)", args, 0, "luminique::std::lang", "String", "luminique::std::network", "URL");
  ObjString* url = httpRawURL(args[0]);

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    runtimeError("Failed to initiate a HEAD request using CURL.");
    RETURN_NIL;
  }

  CURLResponse curlResponse;
  CURLcode curlCode = httpSendRequest(url, HTTP_HEAD, NULL, curl, &curlResponse);
  if (curlCode != CURLE_OK) {
    runtimeError("Failed to complete a HEAD request from URL.");
    curl_easy_cleanup(curl);
    RETURN_NIL;
  }

  ObjInstance* httpResponse = httpCreateResponse(url, curl, curlResponse);
  curl_easy_cleanup(curl);
  RETURN_OBJ(httpResponse);
}

NATIVE_METHOD(HTTPClient, post) {
  assertArgCount("HTTPClient::post(url, data)", 2, argCount);
  assertArgInstanceOfEither("HTTPClient::post(url, data)", args, 0, "luminique::std::lang", "String", "luminique::std::network", "URL");
  assertArgIsDictionary("HTTPClient::post(url, data)", args, 1);
  ObjString* url = httpRawURL(args[0]);
  ObjDictionary* data = AS_DICTIONARY(args[1]);

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    runtimeError("Failed to initiate a POST request using CURL.");
    RETURN_NIL;
  }

  CURLResponse curlResponse;
  CURLcode curlCode = httpSendRequest(url, HTTP_POST, data, curl, &curlResponse);
  if (curlCode != CURLE_OK) {
    runtimeError("Failed to complete a POST request from URL.");
    curl_easy_cleanup(curl);
    RETURN_NIL;
  }

  ObjInstance* httpResponse = httpCreateResponse(url, curl, curlResponse);
  curl_easy_cleanup(curl);
  RETURN_OBJ(httpResponse);
}

NATIVE_METHOD(HTTPClient, options) {
  assertArgCount("HTTPClient::options(url)", 1, argCount);
  assertArgInstanceOfEither("HTTPClient::options(url)", args, 0, "luminique::std::lang", "String", "luminique::std::network", "URL");
  ObjString* url = httpRawURL(args[0]);

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    runtimeError("Failed to initiate an OPTIONS request using CURL.");
    RETURN_NIL;
  }

  CURLResponse curlResponse;
  CURLcode curlCode = httpSendRequest(url, HTTP_OPTIONS, NULL, curl, &curlResponse);
  if (curlCode != CURLE_OK) {
    runtimeError("Failed to complete an OPTIONS request from URL.");
    curl_easy_cleanup(curl);
    RETURN_NIL;
  }

  ObjInstance* httpResponse = httpCreateResponse(url, curl, curlResponse);
  curl_easy_cleanup(curl);
  RETURN_OBJ(httpResponse);
}

NATIVE_METHOD(HTTPClient, patch) {
  assertArgCount("HTTPClient::patch(url, data)", 2, argCount);
  assertArgInstanceOfEither("HTTPClient::patch(url, data)", args, 0, "luminique::std::lang", "String", "luminique::std::network", "URL");
  assertArgIsDictionary("HTTPClient::patch(url, data)", args, 1);
  ObjString* url = httpRawURL(args[0]);
  ObjDictionary* data = AS_DICTIONARY(args[1]);

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    runtimeError("Failed to initiate a PATCH request using CURL.");
    RETURN_NIL;
  }

  CURLResponse curlResponse;
  CURLcode curlCode = httpSendRequest(url, HTTP_PATCH, data, curl, &curlResponse);
  if (curlCode != CURLE_OK) {
    runtimeError("Failed to complete a PATCH request from URL.");
    curl_easy_cleanup(curl);
    RETURN_NIL;
  }

  ObjInstance* httpResponse = httpCreateResponse(url, curl, curlResponse);
  curl_easy_cleanup(curl);
  RETURN_OBJ(httpResponse);
}

NATIVE_METHOD(HTTPClient, put) {
  assertArgCount("HTTPClient::put(url, data)", 2, argCount);
  assertArgInstanceOfEither("HTTPClient::put(url, data)", args, 0, "luminique::std::lang", "String", "luminique::std::network", "URL");
  assertArgIsDictionary("HTTPClient::put(url, data)", args, 1);
  ObjString* url = httpRawURL(args[0]);
  ObjDictionary* data = AS_DICTIONARY(args[1]);

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    runtimeError("Failed to initiate a PUT request using CURL.");
    RETURN_NIL;
  }

  CURLResponse curlResponse;
  CURLcode curlCode = httpSendRequest(url, HTTP_PATCH, data, curl, &curlResponse);
  if (curlCode != CURLE_OK) {
    runtimeError("Failed to complete a PUT request from URL.");
    curl_easy_cleanup(curl);
    RETURN_NIL;
  }

  ObjInstance* httpResponse = httpCreateResponse(url, curl, curlResponse);
  curl_easy_cleanup(curl);
  RETURN_OBJ(httpResponse);
}

NATIVE_METHOD(HTTPClient, send) {
  assertArgCount("HTTPClient::send(request)", 1, argCount);
  assertObjInstanceOfClass("HTTPClient::send(request)", args[0], "luminique::std::network::http", "HTTPRequest", 0);

  CURL* curl = curl_easy_init();
  if (curl == NULL) {
    runtimeError("Failed to initiate a PUT request using CURL.");
    RETURN_NIL;
  }

  ObjInstance* request = AS_INSTANCE(args[0]);
  ObjString* url = AS_STRING(getObjProperty(request, "url"));
  HTTPMethod method = (HTTPMethod)AS_INT(getObjProperty(request, "method"));
  ObjDictionary* headers = AS_DICTIONARY(getObjProperty(request, "headers"));
  ObjDictionary* data = AS_DICTIONARY(getObjProperty(request, "data"));

  struct curl_slist* curlHeaders = httpParseHeaders(headers, curl);
  CURLResponse curlResponse;
  CURLcode curlCode = httpSendRequest(url, method, data, curl, &curlResponse);
  curl_slist_free_all(curlHeaders);
  if (curlCode != CURLE_OK) {
    runtimeError("Failed to complete a PUT request from URL.");
    curl_easy_cleanup(curl);
    RETURN_NIL;
  }

  ObjInstance* httpResponse = httpCreateResponse(url, curl, curlResponse);
  curl_easy_cleanup(curl);
  RETURN_OBJ(httpResponse);
}

NATIVE_METHOD(HTTPRequest, __init__) {
  assertArgCount("HTTPRequest::__init__(url, method, headers, data)", 4, argCount);
  assertArgIsString("HTTPRequest::__init__(url, method, headers, data)", args, 0);
  assertArgIsInt("HTTPRequest::__init__(url, method, headers, data)", args, 1);
  assertArgIsDictionary("HTTPRequest::__init__(url, method, headers, data)", args, 2);
  assertArgIsDictionary("HTTPRequest::__init__(url, method, headers, data)", args, 3);

  ObjInstance* self = AS_INSTANCE(receiver);
  setObjProperty(self, "url", args[0]);
  setObjProperty(self, "method", args[1]);
  setObjProperty(self, "headers", args[2]);
  setObjProperty(self, "data", args[3]);
  RETURN_OBJ(self);
}

NATIVE_METHOD(HTTPRequest, __str__) {
  assertArgCount("HTTPRequest::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* url = AS_STRING(getObjProperty(self, "url"));
  HTTPMethod method = (HTTPMethod)AS_INT(getObjProperty(self, "method"));
  ObjDictionary* data = AS_DICTIONARY(getObjProperty(self, "data"));
  RETURN_STRING_FMT("HTTPRequest - URL: %s; Method: %s; Data: %s", url->chars, httpMapMethod(method), httpParsePostData(data)->chars);
}

NATIVE_METHOD(HTTPRequest, __format__) {
  assertArgCount("HTTPRequest::__format__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* url = AS_STRING(getObjProperty(self, "url"));
  HTTPMethod method = (HTTPMethod)AS_INT(getObjProperty(self, "method"));
  ObjDictionary* data = AS_DICTIONARY(getObjProperty(self, "data"));
  RETURN_STRING_FMT("HTTPRequest - URL: %s; Method: %s; Data: %s", url->chars, httpMapMethod(method), httpParsePostData(data)->chars);
}

NATIVE_METHOD(HTTPResponse, __init__) {
  assertArgCount("HTTPResponse::__init__(url, status, headers, contentType)", 4, argCount);
  assertArgIsString("HTTPResponse::__init__(url, status, headers, contentType)", args, 0);
  assertArgIsInt("HTTPResponse::__init__(url, status, headers, contentType)", args, 1);
  assertArgIsDictionary("HTTPResponse::__init__(url, status, headers, contentType)", args, 2);
  assertArgIsString("HTTPResponse::__init__(url, status, headers, contentType)", args, 3);

  ObjInstance* self = AS_INSTANCE(receiver);
  setObjProperty(self, "url", args[0]);
  setObjProperty(self, "status", args[1]);
  setObjProperty(self, "headers", args[2]);
  setObjProperty(self, "contentType", args[3]);
  RETURN_OBJ(self);
}

NATIVE_METHOD(HTTPResponse, __str__) {
  assertArgCount("HTTPResponse::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* url = AS_STRING(getObjProperty(self, "url"));
  int status = AS_INT(getObjProperty(self, "status"));
  ObjString* contentType = AS_STRING(getObjProperty(self, "contentType"));
  RETURN_STRING_FMT("HTTPResponse - URL: %s; Status: %d; ContentType: %s", url->chars, status, contentType->chars);
}

NATIVE_METHOD(HTTPResponse, __format__) {
  assertArgCount("HTTPResponse::__format__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* url = AS_STRING(getObjProperty(self, "url"));
  int status = AS_INT(getObjProperty(self, "status"));
  ObjString* contentType = AS_STRING(getObjProperty(self, "contentType"));
  RETURN_STRING_FMT("HTTPResponse - URL: %s; Status: %d; ContentType: %s", url->chars, status, contentType->chars);
}

NATIVE_METHOD(IPAddress, getDomain) {
  assertArgCount("IPAddress::getDomain()", 0, argCount);
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

NATIVE_METHOD(IPAddress, getDomainAsync) {
  assertArgCount("IPAddress::getDomainAsync()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjPromise* promise = dnsGetDomainFromIPAddressAsync(self, dnsOnGetNameInfo);
  if (promise == NULL) THROW_EXCEPTION(luminique::std::network, IPAddressException, "Failed to get domain name from IP Address.");
  RETURN_OBJ(promise);
}

NATIVE_METHOD(IPAddress, __init__) {
  assertArgCount("IPAddress::__init__(address)", 1, argCount);
  assertArgIsString("IPAddress::__init__(address)", args, 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjString* address = AS_STRING(args[0]);
  int version = -1;
  int port = -1;
  char ip[40];

  if (extractIPv4AndPort(address->chars, ip, &port)) {
    version = 4;
  } else if (extractIPv6AndPort(address->chars, ip, &port)) {
    version = 6;
  } else {
    runtimeError("Invalid IP address specified.");
    RETURN_NIL;
  }

  setObjProperty(self, "address", OBJ_VAL(copyString(ip, strlen(ip))));
  setObjProperty(self, "version", INT_VAL(version));
  if (port != -1) {
    setObjProperty(self, "port", INT_VAL(port));
  } else {
    setObjProperty(self, "port", INT_VAL(80));
  }

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
  Value addressValue = getObjProperty(self, "address");
  Value portValue = getObjProperty(self, "port");

  if (IS_STRING(addressValue)) {
    ObjString* address = AS_STRING(addressValue);
    char* ipAddress = address->chars;

    if (ipIsV4(address) && AS_NUMBER(portValue) == 80) {
      RETURN_OBJ(copyString(ipAddress, address->length));
    } else if (ipIsV4(address)) {
      char buffer[40];
      snprintf(buffer, sizeof(buffer), "%s:%d", ipAddress, AS_INT(portValue));
      RETURN_OBJ(copyString(buffer, strlen(buffer)));
    } else if (ipIsV6(address) && AS_NUMBER(portValue) == 80) {
      RETURN_OBJ(copyString(ipAddress, address->length));
    } else if (ipIsV6(address)) {
      char buffer[50];
      snprintf(buffer, sizeof(buffer), "[%s]:%d", ipAddress, AS_INT(portValue));
      RETURN_OBJ(copyString(buffer, strlen(buffer)));
    }
  } else {
    runtimeError("Address must be a string.");
  }
  RETURN_NIL;
}

NATIVE_METHOD(IPAddress, __format__) {
  assertArgCount("IPAddress::__format__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  Value addressValue = getObjProperty(self, "address");
  Value portValue = getObjProperty(self, "port");

  if (IS_STRING(addressValue)) {
    ObjString* address = AS_STRING(addressValue);
    char* ipAddress = address->chars;

    if (ipIsV4(address) && AS_NUMBER(portValue) == 80) {
      RETURN_OBJ(copyString(ipAddress, address->length));
    } else if (ipIsV4(address)) {
      char buffer[40];
      snprintf(buffer, sizeof(buffer), "%s:%d", ipAddress, AS_INT(portValue));
      RETURN_OBJ(copyString(buffer, strlen(buffer)));
    } else if (ipIsV6(address) && AS_NUMBER(portValue) == 80) {
      RETURN_OBJ(copyString(ipAddress, address->length));
    } else if (ipIsV6(address)) {
      char buffer[50];
      snprintf(buffer, sizeof(buffer), "[%s]:%d", ipAddress, AS_INT(portValue));
      RETURN_OBJ(copyString(buffer, strlen(buffer)));
    }
  } else {
    runtimeError("Address must be a string.");
  }
  RETURN_NIL;
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

NATIVE_METHOD(URL, __format__) {
  assertArgCount("URL::__format__()", 0, argCount);
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

  ObjClass* networkExceptionClass = defineNativeException("NetworkException", vm.exceptionClass);
  defineNativeException("DomainHostException", networkExceptionClass);
  defineNativeException("HTTPException", networkExceptionClass);
  defineNativeException("IPAddressException", networkExceptionClass);
  defineNativeException("SocketException", networkExceptionClass);
  defineNativeException("URLException", networkExceptionClass);

  ObjClass* urlClass = defineNativeClass("URL");
  bindSuperclass(urlClass, vm.objectClass);
  DEF_METHOD(urlClass, URL, __init__, 6);
  DEF_METHOD(urlClass, URL, isAbsolute, 0);
  DEF_METHOD(urlClass, URL, isRelative, 0);
  DEF_METHOD(urlClass, URL, pathArray, 0);
  DEF_METHOD(urlClass, URL, queryDict, 0);
  DEF_METHOD(urlClass, URL, relativize, 1);
  DEF_METHOD(urlClass, URL, __str__, 0);
  DEF_METHOD(urlClass, URL, __format__, 0);

  ObjClass* urlMetaclass = urlClass->obj.klass;
  DEF_METHOD(urlMetaclass, URLClass, parse, 1);

  ObjClass* ipAddressClass = defineNativeClass("IPAddress");
  bindSuperclass(ipAddressClass, vm.objectClass);
  DEF_METHOD(ipAddressClass, IPAddress, __init__, 1);
  DEF_METHOD(ipAddressClass, IPAddress, getDomain, 0);
  DEF_METHOD(ipAddressClass, IPAddress, getDomainAsync, 0);
  DEF_METHOD(ipAddressClass, IPAddress, isIPV4, 0);
  DEF_METHOD(ipAddressClass, IPAddress, isIPV6, 0);
  DEF_METHOD(ipAddressClass, IPAddress, toArray, 0);
  DEF_METHOD(ipAddressClass, IPAddress, __str__, 0);
  DEF_METHOD(ipAddressClass, IPAddress, __format__, 0);

  ObjClass* domainClass = defineNativeClass("Domain");
  bindSuperclass(domainClass, vm.objectClass);
  DEF_METHOD(domainClass, Domain, __init__, 1);
  DEF_METHOD(domainClass, Domain, getIpAddresses, 0);
  DEF_METHOD(domainClass, Domain, getIpAddressesAsync, 0);
  DEF_METHOD(domainClass, Domain, __str__, 0);
  DEF_METHOD(domainClass, Domain, __format__, 0);


  ObjClass* socketClass = defineNativeClass("Socket");
  bindSuperclass(socketClass, vm.objectClass);
  DEF_METHOD(socketClass, Socket, __init__, 3);
  DEF_METHOD(socketClass, Socket, close, 0);
  DEF_METHOD(socketClass, Socket, connect, 1);
  DEF_METHOD(socketClass, Socket, receive, 0);
  DEF_METHOD(socketClass, Socket, send, 1);
  DEF_METHOD(socketClass, Socket, __str__, 0);
  DEF_METHOD(socketClass, Socket, __format__, 0);

  ObjEnum* socketTypeEnum = defineNativeEnum("SocketType");
  defineNativeArtificialEnumElement(socketTypeEnum, "afUNSPEC", INT_VAL(AF_UNSPEC));
  defineNativeArtificialEnumElement(socketTypeEnum, "afUNIX", INT_VAL(AF_UNIX));
  defineNativeArtificialEnumElement(socketTypeEnum, "afINET", INT_VAL(AF_INET));
  defineNativeArtificialEnumElement(socketTypeEnum, "afIPX", INT_VAL(AF_IPX));
  defineNativeArtificialEnumElement(socketTypeEnum, "afDECnet", INT_VAL(AF_DECnet));
  defineNativeArtificialEnumElement(socketTypeEnum, "afAPPLETALK", INT_VAL(AF_APPLETALK));
  defineNativeArtificialEnumElement(socketTypeEnum, "afINET6", INT_VAL(AF_INET6));
  defineNativeArtificialEnumElement(socketTypeEnum, "sockSTREAM", INT_VAL(SOCK_STREAM));
  defineNativeArtificialEnumElement(socketTypeEnum, "sockDGRAM", INT_VAL(SOCK_DGRAM));
  defineNativeArtificialEnumElement(socketTypeEnum, "sockRAW", INT_VAL(SOCK_RAW));
  defineNativeArtificialEnumElement(socketTypeEnum, "sockRDM", INT_VAL(SOCK_RDM));
  defineNativeArtificialEnumElement(socketTypeEnum, "sockSEQPACKET", INT_VAL(SOCK_SEQPACKET));
  defineNativeArtificialEnumElement(socketTypeEnum, "protoIP", INT_VAL(IPPROTO_IP));
  defineNativeArtificialEnumElement(socketTypeEnum, "protoICMP", INT_VAL(IPPROTO_ICMP));
  defineNativeArtificialEnumElement(socketTypeEnum, "protoTCP", INT_VAL(IPPROTO_TCP));
  defineNativeArtificialEnumElement(socketTypeEnum, "protoUDP", INT_VAL(IPPROTO_UDP));
  defineNativeArtificialEnumElement(socketTypeEnum, "protoICMPV6", INT_VAL(IPPROTO_ICMPV6));
  defineNativeArtificialEnumElement(socketTypeEnum, "protoRAW", INT_VAL(IPPROTO_RAW));

  ObjNamespace* httpNamespace = defineNativeNamespace("http", networkNamespace);
  vm.currentNamespace = httpNamespace;

  ObjClass* httpClientClass = defineNativeClass("HTTPClient");
  bindSuperclass(httpClientClass, vm.objectClass);
  DEF_METHOD(httpClientClass, HTTPClient, __init__, 0);
  DEF_METHOD(httpClientClass, HTTPClient, close, 0);
  DEF_METHOD(httpClientClass, HTTPClient, delete, 1);
  DEF_METHOD(httpClientClass, HTTPClient, get, 1);
  DEF_METHOD(httpClientClass, HTTPClient, head, 1);
  DEF_METHOD(httpClientClass, HTTPClient, options, 1);
  DEF_METHOD(httpClientClass, HTTPClient, patch, 2);
  DEF_METHOD(httpClientClass, HTTPClient, post, 2);
  DEF_METHOD(httpClientClass, HTTPClient, put, 2);
  DEF_METHOD(httpClientClass, HTTPClient, send, 1);

  ObjClass* httpRequestClass = defineNativeClass("HTTPRequest");
  bindSuperclass(httpRequestClass, vm.objectClass);
  DEF_METHOD(httpRequestClass, HTTPRequest, __init__, 4);
  DEF_METHOD(httpRequestClass, HTTPRequest, __str__, 0);
  DEF_METHOD(httpRequestClass, HTTPRequest, __format__, 0);


  ObjEnum* httpRequestTypeEnum = defineNativeEnum("HTTPRequestType");
  defineNativeArtificialEnumElement(httpRequestTypeEnum, "httpHEAD", INT_VAL(HTTP_HEAD));
  defineNativeArtificialEnumElement(httpRequestTypeEnum, "httpGET", INT_VAL(HTTP_GET));
  defineNativeArtificialEnumElement(httpRequestTypeEnum, "httpPOST", INT_VAL(HTTP_POST));
  defineNativeArtificialEnumElement(httpRequestTypeEnum, "httpPUT", INT_VAL(HTTP_PUT));
  defineNativeArtificialEnumElement(httpRequestTypeEnum, "httpDELETE", INT_VAL(HTTP_DELETE));
  defineNativeArtificialEnumElement(httpRequestTypeEnum, "httpPATCH", INT_VAL(HTTP_PATCH));
  defineNativeArtificialEnumElement(httpRequestTypeEnum, "httpOPTIONS", INT_VAL(HTTP_OPTIONS));
  defineNativeArtificialEnumElement(httpRequestTypeEnum, "httpTRACE", INT_VAL(HTTP_TRACE));
  defineNativeArtificialEnumElement(httpRequestTypeEnum, "httpCONNECT", INT_VAL(HTTP_CONNECT));

  ObjClass* httpResponseClass = defineNativeClass("HTTPResponse");
  bindSuperclass(httpResponseClass, vm.objectClass);
  DEF_METHOD(httpResponseClass, HTTPResponse, __init__, 4);
  DEF_METHOD(httpResponseClass, HTTPResponse, __str__, 0);
  DEF_METHOD(httpResponseClass, HTTPResponse, __format__, 0);

  ObjEnum* httpResponseTypeEnum = defineNativeEnum("HTTPResponseType");
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusOK", INT_VAL(200));
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusFound", INT_VAL(302));
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusBadRequest", INT_VAL(400));
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusUnauthorized", INT_VAL(401));
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusForbidden", INT_VAL(403));
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusNotFound", INT_VAL(404));
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusMethodNotAllowed", INT_VAL(405));
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusInternalError", INT_VAL(500));
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusBadGateway", INT_VAL(502));
  defineNativeArtificialEnumElement(httpResponseTypeEnum, "statusServiceUnavailable", INT_VAL(503));

  vm.currentNamespace = vm.rootNamespace;
}
