#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "promise.h"
#include "../collection/collection.h"
#include "../object/object.h"
#include "../vm/vm.h"

ObjPromise* promiseAll(ObjClass* klass, ObjArray* promises) {
  int remainingCount = promises->elements.count;

  ObjPromise* allPromise = newPromise(PROMISE_PENDING, NIL_VAL, NIL_VAL);
  allPromise->obj.klass = klass;
  push(OBJ_VAL(allPromise));
  if (remainingCount == 0) allPromise->state = PROMISE_FULFILLED;
  else {
    ObjArray* results = newArray();
    push(OBJ_VAL(results));
    for (int i = 0; i < promises->elements.count; i++) {
      ObjPromise* promise = AS_PROMISE(promises->elements.values[i]);
      promiseCapture(promise, "promises", OBJ_VAL(promises));
      promiseCapture(promise, "allPromise", OBJ_VAL(allPromise));
      promiseCapture(promise, "results", OBJ_VAL(results));
      promiseCapture(promise, "remainingCount", INT_VAL(remainingCount));
      promiseCapture(promise, "index", INT_VAL(i));
    }
    pop();

    for (int i = 0; i < promises->elements.count; i++) {
      ObjPromise* promise = AS_PROMISE(promises->elements.values[i]);
      Value then = getObjMethod(OBJ_VAL(promise), "then");
      Value thenAll = getObjMethod(OBJ_VAL(promise), "thenAll");
      ObjBoundMethod* thenAllMethod = newBoundMethod(OBJ_VAL(promise), thenAll);
      callReentrantMethod(OBJ_VAL(promise), then, OBJ_VAL(thenAllMethod));

      Value catch = getObjMethod(OBJ_VAL(promise), "catch");
      Value catchAll = getObjMethod(OBJ_VAL(promise), "catchAll");
      ObjBoundMethod* catchAllMethod = newBoundMethod(OBJ_VAL(promise), catchAll);
      callReentrantMethod(OBJ_VAL(promise), catch, OBJ_VAL(catchAllMethod));
    }
  }
  pop();
  return allPromise;
}

bool promiseCapture(ObjPromise* promise, const char* name, Value value) {
  ObjString* key = newString(name);
  return dictSet(promise->captures, OBJ_VAL(key), value);
}

Value promiseLoad(ObjPromise* promise, const char* name) {
  ObjString* key = newString(name);
  Value value;
  if (!dictGet(promise->captures, OBJ_VAL(key), &value)) return NIL_VAL;
  else return value;
}

void promiseExecute(ObjPromise* promise) {
  Value fulfill = getObjMethod(OBJ_VAL(promise), "fulfill");
  Value reject = getObjMethod(OBJ_VAL(promise), "reject");

  ObjBoundMethod* onFulfill = newBoundMethod(OBJ_VAL(promise), fulfill);
  ObjBoundMethod* onReject = newBoundMethod(OBJ_VAL(promise), reject);
  callReentrantMethod(OBJ_VAL(promise), promise->executor, OBJ_VAL(onFulfill), OBJ_VAL(onReject));
}

void promiseFulfill(ObjPromise* promise, Value value) {
  promise->state = PROMISE_FULFILLED;
  promise->value = value;
  for (int i = 0; i < promise->handlers.count; i++) {
    promise->value = callReentrantMethod(OBJ_VAL(promise), promise->handlers.values[i], promise->value);
  }
  initValueArray(&promise->handlers);
  if (IS_CLOSURE(promise->onFinally)) callReentrantMethod(OBJ_VAL(promise), promise->onFinally, promise->value);
}

ObjPromise* promiseRace(ObjClass* klass, ObjArray* promises) {
  ObjPromise* racePromise = newPromise(PROMISE_PENDING, NIL_VAL, NIL_VAL);
  racePromise->obj.klass = klass;
  push(OBJ_VAL(racePromise));

  for (int i = 0; i < promises->elements.count; i++) {
    ObjPromise* promise = AS_PROMISE(promises->elements.values[i]);
    promiseCapture(promise, "racePromise", OBJ_VAL(racePromise));
    Value then = getObjMethod(OBJ_VAL(promise), "then");
    Value raceAll = getObjMethod(OBJ_VAL(promise), "raceAll");
    ObjBoundMethod* raceAllMethod = newBoundMethod(OBJ_VAL(promise), raceAll);
    callReentrantMethod(OBJ_VAL(promise), then, OBJ_VAL(raceAllMethod));
  }
  pop();
  return racePromise;
}

ObjPromise* promiseWithThen(ObjPromise* promise) {
  if (promise->captures->count == 0) return newPromise(PROMISE_PENDING, NIL_VAL, NIL_VAL);
  Value thenPromise = promiseLoad(promise, "thenPromise");
  return IS_NIL(thenPromise) ? newPromise(PROMISE_PENDING, NIL_VAL, NIL_VAL) : AS_PROMISE(thenPromise);
}

void promiseReject(ObjPromise* promise, Value exception) {
  promise->state = PROMISE_REJECTED;
  promise->exception = AS_EXCEPTION(exception);
  if (IS_CLOSURE(promise->onCatch)) callReentrantMethod(OBJ_VAL(promise), promise->onCatch, exception);
  if (IS_CLOSURE(promise->onFinally)) callReentrantMethod(OBJ_VAL(promise), promise->onFinally, promise->value);
}

void promiseThen(ObjPromise* promise, Value value) {
  for (int i = 0; i < promise->handlers.count; i++) {
    ObjBoundMethod* handler = newBoundMethod(OBJ_VAL(promise), promise->handlers.values[i]);
    callReentrantMethod(OBJ_VAL(promise), handler->method, value);
  }
  initValueArray(&promise->handlers);
}

ObjPromise* promiseWithFulfilled(Value value) {
  return newPromise(PROMISE_FULFILLED, value, NIL_VAL);
}

ObjPromise* promiseWithRejected(ObjException* exception) {
  ObjPromise* promise = newPromise(PROMISE_REJECTED, NIL_VAL, NIL_VAL);
  promise->exception = exception;
  return promise;
}

void promisePushHandler(ObjPromise* promise, Value handler, ObjPromise* thenPromise) {
  if (promise->state == PROMISE_FULFILLED)  callReentrantMethod(OBJ_VAL(thenPromise), handler, promise->value);
  else writeValueArray(&promise->handlers, handler);
}
