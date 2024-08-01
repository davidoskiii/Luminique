#ifndef cluminique_value_h
#define cluminique_value_h

#include "../common.h"
#include <string.h>

typedef struct CallFrame CallFrame;

typedef struct Obj Obj;
typedef struct ObjClass ObjClass;
typedef struct ObjInstance ObjInstance;
typedef struct ObjEnum ObjEnum;
typedef struct ObjNamespace ObjNamespace;
typedef struct ObjNode ObjNode;
typedef struct ObjString ObjString;
typedef struct ObjFunction ObjFunction;
typedef struct ObjClosure ObjClosure;
typedef struct ObjUpvalue ObjUpvalue;
typedef struct ObjModule ObjModule;
typedef struct ObjFile ObjFile;
typedef struct ObjArray ObjArray;
typedef struct ObjDictionary ObjDictionary;
typedef struct ObjException ObjException;
typedef struct ObjGenerator ObjGenerator;
typedef struct ObjPromise ObjPromise;
typedef struct ObjNativeFunction ObjNativeFunction;
typedef struct ObjNativeMethod ObjNativeMethod;

typedef struct ExceptionHandler ExceptionHandler;

#ifdef NAN_BOXING

#define SIGN_BIT      ((uint64_t)0x8000000000000000)
#define QNAN          ((uint64_t)0x7ffc000000000000)
#define TAG_NIL       1
#define TAG_FALSE     2
#define TAG_TRUE      3
#define TAG_INT       4
#define TAG_CHAR      5
#define TAG_RESERVED  6
#define TAG_UNDEFINED 7

typedef uint64_t Value;

#define FALSE_VAL           ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL            ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL             ((Value)(uint64_t)(QNAN | TAG_NIL))
#define UNDEFINED_VAL       ((Value)(uint64_t)(QNAN | TAG_UNDEFINED))
#define BOOL_VAL(b)         ((b) ? TRUE_VAL : FALSE_VAL)
#define INT_VAL(i)          ((Value)(QNAN | ((uint64_t)(uint32_t)(i) << 3) | TAG_INT))
#define FLOAT_VAL(f)        numToValue(f)
#define NUMBER_VAL(num)     numToValue(num)
#define OBJ_VAL(obj)        ((Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj)))

#define IS_EMPTY(value)     valueIsEmpty(value)
#define IS_NIL(value)       ((value) == NIL_VAL)
#define IS_UNDEFINED(value) ((value) == UNDEFINED_VAL)
#define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
#define IS_INT(value)       valueIsInt(value)
#define IS_FLOAT(value)     (((value) & QNAN) != QNAN)
#define IS_NUMBER(value)    valueIsNum(value)
#define IS_OBJ(value)       (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)      ((value) == TRUE_VAL)
#define AS_INT(value)       ((int)((value) >> 3))
#define AS_FLOAT(value)     valueToFloat(value)
#define AS_NUMBER(value)    valueToNum(value)
#define AS_OBJ(value)       ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

static inline bool valueIsEmpty(Value value) { 
    return IS_NIL(value) || IS_UNDEFINED(value);
}

static inline bool valueIsInt(Value value) {
    return (value & (QNAN | TAG_INT)) == (QNAN | TAG_INT) && ((value | SIGN_BIT) != value);
}

static inline bool valueIsNum(Value value) {
    return IS_FLOAT(value) || valueIsInt(value);
}

static inline double valueToFloat(Value value) {
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

static inline double valueToNum(Value value) {
    return IS_FLOAT(value) ? AS_FLOAT(value) : AS_INT(value);
}

static inline Value numToValue(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

#else

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
#define TRUE_VAL          ((Value){VAL_BOOL, {.boolean = true}})
#define FALSE_VAL         ((Value){VAL_BOOL, {.boolean = false}})
#define NIL_VAL           ((Value){VAL_NIL, {.integer = 0}})
#define INT_VAL(i)        ((Value){VAL_INT, {.integer = i}})
#define FLOAT_VAL(f)      ((Value){VAL_FLOAT, {.float_ = f}})
#define NUMBER_VAL(num)   ((Value){VAL_FLOAT, {.float_ = num}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

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
bool equalValueArray(ValueArray* aArray, ValueArray* bArray);
void freeValueArray(ValueArray* array);
void printValue(Value value);
char* valueToString(Value value);

#endif
