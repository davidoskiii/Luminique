#include <stdlib.h>
#include <string.h>

#include "../memory/memory.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../value/value.h"
#include "table.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
  uint32_t index = key->hash & (capacity - 1);
  Entry* tombstone = NULL;

  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // We found a tombstone.
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      // We found the key.
      return entry;
    }

    index = (index + 1) & (capacity - 1);
  }
}

static void adjustCapacity(Table* table, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

    Entry* dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  if (table->count == 0) return false;

  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}

bool tableSet(Table* table, ObjString* key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }

  Entry* entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  if (isNewKey && IS_NIL(entry->value)) table->count++;

  entry->key = key;
  entry->value = value;
  return isNewKey;
}


bool tableDelete(Table* table, ObjString* key) {
  if (table->count == 0) return false;

  // Find the entry.
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  // Place a tombstone in the entry.
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
  if (table->count == 0) return NULL;

  uint32_t index = hash & (table->capacity - 1);
  for (;;) {
    Entry* entry = &table->entries[index];
    if (entry->key == NULL) {
      // Stop if we find an empty non-tombstone entry.
      if (IS_NIL(entry->value)) return NULL;
    } else if (entry->key->length == length &&
        entry->key->hash == hash &&
        memcmp(entry->key->chars, chars, length) == 0) {
      // We found it.
      return entry->key;
    }

    index = (index + 1) & (table->capacity - 1);
  }
}

void tableRemoveWhite(Table* table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->obj.isMarked) {
      tableDelete(table, entry->key);
    }
  }
}

void markTable(Table* table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    markObject((Obj*)entry->key);
    markValue(entry->value);
  }
}

bool tableContainsKey(Table* table, ObjString* key) {
  if (table->count == 0) return false;
  Entry* entry = findEntry(table->entries, table->capacity, key);
  return entry->key != NULL;
}

bool tableContainsValue(Table* table, Value value) {
  if (table->count == 0) return false;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;
    if (valuesEqual(entry->value, value)) return true;
  }
  return false;
}


ObjString* tableToString(Table* table) {
  if (table->count == 0) return copyString("{}", 2);
  else {
    char string[UINT8_MAX] = "";
    string[0] = '{';
    size_t offset = 1;
    int startIndex = 0;

    for (int i = 0; i < table->capacity; i++) {
      Entry* entry = &table->entries[i];
      if (entry->key == NULL) continue;
      if (startIndex == 0) startIndex = i;

      ObjString* key = entry->key;
      size_t keyLength = (size_t)key->length;
      Value value = entry->value;
      char* valueChars = valueToString(value);
      size_t valueLength = strlen(valueChars);

      if (i > startIndex) {
        memcpy(string + offset, ", ", 2);
        offset += 2;
      }

      string[offset] = '\"';
      offset++;
     
      memcpy(string + offset, key->chars, keyLength);

      offset += keyLength;
      string[offset] = '\"';
      offset++;

      memcpy(string + offset, ": ", 2);
      offset += 2;

      if (IS_STRING(value)) {
        string[offset] = '\"';
        offset++;
      }
      memcpy(string + offset, valueChars, valueLength);
      offset += valueLength;
      if (IS_STRING(value)) {
        string[offset] = '\"';
        offset++;
      }
    }

    string[offset] = '}';
    string[offset + 1] = '\0';
    return copyString(string, (int)offset + 1);
  }
}

