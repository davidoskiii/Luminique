#ifndef luminique_assert_h
#define luminique_assert_h

#include "../common.h"
#include "../value/value.h"

void assertArgCount(const char* method, int expectedCount, int actualCount);
void assertArgIsBool(const char* method, Value* args, int index);
void assertArgIsClass(const char* method, Value* args, int index);
void assertArgIsDictionary(const char* method, Value* args, int index);
void assertArgIsFloat(const char* method, Value* args, int index);
void assertArgIsInt(const char* method, Value* args, int index);
void assertArgIsClosure(const char* method, Value* args, int index);
void assertArgIsArray(const char* method, Value* args, int index);
void assertArgIsNumber(const char* method, Value* args, int index);
void assertArgIsString(const char* method, Value* args, int index);
void assertArgIsException(const char* method, Value* args, int index);
void assertArgIsFile(const char* method, Value* args, int index);
void assertIntWithinRange(const char* method, int value, int min, int max, int index);
void assertNumberNonNegative(const char* method, double number, int index);
void assertNumberNonZero(const char* method, double number, int index);
void assertNumberPositive(const char* method, double number, int index);
void assertNumberWithinRange(const char* method, double value, double min, double max, int index);
void assertObjInstanceOfClass(const char* method, Value arg, char* className, int index);
void assertError(const char* message);

#endif 
