#ifndef cluminique_collection_h
#define cluminique_collection_h

#include "../common.h"
#include "../string/string.h"

void registerCollectionPackage();
bool dictGet(ObjDictionary* dict, Value key, Value* value);
bool dictSet(ObjDictionary* dict, Value key, Value value);
void arrayAddAll(ObjArray* from, ObjArray* to);
ObjString* arrayToString(ObjArray* array);
ObjString* dictToString(ObjDictionary* dict);
void valueArrayPut(ValueArray* array, int index, Value value);

#endif
