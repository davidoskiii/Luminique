#ifndef cluminique_arrays_h
#define cluminique_arrays_h

#include "../object/object.h"
#include "../vm/vm.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../value/value.h"

int arrayIndexOf(ObjArray* array, Value element);
void arrayInsertAt(ObjArray* array, int index, Value element);
int arrayLastIndexOf(ObjArray* array, Value element);
Value arrayRemoveAt(ObjArray* array, int index);
Value arrayRemoveAt(ObjArray* array, int index);

#endif
