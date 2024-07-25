#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lang.h"
#include "../hash/hash.h"
#include "../interceptor/interceptor.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../vm/vm.h"

#define MAX_BUFFER_SIZE 32

// BOOL

NATIVE_METHOD(Bool, __init__) {
  assertArgCount("Bool::__init__(value)", 1, argCount);
  if (isFalsey(args[0])) {
    RETURN_BOOL(false);
  } else {
    RETURN_BOOL(true);
  }
}

NATIVE_METHOD(Bool, clone) {
	assertArgCount("Bool::clone()", 0, argCount);
	return receiver;
}

NATIVE_METHOD(Bool, __str__) {
	assertArgCount("Bool::__str__()", 0, argCount);
	if (AS_BOOL(receiver)) RETURN_STRING("true", 4);
	else RETURN_STRING("false", 5);
}

NATIVE_METHOD(Bool, __format__) {
	assertArgCount("Bool::__format__()", 0, argCount);
	if (AS_BOOL(receiver)) RETURN_STRING("true", 4);
	else RETURN_STRING("false", 5);
}

// EXCEPTION

NATIVE_METHOD(Exception, __init__) {
  assertArgCount("Exception::__init__(message)", 1, argCount);
  assertArgIsString("Exception::__init__(message)", args, 0);
  ObjException* self = AS_EXCEPTION(receiver);
  self->message = AS_STRING(args[0]);
  RETURN_OBJ(self);
}

NATIVE_METHOD(Exception, message) {
  assertArgCount("Exception::message()", 0, argCount);
  ObjException* self = AS_EXCEPTION(receiver);
  RETURN_OBJ(self->message);
}

NATIVE_METHOD(Exception, stacktrace) {
  assertArgCount("Exception::stacktrace()", 0, argCount);
  ObjException* self = AS_EXCEPTION(receiver);
  RETURN_OBJ(self->stacktrace);
}

NATIVE_METHOD(Exception, __str__) {
  assertArgCount("Exception::__str__()", 0, argCount);
  ObjException* self = AS_EXCEPTION(receiver);
  RETURN_STRING_FMT("<Exception %s - %s>", self->obj.klass->name->chars, self->message->chars);
}

NATIVE_METHOD(Exception, __format__) {
  assertArgCount("Exception::__format__()", 0, argCount);
  ObjException* self = AS_EXCEPTION(receiver);
  RETURN_STRING_FMT("<Exception %s - %s>", self->obj.klass->name->chars, self->message->chars);
}

// CLASS

NATIVE_METHOD(Class, __init__) {
  assertArgCount("Class::__init__(name, superclass)", 2, argCount);
  assertArgIsString("Class::__init__(name, superclass)", args, 0);
  assertArgIsClass("Class::__init__(name, superclass)", args, 1);
  ObjClass* klass = newClass(AS_STRING(args[0]), OBJ_INSTANCE);
  bindSuperclass(klass, AS_CLASS(args[1]));
  RETURN_OBJ(klass);
}

NATIVE_METHOD(Class, __invoke__) { 
  ObjClass* self = AS_CLASS(receiver);
  ObjInstance* instance = newInstance(self);
  push(OBJ_VAL(instance));
  Value initMethod;

  if (tableGet(&self->methods, vm.initString, &initMethod)) {
    callReentrant(receiver, initMethod, args);
  }

  pop();
  RETURN_OBJ(instance);
}

NATIVE_METHOD(Class, clone) {
  assertArgCount("Class::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Class, getClass) {
  assertArgCount("Class::getClass()", 0, argCount);
  RETURN_OBJ(vm.classClass);
}

NATIVE_METHOD(Class, getClassName) {
  assertArgCount("Class::getClassName()", 0, argCount);
  RETURN_OBJ(vm.classClass->name);
}

NATIVE_METHOD(Class, instanceOf) {
  assertArgCount("Class::instanceOf(class)", 1, argCount);
  if (!IS_CLASS(args[0])) RETURN_FALSE;
  ObjClass* klass = AS_CLASS(args[0]);
  if (klass == vm.classClass) RETURN_TRUE;
  else RETURN_FALSE;
}

NATIVE_METHOD(Class, memberOf) {
  assertArgCount("Class::memberOf(class)", 1, argCount);
  if (!IS_CLASS(args[0])) RETURN_FALSE;
  ObjClass* klass = AS_CLASS(args[0]);
  if (klass == vm.classClass) RETURN_TRUE;
  else RETURN_FALSE;
}

NATIVE_METHOD(Class, name) {
  assertArgCount("Class::name()", 0, argCount);
  RETURN_OBJ(AS_CLASS(receiver)->name);
}

NATIVE_METHOD(Class, superclass) {
  assertArgCount("Class::superclass()", 0, argCount);
  ObjClass* klass = AS_CLASS(receiver);
  if (klass->superclass == NULL) RETURN_NIL;
  RETURN_OBJ(klass->superclass);
}

NATIVE_METHOD(Class, __str__) {
  assertArgCount("Class::__str__()", 0, argCount);
  ObjClass* self = AS_CLASS(receiver);
  if (self->namespace_->isRoot) RETURN_STRING_FMT("<class %s>", self->name->chars);
  else RETURN_STRING_FMT("<class %s::%s>", self->namespace_->fullName->chars, self->name->chars);
}

NATIVE_METHOD(Class, __format__) {
  assertArgCount("Class::__format__()", 0, argCount);
  ObjClass* self = AS_CLASS(receiver);
  if (self->namespace_->isRoot) RETURN_STRING_FMT("<class %s>", self->name->chars);
  else RETURN_STRING_FMT("<class %s::%s>", self->namespace_->fullName->chars, self->name->chars);
}

// ENUM

NATIVE_METHOD(Enum, __init__) {
  assertArgCount("Enum::__init__(name)", 1, argCount);
  assertArgIsString("Enum::__init__(name)", args, 0);
  ObjEnum* enum_ = newEnum(AS_STRING(args[0]));
  RETURN_OBJ(enum_);
}

NATIVE_METHOD(Enum, clone) {
  assertArgCount("Enum::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Enum, name) {
  assertArgCount("Enum::name()", 0, argCount);
  RETURN_OBJ(AS_ENUM(receiver)->name);
}

NATIVE_METHOD(Enum, __str__) {
  assertArgCount("Enum::__str__()", 0, argCount);
  ObjEnum* self = AS_ENUM(receiver);
  RETURN_STRING_FMT("<enum %s>", self->name->chars);
}

NATIVE_METHOD(Enum, __format__) {
  assertArgCount("Enum::__format__()", 0, argCount);
  ObjEnum* self = AS_ENUM(receiver);
  RETURN_STRING_FMT("<enum %s>", self->name->chars);
}

// FLOAT

NATIVE_METHOD(Float, __init__) {
  assertArgCount("Float::__init__(value)", 1, argCount);
  assertArgIsNumber("Float::__int__(value)", args, 0);
  RETURN_FLOAT(args[0]);
}

NATIVE_METHOD(Float, clone) {
  assertArgCount("Float::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Float, __str__) {
  assertArgCount("Float::__str__()", 0, argCount);
  RETURN_STRING_FMT("%g", AS_FLOAT(receiver));
}

NATIVE_METHOD(Float, __format__) {
  assertArgCount("Float::__format__()", 0, argCount);
  RETURN_STRING_FMT("%g", AS_FLOAT(receiver));
}

NATIVE_METHOD(Generator, __init__) {
  assertArgCount("Generator::__init__(closure, args)", 2, argCount);
  assertArgIsClosure("Generator::__init__(closure, args)", args, 0);
  assertArgIsArray("Generator::__init__(closure, args)", args, 1);

  ObjGenerator* self = AS_GENERATOR(receiver);
  ObjClosure* closure = AS_CLOSURE(args[0]);
  ObjArray* arguments = AS_ARRAY(args[1]);

  CallFrame callFrame = { 
    .closure = closure, 
    .ip = closure->function->chunk.code, 
    .slots = vm.stackTop - arguments->elements.count - 1 
  };

  ObjFrame* frame = newFrame(&callFrame);

  for (int i = 0; i < arguments->elements.count; i++) {
    push(arguments->elements.values[i]);
  }

  self->frame = frame;
  self->parent = vm.runningGenerator;
  self->state = GENERATOR_START;
  RETURN_OBJ(self);
}

NATIVE_METHOD(Generator, next) {
  assertArgCount("Generator::next()", 0, argCount);
  ObjGenerator* self = AS_GENERATOR(receiver);
  if (self->state == GENERATOR_RETURN) RETURN_NIL;
  else if (self->state == GENERATOR_RESUME) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator is already running.");
  else if (self->state == GENERATOR_THROW) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already thrown an exception.");
  else {
    self->state = GENERATOR_RESUME;
    vm.apiStackDepth++;
    Value result = callGenerator(self);
    vm.stackTop -= self->frame->slotCount;
    push(result);
    vm.apiStackDepth--;
    self->current = result;
    RETURN_VAL(result);
  }
}

NATIVE_METHOD(Generator, returns) {
  assertArgCount("Generator::returns(value)", 1, argCount);
  ObjGenerator* self = AS_GENERATOR(receiver);
  if (self->state == GENERATOR_RETURN) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already returned.");
  else {
    self->state = GENERATOR_RETURN;
    self->current = args[0];
    RETURN_VAL(args[0]);
  }
}

NATIVE_METHOD(Generator, throws) {
  assertArgCount("Generator::throws(exception)", 1, argCount);
  assertArgIsException("Generator::throws(exception)", args, 0);
  ObjGenerator* self = AS_GENERATOR(receiver);
  if (self->state == GENERATOR_RETURN) THROW_EXCEPTION(luminique::std::lang, UnsupportedOperationException, "Generator has already returned.");
  else {
      ObjException* exception = AS_EXCEPTION(args[0]);
      self->state = GENERATOR_THROW;
      THROW_EXCEPTION(luminique::std::lang, exception->obj.klass, exception->message->chars);
  }
}

// FUNCTION

NATIVE_METHOD(Function, __init__) {
  THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class Function.");
}

NATIVE_METHOD(Function, __invoke__) {
  ObjClosure* self = AS_CLOSURE(receiver);
  if (callClosure(self, argCount)) {
    int i = 0;
    while (i < argCount) {
      push(args[i]);
      i++;
    }
    RETURN_VAL(args[argCount - 1]);
  }
  RETURN_NIL;
}

NATIVE_METHOD(Function, arity) {
  assertArgCount("Function::arity()", 0, argCount);

  if (IS_NATIVE_FUNCTION(receiver)) {
     RETURN_INT(AS_NATIVE_FUNCTION(receiver)->arity);
  }

  RETURN_INT(AS_CLOSURE(receiver)->function->arity);
}

NATIVE_METHOD(Function, clone) {
  assertArgCount("Function::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Function, isNative) {
  assertArgCount("Function::isNative()", 0, argCount);
  RETURN_BOOL(IS_NATIVE_FUNCTION(receiver));
}

NATIVE_METHOD(Function, name) {
  assertArgCount("Function::name()", 0, argCount);
  if (IS_NATIVE_FUNCTION(receiver)) {
    RETURN_OBJ(AS_NATIVE_FUNCTION(receiver)->name);
  }
  RETURN_OBJ(AS_CLOSURE(receiver)->function->name);
}

NATIVE_METHOD(Function, __str__) {
  assertArgCount("Function::__str__()", 0, argCount);

  if (IS_NATIVE_FUNCTION(receiver)) {
    RETURN_STRING_FMT("<native function %s>", AS_NATIVE_FUNCTION(receiver)->name->chars);
  }

  RETURN_STRING_FMT("<fn %s>", AS_CLOSURE(receiver)->function->name->chars);
}

NATIVE_METHOD(Function, __format__) {
  assertArgCount("Function::__format_()", 0, argCount);

  if (IS_NATIVE_FUNCTION(receiver)) {
    RETURN_STRING_FMT("<native function %s>", AS_NATIVE_FUNCTION(receiver)->name->chars);
  }

  RETURN_STRING_FMT("<fn %s>", AS_CLOSURE(receiver)->function->name->chars);
}

NATIVE_METHOD(Function, upvalueCount) {
  assertArgCount("Function::upvalueCount()", 0, argCount);
  RETURN_INT(AS_CLOSURE(receiver)->upvalueCount);
}

// METHOD


NATIVE_METHOD(Method, __init__) {
  THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class Method.");
}

NATIVE_METHOD(Method, arity) {
  assertArgCount("Method::arity()", 0, argCount);
  RETURN_INT(AS_BOUND_METHOD(receiver)->method->function->arity);
}

NATIVE_METHOD(Method, clone) {
  assertArgCount("Method::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Method, name) {
  assertArgCount("Method::name()", 0, argCount);
  ObjBoundMethod* bound = AS_BOUND_METHOD(receiver);
  RETURN_STRING_FMT("%s::%s", getObjClass(bound->receiver)->name->chars, bound->method->function->name->chars);
}

NATIVE_METHOD(Method, receiver) {
  assertArgCount("Method::receiver()", 0, argCount);
  RETURN_VAL(AS_BOUND_METHOD(receiver)->receiver);
}

NATIVE_METHOD(Method, __str__) {
  assertArgCount("Method::__str__()", 0, argCount);
  ObjBoundMethod* bound = AS_BOUND_METHOD(receiver);
  RETURN_STRING_FMT("<method %s::%s>", getObjClass(bound->receiver)->name->chars, bound->method->function->name->chars);
}

NATIVE_METHOD(Method, __format__) {
  assertArgCount("Method::__format__()", 0, argCount);
  ObjBoundMethod* bound = AS_BOUND_METHOD(receiver);
  RETURN_STRING_FMT("<method %s::%s>", getObjClass(bound->receiver)->name->chars, bound->method->function->name->chars);
}

NATIVE_METHOD(Method, upvalueCount) {
  assertArgCount("Function::upvalueCount()", 0, argCount);
  RETURN_INT(AS_BOUND_METHOD(receiver)->method->upvalueCount);
}

// INT

NATIVE_METHOD(Int, __init__) {
  assertArgCount("Int::__init__(value)", 1, argCount);
  assertArgIsNumber("Int::__int__(value)", args, 0);
  RETURN_INT(args[0]);
}

NATIVE_METHOD(Int, __add__) {
  assertArgCount("Int::+(other)", 1, argCount);
  assertArgIsNumber("Int::+(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) + AS_INT(args[0])));
  else RETURN_NUMBER((AS_NUMBER(receiver) + AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, __subtract__) {
  assertArgCount("Int::-(other)", 1, argCount);
  assertArgIsNumber("Int::-(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) - AS_INT(args[0])));
  else RETURN_NUMBER((AS_NUMBER(receiver) - AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, __multiply__) {
  assertArgCount("Int::*(other)", 1, argCount);
  assertArgIsNumber("Int::*(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) * AS_INT(args[0])));
  else RETURN_NUMBER((AS_NUMBER(receiver) * AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, __modulo__) {
  assertArgCount("Int::%(other)", 1, argCount);
  assertArgIsNumber("Int::%(other)", args, 0);
  if (IS_INT(args[0])) RETURN_INT((AS_INT(receiver) % AS_INT(args[0])));
  else RETURN_NUMBER(fmod(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Int, clone) {
  assertArgCount("Int::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(Int, __str__) {
  assertArgCount("Int::__str__()", 0, argCount);
  RETURN_STRING_FMT("%d", AS_INT(receiver));
}

NATIVE_METHOD(Int, __format__) {
  assertArgCount("Int::__format__()", 0, argCount);
  RETURN_STRING_FMT("%d", AS_INT(receiver));
}

// NIL

NATIVE_METHOD(Nil, __init__) {
	THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class Nil.");
}

NATIVE_METHOD(Nil, clone) {
	assertArgCount("Nil::clone()", 0, argCount);
	RETURN_NIL;
}

NATIVE_METHOD(Nil, __str__) {
	assertArgCount("Nil::__str__()", 0, argCount);
	RETURN_STRING("nil", 3);
}

NATIVE_METHOD(Nil, __format__) {
	assertArgCount("Nil::__format__()", 0, argCount);
	RETURN_STRING("nil", 3);
}

// NUMBER

NATIVE_METHOD(Number, __init__) {
  assertArgCount("Number::__init__(value)", 1, argCount);
  assertArgIsNumber("Number::__int__(value)", args, 0);
  RETURN_NUMBER(args[0]);
}

NATIVE_METHOD(Number, __equal__) {
  assertArgCount("Number::==(other)", 1, argCount);
  assertArgIsNumber("Number::==(other)", args, 0);
  RETURN_BOOL((AS_NUMBER(receiver) == AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __greater__) {
  assertArgCount("Number::>(other)", 1, argCount);
  assertArgIsNumber("Number::>(other)", args, 0);
  RETURN_BOOL((AS_NUMBER(receiver) > AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __less__) {
  assertArgCount("Number::<(other)", 1, argCount);
  assertArgIsNumber("Number::<(other)", args, 0);
  RETURN_BOOL((AS_NUMBER(receiver) < AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __add__) { 
  assertArgCount("Number::+(other)", 1, argCount);
  assertArgIsNumber("Number::+(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) + AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __subtract__) {
  assertArgCount("Number::-(other)", 1, argCount);
  assertArgIsNumber("Number::-(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) - AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __multiply__) {
  assertArgCount("Number::*(other)", 1, argCount);
  assertArgIsNumber("Number::*(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) * AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __divide__) { 
  assertArgCount("Number::/(other)", 1, argCount);
  assertArgIsNumber("Number::/(other)", args, 0);
  RETURN_NUMBER((AS_NUMBER(receiver) / AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __modulo__) {
  assertArgCount("Number::%(other)", 1, argCount);
  assertArgIsNumber("Number::%(other)", args, 0);
  RETURN_NUMBER(fmod(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}

NATIVE_METHOD(Number, __power__) {
  assertArgCount("Number::**(other)", 1, argCount);
  assertArgIsNumber("Number::**(other)", args, 0);
  RETURN_NUMBER(pow(AS_NUMBER(receiver), AS_NUMBER(args[0])));
}
NATIVE_METHOD(Number, clone) {
	assertArgCount("Number::clone()", 0, argCount);
	return receiver;
}

NATIVE_METHOD(Number, __str__) {
	assertArgCount("Number::__str__()", 0, argCount);
	char chars[24];
	int length = snprintf(chars, 24, "%.14g", AS_NUMBER(receiver));
	RETURN_STRING(chars, length);
}

NATIVE_METHOD(Number, __format__) {
	assertArgCount("Number::__format__()", 0, argCount);
	char chars[24];
	int length = snprintf(chars, 24, "%.14g", AS_NUMBER(receiver));
	RETURN_STRING(chars, length);
}

// OBJECT


NATIVE_METHOD(Object, __equal__) {
  assertArgCount("Object::==(other)", 1, argCount);
  RETURN_BOOL(receiver == args[0]);
}

NATIVE_METHOD(Object, __undefinedProperty__) {
  assertArgCount("Object::__undefinedProperty__(name)", 1, argCount);
  assertArgIsString("Object::__undefinedProperty__(name)", args, 0);
  THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Property %s does not exist in %s.", 
    AS_CSTRING(args[0]), valueToString(receiver));
}

NATIVE_METHOD(Object, __undefinedMethod__) {
  assertArgCount("Object::__undefinedMethod__(name, args)", 2, argCount);
  assertArgIsString("Object::__undefinedMethod__(name, args)", args, 0);
  assertArgIsArray("Object::__undefinedMethod__(name, args)", args, 1);
  THROW_EXCEPTION_FMT(luminique::std::lang, NotImplementedException, "Method %s does not exist in class %s.", 
    AS_CSTRING(args[0]), getObjClass(receiver)->name->chars);
}

NATIVE_METHOD(Object, clone) {
	assertArgCount("Object::clone()", 0, argCount);
	ObjInstance* thisObject = AS_INSTANCE(receiver);
  ObjInstance* thatObject = newInstance(OBJ_KLASS(receiver));
  push(OBJ_VAL(thatObject));
	tableAddAll(&thisObject->fields, &thatObject->fields);
  pop();
	RETURN_OBJ(thatObject);
}

NATIVE_METHOD(Object, equals) {
	assertArgCount("Object::equals(value)", 1, argCount);
	RETURN_BOOL(valuesEqual(receiver, args[0]));
}

NATIVE_METHOD(Object, getClass) {
	assertArgCount("Object::getClass()", 0, argCount);
	RETURN_OBJ(getObjClass(receiver));
}

NATIVE_METHOD(Object, getClassName) {
	assertArgCount("Object::getClassName()", 0, argCount);
	RETURN_OBJ(getObjClass(receiver)->name);
}

NATIVE_METHOD(Object, hasField) {
	assertArgCount("Object::hasField(field)", 1, argCount);
	assertArgIsString("Object::hasField(field)", args, 0);
	if (!IS_INSTANCE(receiver)) RETURN_BOOL(false);
	ObjInstance* instance = AS_INSTANCE(receiver);
	Value value;
	RETURN_BOOL(tableGet(&instance->fields, AS_STRING(args[0]), &value));
}

NATIVE_METHOD(Object, hashCode) {
	assertArgCount("Object::hashCode()", 0, argCount);
	RETURN_INT(hashValue(receiver));
}

NATIVE_METHOD(Object, instanceOf) {
	assertArgCount("Object::instanceOf(class)", 1, argCount);
	if (!IS_CLASS(args[0])) RETURN_FALSE;
  RETURN_BOOL(isObjInstanceOf(receiver, AS_CLASS(args[0])));
}

NATIVE_METHOD(Object, memberOf) {
	assertArgCount("Object::memberOf(class)", 1, argCount);
	if (!IS_CLASS(args[0])) RETURN_FALSE;
	ObjClass* thisClass = getObjClass(receiver);
	ObjClass* thatClass = AS_CLASS(args[0]);
	RETURN_BOOL(thisClass == thatClass);
}

NATIVE_METHOD(Object, __str__) {
	assertArgCount("Object::__str__()", 0, argCount);
  RETURN_STRING_FMT("<object %s>", AS_OBJ(receiver)->klass->name->chars);
}

NATIVE_METHOD(Object, __format__) {
	assertArgCount("Object::__format__()", 0, argCount);
  RETURN_STRING_FMT("<object %s>", AS_OBJ(receiver)->klass->name->chars);
}

// STRING

NATIVE_METHOD(String, __init__) {
  assertArgCount("String::__init__(chars)", 1, argCount);
  assertArgIsString("String::__init__(chars)", args, 0);
  return args[0];
}


NATIVE_METHOD(String, __add__) {
  assertArgCount("String::+(other)", 1, argCount);
  assertArgIsString("String::+(other)", args, 0);
  RETURN_STRING_FMT("%s%s", AS_CSTRING(receiver), AS_CSTRING(args[0]));
}

NATIVE_METHOD(String, __getSubscript__) {
  assertArgCount("String::[](index)", 1, argCount);
  assertArgIsInt("String::[getChar]](index)", args, 0);

  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  assertIntWithinRange("String::[](index)", index, 0, self->length, 0);

  char chars[2] = { self->chars[index], '\0' };
  RETURN_STRING(chars, 1);
}

NATIVE_METHOD(String, capitalize) {
  assertArgCount("String::capitalize()", 0, argCount);
  RETURN_OBJ(capitalizeString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, clone) {
  assertArgCount("String::clone()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(String, contains) {
  assertArgCount("String::contains(chars)", 1, argCount);
  assertArgIsString("String::contains(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  RETURN_BOOL(searchString(haystack, needle, 0) != -1);
}

NATIVE_METHOD(String, decapitalize) {
  assertArgCount("String::decapitalize()", 0, argCount);
  RETURN_OBJ(decapitalizeString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, ends) {
  assertArgCount("String::ends(chars)", 1, argCount);
  assertArgIsString("String::ends(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  if (needle->length > haystack->length) RETURN_FALSE;
  RETURN_BOOL(memcmp(haystack->chars + haystack->length - needle->length, needle->chars, needle->length) == 0);
}

NATIVE_METHOD(String, subscript) {
  assertArgCount("String::subscript(index)", 1, argCount);
  assertArgIsInt("String::subscript(index)", args, 0);

  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  assertNumberWithinRange("String::subscript(index)", index, 0, self->length, 0);

  char chars[2] = { self->chars[index], '\0' };
  RETURN_STRING(chars, 1);
}

NATIVE_METHOD(String, search) {
  assertArgCount("String::search(chars)", 1, argCount);
  assertArgIsString("String::search(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  RETURN_INT(searchString(haystack, needle, 0));
}

NATIVE_METHOD(String, length) {
  assertArgCount("String::length()", 0, argCount);
  RETURN_INT(AS_STRING(receiver)->length);
}

NATIVE_METHOD(String, next) {
  assertArgCount("String::next(index)", 1, argCount);
  ObjString* self = AS_STRING(receiver);
  if (IS_NIL(args[0])) {
    if (self->length == 0) RETURN_FALSE;
    RETURN_INT(0);
  }

  assertArgIsInt("String::next(index)", args, 0);
  int index = AS_INT(args[0]);
  if (index < 0 || index < self->length - 1) RETURN_INT(index + 1);
  RETURN_NIL;
}

NATIVE_METHOD(String, nextValue) {
  assertArgCount("String::nextValue(index)", 1, argCount);
  assertArgIsInt("String::nextValue(index)", args, 0);
  ObjString* self = AS_STRING(receiver);
  int index = AS_INT(args[0]);
  if (index > -1 && index < self->length) {
    char chars[2] = { self->chars[index], '\0' };
    RETURN_STRING(chars, 1);
  }
  RETURN_NIL;
}

NATIVE_METHOD(String, replace) {
  assertArgCount("String::replace(target, replacement)", 2, argCount);
  assertArgIsString("String::replace(target, replacement)", args, 0);
  assertArgIsString("String::replace(target, replacement)", args, 1);
  RETURN_OBJ(replaceString(AS_STRING(receiver), AS_STRING(args[0]), AS_STRING(args[1])));
}

NATIVE_METHOD(String, reverse) {
  assertArgCount("String::reverse()", 0, argCount);
  ObjString* self = AS_STRING(receiver);
  if (self->length <= 1) return receiver;
  return OBJ_VAL(reverseStringBasedOnMemory(self));
}

NATIVE_METHOD(String, split) {
  assertArgCount("String::split(delimiter)", 1, argCount);
  assertArgIsString("String::split(delimiter)", args, 0);
  ObjString* self = AS_STRING(receiver);
  ObjString* delimiter = AS_STRING(args[0]);

  ObjArray* array = newArray();
  push(OBJ_VAL(array));
  char* string = strdup(self->chars);
  char* next = NULL;
  char* token = strtok_r(string, delimiter->chars, &next);
  while (token != NULL) {
    writeValueArray(&array->elements, OBJ_VAL(copyString(token, (int)strlen(token))));
    token = strtok_r(NULL, delimiter->chars, &next);
  }
  free(string);
  pop();
  RETURN_OBJ(array);
}

NATIVE_METHOD(String, starts) {
  assertArgCount("String::starts(chars)", 1, argCount);
  assertArgIsString("String::starts(chars)", args, 0);
  ObjString* haystack = AS_STRING(receiver);
  ObjString* needle = AS_STRING(args[0]);
  if (needle->length > haystack->length) RETURN_FALSE;
  RETURN_BOOL(memcmp(haystack->chars, needle->chars, needle->length) == 0);
}

NATIVE_METHOD(String, cut) {
  assertArgCount("String::cut(from, to)", 2, argCount);
  assertArgIsInt("String::cut(from, to)", args, 0);
  assertArgIsInt("String::cut(from, to)", args, 1);
  RETURN_OBJ(subString(AS_STRING(receiver), AS_INT(args[0]), AS_INT(args[1])));
}

NATIVE_METHOD(String, lower) {
  assertArgCount("String::lower()", 0, argCount);
  RETURN_OBJ(toLowerString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, __str__) {
  assertArgCount("String::__str__()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(String, __format__) {
  assertArgCount("String::__format__()", 0, argCount);
  return receiver;
}

NATIVE_METHOD(String, upper) {
  assertArgCount("String::upper()", 0, argCount);
  RETURN_OBJ(toUpperString(AS_STRING(receiver)));
}

NATIVE_METHOD(String, trim) {
  assertArgCount("String::trim()", 0, argCount);
  RETURN_OBJ(trimString(AS_STRING(receiver)));
}

NATIVE_METHOD(Namespace, __init__) {
  THROW_EXCEPTION(luminique::std::lang, InstantiationException, "Cannot instantiate from class Namespace.");
  RETURN_NIL;
}

NATIVE_METHOD(Namespace, enclosing) {
  assertArgCount("Namespace::enclosing()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  if (self->enclosing != NULL && self->enclosing->enclosing != NULL) RETURN_OBJ(self->enclosing);
  RETURN_NIL;
}

NATIVE_METHOD(Namespace, fullName) {
  assertArgCount("Namespace::fullName()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  RETURN_OBJ(self->fullName);
}

NATIVE_METHOD(Namespace, shortName) {
  assertArgCount("Namespace::shortName()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  RETURN_OBJ(self->shortName);
}

NATIVE_METHOD(Namespace, clone) {
  assertArgCount("Namespace::clone()", 0, argCount);
  RETURN_OBJ(receiver);
}

NATIVE_METHOD(Namespace, __str__) {
  assertArgCount("Namespace::__str__()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  RETURN_STRING_FMT("<namespace %s>", self->fullName->chars);
}

NATIVE_METHOD(Namespace, __format__) {
  assertArgCount("Namespace::__format__()", 0, argCount);
  ObjNamespace* self = AS_NAMESPACE(receiver);
  RETURN_STRING_FMT("<namespace %s>", self->fullName->chars);
}

static void bindNamespaceClass() {
  for (int i = 0; i < vm.namespaces.capacity; i++) {
    Entry* entry = &vm.namespaces.entries[i];
    if (entry->key == NULL) continue;
    entry->key->obj.klass = vm.namespaceClass;
  }
}

static ObjNamespace* defineRootNamespace() {
  ObjString* name = newString("");
  push(OBJ_VAL(name));
  ObjNamespace* rootNamespace = newNamespace(name, NULL);
  rootNamespace->isRoot = true;
  push(OBJ_VAL(rootNamespace));
  tableSet(&vm.namespaces, name, OBJ_VAL(rootNamespace));
  pop();
  pop();
  return rootNamespace;
}

void registerLangPackage() {
  vm.rootNamespace = defineRootNamespace();
  vm.luminiqueNamespace = defineNativeNamespace("luminique", vm.rootNamespace);
  vm.stdNamespace = defineNativeNamespace("std", vm.luminiqueNamespace);
  vm.langNamespace = defineNativeNamespace("lang", vm.stdNamespace);
  vm.currentNamespace = vm.langNamespace;

	vm.objectClass = defineNativeClass("Object");
  vm.objectClass->classType = OBJ_INSTANCE;
  DEF_METHOD(vm.objectClass, Object, clone, 0);
  DEF_METHOD(vm.objectClass, Object, equals, 1);
  DEF_METHOD(vm.objectClass, Object, getClass, 0);
  DEF_METHOD(vm.objectClass, Object, getClassName, 0);
  DEF_METHOD(vm.objectClass, Object, hasField, 1);
  DEF_METHOD(vm.objectClass, Object, hashCode, 0);
  DEF_METHOD(vm.objectClass, Object, instanceOf, 1);
  DEF_METHOD(vm.objectClass, Object, memberOf, 1);
  DEF_METHOD(vm.objectClass, Object, __str__, 0);
  DEF_METHOD(vm.objectClass, Object, __format__, 0);

  DEF_OPERATOR(vm.objectClass, Object, ==, __equal__, 1);
  DEF_INTERCEPTOR(vm.objectClass, Object, INTERCEPTOR_UNDEFINED_PROPERTY, __undefinedProperty__, 1);
  DEF_INTERCEPTOR(vm.objectClass, Object, INTERCEPTOR_UNDEFINED_METHOD, __undefinedMethod__, 2);

  vm.classClass = defineNativeClass("Class");
  bindSuperclass(vm.classClass, vm.objectClass);
  vm.classClass->classType = OBJ_CLASS;
  DEF_METHOD(vm.classClass, Class, __init__, 2);
  DEF_METHOD(vm.classClass, Class, clone, 0);
  DEF_METHOD(vm.classClass, Class, getClass, 0);
  DEF_METHOD(vm.classClass, Class, getClassName, 0);
  DEF_METHOD(vm.classClass, Class, instanceOf, 1);
  DEF_METHOD(vm.classClass, Class, memberOf, 1);
  DEF_METHOD(vm.classClass, Class, name, 0);
  DEF_METHOD(vm.classClass, Class, superclass, 0);
  DEF_METHOD(vm.classClass, Class, __str__, 0);
  DEF_METHOD(vm.classClass, Class, __format__, 0);
  DEF_OPERATOR(vm.classClass, Class, (), __invoke__, -1);
  vm.objectClass->obj.klass = vm.classClass;

  vm.enumClass = defineNativeClass("Enum");
  bindSuperclass(vm.enumClass, vm.objectClass);
  vm.enumClass->classType = OBJ_ENUM;
  DEF_METHOD(vm.enumClass, Enum, __init__, 1);
  DEF_METHOD(vm.enumClass, Enum, clone, 0);
  DEF_METHOD(vm.enumClass, Enum, name, 0);
  DEF_METHOD(vm.enumClass, Enum, __str__, 0);
  DEF_METHOD(vm.enumClass, Enum, __format__, 0);

  vm.namespaceClass = defineNativeClass("Namespace");
  bindSuperclass(vm.namespaceClass, vm.objectClass);
  vm.namespaceClass->classType = OBJ_NAMESPACE;
  DEF_METHOD(vm.namespaceClass, Namespace, __init__, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, clone, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, enclosing, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, fullName, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, shortName, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, __str__, 0);
  DEF_METHOD(vm.namespaceClass, Namespace, __format__, 0);
  bindNamespaceClass();

  vm.exceptionClass = defineNativeClass("Exception");
  bindSuperclass(vm.exceptionClass, vm.objectClass);
  vm.exceptionClass->classType = OBJ_EXCEPTION;
  DEF_METHOD(vm.exceptionClass, Exception, __init__, 1);
  DEF_METHOD(vm.exceptionClass, Exception, message, 0);
  DEF_METHOD(vm.exceptionClass, Exception, stacktrace, 0);
  DEF_METHOD(vm.exceptionClass, Exception, __str__, 0);
  DEF_METHOD(vm.exceptionClass, Exception, __format__, 0);

  ObjClass* runtimeExceptionClass = defineNativeException("RuntimeException", vm.exceptionClass);
  defineNativeException("ArithmeticException", runtimeExceptionClass);
  defineNativeException("IllegalArgumentException", runtimeExceptionClass);
  defineNativeException("IndexOutOfBoundsException", runtimeExceptionClass);
  defineNativeException("UnsupportedOperationException", runtimeExceptionClass);
  defineNativeException("NotImplementedException", runtimeExceptionClass);
  defineNativeException("AssertException", runtimeExceptionClass);
  defineNativeException("InstantiationException", runtimeExceptionClass);
  defineNativeException("CallException", runtimeExceptionClass);

  vm.generatorClass = defineNativeClass("Generator");
  bindSuperclass(vm.generatorClass, vm.objectClass);
  vm.generatorClass->classType = OBJ_GENERATOR;
  DEF_METHOD(vm.generatorClass, Generator, next, 0);
  DEF_METHOD(vm.generatorClass, Generator, returns, 1);
  DEF_METHOD(vm.generatorClass, Generator, throws, 1);

  ObjClass* generatorMetaclass = vm.generatorClass->obj.klass;
  setClassProperty(vm.generatorClass, "stateStart", INT_VAL(GENERATOR_START));
  setClassProperty(vm.generatorClass, "stateYield", INT_VAL(GENERATOR_YIELD));
  setClassProperty(vm.generatorClass, "stateResume", INT_VAL(GENERATOR_RESUME));
  setClassProperty(vm.generatorClass, "stateReturn", INT_VAL(GENERATOR_RETURN));
  setClassProperty(vm.generatorClass, "stateThrow", INT_VAL(GENERATOR_THROW));

	vm.nilClass = defineNativeClass("Nil");
	bindSuperclass(vm.nilClass, vm.objectClass);
	DEF_METHOD(vm.nilClass, Nil, __init__, 0);
	DEF_METHOD(vm.nilClass, Nil, clone, 0);
	DEF_METHOD(vm.nilClass, Nil, __str__, 0);
	DEF_METHOD(vm.nilClass, Nil, __format__, 0);

	vm.boolClass = defineNativeClass("Bool");
	bindSuperclass(vm.boolClass, vm.objectClass);
	DEF_METHOD(vm.boolClass, Bool, __init__, 0);
	DEF_METHOD(vm.boolClass, Bool, clone, 0);
	DEF_METHOD(vm.boolClass, Bool, __str__, 0);
	DEF_METHOD(vm.boolClass, Bool, __format__, 0);

	vm.numberClass = defineNativeClass("Number");
	bindSuperclass(vm.numberClass, vm.objectClass);
  DEF_METHOD(vm.numberClass, Number, __init__, 0);
  DEF_METHOD(vm.numberClass, Number, clone, 0);
  DEF_METHOD(vm.numberClass, Number, __str__, 0);
  DEF_METHOD(vm.numberClass, Number, __format__, 0);
  DEF_OPERATOR(vm.numberClass, Number, ==, __equal__, 1);
  DEF_OPERATOR(vm.numberClass, Number, >, __greater__, 1);
  DEF_OPERATOR(vm.numberClass, Number, <, __less__, 1);
  DEF_OPERATOR(vm.numberClass, Number, +, __add__, 1);
  DEF_OPERATOR(vm.numberClass, Number, -, __subtract__, 1);
  DEF_OPERATOR(vm.numberClass, Number, *, __multiply__, 1);
  DEF_OPERATOR(vm.numberClass, Number, /, __divide__, 1);
  DEF_OPERATOR(vm.numberClass, Number, %, __modulo__, 1);
  DEF_OPERATOR(vm.numberClass, Number, **, __power__, 1);

  vm.intClass = defineNativeClass("Int");
  bindSuperclass(vm.intClass, vm.numberClass);
  DEF_METHOD(vm.intClass, Int, __init__, 0);
  DEF_METHOD(vm.intClass, Int, clone, 0);
  DEF_METHOD(vm.intClass, Int, __str__, 0);
  DEF_METHOD(vm.intClass, Int, __format__, 0);
  DEF_OPERATOR(vm.intClass, Int, +, __add__, 1);
  DEF_OPERATOR(vm.intClass, Int, -, __subtract__, 1);
  DEF_OPERATOR(vm.intClass, Int, *, __multiply__, 1);
  DEF_OPERATOR(vm.intClass, Int, %, __modulo__, 1);

  vm.floatClass = defineNativeClass("Float");
  bindSuperclass(vm.floatClass, vm.numberClass);
  DEF_METHOD(vm.floatClass, Float, __init__, 0);
  DEF_METHOD(vm.floatClass, Float, clone, 0);
  DEF_METHOD(vm.floatClass, Float, __str__, 0);
  DEF_METHOD(vm.floatClass, Float, __format__, 0);


  vm.stringClass = defineNativeClass("String");
  bindSuperclass(vm.stringClass, vm.objectClass);
  vm.stringClass->classType = OBJ_STRING;
  DEF_METHOD(vm.stringClass, String, __init__, 1);
  DEF_METHOD(vm.stringClass, String, capitalize, 0);
  DEF_METHOD(vm.stringClass, String, clone, 0);
  DEF_METHOD(vm.stringClass, String, contains, 1);
  DEF_METHOD(vm.stringClass, String, decapitalize, 0);
  DEF_METHOD(vm.stringClass, String, ends, 1);
  DEF_METHOD(vm.stringClass, String, subscript, 1);
  DEF_METHOD(vm.stringClass, String, search, 1);
  DEF_METHOD(vm.stringClass, String, length, 0);
  DEF_METHOD(vm.stringClass, String, next, 1);
  DEF_METHOD(vm.stringClass, String, nextValue, 1);
  DEF_METHOD(vm.stringClass, String, replace, 2);
  DEF_METHOD(vm.stringClass, String, reverse, 0);
  DEF_METHOD(vm.stringClass, String, split, 1);
  DEF_METHOD(vm.stringClass, String, starts, 1);
  DEF_METHOD(vm.stringClass, String, cut, 2);
  DEF_METHOD(vm.stringClass, String, lower, 0);
  DEF_METHOD(vm.stringClass, String, __str__, 0);
  DEF_METHOD(vm.stringClass, String, __format__, 0);
  DEF_METHOD(vm.stringClass, String, upper, 0);
  DEF_METHOD(vm.stringClass, String, trim, 0);
  DEF_OPERATOR(vm.stringClass, String, +, __add__, 1);
  DEF_OPERATOR(vm.stringClass, String, [], __getSubscript__, 1);

  for (int i = 0; i < vm.strings.capacity; i++) {
    Entry* entry = &vm.strings.entries[i];
    if (entry->key == NULL) continue;
    entry->key->obj.klass = vm.stringClass;
  }

  vm.functionClass = defineNativeClass("Function");
  bindSuperclass(vm.functionClass, vm.objectClass);
  vm.functionClass->classType = OBJ_FUNCTION;
  DEF_METHOD(vm.functionClass, Function, __init__, 0);
  DEF_METHOD(vm.functionClass, Function, arity, 0);
  DEF_METHOD(vm.functionClass, Function, clone, 0);
  DEF_METHOD(vm.functionClass, Function, isNative, 0);
  DEF_METHOD(vm.functionClass, Function, name, 0);
  DEF_METHOD(vm.functionClass, Function, __str__, 0);
  DEF_METHOD(vm.functionClass, Function, __format__, 0);
  DEF_METHOD(vm.functionClass, Function, upvalueCount, 0);
  DEF_OPERATOR(vm.functionClass, Function, (), __invoke__, -1);

  vm.methodClass = defineNativeClass("Method");
  bindSuperclass(vm.methodClass, vm.objectClass);
  vm.methodClass->classType = OBJ_FUNCTION;
  DEF_METHOD(vm.methodClass, Method, __init__, 0);
  DEF_METHOD(vm.methodClass, Method, arity, 0);
  DEF_METHOD(vm.methodClass, Method, clone, 0);
  DEF_METHOD(vm.methodClass, Method, name, 0);
  DEF_METHOD(vm.methodClass, Method, receiver, 0);
  DEF_METHOD(vm.methodClass, Method, __str__, 0);
  DEF_METHOD(vm.methodClass, Method, __format__, 0);
  DEF_METHOD(vm.methodClass, Method, upvalueCount, 0);

  vm.currentNamespace = vm.rootNamespace;
}
