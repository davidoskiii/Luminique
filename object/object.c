#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "object.h"
#include "../loop/loop.h"
#include "../memory/memory.h"
#include "../native/native.h"
#include "../table/table.h"
#include "../value/value.h"
#include "../string/string.h"
#include "../vm/vm.h"

#define ALLOCATE_OBJ(type, objectType, objectClass) (type*)allocateObject(sizeof(type), objectType, objectClass)

Obj* allocateObject(size_t size, ObjType type, ObjClass* klass) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->klass = klass;
  object->isMarked = false;

  object->next = vm.objects;
  initTable(&object->fields);

// #ifdef DEBUG_LOG_GC
//  printf("%p allocate %zu for %d\n", (void*)object, size, type);
// #endif

  vm.objects = object;
  return object;
}

ObjBoundMethod* newBoundMethod(Value receiver, Value method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD, vm.boundMethodClass);
  bound->receiver = receiver;
  bound->method = method;
  bound->isNative = IS_NATIVE_METHOD(method);
  return bound;
}

ObjMethod* newMethod(ObjClass* behavior, ObjClosure* closure) {
  ObjMethod* method = ALLOCATE_OBJ(ObjMethod, OBJ_METHOD, vm.methodClass);
  method->behavior = behavior;
  method->closure = closure;
  return method;
}

ObjFile* newFile(ObjString* name) {
  ObjFile* file = ALLOCATE_OBJ(ObjFile, OBJ_FILE, vm.fileClass);
  file->name = name;
  file->mode = emptyString();
  file->isOpen = false;
  file->offset = 0;
  file->fsStat = NULL;
  file->fsOpen = NULL;
  file->fsRead = NULL;
  file->fsWrite = NULL;
  return file;
}

ObjRecord* newRecord(void* data) {
  ObjRecord* record = ALLOCATE_OBJ(ObjRecord, OBJ_RECORD, NULL);
  record->data = data;
  return record;
}

ObjEntry* newEntry(Value key, Value value) {
  ObjEntry* entry = ALLOCATE_OBJ(ObjEntry, OBJ_ENTRY, vm.entryClass);
  entry->key = key;
  entry->value = value;
  return entry;
}

ObjArray* newArray() {
  ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY, vm.arrayClass);
  initValueArray(&array->elements);
  return array;
}

ObjRange* newRange(int from, int to) {
  ObjRange* range = ALLOCATE_OBJ(ObjRange, OBJ_RANGE, vm.rangeClass);
  range->from = from;
  range->to = to;
  return range;
}

ObjNode* newNode(Value element, ObjNode* prev, ObjNode* next) {
  ObjNode* node = ALLOCATE_OBJ(ObjNode, OBJ_NODE, vm.nodeClass);
  node->element = element;
  node->prev = prev;
  node->next = next;
  return node;
}

ObjWindow* newWindow(const char* title, int width, int height, bool isResizable) {
  ObjWindow* window = ALLOCATE_OBJ(ObjWindow, OBJ_WINDOW, vm.windowClass);
  window->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_HIDDEN);
  window->renderer = SDL_CreateRenderer(window->window, -1, SDL_RENDERER_ACCELERATED);
  window->image = NULL;
  window->font = NULL;
  window->title = copyString(title, strlen(title));
  window->width = width;
  window->height = height;
  window->isResizable = isResizable;

  Uint32 flags = SDL_GetWindowFlags(window->window);
  if (window->isResizable) {
    flags |= SDL_WINDOW_RESIZABLE;
  } else {
    flags &= ~SDL_WINDOW_RESIZABLE;
  }
  SDL_SetWindowResizable(window->window, window->isResizable ? SDL_TRUE : SDL_FALSE);   

  return window;
}

ObjSound* newSound(const char* path) {
  ObjSound* soundObj = ALLOCATE_OBJ(ObjSound, OBJ_SOUND, vm.soundClass);
  soundObj->path = copyString(path, strlen(path));

  soundObj->loops = 0;
  soundObj->duration = 0;
  soundObj->volume = 128;
  soundObj->channel = -1;

  soundObj->sound = Mix_LoadWAV(path);
  if (!soundObj->sound) {
    ObjClass* exceptionClass = getNativeClass("luminique::std::sonus", "AudioException");
    throwException(exceptionClass, "Sound file not loaded.");
    return NULL;
  }

  Uint32 soundLength = soundObj->sound->alen;
  Uint16 format = MIX_DEFAULT_FORMAT;
  int channels = 2;
  int bytesPerSample = SDL_AUDIO_BITSIZE(format) / 8;
  int samples = soundLength / (bytesPerSample * channels);
  soundObj->duration = (samples * 1000) / 44100;

  Mix_VolumeChunk(soundObj->sound, soundObj->volume);

  return soundObj;
}

ObjEvent* newEvent(const SDL_Event* event) {
  ObjEvent* objEvent = ALLOCATE_OBJ(ObjEvent, OBJ_EVENT, vm.eventClass);
  EventInfo* info = ALLOCATE(EventInfo, 1);

  switch (event->type) {
    case SDL_QUIT:
      info->eventType = SDL_QUIT;
      info->keyCode = -1;
      info->quit = true;
      break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      info->eventType = event->key.type;
      info->keyCode = event->key.keysym.sym;
      info->quit = false;
      break;
    default:
      info->eventType = event->type;
      info->keyCode = -1;
      info->quit = false;
      break;
  }

  objEvent->event = *event;
  objEvent->info = info;

  return objEvent;
}

ObjArray* copyArray(ValueArray elements, int fromIndex, int toIndex) {
  ObjArray* array = ALLOCATE_OBJ(ObjArray, OBJ_ARRAY, vm.arrayClass);
  initValueArray(&array->elements);
  for (int i = fromIndex; i < toIndex; i++) {
    writeValueArray(&array->elements, elements.values[i]);
  }
  return array;
}

ObjDictionary* newDictionary() {
  ObjDictionary* dict = ALLOCATE_OBJ(ObjDictionary, OBJ_DICTIONARY, vm.dictionaryClass);
  dict->count = 0;
  dict->capacity = 0;
  dict->entries = NULL;
  return dict;
}

ObjClass* newClass(ObjString* name, ObjType classType, bool isAbstract) {
  ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS, vm.classClass);
  klass->classType = classType;
  klass->name = name;
  klass->namespace_ = vm.currentNamespace;
  klass->superclass = NULL;
  klass->isNative = false;
  klass->isAbstract = isAbstract;
  klass->interceptors = 0;

  initValueArray(&klass->abstractMethodNames);

  initTable(&klass->methods);
  initTable(&klass->fields);
  initTable(&klass->getters);
  initTable(&klass->setters);
  return klass;
}

ObjEnum* newEnum(ObjString* name) {
  ObjEnum* enum_ = ALLOCATE_OBJ(ObjEnum, OBJ_ENUM, vm.enumClass);
  enum_->name = name;
  enum_->nextValue = 0;
  initTable(&enum_->values);
  return enum_;
}

ObjModule* newModule(ObjString* path) {
  ObjModule* module = ALLOCATE_OBJ(ObjModule, OBJ_MODULE, NULL);
  module->path = path;
  module->isNative = false;
  module->closure = NULL;
  module->source = NULL;
  initTable(&module->values);
  tableAddAll(&vm.langNamespace->values, &module->values);
  tableSet(&vm.modules, path, NIL_VAL);
  return module;
}

ObjPromise* newPromise(PromiseState state, Value value, Value executor) {
  ObjPromise* promise = ALLOCATE_OBJ(ObjPromise, OBJ_PROMISE, vm.promiseClass);
  promise->id = ++vm.promiseCount;
  promise->state = state;
  promise->value = value;
  if (state == PROMISE_REJECTED && IS_EXCEPTION(value)) promise->exception = AS_EXCEPTION(value);
  else promise->exception = NULL;
  promise->executor = executor;
  promise->onCatch = NIL_VAL;
  promise->onFinally = NIL_VAL;
  promise->captures = newDictionary();
  initValueArray(&promise->handlers);
  return promise;
}

ObjNamespace* newNamespace(ObjString* shortName, ObjNamespace* enclosing) {
  ObjNamespace* namespace = ALLOCATE_OBJ(ObjNamespace, OBJ_NAMESPACE, vm.namespaceClass);
  namespace->shortName = shortName;
  namespace->enclosing = enclosing;
  namespace->isRoot = false;

  if (namespace->enclosing != NULL && !namespace->enclosing->isRoot) {
    char chars[UINT8_MAX];
    int length = snprintf(chars, UINT8_MAX, "%s::%s", namespace->enclosing->fullName->chars, shortName->chars);
    namespace->fullName = copyString(chars, length);
  }
  else namespace->fullName = namespace->shortName;

  initTable(&namespace->values);
  initTable(&namespace->compilerValues);
  initTable(&namespace->globals);
  initTable(&namespace->compilerGlobals);
  return namespace;
}

ObjClosure* newClosure(ObjFunction* function) {
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE, vm.functionClass);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjFrame* newFrame(CallFrame* callFrame) {
  ObjFrame* frame = ALLOCATE_OBJ(ObjFrame, OBJ_FRAME, NULL);
  frame->closure = callFrame->closure;
  frame->ip = callFrame->ip;
  frame->slotCount = callFrame->closure->function->arity + 1;
  frame->handlerCount = callFrame->handlerCount;

  for (int i = 0; i < frame->slotCount; i++) {
    frame->slots[i] = peek(callFrame->closure->function->arity - i);
  }

  for (int i = 0; i < frame->handlerCount; i++) {
    frame->handlerStack[i] = callFrame->handlerStack[i];
  }

  return frame;
}

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION, NULL);
  function->parameters = ALLOCATE(ParamInfo, 255);
  function->arity = 0;
  function->upvalueCount = 0;
  function->isGenerator = false;
  function->name = NULL;
  function->isAsync = false;
  function->isAbstract = false;
  function->isMutable = true;
  function->optionalArgCount = 0;
  initChunk(&function->chunk);
  return function;
}

ObjGenerator* newGenerator(ObjFrame* frame, ObjGenerator* outer) {
  ObjGenerator* generator = ALLOCATE_OBJ(ObjGenerator, OBJ_GENERATOR, vm.generatorClass);
  generator->frame = frame;
  generator->outer = outer;
  generator->inner = NULL;
  generator->state = GENERATOR_START;
  generator->value = NIL_VAL;
  return generator;
}

ObjException* newException(ObjString* message, ObjClass* klass) {
  ObjException* exception = ALLOCATE_OBJ(ObjException, OBJ_EXCEPTION, klass);
  exception->message = message;
  exception->stacktrace = newArray();
  return exception;
}

ObjInstance* newInstance(ObjClass* klass) {
  ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE, klass);
  initTable(&instance->fields);
  return instance;
}

ObjNativeFunction* newNativeFunction(ObjString* name, int arity, bool isAsync, NativeFunction function) {
  ObjNativeFunction* nativeFunction = ALLOCATE_OBJ(ObjNativeFunction, OBJ_NATIVE_FUNCTION, vm.functionClass);
  nativeFunction->name = name;
  nativeFunction->arity = arity;
  nativeFunction->isAsync = isAsync;
  nativeFunction->function = function;
  return nativeFunction;
}

ObjNativeMethod* newNativeMethod(ObjClass* klass, ObjString* name, int arity, bool isAsync, bool isAbstract, NativeMethod method) {
  ObjNativeMethod* nativeMethod = ALLOCATE_OBJ(ObjNativeMethod, OBJ_NATIVE_METHOD, NULL);
  if (arity != -1) nativeMethod->parameters = ALLOCATE(ParamInfo, arity);
  else nativeMethod->parameters = ALLOCATE(ParamInfo, 255);
  nativeMethod->klass = klass;
  nativeMethod->name = name;
  nativeMethod->arity = arity;
  nativeMethod->isAsync = isAsync;
  nativeMethod->isAbstract = isAbstract;
  nativeMethod->method = method;
  return nativeMethod;
}

ObjTimer* newTimer(ObjClosure* closure, int delay, int interval) {
  ObjTimer* timer = ALLOCATE_OBJ(ObjTimer, OBJ_TIMER, vm.timerClass);
  TimerData* data = ALLOCATE_STRUCT(TimerData);
  if (data != NULL) {
    data->receiver = NIL_VAL;
    data->vm = &vm;
    data->closure = closure;
    data->delay = delay;
    data->interval = interval;

    timer->timer = ALLOCATE_STRUCT(uv_timer_t);
    if (timer->timer != NULL) {
      timer->timer->data = data;
      timer->id = 0;
      timer->isRunning = false;
    }
    else throwNativeException("luminique::std::lang", "OutOfMemoryException", "Not enough memory to allocate timer object.");
  }
  else throwNativeException("luminique::std::lang", "OutOfMemoryException", "Not enough memory to allocate timer data.");
  return timer;
}

ObjUpvalue* newUpvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE, NULL);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

static void printArray(ObjArray* array) {
  printf("[");
  for (int i = 0; i < array->elements.count; i++) {
    if (IS_STRING(array->elements.values[i])) printf("\"");
    printValue(array->elements.values[i]);
    if (IS_STRING(array->elements.values[i])) printf("\"");
    if (i < array->elements.count - 1) printf(", ");
  }
  printf("]");
}

Value getObjProperty(ObjInstance* object, char* name) {
  Value value;
  tableGet(&object->fields, copyString(name, (int)strlen(name)), &value);
  return value;
}

Value getObjMethod(Value object, char* name) {
  ObjClass* klass = getObjClass(object);
  Value method;
  if (!tableGet(&klass->methods, newString(name), &method)) {
    runtimeError("Method %s::%s does not exist.", klass->name->chars, name);
    exit(70);
  }
  return method;
}

bool objMethodExists(Value object, char* name) {
  ObjClass* klass = getObjClass(object);
  Value method;
  if (!tableGet(&klass->methods, newString(name), &method)) {
    return false;
  }
  return true;
}

void setObjProperty(ObjInstance* object, char* name, Value value) {
  tableSet(&object->fields, copyString(name, (int)strlen(name)), value);
}

Value getClassProperty(ObjClass* klass, char* name) {
  Value value;
  tableGet(&klass->fields, newString(name), &value);
  return value;
}

void setClassProperty(ObjClass* klass, char* name, Value value) {
  ObjString* key = newString(name);
  push(OBJ_VAL(key));
  tableSet(&klass->fields, key, ((void*)value == NULL) ? NIL_VAL : value);
  pop();
}

bool isClassExtendingSuperclass(ObjClass* klass, ObjClass* superclass) {
  if (klass == superclass) return true;

  ObjClass* currentClass = klass->superclass;
  while (currentClass != NULL) {
    if (currentClass == superclass) return true;
    currentClass = currentClass->superclass;
  }
  return false;
}

bool isObjInstanceOf(Value value, ObjClass* klass) {
  ObjClass* currentClass = getObjClass(value);
  if (currentClass == klass) return true;
  else if (IS_NATIVE_METHOD(value)) {
    return matchStringName(klass->name, "Method", 6) ? true : false;
  } else if (IS_NATIVE_FUNCTION(value)) {
    return matchStringName(klass->name, "Function", 8) ? true : false;
  }
  if (isClassExtendingSuperclass(currentClass->superclass, klass)) return true;
  return false;
}

static void printDictionary(ObjDictionary* dictionary) {
  printf("{");
  int startIndex = 0;
  for (int i = 0; i < dictionary->capacity; i++) {
    ObjEntry* entry = &dictionary->entries[i];
    if (IS_UNDEFINED(entry->key)) continue;
    if (IS_STRING(entry->key)) printf("\"");
    printValue(entry->key);
    if (IS_STRING(entry->key)) printf("\"");
    printf(": ");
    if (IS_STRING(entry->value)) printf("\"");
    printValue(entry->value);
    if (IS_STRING(entry->value)) printf("\"");
    startIndex = i + 1;
    break;
  }

  for (int i = startIndex; i < dictionary->capacity; i++) {
    ObjEntry* entry = &dictionary->entries[i];
    if (IS_UNDEFINED(entry->key)) continue;
    printf(", ");
    if (IS_STRING(entry->key)) printf("\"");
    printValue(entry->key);
    if (IS_STRING(entry->key)) printf("\"");
    printf(": ");
    if (IS_STRING(entry->value)) printf("\"");
    printValue(entry->value);
    if (IS_STRING(entry->value)) printf("\"");
  }
  printf("}");
}

static void printFunction(ObjFunction* function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  } else if (function->name->length == 0 || matchStringName(function->name, "lambda", function->name->length)) printf("<function>");
  else printf("<function %s>", function->name->chars);
}

void printEvent(ObjEvent* event) {
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

  printf("<Event type: %s, KeyCode: %d, Quit: %s>", eventTypeStr, event->info->keyCode, event->info->quit ? "true" : "false");
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_ARRAY:
      printArray(AS_ARRAY(value));
      break;
    case OBJ_NAMESPACE:
      printf("<namespace %s>", AS_NAMESPACE(value)->fullName->chars);
      break;
    case OBJ_MODULE:
      printf("<module %s>", AS_MODULE(value)->path->chars);
      break;
    case OBJ_NODE:
      printf("<node>");
      break;
    case OBJ_RANGE:
      printf("%d...%d", AS_RANGE(value)->from, AS_RANGE(value)->to);
      break;
    case OBJ_WINDOW:
      printf("<%s window>", AS_WINDOW(value)->title->chars);
      break;
    case OBJ_SOUND:
      printf("<sound %s | %d ms>", AS_SOUND(value)->path->chars, AS_SOUND(value)->duration);
      break;
    case OBJ_EVENT:
      printEvent(AS_EVENT(value));
      break;
    case OBJ_FILE:
      printf("<file \"%s\">", AS_FILE(value)->name->chars);
      break;
    case OBJ_RECORD:
      printf("<record>");
      break;
    case OBJ_DICTIONARY: 
      printDictionary(AS_DICTIONARY(value));
      break;
    case OBJ_BOUND_METHOD: { 
      ObjBoundMethod* boundMethod = AS_BOUND_METHOD(value);
      if (IS_NATIVE_METHOD(boundMethod->method)) printf("<bound method %s::%s>", 
          AS_OBJ(boundMethod->receiver)->klass->name->chars, AS_NATIVE_METHOD(boundMethod->method)->name->chars);
      else printf("<bound method %s::%s>", AS_OBJ(boundMethod->receiver)->klass->name->chars, AS_CLOSURE(boundMethod->method)->function->name->chars);
      break;
    }
    case OBJ_METHOD: { 
      ObjMethod* method = AS_METHOD(value);
      printf("<method %s::%s>", method->behavior->name->chars, method->closure->function->name->chars);
      break;
    }
    case OBJ_CLASS: {
      ObjClass* klass = AS_CLASS(value);
      if (klass->namespace_->isRoot) printf("<class %s>", klass->name->chars);
      else printf("<class %s::%s>", klass->namespace_->fullName->chars, klass->name->chars);
      break;
    }
    case OBJ_CLOSURE:
      printFunction(AS_CLOSURE(value)->function);
      break;
    case OBJ_FRAME: 
      printf("<frame: %s>", AS_FRAME(value)->closure->function->name->chars);
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_GENERATOR: { 
      ObjFunction* function = AS_GENERATOR(value)->frame->closure->function;
      printf("<generator %s>", (function->name == NULL) ? "script" : function->name->chars);
      break;
    }
    case OBJ_ENUM:
      printf("<enum %s>", AS_ENUM(value)->name->chars);
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
        printf("%s", AS_CSTRING(str));
      } else 
      #endif
      printf("<object %s>", AS_OBJ(value)->klass->name->chars);
      break;
    case OBJ_EXCEPTION: {
      printf("<Exception %s - %s>", AS_EXCEPTION(value)->obj.klass->name->chars, AS_EXCEPTION(value)->message->chars);
      break;
    }
    case OBJ_NATIVE_FUNCTION:
      printf("<native function %s>", AS_NATIVE_FUNCTION(value)->name->chars);
      break;
    case OBJ_NATIVE_METHOD:
      printf("<native method %s::%s>", AS_NATIVE_METHOD(value)->klass->name->chars, AS_NATIVE_METHOD(value)->name->chars);
      break;
    case OBJ_PROMISE: {
      printf("<promise: %d>", AS_PROMISE(value)->id);
      break;
    }
    case OBJ_TIMER: {
      printf("<timer: %d>", AS_TIMER(value)->id);
      break;
    }
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_UPVALUE:
      printf("<upvalue>");
      break;
  }
}

ObjClass* getObjClass(Value value) {
  if (IS_BOOL(value)) return vm.boolClass;
  else if (IS_NIL(value)) return vm.nilClass;
  else if (IS_INT(value)) return vm.intClass;
  else if (IS_FLOAT(value)) return vm.numberClass;
  else if (IS_OBJ(value)) return AS_OBJ(value)->klass;
  else return NULL;
}
