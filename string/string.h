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
ObjString* toLowerString(ObjString* string);
ObjString* toUpperString(ObjString* string);
ObjString* trimString(ObjString* string);

int utf8NumBytes(int value);
char* utf8Encode(int value);
int utf8Decode(const uint8_t* bytes, uint32_t length);
ObjString* utf8StringFromByte(uint8_t byte);
ObjString* utf8StringFromCodePoint(int codePoint);
int utf8CodePointOffset(const char* string, int index);
ObjString* utf8CodePointAtIndex(const char* string, int index);

#endif
