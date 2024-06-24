#include <stdio.h>
#include <stdlib.h>

#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../hash/hash.h"
#include "../value/value.h"
#include "../memory/memory.h"
#include "../vm/vm.h"

ObjEntry* dictFindEntry(ObjEntry* entries, int capacity, Value key) {
  uint32_t hash = hashValue(key);
  uint32_t index = hash & (capacity - 1);
  ObjEntry* tombstone = NULL;

  for (;;) {
    ObjEntry* entry = &entries[index];
    if (IS_UNDEFINED(entry->key)) {
      if (IS_NIL(entry->value)) {
        return tombstone != NULL ? tombstone : entry;
      }
      else {
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      return entry;
    }

    index = (index + 1) & (capacity - 1);
  }
}

static bool dictContainsKey(ObjDictionary* dict, Value key) {
  if (dict->count == 0) return false;
  ObjEntry* entry = dictFindEntry(dict->entries, dict->capacity, key);
  return !IS_UNDEFINED(entry->key);
}

static bool dictContainsValue(ObjDictionary* dict, Value value) {
  if (dict->count == 0) return false;
  for (int i = 0; i < dict->capacity; i++) {
    ObjEntry* entry = &dict->entries[i];
    if (IS_UNDEFINED(entry->key)) continue;
    if (entry->value == value) return true;
  }
  return false;
}

bool dictGet(ObjDictionary* dict, Value key, Value* value) {
  if (dict->count == 0) return false;
  ObjEntry* entry = dictFindEntry(dict->entries, dict->capacity, key);
  if (IS_UNDEFINED(entry->key)) return false;
  *value = entry->value;
  return true;
}

void dictAdjustCapacity(ObjDictionary* dict, int capacity) {
  ObjEntry* entries = ALLOCATE(ObjEntry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = UNDEFINED_VAL;
    entries[i].value = NIL_VAL;
  }

  dict->count = 0;
  for (int i = 0; i < dict->capacity; i++) {
    ObjEntry* entry = &dict->entries[i];
    if (IS_UNDEFINED(entry->key)) continue;

    ObjEntry* dest = dictFindEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    dict->count++;
  }

  FREE_ARRAY(ObjEntry, dict->entries, dict->capacity);
  dict->entries = entries;
  dict->capacity = capacity;
}

bool dictSet(ObjDictionary* dict, Value key, Value value) {
  if (dict->count + 1 > dict->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(dict->capacity);
    dictAdjustCapacity(dict, capacity);
  }

  ObjEntry* entry = dictFindEntry(dict->entries, dict->capacity, key);
  bool isNewKey = IS_UNDEFINED(entry->key);
  if (isNewKey && IS_NIL(entry->value)) dict->count++;

  entry->key = key;
  entry->value = value;
  return isNewKey;
}

static bool dictDelete(ObjDictionary* dict, Value key) {
  if (dict->count == 0) return false;

  ObjEntry* entry = dictFindEntry(dict->entries, dict->capacity, key);
  if (IS_UNDEFINED(entry->key)) return false;

  entry->key = UNDEFINED_VAL;
  entry->value = BOOL_VAL(true);
  return true;
}

static void dictAddAll(ObjDictionary* from, ObjDictionary* to) {
  for (int i = 0; i < from->capacity; i++) {
    ObjEntry* entry = &from->entries[i];
    if (!IS_UNDEFINED(entry->key)) {
      dictSet(to, entry->key, entry->value);
    }
  }
}

static ObjDictionary* dictCopy(ObjDictionary* original) {
  ObjDictionary* copied = newDictionary();
  push(OBJ_VAL(copied));
  dictAddAll(original, copied);
  pop();
  return copied;
}


static bool dictsEqual(ObjDictionary* aDict, ObjDictionary* dict2) {
  for (int i = 0; i < aDict->capacity; i++) {
    ObjEntry* entry = &aDict->entries[i];
    if (IS_UNDEFINED(entry->key)) continue;
    Value bValue;
    bool keyExists = dictGet(dict2, entry->key, &bValue);
    if (!keyExists || entry->value != bValue) return false;
  }

  for (int i = 0; i < dict2->capacity; i++) {
    ObjEntry* entry = &dict2->entries[i];
    if (IS_UNDEFINED(entry->key)) continue;
    Value aValue;
    bool keyExists = dictGet(aDict, entry->key, &aValue);
    if (!keyExists || entry->value != aValue) return false;
  }

  return true;
}

static int dictLength(ObjDictionary* dict) {
  if (dict->count == 0) return 0;
  int length = 0;
  for (int i = 0; i < dict->capacity; i++) {
    ObjEntry* entry = &dict->entries[i];
    if (!IS_UNDEFINED(entry->key)) length++;
  }
  return length;
}

static int dictFindIndex(ObjDictionary* dict, Value key) {
  uint32_t hash = hashValue(key);
  uint32_t index = hash & (dict->capacity - 1);
  ObjEntry* tombstone = NULL;

  for (;;) {
    ObjEntry* entry = &dict->entries[index];
    if (IS_UNDEFINED(entry->key)) {
      if (IS_NIL(entry->value)) {
        return -1;
      } else {
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      return index;
    }

    index = (index + 1) & (dict->capacity - 1);
  }
}

ObjString* dictToString(ObjDictionary* dict) {
  if (dict->count == 0) return copyString("{}", 2);
  else {
    char string[UINT8_MAX] = "";
    string[0] = '{';
    size_t offset = 1;
    int startIndex = 0;

    for (int i = 0; i < dict->capacity; i++) {
      ObjEntry* entry = &dict->entries[i];
      if (IS_UNDEFINED(entry->key)) continue;
      Value key = entry->key;
      char* keyChars;
      size_t keyLength;
      if (IS_STRING(key)) {
        keyChars = ALLOCATE(char, AS_STRING(key)->length + 3);
        keyChars[0] = '"';
        memcpy(keyChars + 1, AS_STRING(key)->chars, AS_STRING(key)->length);
        keyChars[AS_STRING(key)->length + 1] = '"';
        keyLength = AS_STRING(key)->length + 2;
      } else {
        keyChars = valueToString(key);
        keyLength = strlen(keyChars);
      }
      Value value = entry->value;
      char* valueChars;
      size_t valueLength;
      if (IS_STRING(value)) {
        valueChars = ALLOCATE(char, AS_STRING(value)->length + 3);
        valueChars[0] = '"';
        memcpy(valueChars + 1, AS_STRING(value)->chars, AS_STRING(value)->length);
        valueChars[AS_STRING(value)->length + 1] = '"';
        valueLength = AS_STRING(value)->length + 2;
      } else {
        valueChars = valueToString(value);
        valueLength = strlen(valueChars);
      }

      memcpy(string + offset, keyChars, keyLength);
      offset += keyLength;
      memcpy(string + offset, ": ", 2);
      offset += 2;
      memcpy(string + offset, valueChars, valueLength);
      offset += valueLength;
      startIndex = i + 1;
      break;
    }

    for (int i = startIndex; i < dict->capacity; i++) {
      ObjEntry* entry = &dict->entries[i];
      if (IS_UNDEFINED(entry->key)) continue;
      Value key = entry->key;
      char* keyChars;
      size_t keyLength;
      if (IS_STRING(key)) {
        keyChars = ALLOCATE(char, AS_STRING(key)->length + 3);
        keyChars[0] = '"';
        memcpy(keyChars + 1, AS_STRING(key)->chars, AS_STRING(key)->length);
        keyChars[AS_STRING(key)->length + 1] = '"';
        keyLength = AS_STRING(key)->length + 2;
      } else {
        keyChars = valueToString(key);
        keyLength = strlen(keyChars);
      }
      Value value = entry->value;
      char* valueChars;
      size_t valueLength;
      if (IS_STRING(value)) {
        valueChars = ALLOCATE(char, AS_STRING(value)->length + 3);
        valueChars[0] = '"';
        memcpy(valueChars + 1, AS_STRING(value)->chars, AS_STRING(value)->length);
        valueChars[AS_STRING(value)->length + 1] = '"';
        valueLength = AS_STRING(value)->length + 2;
      } else {
        valueChars = valueToString(value);
        valueLength = strlen(valueChars);
      }

      memcpy(string + offset, ", ", 2);
      offset += 2;
      memcpy(string + offset, keyChars, keyLength);
      offset += keyLength;
      memcpy(string + offset, ": ", 2);
      offset += 2;
      memcpy(string + offset, valueChars, valueLength);
      offset += valueLength;

      if (IS_STRING(key)) free(keyChars);
      if (IS_STRING(value)) free(valueChars);
    }

    string[offset] = '}';
    string[offset + 1] = '\0';
    return copyString(string, (int)offset + 1);
  }
}

static int arrayIndexOf(ObjArray* array, Value element) {
	for (int i = 0; i < array->elements.count; i++) {
		if (valuesEqual(array->elements.values[i], element)) {
			return i;
		}
	}
	return -1;
}

static void arrayAddAll(ObjArray* from, ObjArray* to) {
	if (from->elements.count == 0) return;
	for (int i = 0; i < from->elements.count; i++) {
		writeValueArray(&to->elements, from->elements.values[i]);
	}
}

static void arrayInsertAt(ObjArray* array, int index, Value element) {
	if (IS_OBJ(element)) push(element);
	writeValueArray(&array->elements, NIL_VAL);
	if (IS_OBJ(element)) pop();

	for (int i = array->elements.count - 1; i > index; i--) {
		array->elements.values[i] = array->elements.values[i - 1];
	}
	array->elements.values[index] = element;
}

static int arrayLastIndexOf(ObjArray* array, Value element) {
	for (int i = array->elements.count - 1; i >= 0; i--) {
		if (valuesEqual(array->elements.values[i], element)) {
			return i;
		}
	}
	return -1;
}

static Value arrayRemoveAt(ObjArray* array, int index) {
	Value element = array->elements.values[index];
	if (IS_OBJ(element)) push(element);

	for (int i = index; i < array->elements.count - 1; i++) {
		array->elements.values[i] = array->elements.values[i + 1];
	}
	array->elements.count--;

	if (IS_OBJ(element)) pop();
	return element;
}

static ObjString* arrayToString(ObjArray* array) {
	if (array->elements.count == 0) return copyString("[]", 2);
	else {
		char string[UINT8_MAX] = "";
		string[0] = '[';
		size_t offset = 1;
		for (int i = 0; i < array->elements.count; i++) {
			Value value = array->elements.values[i];
			char* chars;
			size_t length;
			if (IS_STRING(value)) {
				chars = ALLOCATE(char, AS_STRING(value)->length + 3);
				chars[0] = '"';
				memcpy(chars + 1, AS_STRING(value)->chars, AS_STRING(value)->length);
				chars[AS_STRING(value)->length + 1] = '"';
				length = AS_STRING(value)->length + 2;
			} else {
				chars = valueToString(value);
				length = strlen(chars);
			}
			memcpy(string + offset, chars, length);
			if (i == array->elements.count - 1) {
				offset += length;
			}
			else{
				memcpy(string + offset + length, ", ", 2);
				offset += length + 2;
			}
			if (IS_STRING(value)) free(chars);
		}
		string[offset] = ']';
		string[offset + 1] = '\0';
		return copyString(string, (int)offset + 1);
	}
}

Value newCollection(ObjClass* klass) {
  switch (klass->obj.type) {
    case OBJ_ARRAY: return OBJ_VAL(newArray());
    case OBJ_DICTIONARY: return OBJ_VAL(newDictionary());
    default: {
      ObjInstance* collection = newInstance(klass);
      Value initMethod = getObjMethod(OBJ_VAL(collection), "__init__");
      callReentrant(OBJ_VAL(collection), initMethod);
      return OBJ_VAL(collection);
    }
  }
}

static int linkFindIndex(ObjInstance* linkedList, Value element) {
  int index = 0;
  Value f = getObjProperty(linkedList, "first");
  ObjNode* first = IS_NIL(f) ? NULL : AS_NODE(f);
  for (ObjNode* node = first; node != NULL; node = node->next) {
    if (valuesEqual(element, node->element)) return index;
    index++;
  }
  return -1;
}

static bool collectionIsEmpty(ObjInstance* collection) {
  int length = AS_INT(getObjProperty(collection, "length"));
  return (length == 0);
}

static void collectionLengthDecrement(ObjInstance* collection) {
  int length = AS_INT(getObjProperty(collection, "length"));
  setObjProperty(collection, "length", INT_VAL(length - 1));
}

static void collectionLengthIncrement(ObjInstance* collection) {
  int length = AS_INT(getObjProperty(collection, "length"));
  setObjProperty(collection, "length", INT_VAL(length + 1));
}


static int linkSearchElement(ObjInstance* linkedList, Value element) {
  int length = AS_INT(getObjProperty(linkedList, "length"));
  if (length > 0) {
    ObjNode * first = AS_NODE(getObjProperty(linkedList, "first"));
    for (int i = 0; i < length; i++) {
      if (valuesEqual(element, first->element)) return i;
      first = first->next;
    }
  }
  return -1;
}

static ObjString* linkToString(ObjInstance* linkedList) {
  int size = AS_INT(getObjProperty(linkedList, "length"));
  if (size == 0) return copyString("[]", 2);
  else {
    char string[UINT8_MAX] = "";
    string[0] = '[';
    size_t offset = 1;
    ObjNode* node = AS_NODE(getObjProperty(linkedList, "first"));
    for (int i = 0; i < size; i++) {
      char* chars;
      size_t length;

      if (IS_STRING(node->element)) {
        chars = valueToString(node->element);
        length = strlen(chars);
        string[offset] = '"';
        memcpy(string + offset + 1, chars, length);
        string[offset + 1 + length] = '"';
        length += 2;
      } else {
        // Handle non-string elements
        chars = valueToString(node->element);
        length = strlen(chars);
        memcpy(string + offset, chars, length);
      }

      if (i == size - 1) {
        offset += length;
      }
      else {
        memcpy(string + offset + length, ", ", 2);
        offset += length + 2;
      }
      node = node->next;
    }
    string[offset] = ']';
    string[offset + 1] = '\0';
    return copyString(string, (int)offset + 1);
  }
}

// COLLECTION

NATIVE_METHOD(Collection, append) {
  assertError("Not implemented, subclass responsibility.");
  RETURN_NIL;
}

NATIVE_METHOD(Collection, extend) {
  assertArgCount("Collection::extend(collection)", 1, argCount);
  assertObjInstanceOfClass("Collection::extend(collection)", args[0], "luminique::std::collection", "Collection", 0);
  Value collection = args[0];
  Value addMethod = getObjMethod(receiver, "append");
  Value nextMethod = getObjMethod(collection, "next");
  Value nextValueMethod = getObjMethod(collection, "nextValue");
  Value index = callReentrant(collection, nextMethod, NIL_VAL);

  while (!IS_NIL(index)) {
    Value element = callReentrant(collection, nextValueMethod, index);
    callReentrant(receiver, addMethod, element);
    index = callReentrant(collection, nextMethod, index);
  }

  RETURN_NIL;
}

NATIVE_METHOD(Collection, collect) {
  assertArgCount("Collection::collect(closure)", 1, argCount);
  assertArgIsClosure("Collection::collect(closure)", args, 0);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  Value addMethod = getObjMethod(receiver, "append");
  Value nextMethod = getObjMethod(receiver, "next");
  Value nextValueMethod = getObjMethod(receiver, "nextValue");
  Value index = callReentrant(receiver, nextMethod, NIL_VAL);

  Value collected = newCollection(getObjClass(receiver));
  push(collected);
  while (!IS_NIL(index)) {
    Value element = callReentrant(receiver, nextValueMethod, index);
    Value result = callReentrant(receiver, OBJ_VAL(closure), element);
    callReentrant(collected, addMethod, result);
    index = callReentrant(receiver, nextMethod, index);
  }
  pop();
  return collected;
}

NATIVE_METHOD(Collection, detect) {
  assertArgCount("Collection::detect(closure)", 1, argCount);
  assertArgIsClosure("Collection::detect(closure)", args, 0);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  Value nextMethod = getObjMethod(receiver, "next");
  Value nextValueMethod = getObjMethod(receiver, "nextValue");
  Value index = callReentrant(receiver, nextMethod, NIL_VAL);

  while (!IS_NIL(index)) {
    Value element = callReentrant(receiver, nextValueMethod, index);
    Value result = callReentrant(receiver, OBJ_VAL(closure), element);
    if (!isFalsey(result)) RETURN_VAL(element);
    index = callReentrant(receiver, nextMethod, index);
  }

  RETURN_NIL;
}

NATIVE_METHOD(Collection, each) {
  assertArgCount("Collection::each(closure)", 1, argCount);
  assertArgIsClosure("Collection::each(closure)", args, 0);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  Value nextMethod = getObjMethod(receiver, "next");
  Value nextValueMethod = getObjMethod(receiver, "nextValue");
  Value index = callReentrant(receiver, nextMethod, NIL_VAL);

  while (!IS_NIL(index)) {
    Value element = callReentrant(receiver, nextValueMethod, index);
    Value result = callReentrant(receiver, OBJ_VAL(closure), element);
    index = callReentrant(receiver, nextMethod, index);
  }

  RETURN_NIL;
}

NATIVE_METHOD(Collection, __init__) {
  assertError("Cannot instantiate from class Collection.");
  RETURN_NIL;
}

NATIVE_METHOD(Collection, isEmpty) {
  assertArgCount("Collection::isEmpty()", 1, argCount);
  Value nextMethod = getObjMethod(receiver, "next");
  Value index = callReentrant(receiver, nextMethod, NIL_VAL);
  RETURN_BOOL(IS_NIL(index));
}

NATIVE_METHOD(Collection, length) {
  assertArgCount("Collection::length()", 1, argCount);
  Value nextMethod = getObjMethod(receiver, "next");
  Value index = callReentrant(receiver, nextMethod, NIL_VAL);
  
  int length = 0;

  while (!IS_NIL(index)) {
    index = callReentrant(receiver, nextMethod, index);
    length++;
  }

  RETURN_INT(length);
}

NATIVE_METHOD(Collection, reject) {
  assertArgCount("Collection::reject(closure)", 1, argCount);
  assertArgIsClosure("Collection::reject(closure)", args, 0);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  Value addMethod = getObjMethod(receiver, "append");
  Value nextMethod = getObjMethod(receiver, "next");
  Value nextValueMethod = getObjMethod(receiver, "nextValue");
  Value index = callReentrant(receiver, nextMethod, NIL_VAL);

  Value rejected = newCollection(getObjClass(receiver));
  push(rejected);

  while (!IS_NIL(index)) {
    Value element = callReentrant(receiver, nextValueMethod, index);
    Value result = callReentrant(receiver, OBJ_VAL(closure), element);
    if(isFalsey(result)) callReentrant(rejected, addMethod, element);
    index = callReentrant(receiver, nextMethod, index);
  }

  pop();
  return rejected;
}

NATIVE_METHOD(Collection, select) {
  assertArgCount("Collection::select(closure)", 1, argCount);
  assertArgIsClosure("Collection::select(closure)", args, 0);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  Value addMethod = getObjMethod(receiver, "append");
  Value nextMethod = getObjMethod(receiver, "next");
  Value nextValueMethod = getObjMethod(receiver, "nextValue");
  Value index = callReentrant(receiver, nextMethod, NIL_VAL);

  Value selected = newCollection(getObjClass(receiver));
  push(selected);

  while (!IS_NIL(index)) {
    Value element = callReentrant(receiver, nextValueMethod, index);
    Value result = callReentrant(receiver, OBJ_VAL(closure), element);
    if (!isFalsey(result)) callReentrant(selected, addMethod, element);
    index = callReentrant(receiver, nextMethod, index);
  }

  pop();
  return selected;
}

NATIVE_METHOD(Collection, toArray) {
  assertArgCount("Collection::toArray(closure)", 1, argCount);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  Value addMethod = getObjMethod(receiver, "append");
  Value nextMethod = getObjMethod(receiver, "next");
  Value nextValueMethod = getObjMethod(receiver, "nextValue");
  Value index = callReentrant(receiver, nextMethod, NIL_VAL);

  ObjArray* array = newArray();
  push(OBJ_VAL(array));

  while (!IS_NIL(index)) {
    Value element = callReentrant(receiver, nextValueMethod, index);
    writeValueArray(&array->elements, element);
    index = callReentrant(receiver, nextMethod, index);
  }

  pop();
  RETURN_OBJ(array);
}

// ARRAY

NATIVE_METHOD(Array, __init__) {
	assertArgCount("Array::__init__()", 0, argCount);
	RETURN_OBJ(newArray());
}

NATIVE_METHOD(Array, __getSubscript__) {
  assertArgCount("Array::[](index)", 1, argCount);
  assertArgIsInt("Array::[](index)", args, 0);
  ObjArray* self = AS_ARRAY(receiver);
  int index = AS_INT(args[0]);
  assertIntWithinRange("Array::[](index)", index, 0, self->elements.count - 1, 0);
  RETURN_VAL(self->elements.values[index]);
}

NATIVE_METHOD(Array, __setSubscript__) {
  assertArgCount("Array::[]=(index, element)", 2, argCount);
  assertArgIsInt("Array::[]=(index, element)", args, 0);
  ObjArray* self = AS_ARRAY(receiver);
  int index = AS_INT(args[0]);
  assertIntWithinRange("Array::[]=(index, element)", index, 0, self->elements.count, 0);
  self->elements.values[index] = args[1];
  if (index == self->elements.count) self->elements.count++;
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(Array, append) {
	assertArgCount("Array::append(element)", 1, argCount);
	writeValueArray(&AS_ARRAY(receiver)->elements, args[0]);
	RETURN_OBJ(&receiver);
}

NATIVE_METHOD(Array, extend) {
	assertArgCount("Array::extend(array)", 1, argCount);
	assertArgIsArray("Array::extend(array)", args, 0);
	arrayAddAll(AS_ARRAY(args[0]), AS_ARRAY(receiver));
	return receiver;
}

NATIVE_METHOD(Array, clear) {
	assertArgCount("Array::clear()", 0, argCount);
	freeValueArray(&AS_ARRAY(receiver)->elements);
	RETURN_OBJ(&receiver);
}

NATIVE_METHOD(Array, clone) {
	assertArgCount("Array::clone()", 0, argCount);
	ObjArray* self = AS_ARRAY(receiver);
	RETURN_OBJ(copyArray(self->elements, 0, self->elements.count));
}

NATIVE_METHOD(Array, contains) {
	assertArgCount("Array::contains(element)", 1, argCount);
	RETURN_BOOL(arrayIndexOf(AS_ARRAY(receiver), args[0]) != -1);
}

NATIVE_METHOD(Array, each) {
  assertArgCount("Array::each(closure)", 1, argCount);
  assertArgIsClosure("Array::each(closure)", args, 0);
  ObjArray* self = AS_ARRAY(receiver);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  for (int i = self->elements.count - 1; i >= 0; i--) {
    push(args[0]);
    push(self->elements.values[i]);
    callClosure(closure, 1);
  }
  callClosure(closure, 1);
  push(args[0]);
  push(receiver);
  vm.frameCount--;
  RETURN_NIL;
}

NATIVE_METHOD(Array, equals) {
	assertArgCount("Array::equals(other)", 1, argCount);
	if (!IS_ARRAY(args[0])) RETURN_FALSE;
  RETURN_BOOL(equalValueArray(&AS_ARRAY(receiver)->elements, &AS_ARRAY(args[0])->elements));
}

NATIVE_METHOD(Array, getAt) {
	assertArgCount("Array::getAt(index)", 1, argCount);
	assertArgIsInt("Array::getAt(index)", args, 0);
	ObjArray* self = AS_ARRAY(receiver);
	int index = AS_INT(args[0]);
	assertNumberWithinRange("Array::getAt(index)", index, 0, self->elements.count - 1, 0);
	RETURN_VAL(self->elements.values[index]);
}

NATIVE_METHOD(Array, isEmpty) {
	assertArgCount("Array::isEmpty()", 0, argCount);
	RETURN_BOOL(AS_ARRAY(receiver)->elements.count == 0);
}

NATIVE_METHOD(Array, indexOf) {
	assertArgCount("Array::indexOf(element)", 1, argCount);
	ObjArray* self = AS_ARRAY(receiver);
	if (self->elements.count == 0) RETURN_NIL;
	RETURN_INT(arrayIndexOf(self, args[0]));
}

NATIVE_METHOD(Array, insertAt) {
	assertArgCount("Array::insertAt(index, element)", 2, argCount);
	assertArgIsInt("Array::insertAt(index, element)", args, 0);
	ObjArray* self = AS_ARRAY(receiver);
	int index = AS_INT(args[0]);
	assertNumberWithinRange("Array::insertAt(index)", index, 0, self->elements.count, 0);
	arrayInsertAt(self, index, args[1]);
	RETURN_VAL(args[1]);
}

NATIVE_METHOD(Array, lastIndexOf) {
	assertArgCount("Array::indexOf(element)", 1, argCount);
	ObjArray* self = AS_ARRAY(receiver);
	if (self->elements.count == 0) RETURN_NIL;
	RETURN_INT(arrayLastIndexOf(self, args[0]));
}

NATIVE_METHOD(Array, length) {
	assertArgCount("Array::length()", 0, argCount);
	RETURN_INT(AS_ARRAY(receiver)->elements.count);
}

NATIVE_METHOD(Array, remove) {
	assertArgCount("Array::remove(element)", 1, argCount);
	ObjArray* self = AS_ARRAY(receiver);
	int index = arrayIndexOf(self, args[0]);
	if (index == -1) RETURN_FALSE;
	arrayRemoveAt(self, index);
	RETURN_TRUE;
}


NATIVE_METHOD(Array, next) {
  assertArgCount("Array::next(index)", 1, argCount);
  ObjArray* self = AS_ARRAY(receiver);
  if (IS_NIL(args[0])) {
    if (self->elements.count == 0) RETURN_FALSE;
    RETURN_INT(0);
  }

  assertArgIsInt("Array::next(index)", args, 0);
  int index = AS_INT(args[0]);
  if (index < 0 || index < self->elements.count - 1) RETURN_INT(index + 1);
  RETURN_NIL;
}

NATIVE_METHOD(Array, nextValue) {
  assertArgCount("Array::nextValue(index)", 1, argCount);
  assertArgIsInt("Array::nextValue(index)", args, 0);
  ObjArray* self = AS_ARRAY(receiver);
  int index = AS_INT(args[0]);
  if (index > -1 && index < self->elements.count) RETURN_VAL(self->elements.values[index]);
  RETURN_NIL;
}

NATIVE_METHOD(Array, putAt) {
	assertArgCount("Array::putAt(index, element)", 2, argCount);
	assertArgIsInt("Array::putAt(index, element)", args, 0);
	ObjArray* self = AS_ARRAY(receiver);
	int index = AS_INT(args[0]);
	assertNumberWithinRange("Array::putAt(index)", index, 0, self->elements.count, 0);
	self->elements.values[index] = args[1];
	if (index == self->elements.count) self->elements.count++;
	return receiver;
}

NATIVE_METHOD(Array, removeAt) {
	assertArgCount("Array::removeAt(index)", 1, argCount);
	assertArgIsInt("Array::removeAt(index)", args, 0);
	ObjArray* self = AS_ARRAY(receiver);
	int index = AS_INT(args[0]);
	assertNumberWithinRange("Array::removeAt(index)", AS_INT(args[0]), 0, self->elements.count - 1, 0);
	Value element = arrayRemoveAt(self, index);
	RETURN_VAL(element);
}


NATIVE_METHOD(Array, pop) {
	assertArgCount("Array::pop()", 0, argCount);
	ObjArray* self = AS_ARRAY(receiver);
  if (self->elements.count == 0) {
    THROW_EXCEPTION(luminique::std::lang, "IndexOutOfBoundsException", "Can't pop and element if the array is empty");
  }
	Value element = arrayRemoveAt(self, self->elements.count - 1);
	RETURN_VAL(element);
}

NATIVE_METHOD(Array, subArray) {
	assertArgCount("Array::subArray(from, to)", 2, argCount);
	assertArgIsInt("Array::subArray(from, to)", args, 0);
	assertArgIsInt("Array::subArray(from, to)", args, 1);
	ObjArray* self = AS_ARRAY(receiver);
	int fromIndex = AS_INT(args[0]);
	int toIndex = AS_INT(args[1]);

	assertNumberWithinRange("Array::subArray(from, to)", fromIndex, 0, self->elements.count, 0);
	assertNumberWithinRange("Array::subArray(from, to", toIndex, fromIndex, self->elements.count, 1);
	RETURN_OBJ(copyArray(self->elements, fromIndex, toIndex));
}

NATIVE_METHOD(Array, __str__) {
	assertArgCount("Array::__str__()", 0, argCount);
	RETURN_OBJ(arrayToString(AS_ARRAY(receiver)));
}

NATIVE_METHOD(Array, __format__) {
	assertArgCount("Array::__format__()", 0, argCount);
	RETURN_OBJ(arrayToString(AS_ARRAY(receiver)));
}

// DICTIONARY

NATIVE_METHOD(Dictionary, __init__) {
	assertArgCount("Dictionary::__init__()", 0, argCount);
	RETURN_OBJ(newDictionary());
}

NATIVE_METHOD(Dictionary, __getSubscript__) {
  assertArgCount("Dictionary::[](key)", 1, argCount);
  Value value;
  bool valueExists = dictGet(AS_DICTIONARY(receiver), args[0], &value);
  if (!valueExists) RETURN_NIL;
  RETURN_VAL(value);
}

NATIVE_METHOD(Dictionary, __setSubscript__) {
  assertArgCount("Dictionary::[]=(key, value)", 2, argCount);
  if (!IS_NIL(args[0])) dictSet(AS_DICTIONARY(receiver), args[0], args[1]);
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(Dictionary, clear) {
	assertArgCount("Dictionary::clear()", 0, argCount);
  ObjDictionary* self = AS_DICTIONARY(receiver);
  FREE_ARRAY(ObjEntry, self->entries, self->capacity);
  self->count = 0;
  self->capacity = 0;
  self->entries = NULL;
	return receiver;
}

NATIVE_METHOD(Dictionary, clone) {
	assertArgCount("Dictionary::clone()", 0, argCount);
  RETURN_OBJ(dictCopy(AS_DICTIONARY(receiver)));
}

NATIVE_METHOD(Dictionary, containsKey) {
	assertArgCount("Dictionary::containsKey(key)", 1, argCount);
	RETURN_BOOL(dictContainsKey(AS_DICTIONARY(receiver), args[0]));
}

NATIVE_METHOD(Dictionary, containsValue) {
	assertArgCount("Dictionary::containsValue(value)", 1, argCount);
  RETURN_BOOL(dictContainsValue(AS_DICTIONARY(receiver), args[0]));
}

NATIVE_METHOD(Dictionary, equals) {
  assertArgCount("Dictionary::equals(other)", 1, argCount);
  if (!IS_DICTIONARY(args[0])) RETURN_FALSE;
  RETURN_BOOL(dictsEqual(AS_DICTIONARY(receiver), AS_DICTIONARY(args[0])));
}

NATIVE_METHOD(Dictionary, getAt) {
	assertArgCount("Dictionary::getAt(key)", 1, argCount);
	Value value;
	bool valueExists = dictGet(AS_DICTIONARY(receiver), args[0], &value);
	if (!valueExists) RETURN_NIL;
	RETURN_VAL(value);
}

NATIVE_METHOD(Dictionary, isEmpty) {
	assertArgCount("Dictionary::isEmpty()", 0, argCount);
	RETURN_BOOL(AS_DICTIONARY(receiver)->count == 0);
}

NATIVE_METHOD(Dictionary, length) {
	assertArgCount("Dictionary::length()", 0, argCount);
  RETURN_INT(dictLength(AS_DICTIONARY(receiver)));
}

NATIVE_METHOD(Dictionary, next) {
  assertArgCount("Dictionary::next(index)", 1, argCount);
  ObjDictionary* self = AS_DICTIONARY(receiver);
  if (self->count == 0) RETURN_NIL;

  int index = 0;
  if (!IS_NIL(args[0])) {
    Value key = args[0];
    index = dictFindIndex(self, key);
    if (index < 0 || index >= self->capacity) RETURN_NIL;
    index++;
  }

  for (; index < self->capacity; index++) {
    if (!IS_UNDEFINED(self->entries[index].key)) RETURN_VAL(self->entries[index].key);
  }
  RETURN_NIL;
}

NATIVE_METHOD(Dictionary, nextValue) {
  assertArgCount("Dictionary::nextValue(key)", 1, argCount);
  ObjDictionary* self = AS_DICTIONARY(receiver);
  int index = dictFindIndex(self, args[0]);
  RETURN_VAL(self->entries[index].value);
}

NATIVE_METHOD(Dictionary, putAll) {
	assertArgCount("Dictionary::putAll(dictionary)", 1, argCount);
	assertArgIsDictionary("Dictionary::putAll(dictionary)", args, 0);
  dictAddAll(AS_DICTIONARY(args[0]), AS_DICTIONARY(receiver));
	return receiver;
}

NATIVE_METHOD(Dictionary, put) {
	assertArgCount("Dictionary::put(key, value)", 2, argCount);
  if (!IS_NIL(args[0])) dictSet(AS_DICTIONARY(receiver), args[0], args[1]);
	return receiver;
}

NATIVE_METHOD(Dictionary, removeAt) {
	assertArgCount("Dictionary::removeAt(key)", 1, argCount);
  ObjDictionary* self = AS_DICTIONARY(receiver);
  Value key = args[0];
  Value value;

  bool keyExists = dictGet(self, key, &value);
  if (!keyExists) RETURN_NIL;
  dictDelete(self, key);
  RETURN_VAL(value);
}

NATIVE_METHOD(Dictionary, __str__) {
	assertArgCount("Dictionary::__str__()", 0, argCount);
	RETURN_OBJ(dictToString(AS_DICTIONARY(receiver)));
}

NATIVE_METHOD(Dictionary, __format__) {
	assertArgCount("Dictionary::__format__()", 0, argCount);
	RETURN_OBJ(dictToString(AS_DICTIONARY(receiver)));
}

NATIVE_METHOD(Range, __init__) {
	assertArgCount("Range::__init__()", 2, argCount);
  assertArgIsInt("Range::__init__(from, to)", args, 0);
  assertArgIsInt("Range::__init__(from, to)", args, 1);
  int from = AS_INT(args[0]);
  int to = AS_INT(args[1]);

  ObjRange* range = newRange(from, to);
  RETURN_OBJ(range);
}

NATIVE_METHOD(Range, clone) {
  assertArgCount("Range::clone()", 0, argCount);
  ObjRange* self = AS_RANGE(receiver);
  ObjRange* range = newRange(self->from, self->to);
  RETURN_OBJ(range);
}

NATIVE_METHOD(Range, contains) {
  assertArgCount("Range::contains(element)", 1, argCount);
  if (!IS_INT(args[0])) RETURN_FALSE;
  ObjRange* self = AS_RANGE(receiver);
  int element = AS_INT(args[0]);
  if (self->from < self->to) RETURN_BOOL(element >= self->from && element <= self->to);
  else RETURN_BOOL(element >= self->to && element <= self->from);
}

NATIVE_METHOD(Range, from) {
  assertArgCount("Range::from()", 0, argCount);
  ObjRange* self = AS_RANGE(receiver);
  RETURN_INT(self->from);
}

NATIVE_METHOD(Range, getAt) {
  assertArgCount("Range::getAt(index)", 1, argCount);
  assertArgIsInt("Range::getAt(index)", args, 0);
  ObjRange* self = AS_RANGE(receiver);
  int index = AS_INT(args[0]);

  int min = (self->from < self->to) ? self->from : self->to;
  int max = (self->from < self->to) ? self->to : self->from;
  assertIntWithinRange("Range::getAt(index)", index, min, max, 0);
  RETURN_INT(self->from + index);
}

NATIVE_METHOD(Range, length) {
  assertArgCount("Range::length()", 0, argCount);
  ObjRange* self = AS_RANGE(receiver);
  RETURN_INT(abs(self->to - self->from) + 1);
}

NATIVE_METHOD(Range, max) {
  assertArgCount("Range::max()", 0, argCount);
  ObjRange* self = AS_RANGE(receiver);
  RETURN_INT((self->from < self->to) ? self->to : self->from);
}

NATIVE_METHOD(Range, min) {
  assertArgCount("Range::min()", 0, argCount);
  ObjRange* self = AS_RANGE(receiver);
  RETURN_INT((self->from < self->to) ? self->from : self->to);
}

NATIVE_METHOD(Range, next) {
  assertArgCount("Range::next(index)", 1, argCount);
  ObjRange* self = AS_RANGE(receiver);
  if (IS_NIL(args[0])) {
    if (self->from == self->to) RETURN_FALSE;
    RETURN_INT(0);
  }

  assertArgIsInt("Range::next(index)", args, 0);
  int index = AS_INT(args[0]);
  if (index < 0 || index < abs(self->to - self->from)) RETURN_INT(index + 1);
  RETURN_NIL;
}

NATIVE_METHOD(Range, nextValue) {
  assertArgCount("Range::nextValue(index)", 1, argCount);
  assertArgIsInt("Range::nextValue(index)", args, 0);
  ObjRange* self = AS_RANGE(receiver);
  int index = AS_INT(args[0]);

  int step = (self->from < self->to) ? index : -index;
  if (index > -1 && index < abs(self->to - self->from) + 1) RETURN_INT(self->from + step);
  RETURN_NIL;
}

NATIVE_METHOD(Range, step) {
  assertArgCount("Range::step(by, closure)", 2, argCount);
  assertArgIsNumber("Range::step(by, closure)", args, 0);
  assertArgIsClosure("Range::step(by, closure)", args, 1);
  ObjRange* self = AS_RANGE(receiver);
  double from = self->from;
  double to = self->to;
  double by = AS_NUMBER(args[0]);
  ObjClosure* closure = AS_CLOSURE(args[1]);

  if (by == 0) { 
    THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Step size cannot be 0.");
  } else {
    if (by > 0) {
      for (double num = from; num <= to; num += by) {
        callReentrant(receiver, OBJ_VAL(closure), NUMBER_VAL(num));
      }
    } else {
      for (double num = from; num >= to; num += by) {
        callReentrant(receiver, OBJ_VAL(closure), NUMBER_VAL(num));
      }
    }
  }
  RETURN_NIL;
}

NATIVE_METHOD(Range, to) {
  assertArgCount("Range::to()", 0, argCount);
  ObjRange* self = AS_RANGE(receiver);
  RETURN_INT(self->to);
}

NATIVE_METHOD(Range, toArray) {
  assertArgCount("Range::toArray()", 0, argCount);
  ObjRange* self = AS_RANGE(receiver);
  ObjArray* array = newArray();
  push(OBJ_VAL(array));

  if (self->from < self->to) {
    for (int i = self->from; i <= self->to; i++) {
      writeValueArray(&array->elements, INT_VAL(i));
    }
  } else { 
    for (int i = self->to; i >= self->from; i--) {
      writeValueArray(&array->elements, INT_VAL(i));
    }
  }

  pop();
  RETURN_OBJ(array);
}

NATIVE_METHOD(Range, append) {
  THROW_EXCEPTION(luminique::std::lang, NotImplementedException, "Cannot add an element to instance of class Range.");
}

NATIVE_METHOD(Range, extend) {
  THROW_EXCEPTION(luminique::std::lang, NotImplementedException, "Cannot add a collection to instance of class Range.");
}

NATIVE_METHOD(Range, __str__) {
  assertArgCount("Range::__str__()", 0, argCount);
  ObjRange* self = AS_RANGE(receiver);
  RETURN_STRING_FMT("%d...%d", self->from, self->to);
}

NATIVE_METHOD(Range, __format__) {
  assertArgCount("Range::__format__()", 0, argCount);
  ObjRange* self = AS_RANGE(receiver);
  RETURN_STRING_FMT("%d...%d", self->from, self->to);
}

NATIVE_METHOD(List, eachIndex) {
  assertArgCount("List::eachIndex(closure)", 1, argCount);
  assertArgIsClosure("List::eachIndex(closure)", args, 0);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  Value index = INT_VAL(0);
  Value nextMethod = getObjMethod(receiver, "next");
  Value nextValueMethod = getObjMethod(receiver, "nextValue");

  while (index != NIL_VAL) {
    Value element = callReentrant(receiver, nextValueMethod, index);
    callReentrant(receiver, OBJ_VAL(closure), index, element);
    index = callReentrant(receiver, nextMethod, index);
  }
  RETURN_NIL;
}

NATIVE_METHOD(List, getAt) {
  assertArgCount("List::getAt(index)", 1, argCount);
  assertArgIsInt("List::getAt(index)", args, 0);
  int position = AS_INT(args[0]);
  Value index = INT_VAL(0);
  Value nextMethod = getObjMethod(receiver, "next");
  Value nextValueMethod = getObjMethod(receiver, "nextValue");

  while (index != NIL_VAL) {
    Value element = callReentrant(receiver, nextValueMethod, index);
    if (index == position) RETURN_VAL(element);
    index = callReentrant(receiver, nextMethod, index);
  }
  RETURN_NIL;
}

NATIVE_METHOD(List, putAt) {
  THROW_EXCEPTION(luminique::std::lang, NotImplementedException, "Not implemented, subclass responsibility.");
}

NATIVE_METHOD(Node, __init__) {
  assertArgCount("Node::__init__(element, prev, next)", 3, argCount);
  ObjNode* self = AS_NODE(receiver);
  self->element = args[0];
  if (!IS_NIL(args[1])) self->prev = AS_NODE(args[1]);
  if (!IS_NIL(args[2])) self->next = AS_NODE(args[2]);
  RETURN_OBJ(self);
}

NATIVE_METHOD(Node, clone) {
  assertArgCount("Node::clone()", 0, argCount);
  ObjNode* self = AS_NODE(receiver);
  ObjNode* node = newNode(self->element, self->prev, self->next);
  RETURN_OBJ(node);
}

NATIVE_METHOD(Node, element) {
  assertArgCount("Node::element()", 0, argCount);
  RETURN_VAL(AS_NODE(receiver)->element);
}

NATIVE_METHOD(Node, next) {
  assertArgCount("Node::next()", 0, argCount);
  RETURN_OBJ(AS_NODE(receiver)->next);
}

NATIVE_METHOD(Node, prev) {
  assertArgCount("Node::prev()", 0, argCount);
  RETURN_OBJ(AS_NODE(receiver)->prev);
}

NATIVE_METHOD(Node, __str__) {
  assertArgCount("Node::__str__()", 0, argCount);
  char nodeString[UINT8_MAX] = "";
  memcpy(nodeString, "Node: ", 6);

  Value nodeElement = AS_NODE(receiver)->element;
  char* nodeChars = valueToString(nodeElement);
  size_t nodeLength = strlen(nodeChars);
  memcpy(nodeString + 6, nodeChars, nodeLength);
  nodeString[nodeLength + 6] = '\0';
  RETURN_STRING(nodeString, (int)nodeLength + 6);
}

NATIVE_METHOD(Node, __format__) {
  assertArgCount("Node::__format__()", 0, argCount);
  char nodeString[UINT8_MAX] = "";
  memcpy(nodeString, "Node: ", 6);

  Value nodeElement = AS_NODE(receiver)->element;
  char* nodeChars = valueToString(nodeElement);
  size_t nodeLength = strlen(nodeChars);
  memcpy(nodeString + 6, nodeChars, nodeLength);
  nodeString[nodeLength + 6] = '\0';
  RETURN_STRING(nodeString, (int)nodeLength + 6);
}

NATIVE_METHOD(Stack, clear) {
  assertArgCount("Stack::clear()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  setObjProperty(self, "first", NIL_VAL);
  setObjProperty(self, "current", NIL_VAL);
  setObjProperty(self, "length", INT_VAL(0));
  RETURN_NIL;
}

NATIVE_METHOD(Stack, contains) {
  assertArgCount("Stack::contains(element)", 1, argCount);
  RETURN_BOOL(linkFindIndex(AS_INSTANCE(receiver), args[0]) != -1);
}

NATIVE_METHOD(Stack, getFirst) { 
  assertArgCount("Stack::getFirst()", 0, argCount);
  ObjNode* first = AS_NODE(getObjProperty(AS_INSTANCE(receiver), "first"));
  RETURN_VAL(first->element);
}

NATIVE_METHOD(Stack, __init__) {
  assertArgCount("Stack::__init__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  setObjProperty(self, "first", OBJ_VAL(newNode(NIL_VAL, NULL, NULL)));
  setObjProperty(self, "current", NIL_VAL);
  setObjProperty(self, "length", INT_VAL(0));
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(Stack, isEmpty) {
  assertArgCount("Stack::isEmpty()", 0, argCount);
  RETURN_BOOL(collectionIsEmpty(AS_INSTANCE(receiver)));
}

NATIVE_METHOD(Stack, length) {
  assertArgCount("Stack::length()", 0, argCount);
  Value length = getObjProperty(AS_INSTANCE(receiver), "length");
  RETURN_INT(length);
}

NATIVE_METHOD(Stack, next) {
  assertArgCount("Stack::next(index)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  int length = AS_INT(getObjProperty(self, "length"));

  if (IS_NIL(args[0])) {
    if (length == 0) RETURN_FALSE;
    RETURN_INT(0);
  }

  assertArgIsInt("Stack::next(index)", args, 0);
  int index = AS_INT(args[0]);
  if (index >= 0 && index < length - 1) {
    ObjNode* current = AS_NODE(getObjProperty(self, (index == 0) ? "first" : "current"));
    setObjProperty(self, "current", OBJ_VAL(current->next));
    RETURN_INT(index + 1);
  } else {
    setObjProperty(self, "current", getObjProperty(self, "first"));
    RETURN_NIL;
  }
}

NATIVE_METHOD(Stack, nextValue) {
  assertArgCount("Stack::nextValue(index)", 1, argCount);
  assertArgIsInt("Stack::nextValue(index)", args, 0);
  ObjInstance* self = AS_INSTANCE(receiver);
  int length = AS_INT(getObjProperty(self, "length"));
  int index = AS_INT(args[0]);
  if (index == 0) RETURN_VAL(getObjProperty(self, "first"));
  if (index > 0 && index < length) RETURN_VAL(getObjProperty(self, "current"));
  RETURN_NIL;
}

NATIVE_METHOD(Stack, peek) {
  assertArgCount("Stack::peek()", 0, argCount);
  ObjNode* first = AS_NODE(getObjProperty(AS_INSTANCE(receiver), "first"));
  RETURN_VAL(first->element);
}

NATIVE_METHOD(Stack, pop) {
  assertArgCount("Stack::pop()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjNode* first = AS_NODE(getObjProperty(self, "first"));
  int length = AS_INT(getObjProperty(AS_INSTANCE(receiver), "length"));
  if (length == 0) RETURN_NIL;
  else {
    Value element = first->element;
    setObjProperty(self, "first", first->next == NULL ? NIL_VAL : OBJ_VAL(first->next));
    collectionLengthDecrement(self);
    RETURN_VAL(element);
  }
}

NATIVE_METHOD(Stack, push) {
  assertArgCount("Stack::push(element)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  ObjNode* first = AS_NODE(getObjProperty(self, "first"));
  int length = AS_INT(getObjProperty(AS_INSTANCE(receiver), "length"));
  ObjNode* new = newNode(args[0], NULL, NULL);

  push(OBJ_VAL(new));
  if (length > 0) {
    new->next = first;
  }
  setObjProperty(self, "first", OBJ_VAL(new));
  pop();
  
  collectionLengthIncrement(self);
  RETURN_VAL(args[0]);
}

NATIVE_METHOD(Stack, search) {
  assertArgCount("Stack::search(element)", 1, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  RETURN_INT(linkSearchElement(self, args[0]));
}

NATIVE_METHOD(Stack, toArray) {
  assertArgCount("Stack::toArray()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  int length = AS_INT(getObjProperty(self, "length"));
  ObjArray* array = newArray();
  push(OBJ_VAL(array));
  if (length > 0) {
    for (ObjNode* node = AS_NODE(getObjProperty(self, "first")); node != NULL; node = node->next) {
      writeValueArray(&array->elements, node->element);
    }
  }
  pop();
  RETURN_OBJ(array);
}

NATIVE_METHOD(Stack, __str__) {
  assertArgCount("Stack::__str__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  RETURN_OBJ(linkToString(self));
}

NATIVE_METHOD(Stack, __format__) {
  assertArgCount("Stack::__format__()", 0, argCount);
  ObjInstance* self = AS_INSTANCE(receiver);
  RETURN_OBJ(linkToString(self));
}

void registerCollectionPackage() {
  ObjNamespace* collectionNamespace = defineNativeNamespace("collection", vm.stdNamespace);
  vm.currentNamespace = vm.langNamespace;

  ObjClass* collectionClass = defineNativeClass("Collection");
  bindSuperclass(collectionClass, vm.objectClass);
  DEF_METHOD(collectionClass, Collection, __init__, 0);
  DEF_METHOD(collectionClass, Collection, append, 1);
  DEF_METHOD(collectionClass, Collection, extend, 1);
  DEF_METHOD(collectionClass, Collection, collect, 1);
  DEF_METHOD(collectionClass, Collection, detect, 1);
  DEF_METHOD(collectionClass, Collection, each, 1);
  DEF_METHOD(collectionClass, Collection, isEmpty, 0);
  DEF_METHOD(collectionClass, Collection, length, 0);
  DEF_METHOD(collectionClass, Collection, reject, 1);
  DEF_METHOD(collectionClass, Collection, select, 1);
  DEF_METHOD(collectionClass, Collection, toArray, 0);

  ObjClass* listClass = defineNativeClass("List");
  bindSuperclass(listClass, collectionClass);
  DEF_METHOD(listClass, List, eachIndex, 1);
  DEF_METHOD(listClass, List, getAt, 1);
  DEF_METHOD(listClass, List, putAt, 2);

	vm.arrayClass = defineNativeClass("Array");
	bindSuperclass(vm.arrayClass, listClass);
	DEF_METHOD(vm.arrayClass, Array, __init__, 0);
	DEF_METHOD(vm.arrayClass, Array, append, 1);
  DEF_METHOD(vm.arrayClass, Array, extend, 1);
	DEF_METHOD(vm.arrayClass, Array, clear, 0);
	DEF_METHOD(vm.arrayClass, Array, clone, 0);
	DEF_METHOD(vm.arrayClass, Array, contains, 1);
  DEF_METHOD(vm.arrayClass, Array, each, 1);
	DEF_METHOD(vm.arrayClass, Array, equals, 1);
	DEF_METHOD(vm.arrayClass, Array, getAt, 1);
	DEF_METHOD(vm.arrayClass, Array, indexOf, 1);
	DEF_METHOD(vm.arrayClass, Array, insertAt, 2);
  DEF_METHOD(vm.arrayClass, Array, isEmpty, 0);
	DEF_METHOD(vm.arrayClass, Array, lastIndexOf, 1);
	DEF_METHOD(vm.arrayClass, Array, length, 0);
  DEF_METHOD(vm.arrayClass, Array, next, 1);
  DEF_METHOD(vm.arrayClass, Array, nextValue, 1);
  DEF_METHOD(vm.arrayClass, Array, putAt, 2);
	DEF_METHOD(vm.arrayClass, Array, remove, 1);
	DEF_METHOD(vm.arrayClass, Array, pop, 0);
	DEF_METHOD(vm.arrayClass, Array, removeAt, 1);
  DEF_METHOD(vm.arrayClass, Array, subArray, 2);
	DEF_METHOD(vm.arrayClass, Array, __str__, 0);
	DEF_METHOD(vm.arrayClass, Array, __format__, 0);
  DEF_OPERATOR(vm.arrayClass, Array, [], __getSubscript__, 1);
  DEF_OPERATOR(vm.arrayClass, Array, []=, __setSubscript__, 2);

	vm.dictionaryClass = defineNativeClass("Dictionary");
	bindSuperclass(vm.dictionaryClass, collectionClass);
	DEF_METHOD(vm.dictionaryClass, Dictionary, __init__, 0);
	DEF_METHOD(vm.dictionaryClass, Dictionary, clear, 0);
	DEF_METHOD(vm.dictionaryClass, Dictionary, containsKey, 1);
	DEF_METHOD(vm.dictionaryClass, Dictionary, containsValue, 1);
	DEF_METHOD(vm.dictionaryClass, Dictionary, equals, 1);
	DEF_METHOD(vm.dictionaryClass, Dictionary, getAt, 1);
	DEF_METHOD(vm.dictionaryClass, Dictionary, clone, 0);
	DEF_METHOD(vm.dictionaryClass, Dictionary, isEmpty, 0);
	DEF_METHOD(vm.dictionaryClass, Dictionary, length, 0);
  DEF_METHOD(vm.dictionaryClass, Dictionary, next, 1);
  DEF_METHOD(vm.dictionaryClass, Dictionary, nextValue, 1);
	DEF_METHOD(vm.dictionaryClass, Dictionary, put, 2);
	DEF_METHOD(vm.dictionaryClass, Dictionary, putAll, 1);
	DEF_METHOD(vm.dictionaryClass, Dictionary, removeAt, 1);
	DEF_METHOD(vm.dictionaryClass, Dictionary, __str__, 0);
	DEF_METHOD(vm.dictionaryClass, Dictionary, __format__, 0);
  DEF_OPERATOR(vm.dictionaryClass, Dictionary, [], __getSubscript__, 1);
  DEF_OPERATOR(vm.dictionaryClass, Dictionary, []=, __setSubscript__, 2);

  vm.rangeClass = defineNativeClass("Range");
  bindSuperclass(vm.rangeClass, listClass);
  DEF_METHOD(vm.rangeClass, Range, __init__, 2);
  DEF_METHOD(vm.rangeClass, Range, append, 1);
  DEF_METHOD(vm.rangeClass, Range, extend, 1);
  DEF_METHOD(vm.rangeClass, Range, clone, 0);
  DEF_METHOD(vm.rangeClass, Range, contains, 1);
  DEF_METHOD(vm.rangeClass, Range, from, 0);
  DEF_METHOD(vm.rangeClass, Range, getAt, 1);
  DEF_METHOD(vm.rangeClass, Range, length, 0);
  DEF_METHOD(vm.rangeClass, Range, max, 0);
  DEF_METHOD(vm.rangeClass, Range, min, 0);
  DEF_METHOD(vm.rangeClass, Range, next, 1);
  DEF_METHOD(vm.rangeClass, Range, nextValue, 1);
  DEF_METHOD(vm.rangeClass, Range, step, 2);
  DEF_METHOD(vm.rangeClass, Range, to, 0);
  DEF_METHOD(vm.rangeClass, Range, toArray, 0);
  DEF_METHOD(vm.rangeClass, Range, __str__, 0);
  DEF_METHOD(vm.rangeClass, Range, __format__, 0);

  vm.currentNamespace = collectionNamespace;

  ObjClass* setClass = defineNativeClass("Set");
  bindSuperclass(setClass, collectionClass);

  ObjClass* entryClass = defineNativeClass("Entry");
  bindSuperclass(entryClass, vm.objectClass);

  vm.nodeClass = defineNativeClass("Node");
  bindSuperclass(vm.nodeClass, vm.objectClass);
  DEF_METHOD(vm.nodeClass, Node, __init__, 3);
  DEF_METHOD(vm.nodeClass, Node, clone, 0);
  DEF_METHOD(vm.nodeClass, Node, element, 0);
  DEF_METHOD(vm.nodeClass, Node, next, 0);
  DEF_METHOD(vm.nodeClass, Node, prev, 0);
  DEF_METHOD(vm.nodeClass, Node, __str__, 0);
  DEF_METHOD(vm.nodeClass, Node, __format__, 0);

  ObjClass* stackClass = defineNativeClass("Stack");
  bindSuperclass(stackClass, collectionClass);
  DEF_METHOD(stackClass, Stack, __init__, 0);
  DEF_METHOD(stackClass, Stack, clear, 0);
  DEF_METHOD(stackClass, Stack, contains, 1);
  DEF_METHOD(stackClass, Stack, getFirst, 0);
  DEF_METHOD(stackClass, Stack, isEmpty, 0);
  DEF_METHOD(stackClass, Stack, length, 0);
  DEF_METHOD(stackClass, Stack, next, 1);
  DEF_METHOD(stackClass, Stack, nextValue, 1);
  DEF_METHOD(stackClass, Stack, peek, 0);
  DEF_METHOD(stackClass, Stack, pop, 0);
  DEF_METHOD(stackClass, Stack, push, 1);
  DEF_METHOD(stackClass, Stack, search, 1);
  DEF_METHOD(stackClass, Stack, toArray, 0);
  DEF_METHOD(stackClass, Stack, __str__, 0);
  DEF_METHOD(stackClass, Stack, __format__, 0);

  vm.currentNamespace = vm.rootNamespace;
}
