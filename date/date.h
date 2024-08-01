#ifndef cluminique_date_h
#define cluminique_date_h

#include "../common.h"
#include "../value/value.h"

double dateObjGetTimestamp(ObjInstance* date);
ObjInstance* dateObjNow(ObjClass* klass);
double dateTimeObjGetTimestamp(ObjInstance* dateTime);
ObjInstance* dateObjFromTimestamp(ObjClass* dateClass, double timeValue);
ObjInstance* dateTimeObjFromTimestamp(ObjClass* dateTimeClass, double timeValue);
ObjInstance* dateTimeObjNow(ObjClass* klass);
void durationFromSeconds(int* duration, double seconds);
void durationFromArgs(int* duration, Value* args);
void durationInit(int* duration, Value* args);
double durationTotalSeconds(ObjInstance* duration);

#endif
