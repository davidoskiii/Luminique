#ifndef cluminique_collection_h
#define cluminique_collection_h

#include "../common.h"
#include "../string/string.h"

void registerCollectionPackage();
bool dictGet(ObjDictionary* dict, Value key, Value* value);
bool dictSet(ObjDictionary* dict, Value key, Value value);
void arrayAddAll(ObjArray* from, ObjArray* to);

#endif
