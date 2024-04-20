#ifndef cluminique_table_h
#define cluminique_table_h

#include "../common.h"
#include "../value/value.h"


#define TABLE_MAX_LOAD 0.75

typedef struct {
  ObjString* key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableContainsKey(Table* table, ObjString* key);
bool tableContainsValue(Table* table, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
int tableFindIndex(Table* table, ObjString* key);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
void tableRemoveWhite(Table* table);
void markTable(Table* table);
ObjString* tableToString(Table* table);

#endif
