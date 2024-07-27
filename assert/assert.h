#ifndef luminique_assert_h
#define luminique_assert_h

#include "../common.h"
#include "../value/value.h"

Value assertArgCount(const char* method, uint16_t expectedCount, uint16_t actualCount);
Value assertArgIsBool(const char* method, Value* args, int index);
Value assertArgIsClass(const char* method, Value* args, int index);
Value assertArgIsDictionary(const char* method, Value* args, int index);
Value assertArgIsFloat(const char* method, Value* args, int index);
Value assertArgIsInt(const char* method, Value* args, int index);
Value assertArgIsClosure(const char* method, Value* args, int index);
Value assertArgIsArray(const char* method, Value* args, int index);
Value assertArgIsNumber(const char* method, Value* args, int index);
Value assertArgIsTimer(const char* method, Value* args, int index);
Value assertArgIsString(const char* method, Value* args, int index);
Value assertArgIsException(const char* method, Value* args, int index);
Value assertArgIsPromise(const char* method, Value* args, int index);
Value assertArgIsFile(const char* method, Value* args, int index);
Value assertArgIsNamespace(const char* method, Value* args, int index);
Value assertArgIsMethod(const char* method, Value* args, int index);
Value assertIsNumber(const char* method, Value number);
Value assertArgInstanceOfEither(const char* method, Value* args, int index, const char* namespaceName, const char* className, const char* namespaceName2, const char* className2);
Value assertIntWithinRange(const char* method, int value, int min, int max, int index);
Value assertNumberNonNegative(const char* method, double number, int index);
Value assertNumberNonZero(const char* method, double number, int index);
Value assertNumberPositive(const char* method, double number, int index);
Value assertNumberWithinRange(const char* method, double value, double min, double max, int index);
Value assertObjInstanceOfClass(const char* method, Value arg, char* namespaceName, char* className, int index);
Value assertError(const char* message);

#endif 
