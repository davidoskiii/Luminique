#ifndef cluminique_native_h
#define cluminique_native_h

#include "../object/object.h"
#include "../vm/vm.h"

void initNatives();
void defineNativeFunction(const char* name, NativeFn function);
ObjClass* defineNativeClass(const char* name);
void defineNativeMethod(ObjClass* klass, const char* name, NativeMethod method);

#endif
