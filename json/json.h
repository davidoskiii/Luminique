#ifndef cluminique_json_h
#define cluminique_json_h

#include "../common.h"
#include "../string/string.h"
#include "../cjson/cjson.h"

void registerJsonPackage();

Value convertCJSONArrayToArray(cJSON* jsonArray);
cJSON* convertArrayToCJSONArray(Value array);
Value convertCJSONToDictionary(cJSON* json);

#endif
