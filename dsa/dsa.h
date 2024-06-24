#ifndef cluminique_dsa_h
#define cluminique_dsa_h

#include "../object/object.h"
#include "../vm/vm.h"

typedef struct {
  ObjNamespace** items;
  int capacity;
  int count;
} Stack;

void initStack(Stack* stack);
void freeStack(Stack* stack);
void stackPush(Stack* stack, ObjNamespace* namespace_);
ObjNamespace* stackPop(Stack* stack);

#endif
