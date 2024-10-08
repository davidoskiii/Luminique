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
  } else if (function->name->length == 0 || matchStringName(function->name, "lambda", function->name->length)) return "<function>";
  else return formattedString("<function %s>", function->name->chars)->chars;
}

char* eventToString(ObjEvent* event) {
  const char* eventTypeStr;
  switch (event->event.type) {
    case SDL_QUIT:
      eventTypeStr = "QUIT";
      break;
    case SDL_APP_TERMINATING:
      eventTypeStr = "APP_TERMINATING";
      break;
    case SDL_APP_LOWMEMORY:
      eventTypeStr = "APP_LOWMEMORY";
      break;
    case SDL_APP_WILLENTERBACKGROUND:
      eventTypeStr = "APP_WILLENTERBACKGROUND";
      break;
    case SDL_APP_DIDENTERBACKGROUND:
      eventTypeStr = "APP_DIDENTERBACKGROUND";
      break;
    case SDL_APP_WILLENTERFOREGROUND:
      eventTypeStr = "APP_WILLENTERFOREGROUND";
      break;
    case SDL_APP_DIDENTERFOREGROUND:
      eventTypeStr = "APP_DIDENTERFOREGROUND";
      break;
    case SDL_LOCALECHANGED:
      eventTypeStr = "LOCALECHANGED";
      break;
    case SDL_DISPLAYEVENT:
      eventTypeStr = "DISPLAYEVENT";
      break;
    case SDL_WINDOWEVENT:
      eventTypeStr = "WINDOWEVENT";
      break;
    case SDL_SYSWMEVENT:
      eventTypeStr = "SYSWMEVENT";
      break;
    case SDL_KEYDOWN:
      eventTypeStr = "KEYDOWN";
      break;
    case SDL_KEYUP:
      eventTypeStr = "KEYUP";
      break;
    case SDL_TEXTEDITING:
      eventTypeStr = "TEXTEDITING";
      break;
    case SDL_TEXTINPUT:
      eventTypeStr = "TEXTINPUT";
      break;
    case SDL_KEYMAPCHANGED:
      eventTypeStr = "KEYMAPCHANGED";
      break;
    case SDL_MOUSEMOTION:
      eventTypeStr = "MOUSEMOTION";
      break;
    case SDL_MOUSEBUTTONDOWN:
      eventTypeStr = "MOUSEBUTTONDOWN";
      break;
    case SDL_MOUSEBUTTONUP:
      eventTypeStr = "MOUSEBUTTONUP";
      break;
    case SDL_MOUSEWHEEL:
      eventTypeStr = "MOUSEWHEEL";
      break;
    case SDL_JOYAXISMOTION:
      eventTypeStr = "JOYAXISMOTION";
      break;
    case SDL_JOYBALLMOTION:
      eventTypeStr = "JOYBALLMOTION";
      break;
    case SDL_JOYHATMOTION:
      eventTypeStr = "JOYHATMOTION";
      break;
    case SDL_JOYBUTTONDOWN:
      eventTypeStr = "JOYBUTTONDOWN";
      break;
    case SDL_JOYBUTTONUP:
      eventTypeStr = "JOYBUTTONUP";
      break;
    case SDL_JOYDEVICEADDED:
      eventTypeStr = "JOYDEVICEADDED";
      break;
    case SDL_JOYDEVICEREMOVED:
      eventTypeStr = "JOYDEVICEREMOVED";
      break;
    case SDL_CONTROLLERAXISMOTION:
      eventTypeStr = "CONTROLLERAXISMOTION";
      break;
    case SDL_CONTROLLERBUTTONDOWN:
      eventTypeStr = "CONTROLLERBUTTONDOWN";
      break;
    case SDL_CONTROLLERBUTTONUP:
      eventTypeStr = "CONTROLLERBUTTONUP";
      break;
    case SDL_CONTROLLERDEVICEADDED:
      eventTypeStr = "CONTROLLERDEVICEADDED";
      break;
    case SDL_CONTROLLERDEVICEREMOVED:
      eventTypeStr = "CONTROLLERDEVICEREMOVED";
      break;
    case SDL_CONTROLLERDEVICEREMAPPED:
      eventTypeStr = "CONTROLLERDEVICEREMAPPED";
      break;
    case SDL_CONTROLLERTOUCHPADDOWN:
      eventTypeStr = "CONTROLLERTOUCHPADDOWN";
      break;
    case SDL_CONTROLLERTOUCHPADMOTION:
      eventTypeStr = "CONTROLLERTOUCHPADMOTION";
      break;
    case SDL_CONTROLLERTOUCHPADUP:
      eventTypeStr = "CONTROLLERTOUCHPADUP";
      break;
    case SDL_CONTROLLERSENSORUPDATE:
      eventTypeStr = "CONTROLLERSENSORUPDATE";
      break;
    case SDL_FINGERDOWN:
      eventTypeStr = "FINGERDOWN";
      break;
    case SDL_FINGERUP:
      eventTypeStr = "FINGERUP";
      break;
    case SDL_FINGERMOTION:
      eventTypeStr = "FINGERMOTION";
      break;
    case SDL_DOLLARGESTURE:
      eventTypeStr = "DOLLARGESTURE";
      break;
    case SDL_DOLLARRECORD:
      eventTypeStr = "DOLLARRECORD";
      break;
    case SDL_MULTIGESTURE:
      eventTypeStr = "MULTIGESTURE";
      break;
    case SDL_CLIPBOARDUPDATE:
      eventTypeStr = "CLIPBOARDUPDATE";
      break;
    case SDL_DROPFILE:
      eventTypeStr = "DROPFILE";
      break;
    case SDL_DROPTEXT:
      eventTypeStr = "DROPTEXT";
      break;
    case SDL_DROPBEGIN:
      eventTypeStr = "DROPBEGIN";
      break;
    case SDL_DROPCOMPLETE:
      eventTypeStr = "DROPCOMPLETE";
      break;
    case SDL_AUDIODEVICEADDED:
      eventTypeStr = "AUDIODEVICEADDED";
      break;
    case SDL_AUDIODEVICEREMOVED:
      eventTypeStr = "AUDIODEVICEREMOVED";
      break;
    case SDL_SENSORUPDATE:
      eventTypeStr = "SENSORUPDATE";
      break;
    case SDL_RENDER_TARGETS_RESET:
      eventTypeStr = "RENDER_TARGETS_RESET";
      break;
    case SDL_RENDER_DEVICE_RESET:
      eventTypeStr = "RENDER_DEVICE_RESET";
      break;
    case SDL_POLLSENTINEL:
      eventTypeStr = "POLLSENTINEL";
      break;
    case SDL_USEREVENT:
      eventTypeStr = "USEREVENT";
      break;
    case SDL_LASTEVENT:
      eventTypeStr = "LASTEVENT";
      break;
    default:
      eventTypeStr = "UNKNOWN";
      break;
  }

  return formattedString("<Event type: %s, KeyCode: %d, Quit: %s>", eventTypeStr, event->info->keyCode, event->info->quit ? "true" : "false")->chars;
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
        return formattedString("<%s window>", AS_WINDOW(value)->title->chars)->chars;
        break;
      case OBJ_SOUND:
        return formattedString("<sound %s | %d ms>", AS_SOUND(value)->path->chars, AS_SOUND(value)->duration)->chars;
        break;
      case OBJ_EVENT:
        return eventToString(AS_EVENT(value));
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
      case OBJ_BOUND_METHOD: { 
        ObjBoundMethod* boundMethod = AS_BOUND_METHOD(value);
        if (IS_NATIVE_METHOD(boundMethod->method)) return formattedString("<bound method %s::%s>", 
            AS_OBJ(boundMethod->receiver)->klass->name->chars, AS_NATIVE_METHOD(boundMethod->method)->name->chars)->chars;
        else return formattedString("<bound method %s::%s>", AS_OBJ(boundMethod->receiver)->klass->name->chars, AS_CLOSURE(boundMethod->method)->function->name->chars)->chars;
        break;
      }
      case OBJ_METHOD: { 
        ObjMethod* method = AS_METHOD(value);
        return formattedString("<method %s::%s>", method->behavior->name->chars, method->closure->function->name->chars)->chars;
        break;
      }
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
      case OBJ_GENERATOR: { 
        ObjFunction* function = AS_GENERATOR(value)->frame->closure->function;
        return formattedString("<generator %s>", (function->name == NULL) ? "script" : function->name->chars)->chars;
        break;
      }
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
