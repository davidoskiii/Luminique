#include <stdlib.h>

#include "memory.h"
#include "../compiler/compiler.h"
#include "../loop/loop.h"
#include "../object/object.h"
#include "../vm/vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "../debug/debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  vm.bytesAllocated += newSize - oldSize;
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif

    if (vm.bytesAllocated > vm.nextGC) {
      collectGarbage();
    }
  }

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);
  if (result == NULL) exit(1);
  return result;
}

void markObject(Obj* object) {
  if (object == NULL) return;
  if (object->isMarked) return;

// #ifdef DEBUG_LOG_GC
//  printf("%p mark ", (void*)object);
//  printValue(OBJ_VAL(object));
//  printf("\n");
// #endif

  object->isMarked = true;


  if (vm.grayCapacity < vm.grayCount + 1) {
    vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
    vm.grayStack = (Obj**)realloc(vm.grayStack,
                                  sizeof(Obj*) * vm.grayCapacity);

    if (vm.grayStack == NULL) exit(1);
  }

  vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value) {
  if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void markArray(ValueArray* array) {
  for (int i = 0; i < array->count; i++) {
    markValue(array->values[i]);
  }
}

static void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif

  switch (object->type) {
    case OBJ_ARRAY: {
      markArray(&((ObjArray*)object)->elements);
      break;
    }
    case OBJ_EXCEPTION: {
      ObjException* exception = (ObjException*)object;
      markObject((Obj*)exception->message);
      markObject((Obj*)exception->stacktrace);
      break;
    }
    case OBJ_NODE: {
      ObjNode* node = (ObjNode*)object;
      markValue(node->element);
      markObject((Obj*)node->prev);
      markObject((Obj*)node->next);
      break;
    }
    case OBJ_DICTIONARY: {
      ObjDictionary* dict = (ObjDictionary*)object;
      for (int i = 0; i < dict->capacity; i++) {
        ObjEntry* entry = &dict->entries[i];
        markObject((Obj*)entry);
      }
    }
    case OBJ_WINDOW: {
      ObjWindow* window = (ObjWindow*)object;
      if (window->window != NULL) markValue(OBJ_VAL(window->window));
      markObject((Obj*)window->title);
      break;
    }
    case OBJ_ENTRY: {
      ObjEntry* entry = (ObjEntry*)object;
      markValue(entry->key);
      markValue(entry->value);
      break;
    }
    case OBJ_FILE: {
      ObjFile* file = (ObjFile*)object;
      markObject((Obj*)file->name);
      markObject((Obj*)file->mode);
      break;
    }
    case OBJ_MODULE: {
      ObjModule* module = (ObjModule*)object;
      markObject((Obj*)module->path);
      if (module->closure != NULL) markObject((Obj*)module->closure);
      markTable(&module->values);
      break;
    }
    case OBJ_NAMESPACE: {
      ObjNamespace* namespace = (ObjNamespace*)object;
      markObject((Obj*)namespace->shortName);
      if (namespace->enclosing != NULL) markObject((Obj*)namespace->enclosing);
      markTable(&namespace->values);
      markTable(&namespace->compilerValues);
      markTable(&namespace->globals);
      markTable(&namespace->compilerGlobals);
      break;
    }
    case OBJ_PROMISE: {
      ObjPromise* promise = (ObjPromise*)object;
      markValue(promise->value);
      markObject((Obj*)promise->captures);
      markObject((Obj*)promise->exception);
      markValue(promise->executor);
      markArray(&promise->handlers);
      break;
    }
    case OBJ_RECORD:
      break;
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      markValue(bound->receiver);
      markObject((Obj*)bound->method);
      break;
    }
    case OBJ_METHOD: {
      ObjMethod* method = (ObjMethod*)object;
      markObject((Obj*)method->behavior);
      markObject((Obj*)method->closure);
      break;
    }
    case OBJ_UPVALUE:
      markValue(((ObjUpvalue*)object)->closed);
      break;
    case OBJ_CLASS: {
      ObjClass* klass = (ObjClass*)object;
      markObject((Obj*)klass->name);
      if (klass->superclass != NULL) markObject((Obj*)klass->superclass);
      markObject((Obj*)klass->namespace_);
      markTable(&klass->fields);
      markTable(&klass->methods);
      markTable(&klass->getters);
      markTable(&klass->setters);
      break;
    }
    case OBJ_ENUM: {
      ObjEnum* enum_ = (ObjEnum*)object;
      markObject((Obj*)enum_->name);
      markTable(&enum_->values);
      break;
    }
    case OBJ_TIMER: { 
      ObjTimer* timer = (ObjTimer*)object;
      if (timer->timer != NULL && timer->timer->data != NULL) {
        TimerData* data = (TimerData*)timer->timer->data;
        markValue(data->receiver);
        markObject((Obj*)data->closure);
      }
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      markObject((Obj*)closure->function);
      for (int i = 0; i < closure->upvalueCount; i++) {
        markObject((Obj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FRAME: {
      ObjFrame* frame = (ObjFrame*)object;
      markObject((Obj*)frame->closure);
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      markObject((Obj*)function->name);
      markArray(&function->chunk.constants);
      break;
    }
    case OBJ_GENERATOR: {
      ObjGenerator* generator = (ObjGenerator*)object;
      markObject((Obj*)generator->frame);
      markObject((Obj*)generator->outer);
      markObject((Obj*)generator->inner);
      markValue(generator->value);
      break;
    }
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      markObject((Obj*)object->klass);
      markTable(&instance->fields);
      break;
    }
    case OBJ_NATIVE_FUNCTION: {
      ObjNativeFunction* nativeFunction = (ObjNativeFunction*)object;
      markObject((Obj*)nativeFunction->name);
      break;
    }
    case OBJ_NATIVE_METHOD: {
      ObjNativeMethod* nativeMethod = (ObjNativeMethod*)object;
      markObject((Obj*)nativeMethod->klass);
      markObject((Obj*)nativeMethod->name);
      break;
    }
    case OBJ_STRING:
      break;
  }
}

static void freeObject(Obj* object) {
// #ifdef DEBUG_LOG_GC
//  printf("%p free type %d\n", (void*)object, object->type);
// #endif

  switch (object->type) {
    case OBJ_ARRAY: {
      freeValueArray(&((ObjArray*)object)->elements);
      FREE(ObjArray, object);
      break;
    }
    case OBJ_EXCEPTION: {
      FREE(ObjException, object);
      break;
    }
    case OBJ_DICTIONARY: {
      ObjDictionary* dict = (ObjDictionary*)object;
      FREE_ARRAY(ObjEntry, dict->entries, dict->capacity);
      FREE(ObjDictionary, object);
      break;
    }
    case OBJ_RANGE: {
      FREE(ObjRange, object);
      break;
    }
    case OBJ_NODE: {
      FREE(ObjNode, object);
      break;
    }
    case OBJ_ENTRY: {
      FREE(ObjEntry, object);
      break;
    }
    case OBJ_WINDOW: {
      ObjWindow* window = (ObjWindow*)object;
      if (window->window != NULL) SDL_DestroyWindow(window->window);
      FREE_ARRAY(char, window->title, strlen(window->title));
      FREE(ObjWindow, object);
      break;
    }
    case OBJ_BOUND_METHOD: {
      FREE(ObjBoundMethod, object);
      break;
    }
    case OBJ_METHOD: {
      FREE(ObjMethod, object);
      break;
    }
    case OBJ_CLASS: {
      FREE(ObjClass, object);
      ObjClass* klass = (ObjClass*)object;
      freeTable(&klass->fields);
      freeTable(&klass->methods);
      freeTable(&klass->getters);
      freeTable(&klass->setters);
      break;
    }
    case OBJ_ENUM: {
      FREE(ObjEnum, object);
      ObjEnum* enum_ = (ObjEnum*)object;
      freeTable(&enum_->values);
      break;
    }
    case OBJ_TIMER: { 
      ObjTimer* timer = (ObjTimer*)object;
      FREE(ObjTimer, object);
      break;
    }
    case OBJ_MODULE: {
      ObjModule* module = (ObjModule*)object;
      freeTable(&module->values);
      FREE(ObjModule, object);
      break;
    }             
    case OBJ_NAMESPACE: { 
      ObjNamespace* namespace = (ObjNamespace*)object;
      freeTable(&namespace->values);
      freeTable(&namespace->compilerValues);
      freeTable(&namespace->globals);
      freeTable(&namespace->compilerGlobals);
      FREE(ObjNamespace, object);
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
      FREE(ObjClosure, object);
      break;
    }
    case OBJ_FILE: {
      ObjFile* file = (ObjFile*)object;
      if (file->fsStat != NULL) {
        uv_fs_req_cleanup(file->fsStat);
        free(file->fsStat);
      }
      if (file->fsOpen != NULL) {
        uv_fs_req_cleanup(file->fsOpen);
        free(file->fsOpen);
      }
      if (file->fsRead != NULL) {
        uv_fs_req_cleanup(file->fsRead);
        free(file->fsRead);
      }
      if (file->fsWrite != NULL) {
        uv_fs_req_cleanup(file->fsWrite);
        free(file->fsWrite);
      }
      FREE(ObjFile, object);
      break;
    }
    case OBJ_PROMISE: {
      ObjPromise* promise = (ObjPromise*)object;
      freeValueArray(&promise->handlers);
      FREE(ObjPromise, object);
      break;
    }
    case OBJ_RECORD: {
      ObjRecord* record = (ObjRecord*)object;
      if (record->data != NULL) free(record->data);
      FREE(ObjRecord, object);
      break;
    }
    case OBJ_FRAME: {
      FREE(ObjFrame, object);
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      freeChunk(&function->chunk);
      FREE(ObjFunction, object);
      break;
    }
    case OBJ_GENERATOR: {
      FREE(ObjGenerator, object);
      break; 
    }
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      freeTable(&instance->fields);
      FREE(ObjInstance, object);
      break;
    }
    case OBJ_NATIVE_FUNCTION:
      FREE(ObjNativeFunction, object);
      break;
    case OBJ_NATIVE_METHOD:
      FREE(ObjNativeMethod, object);
      break;
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
    }
  }
}

static void markRoots() {
  for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
    markValue(*slot);
  }

  for (int i = 0; i < vm.frameCount; i++) {
    markObject((Obj*)vm.frames[i].closure);
  }

  for (ObjUpvalue* upvalue = vm.openUpvalues;
       upvalue != NULL;
       upvalue = upvalue->next) {
    markObject((Obj*)upvalue);
  }

  markTable(&vm.currentNamespace->globals);
  markTable(&vm.namespaces);
  markTable(&vm.modules);
  markCompilerRoots();
  markObject((Obj*)vm.initString);
  markObject((Obj*)vm.runningGenerator);
}

static void traceReferences() {
  while (vm.grayCount > 0) {
    Obj* object = vm.grayStack[--vm.grayCount];
    blackenObject(object);
  }
}

static void sweep() {
  Obj* previous = NULL;
  Obj* object = vm.objects;
  while (object != NULL) {
    if (object->isMarked) {
      object->isMarked = false;
      previous = object;
      object = object->next;
    } else {
      Obj* unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm.objects = object;
      }

      freeObject(unreached);
    }
  }
}

void collectGarbage() {
// #ifdef DEBUG_LOG_GC
//  printf("-- gc begin\n");
//  size_t before = vm.bytesAllocated;
// #endif

  markRoots();
  traceReferences();
  tableRemoveWhite(&vm.strings);
  sweep();

  vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

// #ifdef DEBUG_LOG_GC
//  printf("-- gc end\n");
//  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
//         before - vm.bytesAllocated, before, vm.bytesAllocated,
//         vm.nextGC);
// #endif
}

void freeObjects() {
  Obj* object = vm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }

  free(vm.grayStack);
}
