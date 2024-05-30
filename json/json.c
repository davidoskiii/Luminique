#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "json.h"
#include "../collection/collection.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../value/value.h"
#include "../vm/vm.h"

Value convertCJSONArrayToArray(cJSON* jsonArray) {
  ObjArray* array = newArray();
  push(OBJ_VAL(array));

  cJSON* item = NULL;
  cJSON_ArrayForEach(item, jsonArray) {
    Value value;

    if (cJSON_IsString(item)) {
      value = OBJ_VAL(copyString(item->valuestring, strlen(item->valuestring)));
    } else if (cJSON_IsNumber(item)) {
      value = NUMBER_VAL(item->valuedouble);
    } else if (cJSON_IsBool(item)) {
      value = BOOL_VAL(cJSON_IsTrue(item));
    } else if (cJSON_IsNull(item)) {
      value = NIL_VAL;
    } else if (cJSON_IsObject(item)) {
      value = convertCJSONToDictionary(item);
    } else if (cJSON_IsArray(item)) {
      value = convertCJSONArrayToArray(item);
    } else {
      value = NIL_VAL;
    }

    writeValueArray(&array->elements, value);
  }

  pop();
  return OBJ_VAL(array);
}

Value convertCJSONToDictionary(cJSON* json) {
  ObjDictionary* dict = newDictionary();
  push(OBJ_VAL(dict));

  cJSON* item = NULL;
  cJSON_ArrayForEach(item, json) {
    Value key = OBJ_VAL(copyString(item->string, strlen(item->string)));
    Value value;

    if (cJSON_IsString(item)) {
      value = OBJ_VAL(copyString(item->valuestring, strlen(item->valuestring)));
    } else if (cJSON_IsNumber(item)) {
      value = NUMBER_VAL(item->valuedouble);
    } else if (cJSON_IsBool(item)) {
      value = BOOL_VAL(cJSON_IsTrue(item));
    } else if (cJSON_IsNull(item)) {
      value = NIL_VAL;
    } else if (cJSON_IsObject(item)) {
      value = convertCJSONToDictionary(item);
    } else if (cJSON_IsArray(item)) {
      value = convertCJSONArrayToArray(item);
    } else {
      value = NIL_VAL;
    }

    dictSet(dict, key, value);
  }

  pop();
  return OBJ_VAL(dict);
}

NATIVE_FUNCTION(parse) {
  assertArgCount("json::parse(string)", 1, argCount);
  assertArgIsString("json::parse(string)", args, 0);

  const char* jsonString = AS_CSTRING(args[0]);
  cJSON* json = cJSON_Parse(jsonString);

  if (json == NULL) {
    RETURN_NIL;
  }

  Value result = convertCJSONToDictionary(json);
  cJSON_Delete(json);

  RETURN_OBJ(result);
}

void registerJsonPackage() {
  ObjNamespace* jsonNamespace = defineNativeNamespace("json", vm.stdNamespace);
  vm.currentNamespace = jsonNamespace;

  DEF_FUNCTION(parse, 1);

  vm.currentNamespace = vm.rootNamespace;
}
