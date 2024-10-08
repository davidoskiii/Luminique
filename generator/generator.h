#ifndef cluminique_generator_h
#define cluminique_generator_h

#include "../common.h"
#include "../value/value.h"

typedef enum {
  GENERATOR_START,
  GENERATOR_YIELD,
  GENERATOR_RESUME,
  GENERATOR_RETURN,
  GENERATOR_THROW,
  GENERATOR_ERROR
} GeneratorState;

void initGenerator(ObjGenerator* generator, Value callee, ObjArray* arguments);
void resumeGenerator(ObjGenerator* generator);
void loadGeneratorFrame(ObjGenerator* generator);
void saveGeneratorFrame(ObjGenerator* generator, CallFrame* frame, Value result);
Value loadInnerGenerator();
void yieldFromInnerGenerator(ObjGenerator* generator);
Value stepGenerator(ObjGenerator* generator, Value arg);
Value runGeneratorAsync(Value callee, ObjArray* arguments);

#endif
