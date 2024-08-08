#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variable.h"
#include "../object/object.h"
#include "../vm/vm.h"
#include "../string/string.h"

Value getGenericInstanceVariable(Value receiver, ObjString* name) {
  if (!IS_OBJ(receiver)) runtimeError("Only objects can have properties.");
  Obj* object = AS_OBJ(receiver);
  switch (object->type) {
    case OBJ_ARRAY: {
      ObjArray* array = (ObjArray*)object;
      if (matchStringName(name, "length", 6)) return (INT_VAL(array->elements.count));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      if (matchStringName(name, "receiver", 8) == 0) return (OBJ_VAL(bound->receiver));
      else if (matchStringName(name, "method", 6)) return (OBJ_VAL(bound->method));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      if (matchStringName(name, "name", 4)) return (OBJ_VAL(closure->function->name));
      else if (matchStringName(name, "arity", 5)) return (INT_VAL(closure->function->arity));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_DICTIONARY: {
      ObjDictionary* dictionary = (ObjDictionary*)object;
      if (matchStringName(name, "length", 6)) return (INT_VAL(dictionary->count));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_ENTRY: {
      ObjEntry* entry = (ObjEntry*)object;
      if (matchStringName(name, "key", 3)) return (entry->key);
      else if (matchStringName(name, "value", 5)) return (entry->value);
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_EXCEPTION: {
      ObjException* exception = (ObjException*)object;
      if (matchStringName(name, "message", 7)) return (OBJ_VAL(exception->message));
      else if (matchStringName(name, "stacktrace", 10)) return (OBJ_VAL(exception->stacktrace));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_FILE: {
      ObjFile* file = (ObjFile*)object;
      if (matchStringName(name, "name", 4)) return (OBJ_VAL(file->name));
      else if (matchStringName(name, "mode", 4)) return (OBJ_VAL(file->mode));
      else if (matchStringName(name, "isOpen", 6)) return (BOOL_VAL(file->isOpen));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_GENERATOR: {
      ObjGenerator* generator = (ObjGenerator*)object;
      if (matchStringName(name, "state", 5)) return (INT_VAL(generator->state));
      else if (matchStringName(name, "value", 5)) return (generator->value);
      else if (matchStringName(name, "outer", 5)) return (generator->outer != NULL ? OBJ_VAL(generator->outer) : NIL_VAL);
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_METHOD: {
      ObjMethod* method = (ObjMethod*)object;
      if (matchStringName(name, "name", 4)) return (OBJ_VAL(method->closure->function->name));
      else if (matchStringName(name, "arity", 5)) return (INT_VAL(method->closure->function->arity));
      else if (matchStringName(name, "behavior", 8)) return (OBJ_VAL(method->behavior));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_NODE: {
      ObjNode* node = (ObjNode*)object;
      if (matchStringName(name, "element", 7)) return (node->element);
      else if (matchStringName(name, "prev", 4)) return (OBJ_VAL(node->prev));
      else if (matchStringName(name, "next", 4)) return (OBJ_VAL(node->next));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_PROMISE: {
      ObjPromise* promise = (ObjPromise*)object;
      if (matchStringName(name, "state", 5)) return (INT_VAL(promise->state));
      else if (matchStringName(name, "value", 5)) return (promise->value);
      else if (matchStringName(name, "id", 2)) return (INT_VAL(promise->id));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_RANGE: {
      ObjRange* range = (ObjRange*)object;
      if (matchStringName(name, "from", 4)) return (INT_VAL(range->from));
      else if (matchStringName(name, "to", 2)) return (INT_VAL(range->to));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      if (matchStringName(name, "length", 6)) return (INT_VAL(string->length));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_TIMER: { 
      ObjTimer* timer = (ObjTimer*)object;
      if (matchStringName(name, "id", 2)) return (INT_VAL(timer->id));
      else if (matchStringName(name, "isRunning", 9)) return (BOOL_VAL(timer->isRunning));
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    default: ABORT_IFNOPROPRETY(receiver, name);
  }
}

Value setGenericInstanceVariable(Value receiver, ObjString* name, Value value) {
  if (!IS_OBJ(receiver)) runtimeError("Only objects can have properties.");
  Obj* object = AS_OBJ(receiver);
  switch (object->type) {
    case OBJ_ARRAY: {
      ObjArray* array = (ObjArray*)object;
      if (matchStringName(name, "length", 6)) {
        runtimeError("Cannot set property length on Object Array.");
      }
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      if (matchStringName(name, "receiver", 8)) bound->receiver = value;
      else if (matchStringName(name, "method", 6) && (IS_NATIVE_METHOD(value) || IS_CLOSURE(value))) bound->method = value;
      else ABORT_IFNOPROPRETY(receiver, name);
      return value;
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      if (matchStringName(name, "name", 4) || matchStringName(name, "arity", 5)) {
        runtimeError("Cannot set property %s on Object Function.", name->chars);
      }
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_DICTIONARY: {
      ObjDictionary* dictionary = (ObjDictionary*)object;
      if (matchStringName(name, "length", 6)) {
        runtimeError("Cannot set property length on Object Dictionary.");
      }
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_ENTRY: {
      ObjEntry* entry = (ObjEntry*)object;
      if (matchStringName(name, "key", 3)) {
        runtimeError("Cannot set property key on Object Entry.");
      }
      else if (matchStringName(name, "value", 5)) {
        entry->value = value;
        return value;
      }
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_EXCEPTION: {
      ObjException* exception = (ObjException*)object;
      if (matchStringName(name, "message", 7) && IS_STRING(value)) exception->message = AS_STRING(value);
      else if (matchStringName(name, "stacktrace", 10) && IS_ARRAY(value)) exception->stacktrace = AS_ARRAY(value);
      else ABORT_IFNOPROPRETY(receiver, name);
      return value;
    }
    case OBJ_FILE: {
      ObjFile* file = (ObjFile*)object;
      if (matchStringName(name, "name", 4) && IS_STRING(value)) file->name = AS_STRING(value);
      else if (matchStringName(name, "mode", 4) && IS_STRING(value)) file->mode = AS_STRING(value);
      else if (matchStringName(name, "isOpen", 6)) {
        runtimeError("Cannot set property isOpen on Object File.");
        exit(70);
      }
      else ABORT_IFNOPROPRETY(receiver, name);
      return value;
    }
    case OBJ_GENERATOR: {
      ObjGenerator* generator = (ObjGenerator*)object;
      if (matchStringName(name, "state", 5) && IS_INT(value)) generator->state = AS_INT(value);
      else if (matchStringName(name, "value", 5)) generator->value = value;
      if (matchStringName(name, "outer", 5) && IS_GENERATOR(value)) generator->outer = AS_GENERATOR(value);
      else ABORT_IFNOPROPRETY(receiver, name);
      return value;
    }
    case OBJ_METHOD: {
      ObjMethod* method = (ObjMethod*)object;
      if (matchStringName(name, "name", 4) || matchStringName(name, "arity", 5) || matchStringName(name, "behavior", 8)) {
        runtimeError("Cannot set property %s on Object Method.", name->chars);
        exit(70);
      }
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_NODE: {
      ObjNode* node = (ObjNode*)object;
      if (matchStringName(name, "element", 7)) node->element = value;
      else if (matchStringName(name, "prev", 4) && IS_NODE(value)) node->prev = AS_NODE(value);
      else if (matchStringName(name, "next", 4) && IS_NODE(value)) node->next = AS_NODE(value);
      else ABORT_IFNOPROPRETY(receiver, name);
      return value;
    }
    case OBJ_PROMISE: {
      ObjPromise* promise = (ObjPromise*)object;
      if (matchStringName(name, "state", 5) && IS_INT(value)) promise->state = AS_INT(value);
      else if (matchStringName(name, "value", 5)) promise->value = value;
      else if (matchStringName(name, "id", 2)) { 
        runtimeError("Cannot set property id on Object Promise.");
        exit(70);
      }
      else ABORT_IFNOPROPRETY(receiver, name);
      return value;
    }
    case OBJ_RANGE: {
      ObjRange* range = (ObjRange*)object;
      if (matchStringName(name, "from", 4) && IS_INT(value)) range->from = AS_INT(value);
      else if (matchStringName(name, "to", 4) && IS_INT(value)) range->to = AS_INT(value);
      else ABORT_IFNOPROPRETY(receiver, name);
      return value;
    }
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      if (matchStringName(name, "length", 6)) {
        runtimeError("Cannot set property length on Object String.");
      }
      else ABORT_IFNOPROPRETY(receiver, name);
    }
    case OBJ_TIMER: {
      ObjTimer* timer = (ObjTimer*)object;
      if (matchStringName(name, "id", 2) && IS_INT(value)) timer->id = AS_INT(value);
      else if (matchStringName(name, "isRunning", 9)) timer->isRunning = AS_BOOL(value);
      else ABORT_IFNOPROPRETY(receiver, name);
      return value;
    }
    default: ABORT_IFNOPROPRETY(receiver, name);
  }
}
