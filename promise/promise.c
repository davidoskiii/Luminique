#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "promise.h"
#include "../object/object.h"
#include "../vm/vm.h"

ObjPromise* promiseAll(ObjClass* klass, ObjArray* promises) {
  int remainingCount = promises->elements.count;

  ObjPromise* allPromise = newPromise(NIL_VAL);
  push(OBJ_VAL(allPromise));
  allPromise->obj.klass = klass;
  if (remainingCount == 0) allPromise->state = PROMISE_FULFILLED;
  else {
    ObjArray* results = newArray();
    push(OBJ_VAL(results));
    for (int i = 0; i < promises->elements.count; i++) {
      promiseCapture(AS_PROMISE(promises->elements.values[i]), 5, OBJ_VAL(promises), 
                     OBJ_VAL(allPromise), OBJ_VAL(results), INT_VAL(remainingCount), INT_VAL(i));
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

void promiseCapture(ObjPromise* promise, int count, ...) {
  va_list args;
  va_start(args, count);
  for (int i = 0; i < count; i++) {
    Value value = va_arg(args, Value);
    writeValueArray(&promise->capturedValues->elements, value);
  }
  va_end(args);
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
  if (IS_CLOSURE(promise->onFinally)) callReentrantMethod(OBJ_VAL(promise), promise->onFinally, promise->value);
}

ObjPromise* promiseRace(ObjClass* klass, ObjArray* promises) {
  ObjPromise* racePromise = newPromise(NIL_VAL);
  racePromise->obj.klass = klass;
  push(OBJ_VAL(racePromise));

  for (int i = 0; i < promises->elements.count; i++) {
    ObjPromise* promise = AS_PROMISE(promises->elements.values[i]);
    promiseCapture(promise, 1, OBJ_VAL(racePromise));
    Value then = getObjMethod(OBJ_VAL(promise), "then");
    Value raceAll = getObjMethod(OBJ_VAL(promise), "raceAll");
    ObjBoundMethod* raceAllMethod = newBoundMethod(OBJ_VAL(promise), raceAll);
    callReentrantMethod(OBJ_VAL(promise), then, OBJ_VAL(raceAllMethod));
  }
  pop();
  return racePromise;
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
  ObjPromise* promise = newPromise(NIL_VAL);
  promise->state = PROMISE_FULFILLED;
  promise->value = value;
  return promise;
}

ObjPromise* promiseWithRejected(ObjException* exception) {
  ObjPromise* promise = newPromise(NIL_VAL);
  promise->state = PROMISE_REJECTED;
  promise->exception = exception;
  return promise;
}