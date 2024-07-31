#ifndef cluminique_promise_h
#define cluminique_promise_h

#include "../common.h"
#include "../value/value.h"

typedef enum {
  PROMISE_PENDING,
  PROMISE_FULFILLED,
  PROMISE_REJECTED
} PromiseState;

ObjPromise* promiseAll(ObjClass* klass, ObjArray* promises);
void promiseCapture(ObjPromise* promise, int count, ...);
void promiseExecute(ObjPromise* promise);
void promiseFulfill(ObjPromise* promise, Value value);
ObjPromise* promiseRace(ObjClass* klass, ObjArray* promises);
void promiseReject(ObjPromise* promise, Value exception);
void promiseThen(ObjPromise* promise, Value value);

#endif
