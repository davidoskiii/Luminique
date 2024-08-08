#ifndef cluminique_variable_h
#define cluminique_variable_h

#include "../common.h"
#include "../native/native.h"
#include "../value/value.h"

#define ABORT_IFNOPROPRETY(value, name) \
  do {\
    THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", \
      name->chars, valueToString(value)); \
  } while (false)

Value getGenericInstanceVariable(Value receiver, ObjString* name);
Value setGenericInstanceVariable(Value receiver, ObjString* name, Value value);

#endif
