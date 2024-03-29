#ifndef cluminique_native_h
#define cluminique_native_h

#include "../object/object.h"
#include "../vm/vm.h"

#define NATIVE_FUNCTION(name) static Value name##NativeFunction(int argCount, Value* args)
#define NATIVE_METHOD(className, name) static Value name##NativeMethodFor##className(Value receiver, int argCount, Value* args)
#define DEF_FUNCTION(name) defineNativeFunction(#name, name##NativeFunction)
#define DEF_FUNCTION(name) defineNativeFunction(#name, name##NativeFunction)
#define DEF_METHOD(klass, className, name) defineNativeMethod(klass, #name, name##NativeMethodFor##className)

#define RETURN_NIL return NIL_VAL
#define RETURN_FALSE return BOOL_VAL(false)
#define RETURN_TRUE return BOOL_VAL(true)
#define RETURN_BOOL(value) return BOOL_VAL(value)
#define RETURN_NUMBER(value) return NUMBER_VAL(value)
#define RETURN_OBJ(value) return OBJ_VAL(value)
#define RETURN_STRING(chars, length) return OBJ_VAL(copyString(chars, length))
#define RETURN_STRING_FMT(...) return OBJ_VAL(formattedString(__VA_ARGS__))
#define RETURN_STRING_FMTL(...) return OBJ_VAL(formattedLongString(__VA_ARGS__))


void initNatives();
void defineNativeFunction(const char* name, NativeFn function);
ObjClass* defineNativeClass(const char* name);
void defineNativeMethod(ObjClass* klass, const char* name, NativeMethod method);

#endif
