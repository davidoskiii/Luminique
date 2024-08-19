#ifndef cluminique_native_h
#define cluminique_native_h

#include "../value/value.h"
#include "../object/object.h"
#include "../hash/hash.h"
#include "../interceptor/interceptor.h"

#define NATIVE_FUNCTION(name) static Value name##NativeFunction(int argCount, Value* args)
#define NATIVE_METHOD(className, name) static Value name##NativeMethodFor##className(Value receiver, int argCount, Value* args)
#define NATIVE_ABSTRACT_METHOD(className, name) static Value name##AbstractNativeMethodFor##className(Value receiver, int argCount, Value* args) { RETURN_NIL; }
#define DEF_FUNCTION(name, arity) defineNativeFunction(#name, arity, false, name##NativeFunction)
#define DEF_FUNCTION_ASYNC(name, arity) defineNativeFunction(#name, arity, true, name##NativeFunction)
#define DEF_METHOD(klass, className, name, arity) defineNativeMethod(klass, #name, arity, false, name##NativeMethodFor##className)
#define DEF_METHOD_ABSTRACT(klass, className, name, arity, ...) \
  do { \
    if (arity == 0) { \
      defineNativeAbstractMethod(klass, #name, arity, NULL, name##AbstractNativeMethodFor##className); \
    } else if (arity == -1) { \
      const char* chars[] = {__VA_ARGS__}; \
      defineNativeAbstractMethod(klass, #name, 1, hashStringArray(chars, 1), name##AbstractNativeMethodFor##className); \
    } else { \
      const char* chars[] = {__VA_ARGS__}; \
      defineNativeAbstractMethod(klass, #name, arity, hashStringArray(chars, arity), name##AbstractNativeMethodFor##className); \
    } \
  } while (false)

#define DEF_METHOD_ASYNC(klass, className, name, arity) defineNativeMethod(klass, #name, arity, true, name##NativeMethodFor##className)
#define DEF_OPERATOR(klass, className, symbol, name, arity) defineNativeMethod(klass, #symbol, arity, false, name##NativeMethodFor##className)
#define DEF_INTERCEPTOR(klass, className, type, name, arity) defineNativeInterceptor(klass, type, arity, name##NativeMethodFor##className)

#define RETURN_NIL return NIL_VAL
#define RETURN_FALSE return BOOL_VAL(false)
#define RETURN_TRUE return BOOL_VAL(true)
#define RETURN_BOOL(value) return BOOL_VAL(value)
#define RETURN_INT(value) return INT_VAL(value)
#define RETURN_FLOAT(value) return FLOAT_VAL(value)
#define RETURN_NUMBER(value) return NUMBER_VAL(value)
#define RETURN_OBJ(value) return OBJ_VAL(value)
#define RETURN_PROMISE(state, value) return OBJ_VAL(newPromise(state, value, NIL_VAL));
#define RETURN_PROMISE_EX(state, value, executor) return OBJ_VAL(newPromise(state, value, executor));
#define RETURN_STRING(chars, length) return OBJ_VAL(copyString(chars, length))
#define RETURN_STRING_FMT(...) return OBJ_VAL(formattedString(__VA_ARGS__))
#define RETURN_STRING_FMTL(...) return OBJ_VAL(formattedLongString(__VA_ARGS__))
#define THROW_EXCEPTION(namespace_, klass, message) return OBJ_VAL(throwNativeException(#namespace_, #klass, message))
#define THROW_EXCEPTION_FMT(namespace_, klass, message, ...) return OBJ_VAL(throwNativeException(#namespace_, #klass, message, __VA_ARGS__))
#define RETURN_VAL(value) return value


void initNatives();
void initNativePackage(const char* filePath);
void loadSourceFile(const char* filePath);
void defineNativeFunction(const char* name, int arity, bool isAsync, NativeFunction functionion);
void defineNativeMethod(ObjClass* klass, const char* name, int arity, bool isAsync, NativeMethod method);
void defineNativeAbstractMethod(ObjClass* klass, const char* name, int arity, uint32_t* paramHashes, NativeMethod method);
void defineNativeConstant(ObjNamespace* namespace_, const char* name, Value value);
void defineNativeInterceptor(ObjClass* klass, InterceptorType type, int arity, NativeMethod method);
ObjNamespace* defineNativeNamespace(const char* name, ObjNamespace* enclosing);
ObjClass* defineNativeClass(const char* name, bool isAbstract);
ObjEnum* defineNativeEnum(const char* name);
void defineNativeEnumElement(ObjEnum* enum_, const char* name);
void defineNativeArtificialEnumElement(ObjEnum* enum_, const char* name, Value value);
ObjClass* getNativeClass(const char* namespaceName, const char* className);
ObjNativeFunction* getNativeFunction(const char* name);
ObjNamespace* getNativeNamespace(const char* name);
ObjNativeMethod* getNativeMethod(ObjClass* klass, const char* name);
ObjClass* defineNativeException(const char* name, ObjClass* superClass);

#endif
