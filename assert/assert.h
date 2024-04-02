#ifndef luminique_assert_h
#define luminique_assert_h

#include "../common.h"
#include "../value/value.h"

void assertArgCount(const char* method, int expectedCount, int actualCount);
void assertArgIsBool(const char* method, Value* args, int index);
void assertArgIsClass(const char* method, Value* args, int index);
void assertArgIsNumber(const char* method, Value* args, int index);
void assertArgIsString(const char* method, Value* args, int index);
void assertArgIsArray(const char* method, Value* args, int index);
void assertArgIsDictionary(const char* method, Value* args, int index);
void assertArgIsFloat(const char* method, Value* args, int index);
void assertArgIsInt(const char* method, Value* args, int index);
void assertIndexWithinRange(const char* method, int arg, int min, int max, int index);
void assertNonNegativeNumber(const char* method, double number, int index);
void assertPositiveNumber(const char* method, double number, int index);
void assertNonZero(const char* method, double number, int index);
void assertError(const char* message);

#endif 
