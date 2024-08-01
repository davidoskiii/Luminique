#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <curl/curl.h>

#include "cnet.h"
#include "../string/string.h"
#include "../memory/memory.h"
#include "../native/native.h"
#include "../value/value.h"
#include "../vm/vm.h"

static NetworkData* networkLoadData(ObjInstance* network, ObjPromise* promise) {
  NetworkData* data = ALLOCATE_STRUCT(NetworkData);
  if (data != NULL) {
    data->vm = &vm;
    data->network = network;
    data->promise = promise;
  }
  return data;
}

ObjArray* httpCreateCookies(CURL* curl) {
  struct curl_slist* cookies = NULL;
  curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
  ObjArray* cookieArray = newArray();
  if (cookies) {
    push(OBJ_VAL(cookieArray));
    struct curl_slist* cookieNode = cookies;
    while (cookieNode) {
      writeValueArray(&cookieArray->elements, OBJ_VAL(newString(cookieNode->data)));
      cookieNode = cookieNode->next;
    }
    curl_slist_free_all(cookies);
    pop();
  }
  return cookieArray;
}


ObjInstance* httpCreateResponse(ObjString* url, CURL* curl, CURLResponse curlResponse) {
  long statusCode;
  char* contentType;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
  curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);

  ObjInstance* httpResponse = newInstance(getNativeClass("luminique::std::network::http", "HTTPResponse"));
  push(OBJ_VAL(httpResponse));
  setObjProperty(httpResponse, "content", OBJ_VAL(copyString(curlResponse.content, (int)curlResponse.cSize)));
  setObjProperty(httpResponse, "contentType", OBJ_VAL(newString(contentType)));
  setObjProperty(httpResponse, "cookies", OBJ_VAL(httpCreateCookies(curl)));
  setObjProperty(httpResponse, "headers", OBJ_VAL(copyString(curlResponse.headers, (int)curlResponse.hSize)));
  setObjProperty(httpResponse, "status", INT_VAL(statusCode));
  setObjProperty(httpResponse, "url", OBJ_VAL(url));
  pop();
  return httpResponse;
}

size_t httpCURLHeaders(void* headers, size_t size, size_t nitems, void* userdata) {
  size_t realsize = size * nitems;
  if (nitems != 2) {
    CURLResponse* curlResponse = (CURLResponse*)userdata;
    char* ptr = realloc(curlResponse->headers, curlResponse->hSize + realsize + 1);
    if (!ptr) return 0;

    curlResponse->headers = ptr;
    memcpy(&(curlResponse->headers[curlResponse->hSize]), headers, realsize);
    curlResponse->hSize += realsize;
    curlResponse->headers[curlResponse->hSize] = 0;
  }
  return realsize;
}

size_t httpCURLResponse(void* contents, size_t size, size_t nmemb, void* userdata) {
  size_t realsize = size * nmemb;
  CURLResponse* curlResponse = (CURLResponse*)userdata;
  char* ptr = realloc(curlResponse->content, curlResponse->cSize + realsize + 1);
  if (!ptr) return 0;

  curlResponse->content = ptr;
  memcpy(&(curlResponse->content[curlResponse->cSize]), contents, realsize);
  curlResponse->cSize += realsize;
  curlResponse->content[curlResponse->cSize] = 0;
  return realsize;
}

struct curl_slist* httpParseHeaders(ObjDictionary* headers, CURL* curl) {
  struct curl_slist* headerList = NULL;
  for (int i = 0; i < headers->capacity; i++) {
    ObjEntry* entry = &headers->entries[i];
    if (!IS_STRING(entry->key) || !IS_STRING(entry->value)) continue;

    char header[UINT8_MAX] = "";
    snprintf(header, UINT8_MAX, "%s:%s", AS_STRING(entry->key)->chars, AS_STRING(entry->value)->chars);
    headerList = curl_slist_append(headerList, header);
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
  return headerList;
}

ObjString* httpParsePostData(ObjDictionary* dict) {
  if (dict->count == 0) return newString("");
  else {
    char string[UINT8_MAX] = "";
    size_t offset = 0;
    int startIndex = 0;

    for (int i = 0; i < dict->capacity; i++) {
      ObjEntry* entry = &dict->entries[i];
      if (IS_UNDEFINED(entry->key)) continue;
      Value key = entry->key;
      char* keyChars = valueToString(key);
      size_t keyLength = strlen(keyChars);
      Value value = entry->value;
      char* valueChars = valueToString(value);
      size_t valueLength = strlen(valueChars);

      memcpy(string + offset, keyChars, keyLength);
      offset += keyLength;
      memcpy(string + offset, "=", 1);
      offset += 1;
      memcpy(string + offset, valueChars, valueLength);
      offset += valueLength;
      startIndex = i + 1;
      break;
    }

    for (int i = startIndex; i < dict->capacity; i++) {
      ObjEntry* entry = &dict->entries[i];
      if (IS_UNDEFINED(entry->key)) continue;
      Value key = entry->key;
      char* keyChars = valueToString(key);
      size_t keyLength = strlen(keyChars);
      Value value = entry->value;
      char* valueChars = valueToString(value);
      size_t valueLength = strlen(valueChars);

      memcpy(string + offset, "&", 1);
      offset += 1;
      memcpy(string + offset, keyChars, keyLength);
      offset += keyLength;
      memcpy(string + offset, "=", 1);
      offset += 1;
      memcpy(string + offset, valueChars, valueLength);
      offset += valueLength;
    }

    string[offset] = '\0';
    return copyString(string, (int)offset + 1);
  }
}

struct addrinfo* dnsGetDomainInfo(const char* domainName, int* status) {
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags |= AI_CANONNAME;

  uv_getaddrinfo_t netGetAddrInfo;
  *status = uv_getaddrinfo(vm.eventLoop, &netGetAddrInfo, NULL, domainName, "80", &hints);
  return netGetAddrInfo.addrinfo;
}

ObjPromise* dnsGetDomainInfoAsync(ObjInstance* domain, uv_getaddrinfo_cb callback) {
  uv_getaddrinfo_t* netGetAddrInfo = ALLOCATE_STRUCT(uv_getaddrinfo_t);
  ObjPromise* promise = newPromise(PROMISE_PENDING, NIL_VAL, NIL_VAL);
  if (netGetAddrInfo == NULL) return NULL;
  else {
    netGetAddrInfo->data = networkLoadData(domain, promise);
    char* domainName = AS_CSTRING(getObjProperty(domain, "name"));
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags |= AI_CANONNAME;

    uv_getaddrinfo(vm.eventLoop, netGetAddrInfo, callback, domainName, "80", &hints);
    return promise;
  }
}

ObjPromise* dnsGetDomainFromIPAddressAsync(ObjInstance* ipAddress, uv_getnameinfo_cb callback) {
  uv_getnameinfo_t* netGetNameInfo = ALLOCATE_STRUCT(uv_getnameinfo_t);
  ObjPromise* promise = newPromise(PROMISE_PENDING, NIL_VAL, NIL_VAL);
  if (netGetNameInfo == NULL) return NULL;
  else {
    netGetNameInfo->data = networkLoadData(ipAddress, promise);
    char* address = AS_CSTRING(getObjProperty(ipAddress, "address"));
    struct sockaddr_in socketAddress;
    memset(&socketAddress, 0, sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;
    inet_pton(AF_INET, address, &socketAddress.sin_addr);
    uv_getnameinfo(vm.eventLoop, netGetNameInfo, callback, (struct sockaddr*)&socketAddress, 0);
    return promise;
  }
  return newPromise(PROMISE_FULFILLED, NIL_VAL, NIL_VAL);
}

ObjString* dnsGetDomainFromIPAddress(const char* ipAddress, int* status) {
  struct sockaddr_in socketAddress;
  memset(&socketAddress, 0, sizeof(socketAddress));
  socketAddress.sin_family = AF_INET;
  inet_pton(AF_INET, ipAddress, &socketAddress.sin_addr);

  uv_getnameinfo_t netGetNameInfo;
  *status = uv_getnameinfo(vm.eventLoop, &netGetNameInfo, NULL, (struct sockaddr*)&socketAddress, 0);
  return newString(netGetNameInfo.host);
}

void dnsOnGetAddrInfo(uv_getaddrinfo_t* netGetAddrInfo, int status, struct addrinfo* result) {
  NetworkData* data = netGetAddrInfo->data;
  LOOP_PUSH_DATA(data);

  if (status < 0) {
    ObjString* exceptionMessage = newString("Failed to resolve IP addresses for domain.");
    ObjClass* exceptionClass = getNativeClass("luminique::std::network", "DomainHostException");
    promiseReject(data->promise, OBJ_VAL(newException(exceptionMessage, exceptionClass)));
  } else {
    ObjArray* ipAddresses = dnsGetIPAddressesFromDomain(result);
    promiseFulfill(data->promise, OBJ_VAL(ipAddresses));
  }

  uv_freeaddrinfo(result);
  free(netGetAddrInfo);
  LOOP_POP_DATA(data);
}

ObjArray* dnsGetIPAddressesFromDomain(struct addrinfo* result) {
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

void dnsOnGetNameInfo(uv_getnameinfo_t* netGetNameInfo, int status, const char* hostName, const char* service) {
  NetworkData* data = netGetNameInfo->data;
  LOOP_PUSH_DATA(data);
  ObjString* domain = newString(netGetNameInfo->host);
  promiseFulfill(data->promise, OBJ_VAL(domain));
  free(netGetNameInfo);
  LOOP_POP_DATA(data);
}

bool isValidPort(const char* portStr) {
  if (*portStr == '\0') {
    return false;
  }
  
  for (int i = 0; portStr[i] != '\0'; i++) {
    if (!isdigit((unsigned char)portStr[i])) {
      return false;
    }
  }
  int port = atoi(portStr);
  return port >= 0 && port <= 65535;
}

bool isValidIPv4(const char* ipStr) {
  unsigned char b1, b2, b3, b4;
  if (sscanf(ipStr, "%hhu.%hhu.%hhu.%hhu", &b1, &b2, &b3, &b4) != 4) {
    return false;
  }
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%hhu.%hhu.%hhu.%hhu", b1, b2, b3, b4);
  return !strcmp(ipStr, buffer);
}

bool ipIsV4(ObjString* address) {
  char ip[16];
  const char* ipStr = address->chars;
  const char* portStr = NULL;

  char* colonPos = strchr(ipStr, ':');
  if (colonPos) {
    int ipLength = colonPos - ipStr;
    if (ipLength >= sizeof(ip)) {
      return false;
    }
    strncpy(ip, ipStr, ipLength);
    ip[ipLength] = '\0';
    portStr = colonPos + 1;
  } else {
    strncpy(ip, ipStr, sizeof(ip) - 1);
    ip[sizeof(ip) - 1] = '\0';
  }

  if (!isValidIPv4(ip)) {
    return false;
  }

  if (portStr && !isValidPort(portStr)) {
    return false;
  }

  return true;
}

bool isValidIPv6(const char* ipStr) {
  unsigned short b1, b2, b3, b4, b5, b6, b7, b8;
  if (sscanf(ipStr, "%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx", &b1, &b2, &b3, &b4, &b5, &b6, &b7, &b8) != 8) {
    return false;
  }
  char buffer[40];
  snprintf(buffer, sizeof(buffer), "%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx:%04hx", b1, b2, b3, b4, b5, b6, b7, b8);
  return !strcmp(ipStr, buffer);
}

bool ipIsV6(ObjString* address) {
  char ip[40];
  const char* ipStr = address->chars;
  const char* portStr = NULL;

  if (ipStr[0] == '[') {
    char* closingBracket = strchr(ipStr, ']');
    if (!closingBracket) {
      return false;
    }
    int ipLength = closingBracket - ipStr - 1;
    if (ipLength <= 0 || ipLength >= sizeof(ip)) {
      return false;
    }
    strncpy(ip, ipStr + 1, ipLength);
    ip[ipLength] = '\0';

    if (*(closingBracket + 1) == ':') {
      portStr = closingBracket + 2;
    } else if (*(closingBracket + 1) != '\0') {
      return false;
    }
  } else {
    strncpy(ip, ipStr, sizeof(ip) - 1);
    ip[sizeof(ip) - 1] = '\0';
  }

  if (!isValidIPv6(ip)) {
    return false;
  }

  if (portStr && !isValidPort(portStr)) {
    return false;
  }

  return true;
}

bool extractPort(const char* portStr, int* port) {
  if (*portStr == '\0') {
    return false;
  }
  for (int i = 0; portStr[i] != '\0'; i++) {
    if (!isdigit((unsigned char)portStr[i])) {
      return false;
    }
  }
  *port = atoi(portStr);
  return *port >= 0 && *port <= 65535;
}

bool extractIPv6AndPort(const char* address, char* ip, int* port) {
  const char* ipStr = address;
  const char* portStr = NULL;

  if (ipStr[0] == '[') {
    const char* closingBracket = strchr(ipStr, ']');
    if (!closingBracket) {
      return false;
    }
    int ipLength = closingBracket - ipStr - 1;
    if (ipLength <= 0 || ipLength >= 40) {
      return false;
    }
    strncpy(ip, ipStr + 1, ipLength);
    ip[ipLength] = '\0';

    if (*(closingBracket + 1) == ':') {
      portStr = closingBracket + 2;
    } else if (*(closingBracket + 1) != '\0') {
      return false;
    }
  } else {
    strncpy(ip, ipStr, 39);
    ip[39] = '\0';
  }

  if (!isValidIPv6(ip)) {
    return false;
  }

  if (portStr && !extractPort(portStr, port)) {
    return false;
  }

  return true;
}

bool extractIPv4AndPort(const char* address, char* ip, int* port) {
  const char* ipStr = address;
  const char* portStr = NULL;
  char* colonPos = strchr(ipStr, ':');

  if (colonPos) {
    int ipLength = colonPos - ipStr;
    if (ipLength >= 16) {
      return false;
    }
    strncpy(ip, ipStr, ipLength);
    ip[ipLength] = '\0';
    portStr = colonPos + 1;
  } else {
    strncpy(ip, ipStr, 15);
    ip[15] = '\0';
  }

  if (!isValidIPv4(ip)) {
    return false;
  }

  if (portStr && !extractPort(portStr, port)) {
    return false;
  }

  return true;
}

int ipParseBlock(ObjString* address, int startIndex, int endIndex, int radix) {
  ObjString* bString = subString(address, startIndex, endIndex);
  return strtol(bString->chars, NULL, radix);
}

void ipWriteByteArray(ObjArray* array, ObjString* address, int radix) {
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

bool urlIsAbsolute(ObjInstance* url) {
  ObjString* host = AS_STRING(getObjProperty(url, "host"));
  return host->length > 0;
}

ObjString* httpRawURL(Value value) {
  if (IS_INSTANCE(value)) {
    ObjInstance* url = AS_INSTANCE(value);
    return AS_STRING(getObjProperty(url, "raw"));
  }
  else return AS_STRING(value);
}

CURLcode httpSendRequest(ObjString* url, HTTPMethod method, ObjDictionary* data, CURL* curl, CURLResponse* curlResponse) {
  curlResponse->headers = malloc(0);
  curlResponse->content = malloc(0);
  curlResponse->hSize = 0;
  curlResponse->cSize = 0;

  curl_easy_setopt(curl, CURLOPT_URL, url->chars);
  if (method > HTTP_POST) {
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, httpMapMethod(method));
  }

  if (method == HTTP_HEAD) {
      curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  }
  else if (method == HTTP_POST || method == HTTP_PUT || method == HTTP_PATCH) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, httpParsePostData(data)->chars);
  }
  else if (method == HTTP_OPTIONS) {
      curl_easy_setopt(curl, CURLOPT_REQUEST_TARGET, "*");
  }
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpCURLResponse);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)curlResponse);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, httpCURLHeaders);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*)curlResponse);
  return curl_easy_perform(curl);
}

ObjString* urlToString(ObjInstance* url) {
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


