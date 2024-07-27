#include <stdio.h>
#include <string.h>

#include "../object/object.h"
#include "../collection/collection.h"
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

static char* functionToString(ObjFunction* function) {
  if (function->name == NULL) {
    return "<script>";
  } else if (function->name->length == 0) {
    return "<function>";
  } else return formattedString("<function %s>", function->name->chars)->chars;
}

char* valueToString(Value value) {
  if (IS_NIL(value)) return "nil";
  else if (IS_BOOL(value)) return AS_BOOL(value) ? "true" : "false";
  else if (IS_INT(value)) {
    char* chars = ALLOCATE(char, (size_t)11);
    int length = snprintf(chars, 11, "%d", AS_INT(value));
    return chars;
  }
  else if (IS_FLOAT(value)) {
    char* chars = ALLOCATE(char, (size_t)24);
    int length = snprintf(chars, 24, "%.14g", AS_FLOAT(value));
    return chars;
  } else if (IS_OBJ(value)) {
    switch (OBJ_TYPE(value)) {
      case OBJ_ARRAY:
        return arrayToString(AS_ARRAY(value))->chars;
        break;
      case OBJ_NAMESPACE:
        return formattedString("<namespace %s>", AS_NAMESPACE(value)->fullName->chars)->chars;
        break;
      case OBJ_MODULE:
        return formattedString("<module %s>", AS_MODULE(value)->path->chars)->chars;
        break;
      case OBJ_NODE:
        return "<node>";
        break;
      case OBJ_RANGE:
        return formattedString("%d...%d", AS_RANGE(value)->from, AS_RANGE(value)->to)->chars;
        break;
      case OBJ_WINDOW:
        return formattedString("<%s window>", AS_WINDOW(value)->title)->chars;
        break;
      case OBJ_FILE:
        return formattedString("<file \"%s\">", AS_FILE(value)->name->chars)->chars;
        break;
      case OBJ_RECORD:
        return "<record>";
        break;
      case OBJ_DICTIONARY: 
        return dictToString(AS_DICTIONARY(value))->chars;
        break;
      case OBJ_BOUND_METHOD:
        return functionToString(AS_BOUND_METHOD(value)->method->function);
        break;
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(value);
        if (klass->namespace_->isRoot) return formattedString("<class %s>", klass->name->chars)->chars;
        else return formattedString("<class %s::%s>", klass->namespace_->fullName->chars, klass->name->chars)->chars;
        break;
      }
      case OBJ_CLOSURE:
        return functionToString(AS_CLOSURE(value)->function);
        break;
      case OBJ_FRAME: 
        return formattedString("<frame: %s>", AS_FRAME(value)->closure->function->name->chars)->chars;
        break;
      case OBJ_FUNCTION:
        return functionToString(AS_FUNCTION(value));
        break;
      case OBJ_GENERATOR:
        return formattedString("<generator %s>", AS_GENERATOR(value)->frame->closure->function->name->chars)->chars;
        break;
      case OBJ_ENUM:
        return formattedString("<enum %s>", AS_ENUM(value)->name->chars)->chars;
        break;
      case OBJ_INSTANCE:
        #ifdef DEBUG_FORMAT
        if (objMethodExists(value, "__format__")) {
          Value method = getObjMethod(value, "__format__");
          Value str = callReentrantMethod(value, method);
          do {
            Value toStringMethod = getObjMethod(str, "__str__");
            str = callReentrantMethod(str, toStringMethod);
          } while (!IS_STRING(str));
          return AS_CSTRING(str);
        } else 
        #endif
        return formattedString("<object %s>", AS_OBJ(value)->klass->name->chars)->chars;
        break;
      case OBJ_EXCEPTION: {
        return formattedString("<Exception %s - %s>", AS_EXCEPTION(value)->obj.klass->name->chars, AS_EXCEPTION(value)->message->chars)->chars;
        break;
      }
      case OBJ_NATIVE_FUNCTION:
        return formattedString("<native function %s>", AS_NATIVE_FUNCTION(value)->name->chars)->chars;
        break;
      case OBJ_NATIVE_METHOD:
        return formattedString("<native method %s::%s>", AS_NATIVE_METHOD(value)->klass->name->chars, AS_NATIVE_METHOD(value)->name->chars)->chars;
        break;
      case OBJ_STRING:
        return AS_CSTRING(value);
        break;
      case OBJ_UPVALUE:
        return "<upvalue>";
        break;
      case OBJ_PROMISE:
        return formattedString("<promise: %d>", AS_PROMISE(value)->id)->chars;
        break;
      case OBJ_TIMER:
        return formattedString("<timer: %d>", AS_TIMER(value)->id)->chars;
        break;
      default:
        return "";
        break;
    }
  } else return "undefined";
}
