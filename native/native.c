#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "../object/object.h"
#include "../vm/vm.h"

static unsigned int seed = 0; // Global seed variable

static bool clockNative(int argCount, Value* args) {
  args[-1] = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
  return true;
}

static bool printNative(int argCount, Value* args) {
  if (argCount != 1) {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
    return false;
  }

  printValue(args[0]);
  printf("\n");
  args[-1] = NIL_VAL;
  return true;
}

static bool randomNative(int argCount, Value* args) {
  if (seed == 0) {
    seed = (unsigned int)time(NULL);
    srand(seed);
  }
  
  if (argCount == 2) {
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
      args[-1] = OBJ_VAL(copyFormattedString("Arguments must be numbers"));
      return false;
    }

    int min = AS_NUMBER(args[0]);
    int max = AS_NUMBER(args[1]);

    if (min >= max) {
      args[-1] = OBJ_VAL(copyFormattedString("Min value must be less than max value"));
      return false;
    }

    int randomValue = (rand() % (max - min + 1)) + min;
    args[-1] = NUMBER_VAL(randomValue);
    return true;
  } else {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 2 arguments but got %d", argCount));
    return false;
  }
}

static bool currentTimeNative(int argCount, Value* args) {
  time_t currentTime = time(NULL);
  args[-1] = NUMBER_VAL((double)currentTime);
  return true;
}

static bool sqrtNative(int argCount, Value* args) {
  if (argCount != 1) {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
    return false;
  }

  if (!IS_NUMBER(args[0])) {
    args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
    return false;
  }

  double value = AS_NUMBER(args[0]);
  args[-1] = NUMBER_VAL(sqrt(value));
  return true;
}

static bool absNative(int argCount, Value* args) {
  if (argCount != 1) {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
    return false;
  }

  if (!IS_NUMBER(args[0])) {
    args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
    return false;
  }

  double value = AS_NUMBER(args[0]);
  args[-1] = NUMBER_VAL(fabs(value));
  return true;
}

static bool ceilNative(int argCount, Value* args) {
    if (argCount != 1) {
        args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
        return false;
    }

    if (!IS_NUMBER(args[0])) {
        args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
        return false;
    }

    double value = AS_NUMBER(args[0]);
    args[-1] = NUMBER_VAL(ceil(value));
    return true;
}

static bool fabsNative(int argCount, Value* args) {
    if (argCount != 1) {
        args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
        return false;
    }

    if (!IS_NUMBER(args[0])) {
        args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
        return false;
    }

    double value = AS_NUMBER(args[0]);
    args[-1] = NUMBER_VAL(fabs(value));
    return true;
}

static bool factorialNative(int argCount, Value* args) {
    if (argCount != 1) {
        args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
        return false;
    }

    if (!IS_NUMBER(args[0])) {
        args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number"));
        return false;
    }

    int n = AS_NUMBER(args[0]);
    if (n < 0 || n != (int)n) {
        args[-1] = OBJ_VAL(copyFormattedString("Argument must be a non-negative integer"));
        return false;
    }

    int result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }

    args[-1] = NUMBER_VAL((double)result);
    return true;
}

static bool fmodNative(int argCount, Value* args) {
    if (argCount != 2) {
        args[-1] = OBJ_VAL(copyFormattedString("Expected 2 arguments but got %d", argCount));
        return false;
    }

    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        args[-1] = OBJ_VAL(copyFormattedString("Arguments must be numbers"));
        return false;
    }

    double x = AS_NUMBER(args[0]);
    double y = AS_NUMBER(args[1]);

    double result = fmod(x, y);
    args[-1] = NUMBER_VAL(result);
    return true;
}

static bool inputNative(int argCount, Value* args) {
  if (argCount != 1) {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
    return false;
  }

  if (!IS_STRING(args[0])) {
    args[-1] = OBJ_VAL(copyFormattedString("Argument must be a string"));
    return false;
  }

  ObjString* prompt = AS_STRING(args[0]);
  printValue(OBJ_VAL(prompt)); // Print the prompt using printValue

  char inputBuffer[1024]; // Adjust the buffer size as needed
  if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) {
    // Remove newline character from input
    size_t length = strlen(inputBuffer);
    if (length > 0 && inputBuffer[length - 1] == '\n') {
      inputBuffer[length - 1] = '\0';
    }

    args[-1] = OBJ_VAL(copyString(inputBuffer, (int)strlen(inputBuffer)));
    return true;
  } else {
    args[-1] = OBJ_VAL(copyFormattedString("Error reading input"));
    return false;
  }
}

static bool numNative(int argCount, Value* args) {
  if (argCount != 1) {
    args[-1] = OBJ_VAL(copyFormattedString("Expected 1 argument but got %d", argCount));
    return false;
  }

  if (IS_NUMBER(args[0])) {
    // If it's already a number, return it as is
    args[-1] = args[0];
    return true;
  }

  if (!IS_STRING(args[0])) {
    args[-1] = OBJ_VAL(copyFormattedString("Argument must be a number or string"));
    return false;
  }

  ObjString* str = AS_STRING(args[0]);
  char* endptr;
  double num = strtod(str->chars, &endptr);

  if (*endptr != '\0') {
    args[-1] = OBJ_VAL(copyFormattedString("Invalid number format"));
    return false;
  }

  args[-1] = NUMBER_VAL(num);
  return true;
}
