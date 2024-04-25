#include <stdio.h>
#include <string.h>

#include "../object/object.h"
#include "../memory/memory.h"
#include "../vm/vm.h"
#include "value.h"

void initValueArray(ValueArray* array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void writeValueArray(ValueArray* array, Value value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values = GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}

void insertValueArray(ValueArray* array, int index, Value value) {
  if (IS_OBJ(value)) push(value);
  writeValueArray(array, NIL_VAL);
  if (IS_OBJ(value)) pop();

  for (int i = array->count - 1; i > index; i--) {
    array->values[i] = array->values[i - 1];
  }
  array->values[index] = value;
}

int replaceValueArray(ValueArray* array, int index, Value value) {
  if (index < 0 || index >= array->count) {
    // Index out of bounds.
    return 0;
  }

  array->values[index] = value;
  return 1;
}

void freeValueArray(ValueArray* array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array);
}

bool equalValueArray(ValueArray* aArray, ValueArray* bArray) {
  if (aArray->count != bArray->count) return false;
  for (int i = 0; i < aArray->count; i++) {
    if (aArray->values[i] != bArray->values[i]) return false;
  }
  return true;
}

void printValue(Value value) {
#ifdef NAN_BOXING
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_INT(value)) {
    printf("%d", AS_INT(value));
  } else if (IS_FLOAT(value)) {
    printf("%g", AS_FLOAT(value));
  } else if(IS_OBJ(value)) {
    printObject(value);
  } else {
    printf("undefined");
  }
#else
  switch (value.type) {
    case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL: printf("nil"); break;
    case VAL_INT: printf("%d", AS_INT(value)); break;
    case VAL_FLOAT: printf("%g", AS_FLOAT(value)); break;
    case VAL_OBJ: printObject(value); break;
    default: printf("undefined");
  }
#endif
}

bool valuesEqual(Value a, Value b) {
#ifdef NAN_BOXING
  if (IS_NUMBER(a) && IS_NUMBER(b)) {
    return AS_NUMBER(a) == AS_NUMBER(b);
  }
  return a == b;
#else
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:   
      return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    
      return true;
    case VAL_INT:
    case VAL_FLOAT:  
      return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:    
      return AS_OBJ(a) == AS_OBJ(b);
    default:         
      return false;
  }
#endif
}

char* valueToString(Value value) {
  if (IS_BOOL(value)) {
    return AS_BOOL(value) ? "true" : "false";
  } else if (IS_NIL(value)) {
    return "nil";
  } else if (IS_INT(value)) {
    char* chars = ALLOCATE(char, (size_t)11);
    int length = snprintf(chars, 11, "%d", AS_INT(value));
    return chars;
  } else if (IS_FLOAT(value)) {
    char* chars = ALLOCATE(char, (size_t)24);
    int length = snprintf(chars, 24, "%.14g", AS_FLOAT(value));
    return chars;
  } else if (IS_OBJ(value)) {
    Obj* object = AS_OBJ(value);

    if (IS_STRING(value)) {
      return AS_CSTRING(value);
    } else {
      char* chars = ALLOCATE(char, (size_t)(7 + object->klass->name->length));
      int length = snprintf(chars, UINT8_MAX, "<object %s>", object->klass->name->chars);
      return chars;
    }
  }
  else {
    return "";
  }
}
