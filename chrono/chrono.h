#ifndef cluminique_time_h
#define cluminique_time_h

#include "../common.h"
#include "../string/string.h"

ObjInstance* dateTimeObjFromTimestamp(ObjClass* dateTimeClass, double timeValue);
void registerTimePackage();

#endif
