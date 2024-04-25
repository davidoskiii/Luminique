#ifndef cluminique_native_h
#define cluminique_native_h

#include "../object/object.h"
#include "../vm/vm.h"

#define NATIVE_FUNCTION(name) static Value name##NativeFunction(int argCount, Value* args)
#define NATIVE_METHOD(className, name) static Value name##NativeMethodFor##className(Value receiver, int argCount, Value* args)
#define DEF_FUNCTION(name, arity) defineNativeFunction(#name, arity, name##NativeFunction)
#define DEF_METHOD(klass, className, name, arity) defineNativeMethod(klass, #name, arity, name##NativeMethodFor##className)

#define RETURN_NIL return NIL_VAL
#define RETURN_FALSE return BOOL_VAL(false)
#define RETURN_TRUE return BOOL_VAL(true)
#define RETURN_BOOL(value) return BOOL_VAL(value)
#define RETURN_INT(value) return INT_VAL(value)
#define RETURN_FLOAT(value) return FLOAT_VAL(value)
#define RETURN_NUMBER(value) return NUMBER_VAL(value)
#define RETURN_OBJ(value) return OBJ_VAL(value)
#define RETURN_STRING(chars, length) return OBJ_VAL(copyString(chars, length))
#define RETURN_STRING_FMT(...) return OBJ_VAL(formattedString(__VA_ARGS__))
#define RETURN_STRING_FMTL(...) return OBJ_VAL(formattedLongString(__VA_ARGS__))
#define THROW_EXCEPTION(namespace_, klass, message) return OBJ_VAL(throwException(getNativeClass(#namespace_, #klass), message))
#define THROW_EXCEPTION_FMT(namespace_, klass, message, ...) return OBJ_VAL(throwException(getNativeClass(#namespace_ #klass), message, __VA_ARGS__))
#define RETURN_VAL(value) return value
#define DEF_OPERATOR(klass, className, symbol, name, arity) defineNativeMethod(klass, #symbol, arity, name##NativeMethodFor##className)


void initNatives();
void initNativePackage(const char* filePath);
void loadSourceFile(const char* filePath);
void defineNativeFunction(const char* name, int arity, NativeFunction functionion);
void defineNativeMethod(ObjClass* klass, const char* name, int arity, NativeMethod method);
ObjNamespace* defineNativeNamespace(const char* name, ObjNamespace* enclosing);
ObjClass* defineNativeClass(const char* name);
ObjClass* getNativeClass(const char* namespaceName, const char* className);
ObjNativeFunction* getNativeFunction(const char* name);
ObjNamespace* getNativeNamespace(const char* name);
ObjNativeMethod* getNativeMethod(ObjClass* klass, const char* name);
ObjClass* defineNativeException(const char* name, ObjClass* superClass);

#endif
