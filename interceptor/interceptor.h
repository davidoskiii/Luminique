#ifndef cluminique_interceptor_h
#define cluminique_interceptor_h

#include "../common.h"
#include "../value/value.h"

#define HAS_CLASS_INTERCEPTOR(klass, interceptor) ((klass->interceptors) & (1 << interceptor))
#define SET_CLASS_INTERCEPTOR(klass, interceptor) (klass->interceptors = (klass->interceptors) | (1 << interceptor))
#define HAS_OBJ_INTERCEPTOR(object, interceptor) (IS_OBJ(object) && HAS_CLASS_INTERCEPTOR(AS_OBJ(object)->klass, interceptor))

typedef enum {
  INTERCEPTOR_INIT,
  INTERCEPTOR_NEW,
  INTERCEPTOR_BEFORE_GET_PROPERTY,
  INTERCEPTOR_AFTER_GET_PROPERTY,
  INTERCEPTOR_BEFORE_SET_PROPERTY,
  INTERCEPTOR_AFTER_SET_PROPERTY,
  INTERCEPTOR_BEFORE_INVOKE_METHOD,
  INTERCEPTOR_AFTER_INVOKE_METHOD,
  INTERCEPTOR_UNDEFINED_PROPERTY,
  INTERCEPTOR_UNDEFINED_METHOD,
  INTERCEPTOR_BEFORE_THROW,
  INTERCEPTOR_AFTER_THROW
} InterceptorType;

void handleInterceptorMethod(ObjClass* klass, ObjString* name);
bool interceptBeforeGet(ObjClass* klass, ObjString* name);
bool interceptAfterGet(ObjClass* klass, ObjString* name);
bool interceptUndefinedProperty(ObjClass* klass, ObjString* name);
bool interceptUndefinedMethod(ObjClass* klass, ObjString* name, int argCount);

#endif
