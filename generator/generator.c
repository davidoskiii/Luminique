#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../object/object.h"
#include "../vm/vm.h"

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
