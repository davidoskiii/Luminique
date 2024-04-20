#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <regex.h>

#include "util.h"
#include "../pcg/pcg.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../hash/hash.h"
#include "../value/value.h"
#include "../vm/vm.h"

#define MAX_MATCHES 1

char *substr(const char *str, int start, int end) {
  int len = end - start;
  char *substr = malloc(len + 1);
  if (!substr) {
    return NULL;
  }
  strncpy(substr, str + start, len);
  substr[len] = '\0';
  return substr;
}

char *concat(const char *s1, const char *s2) {
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  char *result = malloc(len1 + len2 + 1);
  if (!result) {
    return NULL;
  }
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

static struct tm dateToTm(int year, int month, int day) {
	struct tm cDate = { 
		.tm_year = year - 1900,
		.tm_mon = month - 1,
		.tm_mday = day
	};
	return cDate;
}

static struct tm dateTimeToTm(int year, int month, int day, int hour, int minute, int second) {
	struct tm cDate = {
    .tm_year = year - 1900,
    .tm_mon = month - 1,
    .tm_mday = day, 
		.tm_hour = hour,
		.tm_min = minute,
		.tm_sec = second
	};
	return cDate;
}

static double dateGetTimestamp(int year, int month, int day) {
	struct tm cTime = dateToTm(year, month, day);
	return (double)mktime(&cTime);
}

static double dateTimeGetTimestamp(int year, int month, int day, int hour, int minute, int second) {
	struct tm cTime = dateTimeToTm(year, month, day, hour, minute, second);
	return (double)mktime(&cTime);
}

static double dateObjGetTimestamp(ObjInstance* date) {
	Value year = getObjProperty(date, "year");
	Value month = getObjProperty(date, "month");
	Value day = getObjProperty(date, "day");
	return dateGetTimestamp(AS_INT(year), AS_INT(month), AS_INT(day));
}

static double dateTimeObjGetTimestamp(ObjInstance* dateTime) {
	Value year = getObjProperty(dateTime, "year");
	Value month = getObjProperty(dateTime, "month");
	Value day = getObjProperty(dateTime, "day");
	Value hour = getObjProperty(dateTime, "hour");
	Value minute = getObjProperty(dateTime, "minute");
	Value second = getObjProperty(dateTime, "second");
	return dateTimeGetTimestamp(AS_INT(year), AS_INT(month), AS_INT(day), AS_INT(hour), AS_INT(minute), AS_INT(second));
}

static ObjInstance* dateObjFromTimestamp(ObjClass* dateClass, double timeValue) {
  time_t timestamp = (time_t)timeValue;
  struct tm time;
  localtime_r(&timestamp, &time);
  ObjInstance* date = newInstance(dateClass);
  push(OBJ_VAL(date));
  setObjProperty(date, "year", INT_VAL(1900 + time.tm_year));
  setObjProperty(date, "month", INT_VAL(1 + time.tm_mon));
  setObjProperty(date, "day", INT_VAL(time.tm_mday));
  pop();
  return date;
}

static ObjInstance* dateTimeObjFromTimestamp(ObjClass* dateTimeClass, double timeValue) {
  time_t timestamp = (time_t)timeValue;
  struct tm time;
  localtime_r(&timestamp, &time);
  ObjInstance* dateTime = newInstance(dateTimeClass);
  push(OBJ_VAL(dateTime));
  setObjProperty(dateTime, "year", INT_VAL(1900 + time.tm_year));
  setObjProperty(dateTime, "month", INT_VAL(1 + time.tm_mon));
  setObjProperty(dateTime, "day", INT_VAL(time.tm_mday));
  setObjProperty(dateTime, "hour", INT_VAL(time.tm_hour));
  setObjProperty(dateTime, "minute", INT_VAL(time.tm_min));
  setObjProperty(dateTime, "second", INT_VAL(time.tm_sec));
  pop();
  return dateTime;
}

static void duration__init__(int* duration, Value* args) {
	int days = AS_INT(args[0]);
	int hours = AS_INT(args[1]);
	int minutes = AS_INT(args[2]);
	int seconds = AS_INT(args[3]);

	if (seconds > 60) {
		minutes += seconds / 60;
		seconds %= 60;
	}

	if (minutes > 60) {
		hours += minutes / 60;
		minutes %= 60;
	}

	if (hours > 60) {
		days += hours / 24;
		hours %= 24;
	}

	duration[0] = days;
	duration[1] = hours;
	duration[2] = minutes;
	duration[3] = seconds;
}

static double durationTotalSeconds(ObjInstance* duration) {
	Value days = getObjProperty(duration, "days");
	Value hours = getObjProperty(duration, "hours");
	Value minutes = getObjProperty(duration, "minutes");
	Value seconds = getObjProperty(duration, "seconds");
	return 86400.0 * AS_INT(days) + 3600.0 * AS_INT(hours) + 60.0 * AS_INT(minutes) + AS_INT(seconds);
}

// DATE 

NATIVE_METHOD(Date, __init__) {
	assertArgCount("Date::__init__(day, month, year)", 3, argCount);
	assertArgIsInt("Date::__init__(day, month, year)", args, 0);
	assertArgIsInt("Date::__init__(day, month, year)", args, 1);
	assertArgIsInt("Date::__init__(day, month, year)", args, 2);

	ObjInstance* self = AS_INSTANCE(receiver);
	setObjProperty(self, "day", args[0]);
	setObjProperty(self, "month", args[1]);
	setObjProperty(self, "year", args[2]);
	return receiver;
}

NATIVE_METHOD(Date, minus) {
  assertArgCount("Date::minus(duration)", 1, argCount);
  assertObjInstanceOfClass("Date::minus(duration)", args[0], "Duration", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  double timestamp = dateObjGetTimestamp(self) - durationTotalSeconds(AS_INSTANCE(args[0]));
  ObjInstance* date = dateObjFromTimestamp(self->obj.klass, timestamp);
  RETURN_OBJ(date);
}

NATIVE_METHOD(Date, plus) {
  assertArgCount("Date::plus(duration)", 1, argCount);
  assertObjInstanceOfClass("Date::plus(duration)", args[0], "Duration", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  double timestamp = dateObjGetTimestamp(self) + durationTotalSeconds(AS_INSTANCE(args[0]));
  ObjInstance* date = dateObjFromTimestamp(self->obj.klass, timestamp);
  RETURN_OBJ(date);
}

NATIVE_METHOD(Date, toString) {
	assertArgCount("Date::toString()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	Value year = getObjProperty(self, "year");
	Value month = getObjProperty(self, "month");
	Value day = getObjProperty(self, "day");
	RETURN_STRING_FMT("%02d/%02d/%d", AS_INT(day), AS_INT(month), AS_INT(year));
}

NATIVE_METHOD(Date, toDateTime) {
	assertArgCount("Date::toDateTime()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	ObjInstance* dateTime = newInstance(getNativeClass("DateTime"));
  push(OBJ_VAL(dateTime));
	setObjProperty(dateTime, "year", getObjProperty(self, "year"));
	setObjProperty(dateTime, "month", getObjProperty(self, "month"));
	setObjProperty(dateTime, "day", getObjProperty(self, "day"));
	setObjProperty(dateTime, "hour", INT_VAL(0));
	setObjProperty(dateTime, "minute", INT_VAL(0));
	setObjProperty(dateTime, "second", INT_VAL(0));
  pop();
	RETURN_OBJ(dateTime);
}

NATIVE_METHOD(Date, after) {
	assertArgCount("Date::after(date)", 1, argCount);
	assertObjInstanceOfClass("Date::after(date)", args[0], "Date", 0);
	double timestamp = dateObjGetTimestamp(AS_INSTANCE(receiver));
	double timestamp2 = dateObjGetTimestamp(AS_INSTANCE(args[0]));
	RETURN_BOOL(timestamp > timestamp2);
}

NATIVE_METHOD(Date, before) {
	assertArgCount("Date::before(date)", 1, argCount);
	assertObjInstanceOfClass("Date::before(date)", args[0], "Date", 0);
	double timestamp = dateObjGetTimestamp(AS_INSTANCE(receiver));
	double timestamp2 = dateObjGetTimestamp(AS_INSTANCE(args[0]));
	RETURN_BOOL(timestamp < timestamp2);
}

NATIVE_METHOD(Date, diff) {
	assertArgCount("Date::diff(date)", 1, argCount);
	assertObjInstanceOfClass("Date::diff(date)", args[0], "Date", 0);
	double timestamp = dateObjGetTimestamp(AS_INSTANCE(receiver));
	double timestamp2 = dateObjGetTimestamp(AS_INSTANCE(args[0]));
	RETURN_NUMBER(timestamp - timestamp2);
}

NATIVE_METHOD(Date, getTimestamp) {
	assertArgCount("Date::getTimestamp()", 0, argCount);
	RETURN_NUMBER(dateObjGetTimestamp(AS_INSTANCE(receiver)));
}

// RANDOM

NATIVE_METHOD(Random, __init__) {
	assertArgCount("Random::__init__()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	uint64_t seed = (uint64_t)time(NULL);
	pcg32_seed(seed);
	setObjProperty(self, "seed", INT_VAL(abs((int)seed)));
	return receiver;
}

NATIVE_METHOD(Random, getSeed) {
	assertArgCount("Random::getSeed()", 0, argCount);
	Value seed = getObjProperty(AS_INSTANCE(receiver), "seed");
	RETURN_VAL(seed);
}

NATIVE_METHOD(Random, nextBool) {
	assertArgCount("Random::nextBool()", 0, argCount);
	bool value = pcg32_random_bool();
	RETURN_BOOL(value);
}

NATIVE_METHOD(Random, nextFloat) {
	assertArgCount("Random::nextFloat()", 0, argCount);
	double value = pcg32_random_double();
	RETURN_NUMBER(value);
}

NATIVE_METHOD(Random, nextInt) {
	assertArgCount("Random::nextInt()", 0, argCount);
	uint32_t value = pcg32_random_int();
	RETURN_INT((int)value);
}

NATIVE_METHOD(Random, nextIntBounded) {
	assertArgCount("Random::nextIntBounded(bound)", 1, argCount);
	assertArgIsInt("Random::nextIntBounded(bound)", args, 0);
	assertNumberNonNegative("Random::nextIntBounded(bound)", AS_NUMBER(args[0]), 0);
	uint32_t value = pcg32_random_int_bounded((uint32_t)AS_INT(args[0]));
	RETURN_INT((int)value);
}

NATIVE_METHOD(Random, setSeed) {
	assertArgCount("Random::setSeed(seed)", 1, argCount);
	assertArgIsInt("Random::setSeed(seed)", args, 0);
	assertNumberNonNegative("Random::setSeed(seed)", AS_NUMBER(args[0]), 0);
	pcg32_seed((uint64_t)AS_INT(args[0]));
	setObjProperty(AS_INSTANCE(receiver), "seed", args[0]);
	RETURN_NIL;
}

// DATETIME 

NATIVE_METHOD(DateTime, __init__) {
	assertArgCount("DateTime::__init__(day, month, year, hour, minute, second)", 6, argCount);
	assertArgIsInt("DateTime::__init__(day, month, year, hour, minute, second)", args, 0);
	assertArgIsInt("DateTime::__init__(day, month, year, hour, minute, second)", args, 1);
	assertArgIsInt("DateTime::__init__(day, month, year, hour, minute, second)", args, 2);
	assertArgIsInt("DateTime::__init__(day, month, year, hour, minute, second)", args, 3);
	assertArgIsInt("DateTime::__init__(day, month, year, hour, minute, second)", args, 4);
	assertArgIsInt("DateTime::__init__(day, month, year, hour, minute, second)", args, 5);

	ObjInstance* self = AS_INSTANCE(receiver);
	setObjProperty(self, "day", args[0]);
	setObjProperty(self, "month", args[1]);
	setObjProperty(self, "year", args[2]);
	setObjProperty(self, "hour", args[3]);
	setObjProperty(self, "minute", args[4]);
	setObjProperty(self, "second", args[5]);
	return receiver;
}


NATIVE_METHOD(DateTime, minus) {
  assertArgCount("DateTime::minus(duration)", 1, argCount);
  assertObjInstanceOfClass("DateTime::minus(duration)", args[0], "Duration", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  double timestamp = dateTimeObjGetTimestamp(self) - durationTotalSeconds(AS_INSTANCE(args[0]));
  ObjInstance* dateTime = dateTimeObjFromTimestamp(self->obj.klass, timestamp);
  RETURN_OBJ(dateTime);
}

NATIVE_METHOD(DateTime, plus) {
  assertArgCount("DateTime::plus(duration)", 1, argCount);
  assertObjInstanceOfClass("DateTime::plus(duration)", args[0], "Duration", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  double timestamp = dateTimeObjGetTimestamp(self) + durationTotalSeconds(AS_INSTANCE(args[0]));
  ObjInstance* dateTime = dateTimeObjFromTimestamp(self->obj.klass, timestamp);
  RETURN_OBJ(dateTime);
}

NATIVE_METHOD(DateTime, after) {
	assertArgCount("DateTime::after(date)", 1, argCount);
	assertObjInstanceOfClass("DateTime::after(date)", args[0], "DateTime", 0);
	double timestamp = dateTimeObjGetTimestamp(AS_INSTANCE(receiver));
	double timestamp2 = dateTimeObjGetTimestamp(AS_INSTANCE(args[0]));
	RETURN_BOOL(timestamp > timestamp2);
}

NATIVE_METHOD(DateTime, before) {
	assertArgCount("DateTime::before(date)", 1, argCount);
	assertObjInstanceOfClass("DateTime::before(date)", args[0], "DateTime", 0);
	double timestamp = dateTimeObjGetTimestamp(AS_INSTANCE(receiver));
	double timestamp2 = dateTimeObjGetTimestamp(AS_INSTANCE(args[0]));
	RETURN_BOOL(timestamp < timestamp2);
}

NATIVE_METHOD(DateTime, diff) {
	assertArgCount("DateTime::diff(date)", 1, argCount);
	assertObjInstanceOfClass("DateTime::diff(date)", args[0], "DateTime", 0);
	double timestamp = dateTimeObjGetTimestamp(AS_INSTANCE(receiver));
	double timestamp2 = dateTimeObjGetTimestamp(AS_INSTANCE(args[0]));
	RETURN_NUMBER(timestamp - timestamp2);
}

NATIVE_METHOD(DateTime, getTimestamp) {
	assertArgCount("DateTime::getTimestamp()", 0, argCount);
	RETURN_NUMBER(dateTimeObjGetTimestamp(AS_INSTANCE(receiver)));
}

NATIVE_METHOD(DateTime, toDate) {
	assertArgCount("DateTime::toDate()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	ObjInstance* date = newInstance(getNativeClass("Date"));
  push(OBJ_VAL(date));
	setObjProperty(date, "day", getObjProperty(self, "day"));
	setObjProperty(date, "month", getObjProperty(self, "month"));
	setObjProperty(date, "year", getObjProperty(self, "year"));
  pop();
	RETURN_OBJ(date);
}

NATIVE_METHOD(DateTime, toString) {
	assertArgCount("DateTime::toString()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	Value year = getObjProperty(self, "year");
	Value month = getObjProperty(self, "month");
	Value day = getObjProperty(self, "day");
	Value hour = getObjProperty(self, "hour");
	Value minute = getObjProperty(self, "minute");
	Value second = getObjProperty(self, "second");
	RETURN_STRING_FMT("%02d/%02d/%d %02d:%02d:%02d", AS_INT(day), AS_INT(month), AS_INT(year), AS_INT(hour), AS_INT(minute), AS_INT(second));
}

// DURATION 

NATIVE_METHOD(Duration, __init__) {
	assertArgCount("Duration::__init__(days, hours, minutes, seconds)", 4, argCount);
	assertArgIsInt("Duration::__init__(days, hours, minutes, seconds)", args, 0);
	assertArgIsInt("Duration::__init__(days, hours, minutes, seconds)", args, 1);
	assertArgIsInt("Duration::__init__(days, hours, minutes, seconds)", args, 2);
	assertArgIsInt("Duration::__init__(days, hours, minutes, seconds)", args, 3);

	assertNumberNonNegative("Duration::__init__(days, hours, minutes, seconds)", AS_NUMBER(args[0]), 0);
	assertNumberNonNegative("Duration::__init__(days, hours, minutes, seconds)", AS_NUMBER(args[1]), 1);
	assertNumberNonNegative("Duration::__init__(days, hours, minutes, seconds)", AS_NUMBER(args[2]), 2);
	assertNumberNonNegative("Duration::__init__(days, hours, minutes, seconds)", AS_NUMBER(args[3]), 3);

	ObjInstance* self = AS_INSTANCE(receiver);
	int duration[4];
	duration__init__(duration, args);
	setObjProperty(self, "days", INT_VAL(duration[0]));
	setObjProperty(self, "hours", INT_VAL(duration[1]));
	setObjProperty(self, "minutes", INT_VAL(duration[2]));
	setObjProperty(self, "seconds", INT_VAL(duration[3]));
	return receiver;
}

NATIVE_METHOD(Duration, getTotalSeconds) {
	assertArgCount("Duration::getTotalSeconds()", 0, argCount);
	RETURN_NUMBER(durationTotalSeconds(AS_INSTANCE(receiver)));
}

NATIVE_METHOD(Duration, toString) {
	assertArgCount("Duration::toString()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	Value days = getObjProperty(self, "days");
	Value hours = getObjProperty(self, "hours");
	Value minutes = getObjProperty(self, "minutes");
	Value seconds = getObjProperty(self, "seconds");
	RETURN_STRING_FMT("%d days, %02d hours, %02d minutes, %02d seconds", AS_INT(days), AS_INT(hours), AS_INT(minutes), AS_INT(seconds));
}

// REGEX 

NATIVE_METHOD(Regex, __init__) {
	assertArgCount("Regex::__init__(pattern)", 1, argCount);
	assertArgIsString("Regex::__init__(pattern)", args, 0);
	ObjInstance* self = AS_INSTANCE(receiver);
	setObjProperty(self, "pattern", args[0]);
	RETURN_OBJ(self);
}

NATIVE_METHOD(Regex, match) {
  assertArgCount("Regex::match(string)", 1, argCount);
  assertArgIsString("Regex::match(string)", args, 0);
  Value pattern = getObjProperty(AS_INSTANCE(receiver), "pattern");
  int length;
  regex_t regex;
  regmatch_t matches[MAX_MATCHES];
  int index;
  const char *text = AS_CSTRING(args[0]);

  int reti = regcomp(&regex, AS_CSTRING(pattern), REG_EXTENDED);
  if (reti) {
    regfree(&regex);
    RETURN_FALSE;
  }

  reti = regexec(&regex, text, MAX_MATCHES, matches, 0);
  if (!reti) {
    index = matches[0].rm_so;
  } else {
    index = -1;
  }

  regfree(&regex);

  RETURN_BOOL(index != -1);
}

NATIVE_METHOD(Regex, replace) {
  assertArgCount("Regex::replace(original, replacement)", 2, argCount);
  assertArgIsString("Regex::replace(original, replacement)", args, 0);
  assertArgIsString("Regex::replace(original, replacement)", args, 1);
  Value pattern = getObjProperty(AS_INSTANCE(receiver), "pattern");
  ObjString* original = AS_STRING(args[0]);
  ObjString* replacement = AS_STRING(args[1]);
  regex_t regex;
  regmatch_t matches[MAX_MATCHES];
  const char *pattern_str = AS_CSTRING(pattern);
  const char *original_str = original->chars;
  char *result = NULL;
  int new_length = 0;

  int reti = regcomp(&regex, pattern_str, REG_EXTENDED);
  if (reti) {
    regfree(&regex);
    RETURN_OBJ(original);
  }

  int offset = 0;

  char *accumulator = strdup("");

  // Iterate over all matches and replace each occurrence
  while (regexec(&regex, original_str + offset, MAX_MATCHES, matches, 0) == 0) {
    int start = matches[0].rm_so;
    int end = matches[0].rm_eo;
    int replacement_length = strlen(replacement->chars);

    new_length += start + replacement_length;

    char *prefix = substr(original_str, offset, offset + start);
    accumulator = concat(accumulator, prefix);
    free(prefix);

    accumulator = concat(accumulator, replacement->chars);

    offset += end;
  }

  accumulator = concat(accumulator, original_str + offset);

  result = strdup(accumulator);
  new_length = strlen(result);

  free(accumulator);

  regfree(&regex);

  RETURN_OBJ(takeString(result, new_length));
}

NATIVE_METHOD(Regex, toString) {
	assertArgCount("Regex::toString()", 0, argCount);
	Value pattern = getObjProperty(AS_INSTANCE(receiver), "pattern");
	return pattern;
}

void registerUtilPackage() {
	ObjClass* randomClass = defineNativeClass("Random");
	bindSuperclass(randomClass, vm.objectClass);
	DEF_METHOD(randomClass, Random, getSeed, 0);
	DEF_METHOD(randomClass, Random, __init__, 0);
	DEF_METHOD(randomClass, Random, nextBool, 0);
	DEF_METHOD(randomClass, Random, nextFloat, 0);
	DEF_METHOD(randomClass, Random, nextInt, 0);
	DEF_METHOD(randomClass, Random, nextIntBounded, 1);
	DEF_METHOD(randomClass, Random, setSeed, 1);

	ObjClass* dateClass = defineNativeClass("Date");
	bindSuperclass(dateClass, vm.objectClass);
	DEF_METHOD(dateClass, Date, __init__, 3);
	DEF_METHOD(dateClass, Date, after, 1);
	DEF_METHOD(dateClass, Date, before, 1);
	DEF_METHOD(dateClass, Date, diff, 1);
	DEF_METHOD(dateClass, Date, getTimestamp, 0);
	DEF_METHOD(dateClass, Date, minus, 1);
	DEF_METHOD(dateClass, Date, plus, 1);
	DEF_METHOD(dateClass, Date, toDateTime, 0);
	DEF_METHOD(dateClass, Date, toString, 0);

	ObjClass* dateTimeClass = defineNativeClass("DateTime");
	bindSuperclass(dateTimeClass, dateClass);
	DEF_METHOD(dateTimeClass, DateTime, __init__, 6);
	DEF_METHOD(dateTimeClass, DateTime, after, 1);
	DEF_METHOD(dateTimeClass, DateTime, before, 1);
	DEF_METHOD(dateTimeClass, DateTime, diff, 1);
	DEF_METHOD(dateTimeClass, DateTime, getTimestamp, 0);
	DEF_METHOD(dateTimeClass, DateTime, minus, 1);
	DEF_METHOD(dateTimeClass, DateTime, plus, 1);
	DEF_METHOD(dateTimeClass, DateTime, toDate, 0);
	DEF_METHOD(dateTimeClass, DateTime, toString, 0);

	ObjClass* durationClass = defineNativeClass("Duration");
	bindSuperclass(durationClass, vm.objectClass);
  DEF_METHOD(durationClass, Duration, __init__, 4);
	DEF_METHOD(durationClass, Duration, getTotalSeconds, 0);
	DEF_METHOD(durationClass, Duration, toString, 0);


	ObjClass* regexClass = defineNativeClass("Regex");
	bindSuperclass(regexClass, vm.objectClass);
	DEF_METHOD(regexClass, Regex, __init__, 1);
	DEF_METHOD(regexClass, Regex, match, 1);
	DEF_METHOD(regexClass, Regex, replace, 2);
	DEF_METHOD(regexClass, Regex, toString, 0);
}
