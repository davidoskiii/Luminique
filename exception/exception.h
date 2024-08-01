#ifndef cluminique_exception_h
#define cluminique_exception_h

#include "../common.h"
#include "../value/value.h"

struct ExceptionHandler {
  uint16_t handlerAddress;
  uint16_t finallyAddress;
  ObjClass* exceptionClass;
};


bool propagateException();
void pushExceptionHandler(ObjClass* exceptionClass, uint16_t handlerAddress, uint16_t finallyAddress);
ObjArray* getStackTrace();
ObjException* throwException(ObjClass* exceptionClass, const char* format, ...);
ObjException* throwNativeException(const char* namespace_, const char* exceptionClassName, const char* format, ...);
ObjException* throwException(ObjClass* exceptionClass, const char* format, ...);

#endif
