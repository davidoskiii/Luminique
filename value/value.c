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

void printValue(Value value) {
  switch (value.type) {
    case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL: printf("nil"); break;
    case VAL_INT: printf("%d", AS_INT(value)); break;
    case VAL_FLOAT: printf("%g", AS_FLOAT(value)); break;
    case VAL_OBJ: printObject(value); break;
  }
}

bool valuesEqual(Value a, Value b) {
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
}

char* valueToString(Value value) {
  if (IS_BOOL(value)) {
    return AS_BOOL(value) ? "true" : "false";
  } else if (IS_NIL(value)) {
    return "nil";
  } else if (IS_OBJ(value)) {
    Obj* object = AS_OBJ(value);
    if (IS_STRING(value)) {
      return AS_CSTRING(value);
    }
    else {
      return "Object";
    }
  }
  else {
    return "undefined";
  }
}
