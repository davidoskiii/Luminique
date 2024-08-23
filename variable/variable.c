#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variable.h"
#include "../object/object.h"
#include "../vm/vm.h"
#include "../string/string.h"

Value invokeGetter(Value method, Value receiver) {
  Value getter = callReentrantMethod(receiver, method, NIL_VAL); 
  return getter;
}

Value invokeSetter(Value method, Value receiver, Value value) {
  return callReentrantMethod(OBJ_VAL(receiver), method, value);
}

Value getInstanceProperty(Value receiver, ObjString* name, Obj* obj) {
  Value value;
  if (tableGet(&obj->fields, name, &value)) return value;
  else if (tableGet(&obj->klass->methods, name, &value)) return value;
  else if (tableGet(&obj->klass->getters, name, &value)) return invokeGetter(value, receiver);
  else ABORT_IFNOPROPRETY(receiver, name);
}

Value setInstanceProperty(Value receiver, ObjString* name, Obj* obj, Value value) {
  Value setter;
  bool isSetter = tableGet(&obj->klass->setters, name, &setter);
  if (isSetter) {
    return invokeSetter(setter, receiver, value);
  }

  Value getter;
  bool isGetter = tableGet(&obj->klass->getters, name, &getter);
  if (isGetter) {
    runtimeError("Cannot modify a getter.");
    exit(70);
  }

  Value method;
  if (tableGet(&obj->klass->methods, name, &method)) {
    tableDelete(&obj->klass->methods, name);
  }

  tableSet(&obj->fields, name, value);
  return value;
}

Value getGenericInstanceVariable(Value receiver, ObjString* name) {
  if (!IS_OBJ(receiver)) { 
    runtimeError("Only objects can have properties.");
    exit(70);
  }
  Obj* object = AS_OBJ(receiver);
  switch (object->type) {
    case OBJ_ARRAY: {
      ObjArray* array = (ObjArray*)object;
      if (matchStringName(name, "length", 6)) return (INT_VAL(array->elements.count));
      else return getInstanceProperty(receiver, name, &array->obj);
    }
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      if (matchStringName(name, "receiver", 8) == 0) return (OBJ_VAL(bound->receiver));
      else if (matchStringName(name, "method", 6)) return (OBJ_VAL(bound->method));
      else return getInstanceProperty(receiver, name, &bound->obj);
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      if (matchStringName(name, "name", 4)) return (OBJ_VAL(closure->function->name));
      else if (matchStringName(name, "arity", 5)) return (INT_VAL(closure->function->arity));
      else return getInstanceProperty(receiver, name, &closure->obj);
    }
    case OBJ_DICTIONARY: {
      ObjDictionary* dictionary = (ObjDictionary*)object;
      Value value;
      if (matchStringName(name, "length", 6)) return (INT_VAL(dictionary->count));
      else return getInstanceProperty(receiver, name, &dictionary->obj);
    }
    case OBJ_ENTRY: {
      ObjEntry* entry = (ObjEntry*)object;
      if (matchStringName(name, "key", 3)) return (entry->key);
      else if (matchStringName(name, "value", 5)) return (entry->value);
      else return getInstanceProperty(receiver, name, &entry->obj);
    }
    case OBJ_EXCEPTION: {
      ObjException* exception = (ObjException*)object;
      if (matchStringName(name, "message", 7)) return (OBJ_VAL(exception->message));
      else if (matchStringName(name, "stacktrace", 10)) return (OBJ_VAL(exception->stacktrace));
      else return getInstanceProperty(receiver, name, &exception->obj);
    }
    case OBJ_FILE: {
      ObjFile* file = (ObjFile*)object;
      if (matchStringName(name, "name", 4)) return (OBJ_VAL(file->name));
      else if (matchStringName(name, "mode", 4)) return (OBJ_VAL(file->mode));
      else if (matchStringName(name, "isOpen", 6)) return (BOOL_VAL(file->isOpen));
      else return getInstanceProperty(receiver, name, &file->obj);
    }
    case OBJ_GENERATOR: {
      ObjGenerator* generator = (ObjGenerator*)object;
      if (matchStringName(name, "state", 5)) return (INT_VAL(generator->state));
      else if (matchStringName(name, "value", 5)) return (generator->value);
      else if (matchStringName(name, "outer", 5)) return (generator->outer != NULL ? OBJ_VAL(generator->outer) : NIL_VAL);
      else return getInstanceProperty(receiver, name, &generator->obj);
    }
    case OBJ_METHOD: {
      ObjMethod* method = (ObjMethod*)object;
      if (matchStringName(name, "name", 4)) return (OBJ_VAL(method->closure->function->name));
      else if (matchStringName(name, "arity", 5)) return (INT_VAL(method->closure->function->arity));
      else if (matchStringName(name, "behavior", 8)) return (OBJ_VAL(method->behavior));
      else return getInstanceProperty(receiver, name, &method->obj);
    }
    case OBJ_NODE: {
      ObjNode* node = (ObjNode*)object;
      if (matchStringName(name, "element", 7)) return (node->element);
      else if (matchStringName(name, "prev", 4)) return (OBJ_VAL(node->prev));
      else if (matchStringName(name, "next", 4)) return (OBJ_VAL(node->next));
      else return getInstanceProperty(receiver, name, &node->obj);
    }
    case OBJ_WINDOW: {
      ObjWindow* window = (ObjWindow*)object;
      if (matchStringName(name, "width", 5)) return (INT_VAL(window->width));
      else if (matchStringName(name, "height", 6)) return (INT_VAL(window->height));
      else if (matchStringName(name, "title", 5)) return (OBJ_VAL(copyString(window->title, strlen(window->title))));
      else return getInstanceProperty(receiver, name, &window->obj);
    }
    case OBJ_EVENT: {
      ObjEvent* event = (ObjEvent*)object;
      if (matchStringName(name, "type", 5)) return (INT_VAL(event->info->eventType));
      else if (matchStringName(name, "keyCode", 6)) return (INT_VAL(event->info->keyCode));
      else if (matchStringName(name, "quit", 5)) return (BOOL_VAL(event->info->quit));
      else return getInstanceProperty(receiver, name, &event->obj);
    }
    case OBJ_PROMISE: {
      ObjPromise* promise = (ObjPromise*)object;
      if (matchStringName(name, "state", 5)) return (INT_VAL(promise->state));
      else if (matchStringName(name, "value", 5)) return (promise->value);
      else if (matchStringName(name, "id", 2)) return (INT_VAL(promise->id));
      else return getInstanceProperty(receiver, name, &promise->obj);
    }
    case OBJ_RANGE: {
      ObjRange* range = (ObjRange*)object;
      if (matchStringName(name, "from", 4)) return (INT_VAL(range->from));
      else if (matchStringName(name, "to", 2)) return (INT_VAL(range->to));
      else if (matchStringName(name, "length", 6)) return (INT_VAL(abs(range->to - range->from) + 1));
      else return getInstanceProperty(receiver, name, &range->obj);
    }
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      if (matchStringName(name, "length", 6)) return (INT_VAL(string->length));
      else return getInstanceProperty(receiver, name, &string->obj);
    }
    case OBJ_TIMER: { 
      ObjTimer* timer = (ObjTimer*)object;
      if (matchStringName(name, "id", 2)) return (INT_VAL(timer->id));
      else if (matchStringName(name, "isRunning", 9)) return (BOOL_VAL(timer->isRunning));
      else return getInstanceProperty(receiver, name, &timer->obj);
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
        exit(70);
      }
      else return setInstanceProperty(receiver, name, &array->obj, value);
    }
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      if (matchStringName(name, "receiver", 8)) bound->receiver = value;
      else if (matchStringName(name, "method", 6) && (IS_NATIVE_METHOD(value) || IS_CLOSURE(value))) bound->method = value;
      else return setInstanceProperty(receiver, name, &bound->obj, value);
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      if (matchStringName(name, "name", 4) || matchStringName(name, "arity", 5)) {
        runtimeError("Cannot set property %s on Object Function.", name->chars);
        exit(70);
      }
      else return setInstanceProperty(receiver, name, &closure->obj, value);
    }
    case OBJ_DICTIONARY: {
      ObjDictionary* dictionary = (ObjDictionary*)object;
      if (matchStringName(name, "length", 6)) {
        runtimeError("Cannot set property length on Object Dictionary.");
        exit(70);
      }
      else return setInstanceProperty(receiver, name, &dictionary->obj, value);
    }
    case OBJ_ENTRY: {
      ObjEntry* entry = (ObjEntry*)object;
      if (matchStringName(name, "key", 3)) {
        runtimeError("Cannot set property key on Object Entry.");
        exit(70);
      }
      else if (matchStringName(name, "value", 5)) {
        entry->value = value;
        return value;
      }
      else return setInstanceProperty(receiver, name, &entry->obj, value);
    }
    case OBJ_EXCEPTION: {
      ObjException* exception = (ObjException*)object;
      if (matchStringName(name, "message", 7) && IS_STRING(value)) exception->message = AS_STRING(value);
      else if (matchStringName(name, "stacktrace", 10) && IS_ARRAY(value)) exception->stacktrace = AS_ARRAY(value);
      else return setInstanceProperty(receiver, name, &exception->obj, value);
    }
    case OBJ_FILE: {
      ObjFile* file = (ObjFile*)object;
      if (matchStringName(name, "name", 4) && IS_STRING(value)) file->name = AS_STRING(value);
      else if (matchStringName(name, "mode", 4) && IS_STRING(value)) file->mode = AS_STRING(value);
      else if (matchStringName(name, "isOpen", 6)) {
        runtimeError("Cannot set property isOpen on Object File.");
        exit(70);
      }
      else return setInstanceProperty(receiver, name, &file->obj, value);
    }
    case OBJ_GENERATOR: {
      ObjGenerator* generator = (ObjGenerator*)object;
      if (matchStringName(name, "state", 5) && IS_INT(value)) generator->state = AS_INT(value);
      else if (matchStringName(name, "value", 5)) generator->value = value;
      if (matchStringName(name, "outer", 5) && IS_GENERATOR(value)) generator->outer = AS_GENERATOR(value);
      else return setInstanceProperty(receiver, name, &generator->obj, value);
    }
    case OBJ_METHOD: {
      ObjMethod* method = (ObjMethod*)object;
      if (matchStringName(name, "name", 4) || matchStringName(name, "arity", 5) || matchStringName(name, "behavior", 8)) {
        runtimeError("Cannot set property %s on Object Method.", name->chars);
        exit(70);
      }
      else return setInstanceProperty(receiver, name, &method->obj, value);
    }
    case OBJ_NODE: {
      ObjNode* node = (ObjNode*)object;
      if (matchStringName(name, "element", 7)) node->element = value;
      else if (matchStringName(name, "prev", 4) && IS_NODE(value)) node->prev = AS_NODE(value);
      else if (matchStringName(name, "next", 4) && IS_NODE(value)) node->next = AS_NODE(value);
      else return setInstanceProperty(receiver, name, &node->obj, value);
    }
    case OBJ_WINDOW: {
      ObjWindow* window = (ObjWindow*)object;
      if (matchStringName(name, "width", 5) && IS_INT(value)) window->width = INT_VAL(value);
      else if (matchStringName(name, "height", 6) && IS_INT(value)) window->height = INT_VAL(value);
      else if (matchStringName(name, "title", 5) && IS_STRING(value)) window->title = AS_STRING(value)->chars;
      else return setInstanceProperty(receiver, name, &window->obj, value);
    }
    case OBJ_EVENT: {
      ObjEvent* event = (ObjEvent*)object;
      if (matchStringName(name, "type", 5)) return (INT_VAL(event->info->eventType));
      else if (matchStringName(name, "keyCode", 6)) return (INT_VAL(event->info->keyCode));
      else if (matchStringName(name, "quit", 5)) return (BOOL_VAL(event->info->quit));
      else return setInstanceProperty(receiver, name, &event->obj, value);
    }
    case OBJ_PROMISE: {
      ObjPromise* promise = (ObjPromise*)object;
      if (matchStringName(name, "state", 5) && IS_INT(value)) promise->state = AS_INT(value);
      else if (matchStringName(name, "value", 5)) promise->value = value;
      else if (matchStringName(name, "id", 2)) { 
        runtimeError("Cannot set property id on Object Promise.");
        exit(70);
      }
      else return setInstanceProperty(receiver, name, &promise->obj, value);
    }
    case OBJ_RANGE: {
      ObjRange* range = (ObjRange*)object;
      if (matchStringName(name, "from", 4) && IS_INT(value)) range->from = AS_INT(value);
      else if (matchStringName(name, "to", 4) && IS_INT(value)) range->to = AS_INT(value);
      else return setInstanceProperty(receiver, name, &range->obj, value);
    }
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      if (matchStringName(name, "length", 6)) {
        runtimeError("Cannot set property length on Object String.");
        exit(70);
      }
      else return setInstanceProperty(receiver, name, &string->obj, value);
    }
    case OBJ_TIMER: {
      ObjTimer* timer = (ObjTimer*)object;
      if (matchStringName(name, "id", 2) && IS_INT(value)) timer->id = AS_INT(value);
      else if (matchStringName(name, "isRunning", 9)) timer->isRunning = AS_BOOL(value);
      else return setInstanceProperty(receiver, name, &timer->obj, value);
    }
    default: ABORT_IFNOPROPRETY(receiver, name);
  }
}
