#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "time.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../loop/loop.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../value/value.h"
#include "../vm/vm.h"

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

static void durationInit(int* duration, Value* args) {
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

NATIVE_METHOD(Date, __equal__) { 
  assertArgCount("Date::==(date)", 1, argCount);
	assertObjInstanceOfClass("Date::==(date)", args[0], "luminique::std::chrono", "Date", 0);
  double timestamp = dateObjGetTimestamp(AS_INSTANCE(receiver));
  double timestamp2 = dateObjGetTimestamp(AS_INSTANCE(args[0]));
  RETURN_BOOL(timestamp == timestamp2);
}

NATIVE_METHOD(Date, __greater__) { 
  assertArgCount("Date::>(date)", 1, argCount);
	assertObjInstanceOfClass("Date::>(date)", args[0], "luminique::std::chrono", "Date", 0);
  double timestamp = dateObjGetTimestamp(AS_INSTANCE(receiver));
  double timestamp2 = dateObjGetTimestamp(AS_INSTANCE(args[0]));
  RETURN_BOOL(timestamp > timestamp2);
}

NATIVE_METHOD(Date, __less__) {
  assertArgCount("Date::<(date)", 1, argCount);
	assertObjInstanceOfClass("Date::<(date)", args[0], "luminique::std::chrono", "Date", 0);
  double timestamp = dateObjGetTimestamp(AS_INSTANCE(receiver));
  double timestamp2 = dateObjGetTimestamp(AS_INSTANCE(args[0]));
  RETURN_BOOL(timestamp < timestamp2);
}

NATIVE_METHOD(Date, __add__) {
  assertArgCount("Date::+(duration)", 1, argCount);
	assertObjInstanceOfClass("Date::+(duration)", args[0], "luminique::std::chrono", "Duration", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  double timestamp = dateObjGetTimestamp(self) + durationTotalSeconds(AS_INSTANCE(args[0]));
  ObjInstance* date = dateObjFromTimestamp(self->obj.klass, timestamp);
  RETURN_OBJ(date);
}

NATIVE_METHOD(Date, __subtract__) {
  assertArgCount("Date::-(duration)", 1, argCount);
	assertObjInstanceOfClass("Date::-(duration)", args[0], "luminique::std::chrono", "Duration", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  double timestamp = dateObjGetTimestamp(self) - durationTotalSeconds(AS_INSTANCE(args[0]));
  ObjInstance* date = dateObjFromTimestamp(self->obj.klass, timestamp);
  RETURN_OBJ(date);
}

NATIVE_METHOD(Date, __str__) {
	assertArgCount("Date::__str__()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	Value year = getObjProperty(self, "year");
	Value month = getObjProperty(self, "month");
	Value day = getObjProperty(self, "day");
	RETURN_STRING_FMT("%02d/%02d/%d", AS_INT(day), AS_INT(month), AS_INT(year));
}

NATIVE_METHOD(Date, __format__) {
	assertArgCount("Date::__format__()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	Value year = getObjProperty(self, "year");
	Value month = getObjProperty(self, "month");
	Value day = getObjProperty(self, "day");
	RETURN_STRING_FMT("%02d/%02d/%d", AS_INT(day), AS_INT(month), AS_INT(year));
}

NATIVE_METHOD(Date, toDateTime) {
	assertArgCount("Date::toDateTime()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	ObjInstance* dateTime = newInstance(getNativeClass("luminique::std::chrono", "DateTime"));
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

NATIVE_METHOD(Date, diff) {
	assertArgCount("Date::diff(date)", 1, argCount);
	assertObjInstanceOfClass("Date::diff(date)", args[0], "luminique::std::chrono", "Date", 0);
	double timestamp = dateObjGetTimestamp(AS_INSTANCE(receiver));
	double timestamp2 = dateObjGetTimestamp(AS_INSTANCE(args[0]));
	RETURN_NUMBER(timestamp - timestamp2);
}

NATIVE_METHOD(Date, getTimestamp) {
	assertArgCount("Date::getTimestamp()", 0, argCount);
	RETURN_NUMBER(dateObjGetTimestamp(AS_INSTANCE(receiver)));
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

NATIVE_METHOD(DateTime, __equal__) {
  assertArgCount("DateTime::==(dateTime)", 1, argCount);
	assertObjInstanceOfClass("DateTime::==(dateTime)", args[0], "luminique::std::chrono", "DateTime", 0);
  double timestamp = dateTimeObjGetTimestamp(AS_INSTANCE(receiver));
  double timestamp2 = dateTimeObjGetTimestamp(AS_INSTANCE(args[0]));
  RETURN_BOOL(timestamp == timestamp2);
}

NATIVE_METHOD(DateTime, __greater__) {
  assertArgCount("DateTime::>(dateTime)", 1, argCount);
	assertObjInstanceOfClass("DateTime::>(dateTime)", args[0], "luminique::std::chrono", "DateTime", 0);
  double timestamp = dateTimeObjGetTimestamp(AS_INSTANCE(receiver));
  double timestamp2 = dateTimeObjGetTimestamp(AS_INSTANCE(args[0]));
  RETURN_BOOL(timestamp > timestamp2);
}

NATIVE_METHOD(DateTime, __less__) {
  assertArgCount("DateTime::<(dateTime)", 1, argCount);
	assertObjInstanceOfClass("DateTime::<(dateTime)", args[0], "luminique::std::chrono", "DateTime", 0);
  double timestamp = dateTimeObjGetTimestamp(AS_INSTANCE(receiver));
  double timestamp2 = dateTimeObjGetTimestamp(AS_INSTANCE(args[0]));
  RETURN_BOOL(timestamp < timestamp2);
}

NATIVE_METHOD(DateTime, __add__) {
  assertArgCount("DateTime::+(duration)", 1, argCount);
	assertObjInstanceOfClass("DateTime::+(duration)", args[0], "luminique::std::chrono", "Duration", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  double timestamp = dateTimeObjGetTimestamp(self) + durationTotalSeconds(AS_INSTANCE(args[0]));
  ObjInstance* dateTime = dateTimeObjFromTimestamp(self->obj.klass, timestamp);
  RETURN_OBJ(dateTime);
}

NATIVE_METHOD(DateTime, __subtract__) {
  assertArgCount("DateTime::-(duration)", 1, argCount);
	assertObjInstanceOfClass("DateTime::-(duration)", args[0], "luminique::std::chrono", "Duration", 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  double timestamp = dateTimeObjGetTimestamp(self) - durationTotalSeconds(AS_INSTANCE(args[0]));
  ObjInstance* dateTime = dateTimeObjFromTimestamp(self->obj.klass, timestamp);
  RETURN_OBJ(dateTime);
}

NATIVE_METHOD(DateTime, diff) {
	assertArgCount("DateTime::diff(date)", 1, argCount);
	assertObjInstanceOfClass("DateTime::diff(date)", args[0], "luminique::std::chrono", "DateTime", 0);
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
	ObjInstance* date = newInstance(getNativeClass("luminique::std::chrono", "Date"));
  push(OBJ_VAL(date));
	setObjProperty(date, "day", getObjProperty(self, "day"));
	setObjProperty(date, "month", getObjProperty(self, "month"));
	setObjProperty(date, "year", getObjProperty(self, "year"));
  pop();
	RETURN_OBJ(date);
}

NATIVE_METHOD(DateTime, __str__) {
	assertArgCount("DateTime::__str__()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	Value year = getObjProperty(self, "year");
	Value month = getObjProperty(self, "month");
	Value day = getObjProperty(self, "day");
	Value hour = getObjProperty(self, "hour");
	Value minute = getObjProperty(self, "minute");
	Value second = getObjProperty(self, "second");
	RETURN_STRING_FMT("%02d/%02d/%d %02d:%02d:%02d", AS_INT(day), AS_INT(month), AS_INT(year), AS_INT(hour), AS_INT(minute), AS_INT(second));
}

NATIVE_METHOD(DateTime, __format__) {
	assertArgCount("DateTime::__format__()", 0, argCount);
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
	durationInit(duration, args);
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

NATIVE_METHOD(Duration, __str__) {
	assertArgCount("Duration::__str__()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	Value days = getObjProperty(self, "days");
	Value hours = getObjProperty(self, "hours");
	Value minutes = getObjProperty(self, "minutes");
	Value seconds = getObjProperty(self, "seconds");
	RETURN_STRING_FMT("%d days, %02d hours, %02d minutes, %02d seconds", AS_INT(days), AS_INT(hours), AS_INT(minutes), AS_INT(seconds));
}

NATIVE_METHOD(Duration, __format__) {
	assertArgCount("Duration::__format__()", 0, argCount);
	ObjInstance* self = AS_INSTANCE(receiver);
	Value days = getObjProperty(self, "days");
	Value hours = getObjProperty(self, "hours");
	Value minutes = getObjProperty(self, "minutes");
	Value seconds = getObjProperty(self, "seconds");
	RETURN_STRING_FMT("%d days, %02d hours, %02d minutes, %02d seconds", AS_INT(days), AS_INT(hours), AS_INT(minutes), AS_INT(seconds));
}

NATIVE_METHOD(Timer, __init__) {
  assertArgCount("Timer::__init__(closure, delay, interval)", 3, argCount);
  assertArgIsClosure("Timer::__init__(closure, delay, interval)", args, 0);
  assertArgIsInt("Timer::__init__(closure, delay, interval)", args, 1);
  assertArgIsInt("Timer::__init__(closure, delay, interval)", args, 2);
  ObjTimer* self = AS_TIMER(receiver);
  TimerData* data = (TimerData*)self->timer->data;
  data->receiver = receiver;
  data->closure = AS_CLOSURE(args[0]);
  data->delay = AS_INT(args[1]);
  data->interval = AS_INT(args[2]);
  RETURN_OBJ(self);
}

NATIVE_METHOD(Timer, clear) {
  assertArgCount("Timer::clear()", 0, argCount);
  ObjTimer* self = AS_TIMER(receiver);
  uv_timer_stop(self->timer);
  RETURN_NIL;
}

NATIVE_METHOD(Timer, info) {
  assertArgCount("Timer::info()", 0, argCount);
  RETURN_STRING_FMT("<timer: %d>", AS_TIMER(receiver)->id);
}

NATIVE_METHOD(Timer, isRunning) {
  assertArgCount("Timer::isRunning()", 0, argCount);
  RETURN_BOOL(AS_TIMER(receiver)->isRunning);
}

NATIVE_METHOD(Timer, run) {
  assertArgCount("Timer::run()", 0, argCount);
  ObjTimer* self = AS_TIMER(receiver);
  if (self->isRunning) THROW_EXCEPTION_FMT(luminique::std::lang, UnsupportedOperationException, "Timer ID: %d is already running...", self->id);
  else {
    TimerData* data = (TimerData*)self->timer->data;
    uv_timer_init(vm.eventLoop, self->timer);
    uv_timer_start(self->timer, timerRun, data->delay, data->interval);
    self->id = (int)self->timer->start_id;
    RETURN_OBJ(self);
  }
}

NATIVE_METHOD(Timer, __str__) {
  assertArgCount("Timer::__str__()", 0, argCount);
  ObjTimer* self = AS_TIMER(receiver);
  TimerData* data = (TimerData*)self->timer->data;
  if (data->delay != 0 && data->interval == 0) RETURN_STRING_FMT("Timer: delay after %dms", data->delay);
  if (data->delay == 0 && data->interval != 0) RETURN_STRING_FMT("Timer: interval at %dms", data->interval);
  RETURN_STRING_FMT("Timer: delay after %dms, interval at %dms", data->delay, data->interval);
}

NATIVE_METHOD(Timer, __format__) {
  assertArgCount("Timer::__format__()", 0, argCount);
  ObjTimer* self = AS_TIMER(receiver);
  TimerData* data = (TimerData*)self->timer->data;
  if (data->delay != 0 && data->interval == 0) RETURN_STRING_FMT("Timer: delay after %dms", data->delay);
  if (data->delay == 0 && data->interval != 0) RETURN_STRING_FMT("Timer: interval at %dms", data->interval);
  RETURN_STRING_FMT("Timer: delay after %dms, interval at %dms", data->delay, data->interval);
}

NATIVE_METHOD(TimerClass, interval) {
  assertArgCount("Timer class::interval(closure, interval)", 2, argCount);
  assertArgIsClosure("Timer class::interval(closure, interval)", args, 0);
  assertArgIsInt("Timer class::interval(closure, interval)", args, 1);

  ObjClass* self = AS_CLASS(receiver);
  ObjTimer* timer = newTimer(AS_CLOSURE(args[0]), 0, AS_INT(args[1]));
  TimerData* data = (TimerData*)timer->timer->data;
  timer->obj.klass = self;
  data->receiver = OBJ_VAL(timer);
  uv_timer_init(vm.eventLoop, timer->timer);
  uv_timer_start(timer->timer, timerRun, 0, (uint64_t)data->interval);
  timer->id = (int)timer->timer->start_id;
  RETURN_OBJ(timer);
}

NATIVE_METHOD(TimerClass, timeout) {
  assertArgCount("Timer class::timeout(closure, delay)", 2, argCount);
  assertArgIsClosure("Timer class::timeout(closure, delay)", args, 0);
  assertArgIsInt("Timer class::timeout(closure, delay)", args, 1);

  ObjClass* self = AS_CLASS(receiver);
  ObjTimer* timer = newTimer(AS_CLOSURE(args[0]), AS_INT(args[1]), 0);
  TimerData* data = (TimerData*)timer->timer->data;
  timer->obj.klass = self;
  data->receiver = OBJ_VAL(timer);
  uv_timer_init(vm.eventLoop, timer->timer);
  uv_timer_start(timer->timer, timerRun, (uint64_t)data->delay, 0);
  timer->id = (int)timer->timer->start_id;
  RETURN_OBJ(timer);
}

NATIVE_FUNCTION(sleep) {
  assertArgCount("sleep(seconds)", 1, argCount);
  assertArgIsNumber("sleep(seconds)", args, 0);
  sleep(AS_NUMBER(args[0]));
  RETURN_NIL;
}

NATIVE_FUNCTION(timeNow) {
  assertArgCount("timeNow()", 0, argCount);
  RETURN_NUMBER((double)clock() / CLOCKS_PER_SEC);
}

NATIVE_FUNCTION(dateNow) {
  assertArgCount("dateNow()", 0, argCount);
  time_t nowTime;
  time(&nowTime);
  struct tm now;
  localtime_r(&nowTime, &now);
  ObjInstance* date = newInstance(getNativeClass("luminique::std::chrono", "Date"));
  setObjProperty(date, "year", INT_VAL(1900 + now.tm_year));
  setObjProperty(date, "month", INT_VAL(1 + now.tm_mon));
  setObjProperty(date, "day", INT_VAL(now.tm_mday));
  RETURN_OBJ(date);
}

NATIVE_FUNCTION(dateTimeNow) {
  assertArgCount("dateTimeNow()", 0, argCount);
  time_t nowTime;
  time(&nowTime);
  struct tm now;
  localtime_r(&nowTime, &now);
  ObjInstance* date = newInstance(getNativeClass("luminique::std::chrono", "DateTime"));
  setObjProperty(date, "year", INT_VAL(1900 + now.tm_year));
  setObjProperty(date, "month", INT_VAL(1 + now.tm_mon));
  setObjProperty(date, "day", INT_VAL(now.tm_mday));
  setObjProperty(date, "hour", INT_VAL(now.tm_hour));
  setObjProperty(date, "minute", INT_VAL(now.tm_min));
  setObjProperty(date, "second", INT_VAL(now.tm_sec));
  RETURN_OBJ(date);
}

void registerTimePackage() {
  ObjNamespace* timeNamespace = defineNativeNamespace("chrono", vm.stdNamespace);
  vm.currentNamespace = timeNamespace;

  DEF_FUNCTION(sleep, 0);
  DEF_FUNCTION(timeNow, 0);
  DEF_FUNCTION(dateNow, 0);
  DEF_FUNCTION(dateTimeNow, 0);


	ObjClass* dateClass = defineNativeClass("Date");
	bindSuperclass(dateClass, vm.objectClass);
	DEF_METHOD(dateClass, Date, __init__, 3);
	DEF_METHOD(dateClass, Date, diff, 1);
	DEF_METHOD(dateClass, Date, getTimestamp, 0);
	DEF_METHOD(dateClass, Date, toDateTime, 0);
	DEF_METHOD(dateClass, Date, __str__, 0);
	DEF_METHOD(dateClass, Date, __format__, 0);
  DEF_OPERATOR(dateClass, Date, ==, __equal__, 1);
  DEF_OPERATOR(dateClass, Date, >, __greater__, 1);
  DEF_OPERATOR(dateClass, Date, <, __less__, 1);
  DEF_OPERATOR(dateClass, Date, +, __add__, 1);
  DEF_OPERATOR(dateClass, Date, -, __subtract__, 1);

	ObjClass* dateTimeClass = defineNativeClass("DateTime");
	bindSuperclass(dateTimeClass, dateClass);
	DEF_METHOD(dateTimeClass, DateTime, __init__, 6);
	DEF_METHOD(dateTimeClass, DateTime, diff, 1);
	DEF_METHOD(dateTimeClass, DateTime, getTimestamp, 0);
	DEF_METHOD(dateTimeClass, DateTime, toDate, 0);
	DEF_METHOD(dateTimeClass, DateTime, __str__, 0);
	DEF_METHOD(dateTimeClass, DateTime, __format__, 0);
  DEF_OPERATOR(dateTimeClass, DateTime, ==, __equal__, 1);
  DEF_OPERATOR(dateTimeClass, DateTime, >, __greater__, 1);
  DEF_OPERATOR(dateTimeClass, DateTime, <, __less__, 1);
  DEF_OPERATOR(dateTimeClass, DateTime, +, __add__, 1);
  DEF_OPERATOR(dateTimeClass, DateTime, -, __subtract__, 1);

	ObjClass* durationClass = defineNativeClass("Duration");
	bindSuperclass(durationClass, vm.objectClass);
  DEF_METHOD(durationClass, Duration, __init__, 4);
	DEF_METHOD(durationClass, Duration, getTotalSeconds, 0);
	DEF_METHOD(durationClass, Duration, __str__, 0);
	DEF_METHOD(durationClass, Duration, __format__, 0);

  vm.timerClass = defineNativeClass("Timer");
  bindSuperclass(vm.timerClass, vm.objectClass);
  vm.timerClass->classType = OBJ_TIMER;
  DEF_INTERCEPTOR(vm.timerClass, Timer, INTERCEPTOR_INIT, __init__, 3);
  DEF_METHOD(vm.timerClass, Timer, clear, 0);
  DEF_METHOD(vm.timerClass, Timer, isRunning, 0);
  DEF_METHOD(vm.timerClass, Timer, run, 0);
  DEF_METHOD(vm.timerClass, Timer, info, 0);
  DEF_METHOD(vm.timerClass, Timer, __str__, 0);
  DEF_METHOD(vm.timerClass, Timer, __format__, 0);

  ObjClass* timerMetaclass = vm.timerClass->obj.klass;
  DEF_METHOD(timerMetaclass, TimerClass, interval, 2);
  DEF_METHOD(timerMetaclass, TimerClass, timeout, 2);

  vm.currentNamespace = vm.rootNamespace;
}
