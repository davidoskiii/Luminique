#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "promise.h"
#include "../object/object.h"
#include "../vm/vm.h"

ObjPromise* promiseAll(ObjClass* klass, ObjArray* promises) {
  ObjArray* results = newArray();
  push(OBJ_VAL(results));
  int numCompleted = 0;
  for (int i = 0; i < promises->elements.count; i++) {
    promiseCapture(AS_PROMISE(promises->elements.values[i]), 4, OBJ_VAL(promises), OBJ_VAL(results), INT_VAL(numCompleted), INT_VAL(i));
  }
  pop();

  ObjPromise* allPromise = newPromise(NIL_VAL);
  push(OBJ_VAL(allPromise));
  allPromise->obj.klass = klass;
  Value execute = getObjMethod(OBJ_VAL(allPromise), "execute");
  ObjBoundMethod* executor = newBoundMethod(OBJ_VAL(allPromise), execute);
  allPromise->executor = OBJ_VAL(executor);

  promiseCapture(allPromise, 3, OBJ_VAL(promises), OBJ_VAL(results), INT_VAL(numCompleted));
  promiseExecute(allPromise);
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
  for (int i = 0; i < promise->handlers.count; i++) {
    ObjBoundMethod* handler = newBoundMethod(OBJ_VAL(promise), promise->handlers.values[i]);
    callReentrantMethod(OBJ_VAL(promise), handler->method, value);
  }
  initValueArray(&promise->handlers);
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
