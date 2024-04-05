#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../hash/hash.h"
#include "../memory/memory.h"
#include "../vm/vm.h"
#include "string.h"

#define ALLOCATE_OBJ(type, objectType, objectClass) (type*)allocateObject(sizeof(type), objectType, objectClass)

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING, vm.stringClass);
  string->length = length;
  string->chars = chars;
  string->hash = hash;

  push(OBJ_VAL(string));
  tableSet(&vm.strings, string, NIL_VAL);
  pop();

  return string;
}


ObjString* takeString(char* chars, int length) {
  uint32_t hash = hashString(chars, length);

  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }

  return allocateString(chars, length, hash);
}

ObjString* copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);


  ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) return interned;

  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length, hash);
}

ObjString* newString(const char* chars) {
  return copyString(chars, (int)strlen(chars));
}

ObjString* formattedString(const char* format, ...) {
  char chars[UINT8_MAX];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(chars, UINT8_MAX, format, args);
  va_end(args);
  return copyString(chars, length);
}

ObjString* formattedLongString(const char* format, ...) {
  char chars[UINT16_MAX];
  va_list args;
  va_start(args, format);
  int length = vsnprintf(chars, UINT16_MAX, format, args);
  va_end(args);
  return copyString(chars, length);
}


char* createFormattedString(const char* format, ...) {
  va_list args;
  va_start(args, format);
  size_t len = vsnprintf(NULL, 0, format, args);
  va_end(args);
  char* result = ALLOCATE(char, len + 1);

  va_start(args, format);
  vsnprintf(result, len + 1, format, args);
  va_end(args);

  return result;
}

ObjString* copyFormattedString(const char* format, ...) {
  va_list args;
  va_start(args, format);
  size_t len = vsnprintf(NULL, 0, format, args);
  va_end(args);

  char* heapChars = ALLOCATE(char, len + 1);

  va_start(args, format);
  vsnprintf(heapChars, len + 1, format, args);
  va_end(args);
  uint32_t hash = hashString(heapChars, len);
  ObjString* interned = tableFindString(&vm.strings, heapChars, len, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, heapChars, len + 1);
    return interned;
  }

  return allocateString(heapChars, len, hash);
}

int searchString(ObjString* haystack, ObjString* needle, uint32_t start) {
  if (needle->length == 0) return start;
  if (start + needle->length > (uint32_t)haystack->length || start >= (uint32_t)haystack->length) return -1;
  uint32_t shift[UINT8_MAX];
  uint32_t needleEnd = needle->length - 1;

  for (uint32_t index = 0; index < UINT8_MAX; index++) {
    shift[index] = needle->length;
  }

  for (uint32_t index = 0; index < needleEnd; index++) {
    char c = needle->chars[index];
    shift[(uint8_t)c] = needleEnd - index;
  }

  char lastChar = needle->chars[needleEnd];
  uint32_t range = haystack->length - needle->length;

  for (uint32_t index = start; index <= range; ) {
    char c = haystack->chars[index + needleEnd];
    if (lastChar == c && memcmp(haystack->chars + index, needle->chars, needleEnd) == 0) {
      return index;
    }

    index += shift[(uint8_t)c];
  }

  return -1;
}

ObjString* capitalizeString(ObjString* string) {
  if (string->length == 0) return string;
  char* heapChars = ALLOCATE(char, (size_t)string->length + 1);

  heapChars[0] = (char)toupper(string->chars[0]);
  for (int offset = 1; offset < string->length; offset++) {
    heapChars[offset] = string->chars[offset];
  }
  heapChars[string->length] = '\0';
  return takeString(heapChars, (int)string->length);
}

ObjString* decapitalizeString(ObjString* string) {
  if (string->length == 0) return string;
  char* heapChars = ALLOCATE(char, (size_t)string->length + 1);

  heapChars[0] = (char)tolower(string->chars[0]);
  for (int offset = 1; offset < string->length; offset++) {
    heapChars[offset] = string->chars[offset];
  }
  heapChars[string->length] = '\0';
  return takeString(heapChars, (int)string->length);
}

ObjString* replaceString(ObjString* original, ObjString* target, ObjString* replace) {
  if (original->length == 0 || target->length == 0 || original->length < target->length) return original;
  int startIndex = searchString(original, target, 0);
  if (startIndex == -1) return original;

  int newLength = original->length - target->length + replace->length;
  char* heapChars = ALLOCATE(char, (size_t)newLength + 1);

  int offset = 0;
  while (offset < startIndex) {
    heapChars[offset] = original->chars[offset];
    offset++;
  }

  while (offset < startIndex + replace->length) {
    heapChars[offset] = replace->chars[offset - startIndex];
    offset++;
  }

  while (offset < newLength) {
    heapChars[offset] = original->chars[offset - replace->length + target->length];
    offset++;
  }

  heapChars[newLength] = '\n';
  return takeString(heapChars, (int)newLength);
}

ObjString* subString(ObjString* original, int fromIndex, int toIndex) {
  if (fromIndex >= original->length || toIndex > original->length || fromIndex > toIndex) {
    return copyString("", 0);
  }

  int newLength = toIndex - fromIndex + 1;
  char* heapChars = ALLOCATE(char, (size_t)newLength + 1);
  for (int i = 0; i < newLength; i++) {
    heapChars[i] = original->chars[fromIndex + i];
  }

  heapChars[newLength] = '\n';
  return takeString(heapChars, (int)newLength);
}

ObjString* reverseStringBasedOnMemory(ObjString* string) {
  if (string->length <= 1) return string;

  char* reversedChars = ALLOCATE(char, string->length + 1);
  if (reversedChars == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }

  for (int i = 0; i < string->length; i++) {
    reversedChars[i] = string->chars[string->length - i - 1];
  }
  reversedChars[string->length] = '\0';

  return takeString(reversedChars, string->length);
}

ObjString* toLowerString(ObjString* string) {
  if (string->length == 0) return string;
  char* heapChars = ALLOCATE(char, (size_t)string->length + 1);

  for (int offset = 0; offset < string->length; offset++) {
    heapChars[offset] = (char)tolower(string->chars[offset]);
  }
  heapChars[string->length] = '\0';
  return takeString(heapChars, (int)string->length);
}

ObjString* toUpperString(ObjString* string) {
  if (string->length == 0) return string;
  char* heapChars = ALLOCATE(char, (size_t)string->length + 1);

  for (int offset = 0; offset < string->length; offset++) {
    heapChars[offset] = (char)toupper(string->chars[offset]);
  }
  heapChars[string->length] = '\0';
  return takeString(heapChars, (int)string->length);
}

ObjString* trimString(ObjString* string) {
  int ltLen = 0, rtLen = 0;

  for (int i = 0; i < string->length; i++) {
    char c = string->chars[i];
    if (c != ' ' && c != '\t' && c != '\n') break;
    ltLen++;
  }

  for (int i = string->length - 1; i >= 0; i--) {
    char c = string->chars[i];
    if (c != ' ' && c != '\t' && c != '\n') break;
    rtLen++;
  }

  int newLength = string->length - ltLen - rtLen + 1;
  char* heapChars = ALLOCATE(char, (size_t)newLength + 1);

  for (int i = 0; i < newLength; i++) {
    heapChars[i] = string->chars[i + ltLen];
  }

  heapChars[newLength] = '\n';
  return takeString(heapChars, (int)newLength);
}

int utf8NumBytes(int value) {
  if (value < 0) return -1;
  if (value <= 0x7f) return 1;
  if (value <= 0x7ff) return 2;
  if (value <= 0xffff) return 3;
  if (value <= 0x10ffff) return 4;
  return 0;
}

char* utf8Encode(int value) {
  int length = utf8NumBytes(value);
  if (value == -1) return NULL;
  char* utfChar = (char*)malloc((size_t)length + 1);

  if (utfChar != NULL) {
    if (value <= 0x7f) {
      utfChar[0] = (char)(value & 0x7f);
      utfChar[1] = '\0';
    }
    else if (value <= 0x7ff) {
      utfChar[0] = 0xc0 | ((value & 0x7c0) >> 6);
      utfChar[1] = 0x80 | (value & 0x3f);
    }
    else if (value <= 0xffff) {
      utfChar[0] = 0xe0 | ((value & 0xf000) >> 12);
      utfChar[1] = 0x80 | ((value & 0xfc0) >> 6);
      utfChar[2] = 0x80 | (value & 0x3f);
    }
    else if (value <= 0x10ffff) {
      utfChar[0] = 0xf0 | ((value & 0x1c0000) >> 18);
      utfChar[1] = 0x80 | ((value & 0x3f000) >> 12);
      utfChar[2] = 0x80 | ((value & 0xfc0) >> 6);
      utfChar[3] = 0x80 | (value & 0x3f);
    }
    else {
      utfChar[0] = 0xbd;
      utfChar[1] = 0xbf;
      utfChar[2] = 0xef;
    }
  }
  return utfChar;
}

int utf8Decode(const uint8_t* bytes, uint32_t length) {
  if (*bytes <= 0x7f) return *bytes;
  int value;
  uint32_t remainingBytes;

  if ((*bytes & 0xe0) == 0xc0) {
    value = *bytes & 0x1f;
    remainingBytes = 1;
  }
  else if ((*bytes & 0xf0) == 0xe0) {
    value = *bytes & 0x0f;
    remainingBytes = 2;
  }
  else if ((*bytes & 0xf8) == 0xf0) {
    value = *bytes & 0x07;
    remainingBytes = 3;
  }
  else return -1;

  if (remainingBytes > length - 1) return -1;
  while (remainingBytes > 0) {
    bytes++;
    remainingBytes--;
    if ((*bytes & 0xc0) != 0x80) return -1;
    value = value << 6 | (*bytes & 0x3f);
  }
  return value;
}

ObjString* utf8StringFromByte(uint8_t byte) {
  char chars[2] = { byte, '\0' };
  return copyString(chars, 1);
}

ObjString* utf8StringFromCodePoint(int codePoint) {
  int length = utf8NumBytes(codePoint);
  if (length <= 0) return NULL;
  char* utfChars = utf8Encode(codePoint);
  return takeString(utfChars, length);
}

int utf8CodePointOffset(const char* string, int index) {
  int offset = 0;
  do {
    offset++;
  } while ((string[index + offset] & 0xc0) == 0x80);
  return offset;
}

ObjString* utf8CodePointAtIndex(const char* string, int index) {
  int length = utf8CodePointOffset(string, index);
  switch (length) {
    case 1: return copyString((char[]) { string[index], '\0' }, 1);
    case 2: return copyString((char[]) { string[index], string[index + 1], '\0' }, 2);
    case 3: return copyString((char[]) { string[index], string[index + 1], string[index + 2], '\0' }, 3);
    case 4: return copyString((char[]) { string[index], string[index + 1], string[index + 2], string[index + 3], '\0' }, 4);
    default: return copyString("", 0);
  }
}
