#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../native/native.h"
#include "dsa.h"

void initStack(Stack* stack) {
  stack->capacity = 8;
  stack->count = 0;
  stack->items = malloc(sizeof(ObjNamespace*) * stack->capacity);
}

void freeStack(Stack* stack) {
  free(stack->items);
}

void stackPush(Stack* stack, ObjNamespace* namespace_) {
  if (stack->count == stack->capacity) {
    stack->capacity *= 2;
    stack->items = realloc(stack->items, sizeof(ObjNamespace*) * stack->capacity);
  }
  stack->items[stack->count++] = namespace_;
}

ObjNamespace* stackPop(Stack* stack) {
  if (stack->count == 0) return NULL;  // Handle underflow
  return stack->items[--stack->count];
}
