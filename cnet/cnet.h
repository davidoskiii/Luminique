#ifndef cluminique_cnet_h
#define cluminique_cnet_h

#include <curl/curl.h>

#include "../common.h"
#include "../value/value.h"
#include "../loop/loop.h"

typedef struct CURLResponse {
  char* headers;
  char* content;
  size_t hSize;
  size_t cSize;
} CURLResponse;

typedef enum HTTPMethod {
  HTTP_HEAD, 
  HTTP_GET, 
  HTTP_POST, 
  HTTP_PUT, 
  HTTP_DELETE, 
  HTTP_PATCH, 
  HTTP_OPTIONS, 
  HTTP_TRACE,
  HTTP_CONNECT,
  HTTP_QUERY
} HTTPMethod;

typedef struct NetworkData {
  VM* vm;
  ObjInstance* network;
  ObjPromise* promise;
} NetworkData;

struct addrinfo* dnsGetDomainInfo(const char* domainName, int* status);
ObjPromise* dnsGetDomainInfoAsync(ObjInstance* domain, uv_getaddrinfo_cb callback);
ObjString* dnsGetDomainFromIPAddress(const char* ipAddress, int* status);
ObjPromise* dnsGetDomainFromIPAddressAsync(ObjInstance* ipAddress, uv_getnameinfo_cb callback);
ObjArray* dnsGetIPAddressesFromDomain(struct addrinfo* result);
void dnsOnGetAddrInfo(uv_getaddrinfo_t* netGetAddrInfo, int status, struct addrinfo* result);
void dnsOnGetNameInfo(uv_getnameinfo_t* netGetNameInfo, int status, const char* hostName, const char* service);
ObjArray* httpCreateCookies(CURL* curl);
ObjArray* httpCreateHeaders(CURLResponse curlResponse);
ObjInstance* httpCreateResponse(ObjString* url, CURL* curl, CURLResponse curlResponse);
size_t httpCURLHeaders(void* headers, size_t size, size_t nitems, void* userdata);
size_t httpCURLResponse(void* contents, size_t size, size_t nmemb, void* userdata);
struct curl_slist* httpParseHeaders(ObjDictionary* headers, CURL* curl);
ObjString* httpParsePostData(ObjDictionary* postData);
ObjString* httpRawURL(Value value);
CURLcode httpSendRequest(ObjString* url, HTTPMethod method, ObjDictionary* data, CURL* curl, CURLResponse* curlResponse);
bool ipIsV4(ObjString* address);
bool ipIsV6(ObjString* address);
int ipParseBlock(ObjString* address, int startIndex, int endIndex, int radix);
void ipWriteByteArray(ObjArray* array, ObjString* address, int radix);
bool urlIsAbsolute(ObjInstance* url);
ObjString* urlToString(ObjInstance* url);

bool extractIPv6AndPort(const char* address, char* ip, int* port);
bool extractIPv4AndPort(const char* address, char* ip, int* port);

static inline char* httpMapMethod(HTTPMethod method) {
  switch (method) {
    case HTTP_GET: return "GET";
    case HTTP_POST: return "POST";
    case HTTP_PUT: return "PUT";
    case HTTP_DELETE: return "DELETE";
    case HTTP_PATCH: return "PATCH";
    case HTTP_OPTIONS: return "OPTIONS";
    case HTTP_TRACE: return "TRACE";
    case HTTP_CONNECT: return "CONNECT";
    case HTTP_QUERY: return "QUERY";
    default: return "HEAD";
  }
}


#endif
