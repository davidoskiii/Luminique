#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lang.h"
#include "../hash/hash.h"
#include "../collection/collection.h"
#include "../interceptor/interceptor.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../vm/vm.h"

#define MAX_BUFFER_SIZE 32

// BOOL

NATIVE_METHOD(Bool, __init__) {
  assertArgCount("Bool::__init__(value)", 1, argCount);
  if (isFalsey(args[0])) {
    RETURN_BOOL(false);
  } else {
    RETURN_BOOL(true);
  }
}

NATIVE_METHOD(Bool, clone) {
	assertArgCount("Bool::clone()", 0, argCount);
	return receiver;
}

NATIVE_METHOD(Bool, __str__) {
	assertArgCount("Bool::__str__()", 0, argCount);
	if (AS_BOOL(receiver)) RETURN_STRING("true", 4);
	else RETURN_STRING("false", 5);
}

NATIVE_METHOD(Bool, __format__) {
	assertArgCount("Bool::__format__()", 0, argCount);
	if (AS_BOOL(receiver)) RETURN_STRING("true", 4);
	else RETURN_STRING("false", 5);
}

// EXCEPTION

NATIVE_METHOD(Exception, __init__) {
  assertArgCount("Exception::__init__(message)", 1, argCount);
  assertArgIsString("Exception::__init__(message)", args, 0);
  ObjException* self = AS_EXCEPTION(receiver);
  self->message = AS_STRING(args[0]);
  RETURN_OBJ(self);
}

NATIVE_METHOD(Exception, __str__) {
  assertArgCount("Exception::__str__()", 0, argCount);
  ObjException* self = AS_EXCEPTION(receiver);
  RETURN_STRING_FMT("<Exception %s - %s>", self->obj.klass->name->chars, self->message->chars);
}

NATIVE_METHOD(Exception, __format__) {
  assertArgCount("Exception::__format__()", 0, argCount);
  ObjException* self = AS_EXCEPTION(receiver);
  RETURN_STRING_FMT("<Exception %s - %s>", self->obj.klass->name->chars, self->message->chars);
}

NATIVE_METHOD(Exception, __undefinedProperty__) {
  assertArgCount("Exception::__undefinedProperty__(name)", 1, argCount);
  assertArgIsString("Exception::__undefinedProperty__(name)", args, 0);
  ObjString* property = AS_STRING(args[0]);
  ObjException* self = AS_EXCEPTION(receiver);

  if (matchStringName(property, "message", 7)) {
    RETURN_OBJ(self->message);
  } else if (matchStringName(property, "stacktrace", 10)) {
    RETURN_OBJ(self->stacktrace);
  } else THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", 
    AS_CSTRING(args[0]), valueToString(receiver));
}

// CLASS

NATIVE_METHOD(Class, __init__) {
  assertArgCount("Class::__init__(name, superclass)", 2, argCount);
  assertArgIsString("Class::__init__(name, superclass)", args, 0);
  assertArgIsClass("Class::__init__(name, superclass)", args, 1);
  ObjClass* klass = newClass(AS_STRING(args[0]), OBJ_INSTANCE);
  bindSuperclass(klass, AS_CLASS(args[1]));
  RETURN_OBJ(klass);
}

NATIVE_METHOD(Class, __invoke__) { 
  ObjClass* self = AS_CLASS(receiver);
  ObjInstance* instance = newInstance(self);
  push(OBJ_VAL(instance));
  Value initMethod;

  if (tableGet(&self->methods, vm.initString, &initMethod)) {
    callReentrantMethod(receiver, initMethod, args);
  }

  pop();
  RETURN_OBJ(instance);
}

NATIVE_METHOD(Class, clone) {
  assertArgCount("Class::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Class, getClass) {
  assertArgCount("Class::getClass()", 0, argCount);
  RETURN_OBJ(vm.classClass);
}

NATIVE_METHOD(Class, getClassName) {
  assertArgCount("Class::getClassName()", 0, argCount);
  RETURN_OBJ(vm.classClass->name);
}

NATIVE_METHOD(Class, instanceOf) {
  assertArgCount("Class::instanceOf(class)", 1, argCount);
  if (!IS_CLASS(args[0])) RETURN_FALSE;
  ObjClass* klass = AS_CLASS(args[0]);
  if (klass == vm.classClass) RETURN_TRUE;
  else RETURN_FALSE;
}

NATIVE_METHOD(Class, memberOf) {
  assertArgCount("Class::memberOf(class)", 1, argCount);
  if (!IS_CLASS(args[0])) RETURN_FALSE;
  ObjClass* klass = AS_CLASS(args[0]);
  if (klass == vm.classClass) RETURN_TRUE;
  else RETURN_FALSE;
}

NATIVE_METHOD(Class, name) {
  assertArgCount("Class::name()", 0, argCount);
  RETURN_OBJ(AS_CLASS(receiver)->name);
}

NATIVE_METHOD(Class, superclass) {
  assertArgCount("Class::superclass()", 0, argCount);
  ObjClass* klass = AS_CLASS(receiver);
  if (klass->superclass == NULL) RETURN_NIL;
  RETURN_OBJ(klass->superclass);
}

NATIVE_METHOD(Class, __str__) {
  assertArgCount("Class::__str__()", 0, argCount);
  ObjClass* self = AS_CLASS(receiver);
  if (self->namespace_->isRoot) RETURN_STRING_FMT("<class %s>", self->name->chars);
  else RETURN_STRING_FMT("<class %s::%s>", self->namespace_->fullName->chars, self->name->chars);
}

NATIVE_METHOD(Class, __format__) {
  assertArgCount("Class::__format__()", 0, argCount);
  ObjClass* self = AS_CLASS(receiver);
  if (self->namespace_->isRoot) RETURN_STRING_FMT("<class %s>", self->name->chars);
  else RETURN_STRING_FMT("<class %s::%s>", self->namespace_->fullName->chars, self->name->chars);
}

// ENUM

NATIVE_METHOD(Enum, __init__) {
  assertArgCount("Enum::__init__(name)", 1, argCount);
  assertArgIsString("Enum::__init__(name)", args, 0);
  ObjEnum* enum_ = newEnum(AS_STRING(args[0]));
  RETURN_OBJ(enum_);
}

NATIVE_METHOD(Enum, clone) {
  assertArgCount("Enum::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Enum, name) {
  assertArgCount("Enum::name()", 0, argCount);
  RETURN_OBJ(AS_ENUM(receiver)->name);
}

NATIVE_METHOD(Enum, __str__) {
  assertArgCount("Enum::__str__()", 0, argCount);
  ObjEnum* self = AS_ENUM(receiver);
  RETURN_STRING_FMT("<enum %s>", self->name->chars);
}

NATIVE_METHOD(Enum, __format__) {
  assertArgCount("Enum::__format__()", 0, argCount);
  ObjEnum* self = AS_ENUM(receiver);
  RETURN_STRING_FMT("<enum %s>", self->name->chars);
}

// FLOAT

NATIVE_METHOD(Float, __init__) {
  assertArgCount("Float::__init__(value)", 1, argCount);
  assertArgIsNumber("Float::__int__(value)", args, 0);
  RETURN_FLOAT(args[0]);
}

NATIVE_METHOD(Float, clone) {
  assertArgCount("Float::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Float, __str__) {
  assertArgCount("Float::__str__()", 0, argCount);
  RETURN_STRING_FMT("%g", AS_FLOAT(receiver));
}

NATIVE_METHOD(Float, __format__) {
  assertArgCount("Float::__format__()", 0, argCount);
  RETURN_STRING_FMT("%g", AS_FLOAT(receiver));
}

NATIVE_METHOD(Generator, __init__) {
  assertArgCount("Generator::__init__(closure, args)", 2, argCount);
  assertArgIsClosure("Generator::__init__(closure, args)", args, 0);
  assertArgIsArray("Generator::__init__(closure, args)", args, 1);

  ObjGenerator* self = AS_GENERATOR(receiver);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  ObjArray* arguments = AS_ARRAY(args[1]);

  CallFrame callFrame = { 
    .closure = closure, 
    .ip = closure->function->chunk.code, 
    .slots = vm.stackTop - arguments->elements.count - 1 
  };

  ObjFrame* frame = newFrame(&callFrame);

  for (int i = 0; i < arguments->elements.count; i++) {
    push(arguments->elements.values[i]);
  }

  self->frame = frame;
  self->outer = vm.runningGenerator;
  self->inner = NULL;
  self->state = GENERATOR_START;
  self->value = NIL_VAL;
  RETURN_OBJ(self);
}

NATIVE_METHOD(Generator, isFinished) { 
  assertArgCount("Generator::isFinished()", 0, argCount);
  RETURN_BOOL(AS_GENERATOR(receiver)->state == GENERATOR_RETURN);
}

NATIVE_METHOD(Generator, isSuspended) {
  assertArgCount("Generator::isSuspended()", 0, argCount);
  RETURN_BOOL(AS_GENERATOR(receiver)->state == GENERATOR_YIELD);
}

NATIVE_METHOD(Generator, isReady) { 
  assertArgCount("Generator::isReady()", 0, argCount);
  RETURN_BOOL(AS_GENERATOR(receiver)->state == GENERATOR_START);
}

NATIVE_METHOD(Generator, getReceiver) {
  assertArgCount("Generator::getReceiver()", 0, argCount);
  RETURN_VAL(AS_GENERATOR(receiver)->frame->slots[0]);
}

NATIVE_METHOD(Generator, setReceiver) {
  assertArgCount("Generator::setReceiver(receiver)", 1, argCount);
  ObjGenerator* self = AS_GENERATOR(receiver);
  self->frame->slots[0] = args[0];
  RETURN_NIL;
}

NATIVE_METHOD(Generator, nextFinished) {
  assertArgCount("Generator::nextFinished()", 0, argCount);
  ObjGenerator* self = AS_GENERATOR(receiver);
  if (self->state == GENERATOR_RETURN) RETURN_TRUE;
  else if (self->state == GENERATOR_RESUME) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator is already running.");
  else if (self->state == GENERATOR_THROW) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already thrown an exception.");
  else {
    resumeGenerator(self);
    RETURN_BOOL(self->state == GENERATOR_RETURN);
  }
}

NATIVE_METHOD(Generator, next) {
  assertArgCount("Generator::next()", 0, argCount);
  ObjGenerator* self = AS_GENERATOR(receiver);
  if (self->state == GENERATOR_RETURN) RETURN_OBJ(self);
  else if (self->state == GENERATOR_RESUME) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator is already running.");
  else if (self->state == GENERATOR_THROW) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already thrown an exception.");
  else {
    resumeGenerator(self);
    RETURN_OBJ(self);
  }
}

NATIVE_METHOD(Generator, send) {
  assertArgCount("Generator::send(value)", 1, argCount);
  ObjGenerator* self = AS_GENERATOR(receiver);
  if (self->state == GENERATOR_RETURN) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already returned.");
  else if (self->state == GENERATOR_RESUME) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator is already running.");
  else if (self->state == GENERATOR_THROW) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already thrown an exception.");
  else {
    self->value = args[0];
    resumeGenerator(self);
    RETURN_OBJ(self);
  }
}

NATIVE_METHOD(Generator, returns) {
  assertArgCount("Generator::returns(value)", 1, argCount);
  ObjGenerator* self = AS_GENERATOR(receiver);
  if (self->state == GENERATOR_RETURN) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already returned.");
  else {
    self->state = GENERATOR_RETURN;
    self->value = args[0];
    RETURN_VAL(args[0]);
  }
}

NATIVE_METHOD(Generator, throws) {
  assertArgCount("Generator::throws(exception)", 1, argCount);
  assertArgIsException("Generator::throws(exception)", args, 0);
  ObjGenerator* self = AS_GENERATOR(receiver);
  if (self->state == GENERATOR_RETURN) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already returned.");
  else {
    ObjException* exception = AS_EXCEPTION(args[0]);
    self->state = GENERATOR_THROW;
    THROW_EXCEPTION(luminique::std::lang, exception->obj.klass, exception->message->chars);
  }
}

NATIVE_METHOD(Generator, __str__) {
  assertArgCount("Generator::__str__()", 0, argCount);
  ObjGenerator* self = AS_GENERATOR(receiver);
  RETURN_STRING_FMT("<generator %s>", self->frame->closure->function->name->chars);
}

NATIVE_METHOD(Generator, __format__) {
  assertArgCount("Generator::__format__()", 0, argCount);
  ObjGenerator* self = AS_GENERATOR(receiver);
  RETURN_STRING_FMT("<generator %s>", self->frame->closure->function->name->chars);
}

NATIVE_METHOD(Generator, __invoke__) {
  if(argCount > 1) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator::() accepts 0 or 1 argument.");
  ObjGenerator* self = AS_GENERATOR(receiver);
  if (self->state == GENERATOR_RETURN) RETURN_OBJ(self);
  else if (self->state == GENERATOR_RESUME) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator is already running.");
  else if (self->state == GENERATOR_THROW) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already thrown an exception.");
  else {
    if(argCount == 1) self->value = args[0];
    resumeGenerator(self);
    RETURN_OBJ(self);
  }
}

NATIVE_METHOD(Generator, __undefinedProperty__) {
  assertArgCount("Generator::__undefinedProperty__(name)", 1, argCount);
  assertArgIsString("Generator::__undefinedProperty__(name)", args, 0);
  ObjString* property = AS_STRING(args[0]);
  ObjGenerator* self = AS_GENERATOR(receiver);

  if (matchStringName(property, "outer", 5)) {
    return self->outer != NULL ? OBJ_VAL(self->outer) : NIL_VAL;
  } else if (matchStringName(property, "state", 5)) {
    RETURN_INT(self->state);
  } else if (matchStringName(property, "value", 5)) {
    return self->value;
  } else THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", 
    AS_CSTRING(args[0]), valueToString(receiver));
}


NATIVE_METHOD(Promise, __init__) {
  assertArgCount("Promise::__init__(executor)", 1, argCount);
  assertArgInstanceOfEither("Promise::__init__(executor)", args, 0, "luminique::std::lang", "BoundMethod", "luminique::std::lang", "Function");
  ObjPromise* self = AS_PROMISE(receiver);
  self->executor = args[0];
  promiseExecute(self);
  RETURN_OBJ(self);
}

NATIVE_METHOD(Promise, catch) {
  assertArgCount("Promise::catch(closure)", 1, argCount);
  assertArgInstanceOfEither("Promise::catch(closure)", args, 0, "luminique::std::lang", "Function", "luminique::std::lang", "BoundMethod");
  ObjPromise* self = AS_PROMISE(receiver);
  if (self->state == PROMISE_REJECTED) callReentrantMethod(OBJ_VAL(self), args[0], OBJ_VAL(self->exception));
  else self->onCatch = args[0];
  RETURN_OBJ(self);
}

NATIVE_METHOD(Promise, catchAll) {
  assertArgCount("Promise::catchAll(exception)", 1, argCount);
  assertArgIsException("Promise::catchAll(exception)", args, 0);
  ObjPromise* self = AS_PROMISE(receiver);
  ObjBoundMethod* reject = AS_BOUND_METHOD(self->capturedValues->elements.values[5]);
  callReentrantMethod(reject->receiver, reject->method, args[0]);
  RETURN_OBJ(self);
}

NATIVE_METHOD(Promise, finally) {
  assertArgCount("Promise::finally(closure)", 1, argCount);
  assertArgInstanceOfEither("Promise::finally(closure)", args, 0, "luminique::std::lang", "Function", "luminique::std::lang", "BoundMethod");
  ObjPromise* self = AS_PROMISE(receiver);
  if (self->state == PROMISE_FULFILLED || self->state == PROMISE_REJECTED) callReentrantMethod(OBJ_VAL(self), args[0], self->value);
  else self->onFinally = args[0];
  RETURN_OBJ(self);
}

NATIVE_METHOD(Promise, fulfill) {
  assertArgCount("Promise::fulfill(value)", 1, argCount);
  promiseFulfill(AS_PROMISE(receiver), args[0]);
  RETURN_NIL;
}

NATIVE_METHOD(Promise, isResolved) {
  assertArgCount("Promise::isResolved()", 0, argCount);
  ObjPromise* self = AS_PROMISE(receiver);
  RETURN_BOOL(self->state == PROMISE_FULFILLED || self->state == PROMISE_REJECTED);
}

NATIVE_METHOD(Promise, reject) {
  assertArgCount("Promise::reject(exception)", 1, argCount);
  assertArgIsException("Promise::reject(exception)", args, 0);
  promiseReject(AS_PROMISE(receiver), args[0]);
  RETURN_NIL;
}

NATIVE_METHOD(Promise, then) {
  assertArgCount("Promise::then(onFulfilled)", 1, argCount);
  assertArgInstanceOfEither("Promise::then(onFulfilled)", args, 0, "luminique::std::lang", "Function", "luminique::std::lang", "BoundMethod");
  ObjPromise* self = AS_PROMISE(receiver);
  if (self->state == PROMISE_FULFILLED) self->value = callReentrantMethod(OBJ_VAL(self), args[0], self->value);
  else writeValueArray(&self->handlers, args[0]);
  RETURN_OBJ(self);
}

NATIVE_METHOD(Promise, raceAll) {
  assertArgCount("Promise::raceAll(result)", 1, argCount);
  ObjPromise* self = AS_PROMISE(receiver);
  ObjPromise* racePromise = AS_PROMISE(self->capturedValues->elements.values[0]);
  if (racePromise->state == PROMISE_PENDING) { 
    self->value = args[0];
    self->state = PROMISE_FULFILLED;
    promiseThen(racePromise, args[0]);
  }
  RETURN_NIL;
}

NATIVE_METHOD(Promise, thenAll) {
  assertArgCount("Promise::thenAll(result)", 1, argCount);
  ObjPromise* self = AS_PROMISE(receiver);
  ObjArray* promises = AS_ARRAY(self->capturedValues->elements.values[0]);
  ObjPromise* allPromise = AS_PROMISE(self->capturedValues->elements.values[1]);
  ObjArray* results = AS_ARRAY(self->capturedValues->elements.values[2]);
  int remainingCount = AS_INT(self->capturedValues->elements.values[3]);
  int index = AS_INT(self->capturedValues->elements.values[4]);

  valueArrayPut(&results->elements, index, args[0]);
  remainingCount--;
  for (int i = 0; i < promises->elements.count; i++) {
    ObjPromise* promise = AS_PROMISE(promises->elements.values[i]);
    promise->capturedValues->elements.values[3] = INT_VAL(remainingCount);
  }

  if (remainingCount <= 0 && allPromise->state == PROMISE_PENDING) {
    promiseThen(allPromise, OBJ_VAL(results));
  }
  RETURN_OBJ(self);
}

NATIVE_METHOD(Promise, __str__) {
  assertArgCount("Promise::__str__()", 0, argCount);
  RETURN_STRING_FMT("<promise: %d>", AS_PROMISE(receiver)->id);
}

NATIVE_METHOD(Promise, __format__) {
  assertArgCount("Promise::__format__()", 0, argCount);
  RETURN_STRING_FMT("<promise: %d>", AS_PROMISE(receiver)->id);
}

NATIVE_METHOD(Promise, __undefinedProperty__) {
  assertArgCount("Promise::__undefinedProperty__(name)", 1, argCount);
  assertArgIsString("Promise::__undefinedProperty__(name)", args, 0);
  ObjString* property = AS_STRING(args[0]);
  ObjPromise* self = AS_PROMISE(receiver);

  if (matchStringName(property, "state", 5)) {
    RETURN_INT(self->state);
  } else if (matchStringName(property, "value", 5)) {
    return self->value;
  } else if (matchStringName(property, "id", 2)) {
    RETURN_INT(self->id);
  } else THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", 
    AS_CSTRING(args[0]), valueToString(receiver));
}

NATIVE_METHOD(PromiseClass, fulfill) {
  assertArgCount("Promise class::fulfill(value)", 1, argCount);
  ObjClass* klass = AS_CLASS(receiver);
  if (IS_PROMISE(args[0])) RETURN_VAL(args[0]);
  else {
    ObjPromise* promise = newPromise(NIL_VAL);
    promise->state = PROMISE_FULFILLED;
    promise->obj.klass = klass;
    promise->value = args[0];
    promise->executor = getObjMethod(receiver, "fulfill");
    RETURN_OBJ(promise);
  }
}

NATIVE_METHOD(PromiseClass, all) {
  assertArgCount("Promise class::all(promises)", 1 ,argCount);
  assertArgIsArray("Promise class::all(promises)", args, 0);
  RETURN_OBJ(promiseAll(AS_CLASS(receiver), AS_ARRAY(args[0])));
}


NATIVE_METHOD(PromiseClass, race) {
  assertArgCount("Promise class::race(promises)", 1, argCount);
  assertArgIsArray("Promise class::race(promises)", args, 0);
  RETURN_OBJ(promiseRace(AS_CLASS(receiver), AS_ARRAY(args[0])));
}

NATIVE_METHOD(PromiseClass, reject) {
  assertArgCount("Promise class::reject(exception)", 1, argCount);
  assertArgIsException("Promise class::reject(exception)", args, 0);
  ObjClass* klass = AS_CLASS(receiver);
  Value reject;
  tableGet(&klass->methods, copyString("reject", 6), &reject);
  ObjPromise* promise = newPromise(reject);
  promise->state = PROMISE_REJECTED;
  promise->obj.klass = klass;
  promise->exception = AS_EXCEPTION(args[0]);
  RETURN_OBJ(promise);
}

// FUNCTION

NATIVE_METHOD(Function, __init__) {
  THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class Function.");
}

NATIVE_METHOD(Function, __invoke__) {
  ObjClosure* self = AS_CLOSURE(receiver);
  if (callClosure(self, argCount)) {
    int i = 0;
    while (i < argCount) {
      push(args[i]);
      i++;
    }
    RETURN_VAL(args[argCount - 1]);
  }
  RETURN_NIL;
}

NATIVE_METHOD(Function, clone) {
  assertArgCount("Function::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Function, isNative) {
  assertArgCount("Function::isNative()", 0, argCount);
  RETURN_BOOL(IS_NATIVE_FUNCTION(receiver));
}

NATIVE_METHOD(Function, __str__) {
  assertArgCount("Function::__str__()", 0, argCount);

  if (IS_NATIVE_FUNCTION(receiver)) {
    RETURN_STRING_FMT("<native function %s>", AS_NATIVE_FUNCTION(receiver)->name->chars);
  }

  RETURN_STRING_FMT("<fn %s>", AS_CLOSURE(receiver)->function->name->chars);
}

NATIVE_METHOD(Function, __format__) {
  assertArgCount("Function::__format_()", 0, argCount);

  if (IS_NATIVE_FUNCTION(receiver)) {
    RETURN_STRING_FMT("<native function %s>", AS_NATIVE_FUNCTION(receiver)->name->chars);
  }

  RETURN_STRING_FMT("<fn %s>", AS_CLOSURE(receiver)->function->name->chars);
}

NATIVE_METHOD(Function, __undefinedProperty__) {
  assertArgCount("Function::__undefinedProperty__(name)", 1, argCount);
  assertArgIsString("Function::__undefinedProperty__(name)", args, 0);
  ObjString* property = AS_STRING(args[0]);
  ObjClosure* self = AS_CLOSURE(receiver);

  if (matchStringName(property, "name", 4)) {
    if (IS_NATIVE_FUNCTION(receiver)) {
      RETURN_OBJ(AS_NATIVE_FUNCTION(receiver)->name);
    }

    RETURN_OBJ(self->function->name);
  } else if (matchStringName(property, "arity", 5)) {
    if (IS_NATIVE_FUNCTION(receiver)) {
      RETURN_INT(AS_NATIVE_FUNCTION(receiver)->arity);
    }

    RETURN_INT(self->function->arity);
  } else if (matchStringName(property, "upvalueCount", 12)) {
    RETURN_INT(self->upvalueCount);
  } else THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", 
    AS_CSTRING(args[0]), valueToString(receiver));
}

// BOUND METHOD

NATIVE_METHOD(BoundMethod, __init__) {
  assertArgCount("BoundMethod::__init__(object, method)", 2, argCount);
  if (IS_METHOD(args[1])) {
    ObjMethod* method = AS_METHOD(args[1]);
    if (!isObjInstanceOf(args[0], method->behavior)) {
      THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Cannot bound method to object.");
    }

    ObjBoundMethod* boundMethod = AS_BOUND_METHOD(receiver);
    boundMethod->receiver = args[0];
    boundMethod->method = OBJ_VAL(method->closure);
    RETURN_OBJ(boundMethod);
  } else if (IS_STRING(args[1])) {
    ObjClass* klass = getObjClass(args[0]);
    Value value;
    if (!tableGet(&klass->methods, AS_STRING(args[1]), &value)) {
      THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Cannot bound method to object.");
    }

    ObjBoundMethod* boundMethod = AS_BOUND_METHOD(receiver);
    boundMethod->receiver = args[0];
    boundMethod->method = value;
    RETURN_OBJ(boundMethod);
  } else {
    THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Method BoundMethod::__init__(object, method) expects argument 2 to be a method or string.");
  }
}

NATIVE_METHOD(BoundMethod, arity) {
  assertArgCount("BoundMethod::arity()", 0, argCount);
  Value method = AS_BOUND_METHOD(receiver)->method;
  RETURN_INT(IS_NATIVE_METHOD(method) ? AS_NATIVE_METHOD(method)->arity : AS_CLOSURE(method)->function->arity);
}

NATIVE_METHOD(BoundMethod, clone) {
  assertArgCount("BoundMethod::clone()", 0, argCount);
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(BoundMethod, isNative) { 
  assertArgCount("BoundMethod::isNative()", 0, argCount);
  RETURN_BOOL(AS_BOUND_METHOD(receiver)->isNative);
}

NATIVE_METHOD(BoundMethod, isVariadic) {
  assertArgCount("BoundMethod::isVariadic()", 0, argCount);
  Value method = AS_BOUND_METHOD(receiver)->method;
  int arity = IS_NATIVE_METHOD(method) ? AS_NATIVE_METHOD(method)->arity : AS_CLOSURE(method)->function->arity;
  RETURN_BOOL(arity == -1);
}

NATIVE_METHOD(BoundMethod, name) {
  assertArgCount("BoundMethod::name()", 0, argCount);
  ObjBoundMethod* boundMethod = AS_BOUND_METHOD(receiver);
  char* methodName = IS_NATIVE_METHOD(boundMethod->method) ? AS_NATIVE_METHOD(boundMethod->method)->name->chars : AS_CLOSURE(boundMethod->method)->function->name->chars;
  RETURN_STRING_FMT("%s::%s", getObjClass(boundMethod->receiver)->name->chars, methodName);
}

NATIVE_METHOD(BoundMethod, receiver) {
  assertArgCount("BoundMethod::receiver()", 0, argCount);
  RETURN_VAL(AS_BOUND_METHOD(receiver)->receiver);
}

NATIVE_METHOD(BoundMethod, __str__) {
  assertArgCount("BoundMethod::__str__()", 0, argCount);
  ObjBoundMethod* boundMethod = AS_BOUND_METHOD(receiver);
  char* methodName = IS_NATIVE_METHOD(boundMethod->method) ? AS_NATIVE_METHOD(boundMethod->method)->name->chars : AS_CLOSURE(boundMethod->method)->function->name->chars;
  RETURN_STRING_FMT("<bound method %s::%s>", getObjClass(boundMethod->receiver)->name->chars, methodName);
}

NATIVE_METHOD(BoundMethod, __format__) {
  assertArgCount("BoundMethod::__format__()", 0, argCount);
  ObjBoundMethod* boundMethod = AS_BOUND_METHOD(receiver);
  char* methodName = IS_NATIVE_METHOD(boundMethod->method) ? AS_NATIVE_METHOD(boundMethod->method)->name->chars : AS_CLOSURE(boundMethod->method)->function->name->chars;
  RETURN_STRING_FMT("<bound method %s::%s>", getObjClass(boundMethod->receiver)->name->chars, methodName);
}

NATIVE_METHOD(BoundMethod, upvalueCount) {
  assertArgCount("BoundMethod::upvalueCount()", 0, argCount);
  Value method = AS_BOUND_METHOD(receiver)->method;
  if (!IS_CLOSURE(method)) RETURN_INT(0);
  RETURN_INT(AS_CLOSURE(method)->upvalueCount);
}

NATIVE_METHOD(BoundMethod, __invoke__) {
  ObjBoundMethod* self = AS_BOUND_METHOD(receiver);
  RETURN_VAL(callMethod(self->method, argCount));
}

NATIVE_METHOD(BoundMethod, __undefinedProperty__) {
  assertArgCount("BoundMethod::__undefinedProperty__(name)", 1, argCount);
  assertArgIsString("BoundMethod::__undefinedProperty__(name)", args, 0);
  ObjString* property = AS_STRING(args[0]);
  ObjBoundMethod* self = AS_BOUND_METHOD(receiver);

  if (matchStringName(property, "method", 6)) {
    RETURN_OBJ(self->method);
  } else if (matchStringName(property, "arity", 5)) {
    RETURN_INT(AS_CLOSURE(self->method)->function->arity);
  } else if (matchStringName(property, "name", 4)) {
    RETURN_OBJ(AS_CLOSURE(self->method)->function->name);
  } else if (matchStringName(property, "receiver", 8)) {
    RETURN_OBJ(self->receiver);
  } else THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", 
    AS_CSTRING(args[0]), valueToString(receiver));
}

// METHOD


NATIVE_METHOD(Method, __init__) {
  assertArgCount("Method::__init__(behavior, name, closure)", 3, argCount);
  assertArgIsClass("Method::__init__(behavior, name, closure)", args, 0);
  assertArgIsString("Method::__init__(behavior, name, closure)", args, 1);
  assertArgIsClosure("Method::__init__(behavior, name, closure)", args, 2);

  ObjMethod* self = AS_METHOD(receiver);
  ObjClass* behavior = AS_CLASS(args[0]);
  ObjString* name = AS_STRING(args[1]);
  ObjClosure* closure = AS_CLOSURE(args[2]);

  Value value;
  if (tableGet(&behavior->methods, name, &value)) {
    THROW_EXCEPTION_FMT(luminique::std::lang, UnsupportedOperationException, "Method %s already exists in behavior %s.", name->chars, behavior->name->chars);
  }
  tableSet(&behavior->methods, name, OBJ_VAL(closure));

  self->behavior = behavior;
  self->closure = closure;
  self->closure->function->name = name;
  RETURN_OBJ(self);
}

NATIVE_METHOD(Method, arity) {
  assertArgCount("Method::arity()", 0, argCount);
  if (IS_NATIVE_METHOD(receiver)) {
    RETURN_INT(AS_NATIVE_METHOD(receiver)->arity);
  }
  RETURN_INT(AS_METHOD(receiver)->closure->function->arity);
}

NATIVE_METHOD(Method, behavior) {
  assertArgCount("Method::behavior()", 0, argCount);
  if (IS_NATIVE_METHOD(receiver)) {
    RETURN_OBJ(AS_NATIVE_METHOD(receiver)->klass);
  }
  RETURN_OBJ(AS_METHOD(receiver)->behavior);
}

NATIVE_METHOD(Method, bind) {
  assertArgCount("Method::bind(receiver)", 1, argCount);
  Value method = IS_NATIVE_METHOD(receiver) ? receiver : OBJ_VAL(AS_METHOD(receiver)->closure);
  RETURN_OBJ(newBoundMethod(args[1], method));
}

NATIVE_METHOD(Method, clone) {
  assertArgCount("Method::clone()", 0, argCount);
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(Method, isNative) {
  assertArgCount("method::isNative()", 0, argCount);
  RETURN_BOOL(IS_NATIVE_METHOD(receiver));
}

NATIVE_METHOD(Method, isVariadic) {
  assertArgCount("Method::isVariadic()", 0, argCount);
  RETURN_BOOL(AS_METHOD(receiver)->closure->function->arity == -1);
}

NATIVE_METHOD(Method, name) {
  assertArgCount("Method::name()", 0, argCount);
  if (IS_NATIVE_METHOD(receiver)) {
    ObjNativeMethod* nativeMethod = AS_NATIVE_METHOD(receiver);
    RETURN_STRING_FMT("%s::%s", nativeMethod->klass->name->chars, nativeMethod->name->chars);
  }
  ObjMethod* method = AS_METHOD(receiver);
  RETURN_STRING_FMT("%s::%s", method->behavior->name->chars, method->closure->function->name->chars);
}

NATIVE_METHOD(Method, __str__) {
  assertArgCount("Method::__str__()", 0, argCount);
  if (IS_NATIVE_METHOD(receiver)) {
    ObjNativeMethod* nativeMethod = AS_NATIVE_METHOD(receiver);
    RETURN_STRING_FMT("<method: %s::%s>", nativeMethod->klass->name->chars, nativeMethod->name->chars);
  }
  ObjMethod* method = AS_METHOD(receiver);
  RETURN_STRING_FMT("<method %s::%s>", method->behavior->name->chars, method->closure->function->name->chars);
}

NATIVE_METHOD(Method, __format__) {
  assertArgCount("Method::__format__()", 0, argCount);
  if (IS_NATIVE_METHOD(receiver)) {
    ObjNativeMethod* nativeMethod = AS_NATIVE_METHOD(receiver);
    RETURN_STRING_FMT("<method: %s::%s>", nativeMethod->klass->name->chars, nativeMethod->name->chars);
  }
  ObjMethod* method = AS_METHOD(receiver);
  RETURN_STRING_FMT("<method %s::%s>", method->behavior->name->chars, method->closure->function->name->chars);
}

NATIVE_METHOD(Method, __undefinedProperty__) {
  assertArgCount("Method::__undefinedProperty__(name)", 1, argCount);
  assertArgIsString("Method::__undefinedProperty__(name)", args, 0);
  ObjString* property = AS_STRING(args[0]);
  ObjMethod* self = AS_METHOD(receiver);

  if (matchStringName(property, "name", 4)) {
    if (IS_NATIVE_METHOD(receiver)) {
      RETURN_OBJ(AS_NATIVE_METHOD(receiver)->name);
    }
    RETURN_OBJ(self->closure->function->name);
  } else if (matchStringName(property, "behavior", 8)) {
    RETURN_OBJ(self->behavior);
  } else if (matchStringName(property, "arity", 5)) {
    if (IS_NATIVE_METHOD(receiver)) {
      RETURN_INT(AS_NATIVE_METHOD(receiver)->arity);
    }
    RETURN_INT(self->closure->function->arity);
  } else THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", 
    AS_CSTRING(args[0]), valueToString(receiver));
}

// INT

NATIVE_METHOD(Int, __init__) {
  assertArgCount("Int::__init__(value)", 1, argCount);
  assertArgIsNumber("Int::__int__(value)", args, 0);
  RETURN_INT(args[0]);
}

NATIVE_METHOD(Int, __add__) {
  assertArgCount("Int::+(other)", 1, argCount);
  assertArgIsNumber("Int::+(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) + AS_INT(args[0])));
  else RETURN_NUMBER((AS_NUMBER(receiver) + AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, __subtract__) {
  assertArgCount("Int::-(other)", 1, argCount);
  assertArgIsNumber("Int::-(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) - AS_INT(args[0])));
  else RETURN_NUMBER((AS_NUMBER(receiver) - AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, __multiply__) {
  assertArgCount("Int::*(other)", 1, argCount);
  assertArgIsNumber("Int::*(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) * AS_INT(args[0])));
  else RETURN_NUMBER((AS_NUMBER(receiver) * AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, __modulo__) {
  assertArgCount("Int::%(other)", 1, argCount);
  assertArgIsNumber("Int::%(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) % AS_INT(args[0])));
  else RETURN_NUMBER(fmod(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, clone) {
  assertArgCount("Int::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Int, __str__) {
  assertArgCount("Int::__str__()", 0, argCount);
  RETURN_STRING_FMT("%d", AS_INT(receiver));
}

NATIVE_METHOD(Int, __format__) {
  assertArgCount("Int::__format__()", 0, argCount);
  RETURN_STRING_FMT("%d", AS_INT(receiver));
}

// NIL

NATIVE_METHOD(Nil, __init__) {
	THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class Nil.");
}

NATIVE_METHOD(Nil, clone) {
	assertArgCount("Nil::clone()", 0, argCount);
	RETURN_NIL;
}

NATIVE_METHOD(Nil, __str__) {
	assertArgCount("Nil::__str__()", 0, argCount);
	RETURN_STRING("nil", 3);
}

NATIVE_METHOD(Nil, __format__) {
	assertArgCount("Nil::__format__()", 0, argCount);
	RETURN_STRING("nil", 3);
}

// NUMBER

NATIVE_METHOD(Number, __init__) {
  assertArgCount("Number::__init__(value)", 1, argCount);
  assertArgIsNumber("Number::__int__(value)", args, 0);
  RETURN_NUMBER(args[0]);
}

NATIVE_METHOD(Number, __equal__) {
  assertArgCount("Number::==(other)", 1, argCount);
  assertArgIsNumber("Number::==(other)", args, 0);
  RETURN_BOOL((AS_NUMBER(receiver) == AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __greater__) {
  assertArgCount("Number::>(other)", 1, argCount);
  assertArgIsNumber("Number::>(other)", args, 0);
  RETURN_BOOL((AS_NUMBER(receiver) > AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __less__) {
  assertArgCount("Number::<(other)", 1, argCount);
  assertArgIsNumber("Number::<(other)", args, 0);
  RETURN_BOOL((AS_NUMBER(receiver) < AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __add__) { 
  assertArgCount("Number::+(other)", 1, argCount);
  assertArgIsNumber("Number::+(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) + AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __subtract__) {
  assertArgCount("Number::-(other)", 1, argCount);
  assertArgIsNumber("Number::-(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) - AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __multiply__) {
  assertArgCount("Number::*(other)", 1, argCount);
  assertArgIsNumber("Number::*(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) * AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __divide__) { 
  assertArgCount("Number::/(other)", 1, argCount);
  assertArgIsNumber("Number::/(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) / AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __modulo__) {
  assertArgCount("Number::%(other)", 1, argCount);
  assertArgIsNumber("Number::%(other)", args, 0);
  RETURN_NUMBER(fmod(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __power__) {
  assertArgCount("Number::**(other)", 1, argCount);
  assertArgIsNumber("Number::**(other)", args, 0);
  RETURN_NUMBER(pow(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}
NATIVE_METHOD(Number, clone) {
	assertArgCount("Number::clone()", 0, argCount);
	return receiver;
}

NATIVE_METHOD(Number, __str__) {
	assertArgCount("Number::__str__()", 0, argCount);
	char chars[24];
	int length = snprintf(chars, 24, "%.14g", AS_NUMBER(receiver));
	RETURN_STRING(chars, length);
}

NATIVE_METHOD(Number, __format__) {
	assertArgCount("Number::__format__()", 0, argCount);
	char chars[24];
	int length = snprintf(chars, 24, "%.14g", AS_NUMBER(receiver));
	RETURN_STRING(chars, length);
}

// OBJECT


NATIVE_METHOD(Object, __equal__) {
  assertArgCount("Object::==(other)", 1, argCount);
  RETURN_BOOL(receiver == args[0]);
}

NATIVE_METHOD(Object, __undefinedProperty__) {
  assertArgCount("Object::__undefinedProperty__(name)", 1, argCount);
  assertArgIsString("Object::__undefinedProperty__(name)", args, 0);
  THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", 
    AS_CSTRING(args[0]), valueToString(receiver));
}

NATIVE_METHOD(Object, __undefinedMethod__) {
  assertArgCount("Object::__undefinedMethod__(name, args)", 2, argCount);
  assertArgIsString("Object::__undefinedMethod__(name, args)", args, 0);
  assertArgIsArray("Object::__undefinedMethod__(name, args)", args, 1);
  THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Method %s does not exist in class %s.", 
    AS_CSTRING(args[0]), getObjClass(receiver)->name->chars);
}

NATIVE_METHOD(Object, clone) {
	assertArgCount("Object::clone()", 0, argCount);
	ObjInstance* thisObject = AS_INSTANCE(receiver);
  ObjInstance* thatObject = newInstance(OBJ_KLASS(receiver));
  push(OBJ_VAL(thatObject));
	tableAddAll(&thisObject->fields, &thatObject->fields);
  pop();
	RETURN_OBJ(thatObject);
}

NATIVE_METHOD(Object, equals) {
	assertArgCount("Object::equals(value)", 1, argCount);
	RETURN_BOOL(valuesEqual(receiver, args[0]));
}

NATIVE_METHOD(Object, getClass) {
	assertArgCount("Object::getClass()", 0, argCount);
	RETURN_OBJ(getObjClass(receiver));
}

NATIVE_METHOD(Object, getClassName) {
	assertArgCount("Object::getClassName()", 0, argCount);
	RETURN_OBJ(getObjClass(receiver)->name);
}

NATIVE_METHOD(Object, hasField) {
	assertArgCount("Object::hasField(field)", 1, argCount);
	assertArgIsString("Object::hasField(field)", args, 0);
	if (!IS_INSTANCE(receiver)) RETURN_BOOL(false);
	ObjInstance* instance = AS_INSTANCE(receiver);
	Value value;
	RETURN_BOOL(tableGet(&instance->fields, AS_STRING(args[0]), &value));
}

NATIVE_METHOD(Object, hashCode) {
	assertArgCount("Object::hashCode()", 0, argCount);
	RETURN_INT(hashValue(receiver));
}

NATIVE_METHOD(Object, instanceOf) {
	assertArgCount("Object::instanceOf(class)", 1, argCount);
	if (!IS_CLASS(args[0])) RETURN_FALSE;
  RETURN_BOOL(isObjInstanceOf(receiver, AS_CLASS(args[0])));
}

NATIVE_METHOD(Object, memberOf) {
	assertArgCount("Object::memberOf(class)", 1, argCount);
	if (!IS_CLASS(args[0])) RETURN_FALSE;
	ObjClass* thisClass = getObjClass(receiver);
	ObjClass* thatClass = AS_CLASS(args[0]);
	RETURN_BOOL(thisClass == thatClass);
}

NATIVE_METHOD(Object, __str__) {
	assertArgCount("Object::__str__()", 0, argCount);
  RETURN_STRING_FMT("<object %s>", AS_OBJ(receiver)->klass->name->chars);
}

NATIVE_METHOD(Object, __format__) {
	assertArgCount("Object::__format__()", 0, argCount);
  RETURN_STRING_FMT("<object %s>", AS_OBJ(receiver)->klass->name->chars);
}

// STRING

NATIVE_METHOD(String, __init__) {
  assertArgCount("String::__init__(chars)", 1, argCount);
  assertArgIsString("String::__init__(chars)", args, 0);
  return args[0];
}


NATIVE_METHOD(String, __add__) {
  assertArgCount("String::+(other)", 1, argCount);
  assertArgIsString("String::+(other)", args, 0);
  RETURN_STRING_FMT("%s%s", AS_CSTRING(receiver), AS_CSTRING(args[0]));
}

NATIVE_METHOD(String, __getSubscript__) {
  assertArgCount("String::[](index)", 1, argCount);
  assertArgIsInt("String::[getChar]](index)", args, 0);

  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  assertIntWithinRange("String::[](index)", index, 0, self->length, 0);

  char chars[2] = { self->chars[index], '\0' };
  RETURN_STRING(chars, 1);
}

NATIVE_METHOD(String, capitalize) {
  assertArgCount("String::capitalize()", 0, argCount);
  RETURN_OBJ(capitalizeString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, clone) {
  assertArgCount("String::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(String, contains) {
  assertArgCount("String::contains(chars)", 1, argCount);
  assertArgIsString("String::contains(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  RETURN_BOOL(searchString(haystack, needle, 0) != -1);
}

NATIVE_METHOD(String, decapitalize) {
  assertArgCount("String::decapitalize()", 0, argCount);
  RETURN_OBJ(decapitalizeString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, ends) {
  assertArgCount("String::ends(chars)", 1, argCount);
  assertArgIsString("String::ends(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  if (needle->length > haystack->length) RETURN_FALSE;
  RETURN_BOOL(memcmp(haystack->chars + haystack->length - needle->length, needle->chars, needle->length) == 0);
}

NATIVE_METHOD(String, subscript) {
  assertArgCount("String::subscript(index)", 1, argCount);
  assertArgIsInt("String::subscript(index)", args, 0);

  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  assertNumberWithinRange("String::subscript(index)", index, 0, self->length, 0);

  char chars[2] = { self->chars[index], '\0' };
  RETURN_STRING(chars, 1);
}

NATIVE_METHOD(String, search) {
  assertArgCount("String::search(chars)", 1, argCount);
  assertArgIsString("String::search(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  RETURN_INT(searchString(haystack, needle, 0));
}

NATIVE_METHOD(String, next) {
  assertArgCount("String::next(index)", 1, argCount);
  ObjString* self = AS_STRING(receiver);
  if (IS_NIL(args[0])) {
    if (self->length == 0) RETURN_FALSE;
    RETURN_INT(0);
  }

  assertArgIsInt("String::next(index)", args, 0);
  int index = AS_INT(args[0]);
  if (index < 0 || index < self->length - 1) RETURN_INT(index + 1);
  RETURN_NIL;
}

NATIVE_METHOD(String, nextValue) {
  assertArgCount("String::nextValue(index)", 1, argCount);
  assertArgIsInt("String::nextValue(index)", args, 0);
  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  if (index > -1 && index < self->length) {
    char chars[2] = { self->chars[index], '\0' };
    RETURN_STRING(chars, 1);
  }
  RETURN_NIL;
}

NATIVE_METHOD(String, replace) {
  assertArgCount("String::replace(target, replacement)", 2, argCount);
  assertArgIsString("String::replace(target, replacement)", args, 0);
  assertArgIsString("String::replace(target, replacement)", args, 1);
  RETURN_OBJ(replaceString(AS_STRING(receiver), AS_STRING(args[0]), AS_STRING(args[1])));
}

NATIVE_METHOD(String, reverse) {
  assertArgCount("String::reverse()", 0, argCount);
  ObjString* self = AS_STRING(receiver);
  if (self->length <= 1) return receiver;
  return OBJ_VAL(reverseStringBasedOnMemory(self));
}

NATIVE_METHOD(String, split) {
  assertArgCount("String::split(delimiter)", 1, argCount);
  assertArgIsString("String::split(delimiter)", args, 0);
  ObjString* self = AS_STRING(receiver);
  ObjString* delimiter = AS_STRING(args[0]);

  ObjArray* array = newArray();
  push(OBJ_VAL(array));
  char* string = strdup(self->chars);
  char* next = NULL;
  char* token = strtok_r(string, delimiter->chars, &next);
  while (token != NULL) {
    writeValueArray(&array->elements, OBJ_VAL(copyString(token, (int)strlen(token))));
    token = strtok_r(NULL, delimiter->chars, &next);
  }
  free(string);
  pop();
  RETURN_OBJ(array);
}

NATIVE_METHOD(String, starts) {
  assertArgCount("String::starts(chars)", 1, argCount);
  assertArgIsString("String::starts(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  if (needle->length > haystack->length) RETURN_FALSE;
  RETURN_BOOL(memcmp(haystack->chars, needle->chars, needle->length) == 0);
}

NATIVE_METHOD(String, cut) {
  assertArgCount("String::cut(from, to)", 2, argCount);
  assertArgIsInt("String::cut(from, to)", args, 0);
  assertArgIsInt("String::cut(from, to)", args, 1);
  RETURN_OBJ(subString(AS_STRING(receiver), AS_INT(args[0]), AS_INT(args[1])));
}

NATIVE_METHOD(String, lower) {
  assertArgCount("String::lower()", 0, argCount);
  RETURN_OBJ(toLowerString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, __str__) {
  assertArgCount("String::__str__()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(String, __format__) {
  assertArgCount("String::__format__()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(String, upper) {
  assertArgCount("String::upper()", 0, argCount);
  RETURN_OBJ(toUpperString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, trim) {
  assertArgCount("String::trim()", 0, argCount);
  RETURN_OBJ(trimString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, __undefinedProperty__) {
  assertArgCount("String::__undefinedProperty__(name)", 1, argCount);
  assertArgIsString("String::__undefinedProperty__(name)", args, 0);
  ObjString* property = AS_STRING(args[0]);
  ObjString* self = AS_STRING(receiver);

  if (matchStringName(property, "length", 6)) {
    RETURN_INT(self->length);
  } else THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", 
    AS_CSTRING(args[0]), valueToString(receiver));
}

NATIVE_METHOD(Namespace, __init__) {
  THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class Namespace.");
  RETURN_NIL;
}

NATIVE_METHOD(Namespace, enclosing) {
  assertArgCount("Namespace::enclosing()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  if (self->enclosing != NULL && self->enclosing->enclosing != NULL) RETURN_OBJ(self->enclosing);
  RETURN_NIL;
}

NATIVE_METHOD(Namespace, fullName) {
  assertArgCount("Namespace::fullName()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  RETURN_OBJ(self->fullName);
}

NATIVE_METHOD(Namespace, shortName) {
  assertArgCount("Namespace::shortName()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  RETURN_OBJ(self->shortName);
}

NATIVE_METHOD(Namespace, clone) {
  assertArgCount("Namespace::clone()", 0, argCount);
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(Namespace, __str__) {
  assertArgCount("Namespace::__str__()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  RETURN_STRING_FMT("<namespace %s>", self->fullName->chars);
}

NATIVE_METHOD(Namespace, __format__) {
  assertArgCount("Namespace::__format__()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  RETURN_STRING_FMT("<namespace %s>", self->fullName->chars);
}

static void bindNamespaceClass() {
  for (int i = 0; i < vm.namespaces.capacity; i++) {
    Entry* entry = &vm.namespaces.entries[i];
    if (entry->key == NULL) continue;
    entry->key->obj.klass = vm.namespaceClass;
  }
}

static ObjNamespace* defineRootNamespace() {
  ObjString* name = newString("");
  push(OBJ_VAL(name));
  ObjNamespace* rootNamespace = newNamespace(name, NULL);
  rootNamespace->isRoot = true;
  push(OBJ_VAL(rootNamespace));
  tableSet(&vm.namespaces, name, OBJ_VAL(rootNamespace));
  pop();
  pop();
  return rootNamespace;
}

void registerLangPackage() {
  vm.rootNamespace = defineRootNamespace();
  vm.luminiqueNamespace = defineNativeNamespace("luminique", vm.rootNamespace);
  vm.stdNamespace = defineNativeNamespace("std", vm.luminiqueNamespace);
  vm.langNamespace = defineNativeNamespace("lang", vm.stdNamespace);
  vm.currentNamespace = vm.langNamespace;

	vm.objectClass = defineNativeClass("Object");
  vm.objectClass->classType = OBJ_INSTANCE;
  DEF_METHOD(vm.objectClass, Object, clone, 0);
  DEF_METHOD(vm.objectClass, Object, equals, 1);
  DEF_METHOD(vm.objectClass, Object, getClass, 0);
  DEF_METHOD(vm.objectClass, Object, getClassName, 0);
  DEF_METHOD(vm.objectClass, Object, hasField, 1);
  DEF_METHOD(vm.objectClass, Object, hashCode, 0);
  DEF_METHOD(vm.objectClass, Object, instanceOf, 1);
  DEF_METHOD(vm.objectClass, Object, memberOf, 1);
  DEF_METHOD(vm.objectClass, Object, __str__, 0);
  DEF_METHOD(vm.objectClass, Object, __format__, 0);

  DEF_OPERATOR(vm.objectClass, Object, ==, __equal__, 1);
  DEF_INTERCEPTOR(vm.objectClass, Object, INTERCEPTOR_UNDEFINED_PROPERTY, __undefinedProperty__, 1);
  DEF_INTERCEPTOR(vm.objectClass, Object, INTERCEPTOR_UNDEFINED_METHOD, __undefinedMethod__, 2);

  vm.classClass = defineNativeClass("Class");
  bindSuperclass(vm.classClass, vm.objectClass);
  vm.classClass->classType = OBJ_CLASS;
  DEF_METHOD(vm.classClass, Class, __init__, 2);
  DEF_METHOD(vm.classClass, Class, clone, 0);
  DEF_METHOD(vm.classClass, Class, getClass, 0);
  DEF_METHOD(vm.classClass, Class, getClassName, 0);
  DEF_METHOD(vm.classClass, Class, instanceOf, 1);
  DEF_METHOD(vm.classClass, Class, memberOf, 1);
  DEF_METHOD(vm.classClass, Class, name, 0);
  DEF_METHOD(vm.classClass, Class, superclass, 0);
  DEF_METHOD(vm.classClass, Class, __str__, 0);
  DEF_METHOD(vm.classClass, Class, __format__, 0);
  DEF_OPERATOR(vm.classClass, Class, (), __invoke__, -1);
  vm.objectClass->obj.klass = vm.classClass;

  vm.enumClass = defineNativeClass("Enum");
  bindSuperclass(vm.enumClass, vm.objectClass);
  vm.enumClass->classType = OBJ_ENUM;
  DEF_METHOD(vm.enumClass, Enum, __init__, 1);
  DEF_METHOD(vm.enumClass, Enum, clone, 0);
  DEF_METHOD(vm.enumClass, Enum, name, 0);
  DEF_METHOD(vm.enumClass, Enum, __str__, 0);
  DEF_METHOD(vm.enumClass, Enum, __format__, 0);

  vm.namespaceClass = defineNativeClass("Namespace");
  bindSuperclass(vm.namespaceClass, vm.objectClass);
  vm.namespaceClass->classType = OBJ_NAMESPACE;
  DEF_METHOD(vm.namespaceClass, Namespace, __init__, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, clone, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, enclosing, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, fullName, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, shortName, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, __str__, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, __format__, 0);
  bindNamespaceClass();

  vm.exceptionClass = defineNativeClass("Exception");
  bindSuperclass(vm.exceptionClass, vm.objectClass);
  vm.exceptionClass->classType = OBJ_EXCEPTION;
  DEF_METHOD(vm.exceptionClass, Exception, __init__, 1);
  DEF_METHOD(vm.exceptionClass, Exception, __str__, 0);
  DEF_METHOD(vm.exceptionClass, Exception, __format__, 0);

  DEF_INTERCEPTOR(vm.exceptionClass, Exception, INTERCEPTOR_UNDEFINED_PROPERTY, __undefinedProperty__, 1);

  ObjClass* runtimeExceptionClass = defineNativeException("RuntimeException", vm.exceptionClass);
  defineNativeException("ArithmeticException", runtimeExceptionClass);
  defineNativeException("IllegalArgumentException", runtimeExceptionClass);
  defineNativeException("OutOfMemoryException", runtimeExceptionClass);
  defineNativeException("IndexOutOfBoundsException", runtimeExceptionClass);
  defineNativeException("UnsupportedOperationException", runtimeExceptionClass);
  defineNativeException("NotImplementedException", runtimeExceptionClass);
  defineNativeException("AssertException", runtimeExceptionClass);
  defineNativeException("InstantiationException", runtimeExceptionClass);
  defineNativeException("CallException", runtimeExceptionClass);

  vm.generatorClass = defineNativeClass("Generator");
  bindSuperclass(vm.generatorClass, vm.objectClass);
  vm.generatorClass->classType = OBJ_GENERATOR;
  DEF_METHOD(vm.generatorClass, Generator, __init__, 1);
  DEF_METHOD(vm.generatorClass, Generator, isFinished, 0);
  DEF_METHOD(vm.generatorClass, Generator, isSuspended, 0);
  DEF_METHOD(vm.generatorClass, Generator, isReady, 0);
  DEF_METHOD(vm.generatorClass, Generator, getReceiver, 0);
  DEF_METHOD(vm.generatorClass, Generator, setReceiver, 1);
  DEF_METHOD(vm.generatorClass, Generator, nextFinished, 0);
  DEF_METHOD(vm.generatorClass, Generator, next, 0);
  DEF_METHOD(vm.generatorClass, Generator, returns, 1);
  DEF_METHOD(vm.generatorClass, Generator, send, 1);
  DEF_METHOD(vm.generatorClass, Generator, throws, 1);
  DEF_METHOD(vm.generatorClass, Generator, __str__, 0);
  DEF_METHOD(vm.generatorClass, Generator, __format__, 0);
  DEF_OPERATOR(vm.generatorClass, Generator, (), __invoke__, -1);

  DEF_INTERCEPTOR(vm.generatorClass, Generator, INTERCEPTOR_UNDEFINED_PROPERTY, __undefinedProperty__, 1);

  ObjEnum* generatorStateEnum = defineNativeEnum("GeneratorState");
  defineNativeArtificialEnumElement(generatorStateEnum, "stateStart", INT_VAL(GENERATOR_START));
  defineNativeArtificialEnumElement(generatorStateEnum, "stateStart", INT_VAL(GENERATOR_START));
  defineNativeArtificialEnumElement(generatorStateEnum, "stateYield", INT_VAL(GENERATOR_YIELD));
  defineNativeArtificialEnumElement(generatorStateEnum, "stateResume", INT_VAL(GENERATOR_RESUME));
  defineNativeArtificialEnumElement(generatorStateEnum, "stateReturn", INT_VAL(GENERATOR_RETURN));
  defineNativeArtificialEnumElement(generatorStateEnum, "stateThrow", INT_VAL(GENERATOR_THROW));
  defineNativeArtificialEnumElement(generatorStateEnum, "stateError", INT_VAL(GENERATOR_ERROR));

  vm.promiseClass = defineNativeClass("Promise");
  bindSuperclass(vm.promiseClass, vm.objectClass);
  vm.promiseClass->classType = OBJ_PROMISE;
  DEF_METHOD(vm.promiseClass, Promise, __init__, 1);
  DEF_METHOD(vm.promiseClass, Promise, catch, 1);
  DEF_METHOD(vm.promiseClass, Promise, catchAll, 1);
  DEF_METHOD(vm.promiseClass, Promise, finally, 1);
  DEF_METHOD(vm.promiseClass, Promise, fulfill, 1);
  DEF_METHOD(vm.promiseClass, Promise, isResolved, 0);
  DEF_METHOD(vm.promiseClass, Promise, raceAll, 1);
  DEF_METHOD(vm.promiseClass, Promise, reject, 1);
  DEF_METHOD(vm.promiseClass, Promise, then, 1);
  DEF_METHOD(vm.promiseClass, Promise, thenAll, 1);
  DEF_METHOD(vm.promiseClass, Promise, __str__, 0);
  DEF_METHOD(vm.promiseClass, Promise, __format__, 0);

  DEF_INTERCEPTOR(vm.promiseClass, Promise, INTERCEPTOR_UNDEFINED_PROPERTY, __undefinedProperty__, 1);

  ObjEnum* promiseStateEnum = defineNativeEnum("PromiseState");
  defineNativeArtificialEnumElement(promiseStateEnum, "statePending", INT_VAL(PROMISE_PENDING));
  defineNativeArtificialEnumElement(promiseStateEnum, "stateFulfilled", INT_VAL(PROMISE_FULFILLED));
  defineNativeArtificialEnumElement(promiseStateEnum, "stateRejected", INT_VAL(PROMISE_REJECTED));

  ObjClass* promiseMetaclass = vm.promiseClass->obj.klass;
  DEF_METHOD(promiseMetaclass, PromiseClass, all, 1);
  DEF_METHOD(promiseMetaclass, PromiseClass, fulfill, 1);
  DEF_METHOD(promiseMetaclass, PromiseClass, race, 1);
  DEF_METHOD(promiseMetaclass, PromiseClass, reject, 1);

	vm.nilClass = defineNativeClass("Nil");
	bindSuperclass(vm.nilClass, vm.objectClass);
	DEF_METHOD(vm.nilClass, Nil, __init__, 0);
	DEF_METHOD(vm.nilClass, Nil, clone, 0);
	DEF_METHOD(vm.nilClass, Nil, __str__, 0);
	DEF_METHOD(vm.nilClass, Nil, __format__, 0);

	vm.boolClass = defineNativeClass("Bool");
	bindSuperclass(vm.boolClass, vm.objectClass);
	DEF_METHOD(vm.boolClass, Bool, __init__, 0);
	DEF_METHOD(vm.boolClass, Bool, clone, 0);
	DEF_METHOD(vm.boolClass, Bool, __str__, 0);
	DEF_METHOD(vm.boolClass, Bool, __format__, 0);

	vm.numberClass = defineNativeClass("Number");
	bindSuperclass(vm.numberClass, vm.objectClass);
  DEF_METHOD(vm.numberClass, Number, __init__, 0);
  DEF_METHOD(vm.numberClass, Number, clone, 0);
  DEF_METHOD(vm.numberClass, Number, __str__, 0);
  DEF_METHOD(vm.numberClass, Number, __format__, 0);
  DEF_OPERATOR(vm.numberClass, Number, ==, __equal__, 1);
  DEF_OPERATOR(vm.numberClass, Number, >, __greater__, 1);
  DEF_OPERATOR(vm.numberClass, Number, <, __less__, 1);
  DEF_OPERATOR(vm.numberClass, Number, +, __add__, 1);
  DEF_OPERATOR(vm.numberClass, Number, -, __subtract__, 1);
  DEF_OPERATOR(vm.numberClass, Number, *, __multiply__, 1);
  DEF_OPERATOR(vm.numberClass, Number, /, __divide__, 1);
  DEF_OPERATOR(vm.numberClass, Number, %, __modulo__, 1);
  DEF_OPERATOR(vm.numberClass, Number, **, __power__, 1);

  vm.intClass = defineNativeClass("Int");
  bindSuperclass(vm.intClass, vm.numberClass);
  DEF_METHOD(vm.intClass, Int, __init__, 0);
  DEF_METHOD(vm.intClass, Int, clone, 0);
  DEF_METHOD(vm.intClass, Int, __str__, 0);
  DEF_METHOD(vm.intClass, Int, __format__, 0);
  DEF_OPERATOR(vm.intClass, Int, +, __add__, 1);
  DEF_OPERATOR(vm.intClass, Int, -, __subtract__, 1);
  DEF_OPERATOR(vm.intClass, Int, *, __multiply__, 1);
  DEF_OPERATOR(vm.intClass, Int, %, __modulo__, 1);

  vm.floatClass = defineNativeClass("Float");
  bindSuperclass(vm.floatClass, vm.numberClass);
  DEF_METHOD(vm.floatClass, Float, __init__, 0);
  DEF_METHOD(vm.floatClass, Float, clone, 0);
  DEF_METHOD(vm.floatClass, Float, __str__, 0);
  DEF_METHOD(vm.floatClass, Float, __format__, 0);

  vm.stringClass = defineNativeClass("String");
  bindSuperclass(vm.stringClass, vm.objectClass);
  vm.stringClass->classType = OBJ_STRING;
  DEF_METHOD(vm.stringClass, String, __init__, 1);
  DEF_METHOD(vm.stringClass, String, capitalize, 0);
  DEF_METHOD(vm.stringClass, String, clone, 0);
  DEF_METHOD(vm.stringClass, String, contains, 1);
  DEF_METHOD(vm.stringClass, String, decapitalize, 0);
  DEF_METHOD(vm.stringClass, String, ends, 1);
  DEF_METHOD(vm.stringClass, String, subscript, 1);
  DEF_METHOD(vm.stringClass, String, search, 1);
  DEF_METHOD(vm.stringClass, String, next, 1);
  DEF_METHOD(vm.stringClass, String, nextValue, 1);
  DEF_METHOD(vm.stringClass, String, replace, 2);
  DEF_METHOD(vm.stringClass, String, reverse, 0);
  DEF_METHOD(vm.stringClass, String, split, 1);
  DEF_METHOD(vm.stringClass, String, starts, 1);
  DEF_METHOD(vm.stringClass, String, cut, 2);
  DEF_METHOD(vm.stringClass, String, lower, 0);
  DEF_METHOD(vm.stringClass, String, __str__, 0);
  DEF_METHOD(vm.stringClass, String, __format__, 0);
  DEF_METHOD(vm.stringClass, String, upper, 0);
  DEF_METHOD(vm.stringClass, String, trim, 0);

  DEF_OPERATOR(vm.stringClass, String, +, __add__, 1);
  DEF_OPERATOR(vm.stringClass, String, [], __getSubscript__, 1);
  DEF_INTERCEPTOR(vm.stringClass, String, INTERCEPTOR_UNDEFINED_PROPERTY, __undefinedProperty__, 1);

  for (int i = 0; i < vm.strings.capacity; i++) {
    Entry* entry = &vm.strings.entries[i];
    if (entry->key == NULL) continue;
    entry->key->obj.klass = vm.stringClass;
  }

  vm.functionClass = defineNativeClass("Function");
  bindSuperclass(vm.functionClass, vm.objectClass);
  vm.functionClass->classType = OBJ_FUNCTION;
  DEF_METHOD(vm.functionClass, Function, __init__, 0);
  DEF_METHOD(vm.functionClass, Function, clone, 0);
  DEF_METHOD(vm.functionClass, Function, isNative, 0);
  DEF_METHOD(vm.functionClass, Function, __str__, 0);
  DEF_METHOD(vm.functionClass, Function, __format__, 0);
  DEF_OPERATOR(vm.functionClass, Function, (), __invoke__, -1);

  DEF_INTERCEPTOR(vm.functionClass, Function, INTERCEPTOR_UNDEFINED_PROPERTY, __undefinedProperty__, 1);

  vm.methodClass = defineNativeClass("Method");
  bindSuperclass(vm.methodClass, vm.objectClass);
  vm.methodClass->classType = OBJ_METHOD;
  DEF_INTERCEPTOR(vm.methodClass, Method, INTERCEPTOR_INIT, __init__, 0);
  DEF_METHOD(vm.methodClass, Method, arity, 0);
  DEF_METHOD(vm.methodClass, Method, behavior, 0);
  DEF_METHOD(vm.methodClass, Method, bind, 1);
  DEF_METHOD(vm.methodClass, Method, clone, 0);
  DEF_METHOD(vm.methodClass, Method, isNative, 0);
  DEF_METHOD(vm.methodClass, Method, isVariadic, 0);
  DEF_METHOD(vm.methodClass, Method, name, 0);
  DEF_METHOD(vm.methodClass, Method, __str__, 0);
  DEF_METHOD(vm.methodClass, Method, __format__, 0);

  DEF_INTERCEPTOR(vm.methodClass, Method, INTERCEPTOR_UNDEFINED_PROPERTY, __undefinedProperty__, 1);

  vm.boundMethodClass = defineNativeClass("BoundMethod");
  bindSuperclass(vm.boundMethodClass, vm.objectClass);
  vm.boundMethodClass->classType = OBJ_BOUND_METHOD;
  DEF_INTERCEPTOR(vm.boundMethodClass, BoundMethod, INTERCEPTOR_INIT, __init__, 2);
  DEF_METHOD(vm.boundMethodClass, BoundMethod, arity, 0);
  DEF_METHOD(vm.boundMethodClass, BoundMethod, clone, 0);
  DEF_METHOD(vm.boundMethodClass, BoundMethod, isNative, 0);
  DEF_METHOD(vm.boundMethodClass, BoundMethod, isVariadic, 0);
  DEF_METHOD(vm.boundMethodClass, BoundMethod, name, 0);
  DEF_METHOD(vm.boundMethodClass, BoundMethod, receiver, 0);
  DEF_METHOD(vm.boundMethodClass, BoundMethod, upvalueCount, 0);
  DEF_METHOD(vm.boundMethodClass, BoundMethod, __str__, 0);
  DEF_METHOD(vm.boundMethodClass, BoundMethod, __format__, 0);
  DEF_OPERATOR(vm.boundMethodClass, BoundMethod, (), __invoke__, -1);

  DEF_INTERCEPTOR(vm.boundMethodClass, BoundMethod, INTERCEPTOR_UNDEFINED_PROPERTY, __undefinedProperty__, 1);

  vm.currentNamespace = vm.rootNamespace;
}
