#ifndef cluminique_compiler_h
#define cluminique_compiler_h

#include "../object/object.h"
#include "../vm/vm.h"

ObjFunction* compile(const char* source);

#endif
