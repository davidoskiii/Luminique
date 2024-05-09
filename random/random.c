#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "random.h"
#include "../assert/assert.h"
#include "../pcg/pcg.h"
#include "../native/native.h"
#include "../value/value.h"
#include "../vm/vm.h"

uint64_t getseed() {
  uint64_t seed = (uint64_t)time(NULL);
  pcg32_seed(seed);
  return seed;
}

uint64_t seed;

NATIVE_FUNCTION(randbool) {
	assertArgCount("randbool()", 0, argCount);
	bool value = pcg32_random_bool();
	RETURN_BOOL(value);
}

NATIVE_FUNCTION(randfloat) {
	assertArgCount("randfloat()", 0, argCount);
	double value = pcg32_random_double();
	RETURN_NUMBER(value);
}

NATIVE_FUNCTION(randint) {
	assertArgCount("randint()", 0, argCount);
	uint32_t value = pcg32_random_int();
	RETURN_INT((int)value);
}

NATIVE_FUNCTION(randbint) {
	assertArgCount("randbint(bound)", 1, argCount);
	assertArgIsInt("randbint(bound)", args, 0);
	assertNumberNonNegative("randbint(bound)", AS_NUMBER(args[0]), 0);
	uint32_t value = pcg32_random_int_bounded((uint32_t)AS_INT(args[0]));
	RETURN_INT((int)value);
}

NATIVE_FUNCTION(getseed) {
	assertArgCount("getseed()", 0, argCount);
  RETURN_INT(seed);
}

NATIVE_FUNCTION(setseed) {
	assertArgCount("setseed(seed)", 1, argCount);
	assertArgIsInt("setseed(seed)", args, 0);
	assertNumberNonNegative("setseed(seed)", AS_NUMBER(args[0]), 0);
  if (AS_NUMBER(args[0]) == 0) runtimeError("Method setseed(seed) expects argument 1 to be a non-zero number but got 0.");
	uint32_t value = pcg32_random_int_bounded((uint32_t)AS_INT(args[0]));
	pcg32_seed((uint64_t)AS_INT(args[0]));

  seed = (uint64_t)AS_INT(args[0]);
  RETURN_NIL;
}

void registerRandomPackage() {
  ObjNamespace* randomNamespace = defineNativeNamespace("random", vm.stdNamespace);
  vm.currentNamespace = randomNamespace;

  seed = getseed();

  DEF_FUNCTION(randbool, 0);
  DEF_FUNCTION(randfloat, 0);
  DEF_FUNCTION(randint, 0);
  DEF_FUNCTION(randbint, 1);
  DEF_FUNCTION(setseed, 1);
  DEF_FUNCTION(getseed, 0);

  vm.currentNamespace = vm.rootNamespace;
}
