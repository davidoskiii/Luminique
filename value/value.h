#ifndef cluminique_value_h
#define cluminique_value_h

#include "../common.h"
#include <string.h>

typedef struct Obj Obj;
typedef struct ObjClass ObjClass;
typedef struct ObjString ObjString;

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_INT,
  VAL_FLOAT,
  VAL_OBJ
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    int integer;
    double float_;
    Obj* obj;
  } as; 
} Value;


#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_INT(value)     ((value).type == VAL_INT)
#define IS_FLOAT(value)   ((value).type == VAL_FLOAT)
#define IS_NUMBER(value)  valueIsNum(value)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_INT(value)     ((value).as.integer)
#define AS_FLOAT(value)   ((value).as.float_)
#define AS_NUMBER_LANG(value)  (IS_INT(value) ? (value).as.integer : (value).as.float_)
#define AS_NUMBER(value)  ((value).as.float_)
#define AS_OBJ(value)     ((value).as.obj)

#define BOOL_VAL(b)       ((Value){VAL_BOOL, {.boolean = b}})
#define NIL_VAL           ((Value){VAL_NIL, {.integer = 0}})
#define INT_VAL(i)        ((Value){VAL_INT, {.integer = i}})
#define FLOAT_VAL(f)      ((Value){VAL_FLOAT, {.float_ = f}})
#define NUMBER_VAL(num)   ((Value){VAL_FLOAT, {.float_ = num}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

typedef struct {
  int capacity;
  int count;
  Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void insertValueArray(ValueArray* array, int index, Value value);
int replaceValueArray(ValueArray* array, int index, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);
char* valueToString(Value value);

static inline bool valueIsInt(Value value) {
  return value.type == VAL_INT;
}

static inline bool valueIsNum(Value value) {
  return IS_FLOAT(value) || valueIsInt(value);
}

static inline double valueToFloat(Value value) {
  return AS_FLOAT(value);
}

static inline Value numToValue(double num) {
  return NUMBER_VAL(num);
}

#endif
