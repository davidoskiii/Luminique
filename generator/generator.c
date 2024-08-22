#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../object/object.h"
#include "../vm/vm.h"


void initGenerator(ObjGenerator* generator, Value callee, ObjArray* arguments) {
  ObjClosure* closure = AS_CLOSURE(IS_BOUND_METHOD(callee) ? AS_BOUND_METHOD(callee)->method : callee);
  for (int i = 0; i < arguments->elements.count; i++) {
    push(arguments->elements.values[i]);
  }

  CallFrame callFrame = {
    .closure = closure,
    .ip = closure->function->chunk.code,
    .slots = vm.stackTop - arguments->elements.count - 1
  };
  ObjFrame* frame = newFrame(&callFrame);

  generator->frame = frame;
  generator->outer = vm.runningGenerator;
  generator->inner = NULL;
  generator->state = GENERATOR_START;
  generator->value = NIL_VAL;
}

void resumeGenerator(ObjGenerator* generator) {
  vm.apiStackDepth++;
  Value result = callGenerator(generator);
  vm.stackTop -= generator->frame->slotCount;
  push(OBJ_VAL(generator));
  vm.apiStackDepth--;
  generator->value = result;
}

void loadGeneratorFrame(ObjGenerator* generator) {
  CallFrame* frame = &vm.frames[vm.frameCount++];
  frame->closure = generator->frame->closure;
  frame->ip = generator->frame->ip;
  frame->slots = vm.stackTop - 1;
  frame->slots[0] = generator->frame->slots[0];

  for (int i = 1; i < generator->frame->slotCount; i++) {
    push(generator->frame->slots[i]);
  }
  if (generator->state != GENERATOR_START) push(generator->value);
  generator->state = GENERATOR_RESUME;
}

void saveGeneratorFrame(ObjGenerator* generator, CallFrame* frame, Value result) {
  generator->frame->closure = frame->closure;
  generator->frame->ip = frame->ip;
  generator->state = GENERATOR_YIELD;
  generator->value = result;

  generator->frame->slotCount = 0;
  for (Value* slot = frame->slots; slot < vm.stackTop - 1; slot++) {
    generator->frame->slots[generator->frame->slotCount++] = *slot;
  }
}

Value loadInnerGenerator() {
  if (vm.runningGenerator->inner == NULL) {
    throwNativeException("luminique::std::lang", "IllegalArgumentException", "Cannot only yield from a generator.");
  }
  Value result = vm.runningGenerator->inner != NULL ? OBJ_VAL(vm.runningGenerator->inner) : NIL_VAL;
  pop();
  push(result);
  return result;
}

void yieldFromInnerGenerator(ObjGenerator* generator) {
  vm.runningGenerator->frame->ip--;
  vm.runningGenerator->inner = generator;
  Value result = callGenerator(generator);
  for (int i = 0; i < generator->frame->closure->function->arity + 1; i++) {
    pop();
  }
  if (generator->state != GENERATOR_RETURN) push(result);
}

Value stepGenerator(ObjGenerator* generator, Value arg) {
  Value send = getObjMethod(OBJ_VAL(generator), "send");
  callReentrantMethod(OBJ_VAL(generator), send, arg);

  if (generator->state == GENERATOR_RETURN && IS_PROMISE(generator->value)) return generator->value;
  else {
    ObjPromise* promise = IS_PROMISE(generator->value) ? AS_PROMISE(generator->value) : newPromise(PROMISE_FULFILLED, generator->value, NIL_VAL);
    if (generator->state == GENERATOR_RETURN) return OBJ_VAL(promise);
    else {
      Value step = getObjMethod(OBJ_VAL(generator), "step");
      ObjBoundMethod* stepMethod = newBoundMethod(OBJ_VAL(generator), step);
      Value then = getObjMethod(OBJ_VAL(promise), "then");
      return callReentrantMethod(OBJ_VAL(promise), then, OBJ_VAL(stepMethod));
    }
  }
}

Value runGeneratorAsync(Value callee, ObjArray* arguments) {
  ObjGenerator* generator = newGenerator(NULL, NULL);
  push(OBJ_VAL(generator));
  initGenerator(generator, callee, arguments);

  for (int i = 0; i < arguments->elements.count; i++) {
    pop();
  }
  pop();
  return stepGenerator(generator, NIL_VAL);
}
