#include <stdlib.h>

#include "chunk.h"
#include "../memory/memory.h"
#include "../vm/vm.h"

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

int addConstant(Chunk* chunk, Value value) {
  push(value);
  writeValueArray(&chunk->constants, value);
  pop();
  return chunk->constants.count - 1;
}

void printCode(OpCode code) {
  const char* opcodeNames[] = {
    "OP_CONSTANT",
    "OP_CONSTANT_16",
    "OP_NIL",
    "OP_TRUE",
    "OP_FALSE",
    "OP_ARRAY",
    "OP_DICTIONARY",
    "OP_NAMESPACE",
    "OP_USING",
    "OP_SUBNAMESPACE",
    "OP_GET_NAMESPACE",
    "OP_ASSERT",
    "OP_POP",
    "OP_GET_LOCAL",
    "OP_SET_LOCAL",
    "OP_GET_GLOBAL",
    "OP_DEFINE_GLOBAL",
    "OP_DEFINE_CONST",
    "OP_SET_GLOBAL",
    "OP_GET_UPVALUE",
    "OP_SET_UPVALUE",
    "OP_TYPEOF",
    "OP_EQUAL",
    "OP_GREATER",
    "OP_LESS",
    "OP_ADD",
    "OP_SUBTRACT",
    "OP_MULTIPLY",
    "OP_DIVIDE",
    "OP_MODULO",
    "OP_POWER",
    "OP_NOT",
    "OP_NEGATE",
    "OP_AFFERM",
    "OP_JUMP",
    "OP_JUMP_IF_FALSE",
    "OP_LOOP",
    "OP_END",
    "OP_CALL",
    "OP_CLOSURE",
    "OP_DUP",
    "OP_CLOSE_UPVALUE",
    "OP_CLASS",
    "OP_GET_PROPERTY",
    "OP_SET_PROPERTY",
    "OP_GET_SUBSCRIPT",
    "OP_SET_SUBSCRIPT",
    "OP_REQUIRE",
    "OP_GET_SUPER",
    "OP_METHOD",
    "OP_STATIC_METHOD",
    "OP_INVOKE",
    "OP_SUPER_INVOKE",
    "OP_THROW",
    "OP_TRY",
    "OP_CATCH",
    "OP_FINALLY",
    "OP_INHERIT",
    "OP_BITAND",
    "OP_BITOR",
    "OP_BITXOR",
    "OP_BITNOT",
    "OP_SHOWEL_L",
    "OP_SHOWEL_R",
    "OP_RETURN",
    "OP_RETURN_NONLOCAL"
  };

  if (code < 0 || code >= OP_CODE_COUNT) {
    printf("Unknown OPCode: %d\n", code);
    return;
  }
  if (strcmp(opcodeNames[code], "OP_CLOSURE") == 0) printf("%s <------\n", opcodeNames[code]);
  else printf("%s\n", opcodeNames[code]);
}


int opCodeOffset(Chunk* chunk, int ip) {
    OpCode code = chunk->code[ip];
    printCode(code);
    switch (code) {
        case OP_CONSTANT: return 2;
        case OP_CONSTANT_16: return 3;
        case OP_NAMESPACE: return 1;
        case OP_NIL: return 1;
        case OP_TRUE: return 1;
        case OP_FALSE: return 1;
        case OP_POP: return 1;
        case OP_DUP: return 1;
        case OP_GET_LOCAL: return 2;
        case OP_SET_LOCAL: return 2;
        case OP_DEFINE_GLOBAL: return 3;
        case OP_DEFINE_CONST: return 3;
        case OP_GET_GLOBAL: return 3;
        case OP_SET_GLOBAL: return 3;
        case OP_GET_UPVALUE: return 2;
        case OP_SET_UPVALUE: return 2;
        case OP_GET_PROPERTY: return 3;
        case OP_SET_PROPERTY: return 3;
        case OP_GET_SUBSCRIPT: return 1;
        case OP_SET_SUBSCRIPT: return 1;
        case OP_GET_SUPER: return 3;
        case OP_ASSERT: return 1;
        case OP_EQUAL: return 1;
        case OP_GREATER: return 1;
        case OP_LESS: return 1;
        case OP_ADD: return 1;
        case OP_SUBTRACT: return 1;
        case OP_MULTIPLY: return 1;
        case OP_DIVIDE: return 1;
        case OP_MODULO: return 1;
        case OP_POWER: return 1;
        case OP_NOT: return 1;
        case OP_BITAND: return 1;
        case OP_BITOR: return 1;
        case OP_BITXOR: return 1;
        case OP_BITNOT: return 1;
        case OP_SHOWEL_L: return 1;
        case OP_SHOWEL_R: return 1;
        case OP_NEGATE: return 1;
        case OP_AFFERM: return 1;
        case OP_JUMP: return 3;
        case OP_JUMP_IF_FALSE: return 3;
        case OP_LOOP: return 3;
        case OP_CALL: return 2;
        case OP_TYPEOF: return 1;
        case OP_INVOKE: return 3;
        case OP_SUPER_INVOKE: return 3;
        case OP_CLOSURE: {
            int constant = (chunk->code[ip + 1] << 8) | chunk->code[ip + 2];
            printValue(chunk->constants.values[constant]);
            ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
            return 3 + (function->upvalueCount * 2);
        }
        case OP_CLOSE_UPVALUE: return 1;
        case OP_CLASS: return 3;
        case OP_STATIC_METHOD: return 3;
        case OP_METHOD: return 3; 
        case OP_INHERIT: return 1;
        case OP_ARRAY: return 2;
        case OP_DICTIONARY: return 2;
        case OP_REQUIRE: return 1;
        case OP_SUBNAMESPACE: return 2;
        case OP_GET_NAMESPACE: return 3;
        case OP_USING: return 2;
        case OP_THROW: return 2;
        case OP_TRY: return 6;
        case OP_CATCH: return 1;
        case OP_FINALLY: return 1;
        case OP_RETURN: return 1;
        case OP_RETURN_NONLOCAL: return 2;
        case OP_END: return 1;
        default: return 0;
    }
}

