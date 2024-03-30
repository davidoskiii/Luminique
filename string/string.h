#ifndef cluminique_string_h
#define cluminique_string_h

#include "../common.h"
#include "../table/table.h"
#include "../chunk/chunk.h"
#include "../value/value.h"
#include "../object/object.h"

ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjString* formattedString(const char* format, ...);
ObjString* formattedLongString(const char* format, ...);
char* createFormattedString(const char* format, ...);
ObjString* copyFormattedString(const char* format, ...);

int searchString(ObjString* haystack, ObjString* needle, uint32_t start);
ObjString* capitalizeString(ObjString* string);
ObjString* decapitalizeString(ObjString* string);
ObjString* replaceString(ObjString* original, ObjString* target, ObjString* replace);
ObjString* subString(ObjString* original, int fromIndex, int toIndex);
ObjString* reverseStringBasedOnMemory(ObjString* string);

#endif
