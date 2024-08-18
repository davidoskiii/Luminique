#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <regex.h>

#include "util.h"
#include "../pcg/pcg.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../value/value.h"
#include "../vm/vm.h"

#define MAX_MATCHES 1

char *substr(const char *str, int start, int end) {
  int len = end - start;
  char *substr = malloc(len + 1);
  if (!substr) {
    return NULL;
  }
  strncpy(substr, str + start, len);
  substr[len] = '\0';
  return substr;
}

char *concat(const char *s1, const char *s2) {
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  char *result = malloc(len1 + len2 + 1);
  if (!result) {
    return NULL;
  }
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

// REGEX 

NATIVE_METHOD(Regex, __init__) {
	assertArgCount("Regex::__init__(pattern)", 1, argCount);
	assertArgIsString("Regex::__init__(pattern)", args, 0);
	ObjInstance* self = AS_INSTANCE(receiver);
	setObjProperty(self, "pattern", args[0]);
	RETURN_OBJ(self);
}

NATIVE_METHOD(Regex, match) {
  assertArgCount("Regex::match(string)", 1, argCount);
  assertArgIsString("Regex::match(string)", args, 0);
  Value pattern = getObjProperty(AS_INSTANCE(receiver), "pattern");
  int length;
  regex_t regex;
  regmatch_t matches[MAX_MATCHES];
  int index;
  const char *text = AS_CSTRING(args[0]);

  int reti = regcomp(&regex, AS_CSTRING(pattern), REG_EXTENDED);
  if (reti) {
    regfree(&regex);
    RETURN_FALSE;
  }

  reti = regexec(&regex, text, MAX_MATCHES, matches, 0);
  if (!reti) {
    index = matches[0].rm_so;
  } else {
    index = -1;
  }

  regfree(&regex);

  RETURN_BOOL(index != -1);
}

NATIVE_METHOD(Regex, replace) {
  assertArgCount("Regex::replace(original, replacement)", 2, argCount);
  assertArgIsString("Regex::replace(original, replacement)", args, 0);
  assertArgIsString("Regex::replace(original, replacement)", args, 1);
  Value pattern = getObjProperty(AS_INSTANCE(receiver), "pattern");
  ObjString* original = AS_STRING(args[0]);
  ObjString* replacement = AS_STRING(args[1]);
  regex_t regex;
  regmatch_t matches[MAX_MATCHES];
  const char *pattern_str = AS_CSTRING(pattern);
  const char *original_str = original->chars;
  char *result = NULL;
  int new_length = 0;

  int reti = regcomp(&regex, pattern_str, REG_EXTENDED);
  if (reti) {
    regfree(&regex);
    RETURN_OBJ(original);
  }

  int offset = 0;

  char *accumulator = strdup("");

  // Iterate over all matches and replace each occurrence
  while (regexec(&regex, original_str + offset, MAX_MATCHES, matches, 0) == 0) {
    int start = matches[0].rm_so;
    int end = matches[0].rm_eo;
    int replacement_length = strlen(replacement->chars);

    new_length += start + replacement_length;

    char *prefix = substr(original_str, offset, offset + start);
    accumulator = concat(accumulator, prefix);
    free(prefix);

    accumulator = concat(accumulator, replacement->chars);

    offset += end;
  }

  accumulator = concat(accumulator, original_str + offset);

  result = strdup(accumulator);
  new_length = strlen(result);

  free(accumulator);

  regfree(&regex);

  RETURN_OBJ(takeString(result, new_length));
}

NATIVE_METHOD(Regex, __str__) {
	assertArgCount("Regex::__str__()", 0, argCount);
	Value pattern = getObjProperty(AS_INSTANCE(receiver), "pattern");
	return pattern;
}

NATIVE_METHOD(Regex, __format__) {
	assertArgCount("Regex::__format__()", 0, argCount);
	Value pattern = getObjProperty(AS_INSTANCE(receiver), "pattern");
	return pattern;
}

void registerUtilPackage() {
  ObjNamespace* utilNamespace = defineNativeNamespace("util", vm.stdNamespace);
  vm.currentNamespace = utilNamespace;

	ObjClass* regexClass = defineNativeClass("Regex", false);
	bindSuperclass(regexClass, vm.objectClass);
	DEF_METHOD(regexClass, Regex, __init__, 1);
	DEF_METHOD(regexClass, Regex, match, 1);
	DEF_METHOD(regexClass, Regex, replace, 2);
	DEF_METHOD(regexClass, Regex, __str__, 0);
	DEF_METHOD(regexClass, Regex, __format__, 0);

  vm.currentNamespace = vm.rootNamespace;
}
