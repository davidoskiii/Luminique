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

int opCodeOffset(Chunk* chunk, int ip) {
  OpCode code = chunk->code[ip];
  switch (code) {
    case OP_CONSTANT: return 2;
    case OP_NIL: return 1;
    case OP_TRUE: return 1;
    case OP_FALSE: return 1;
    case OP_POP: return 1;
    case OP_DUP: return 1;
    case OP_GET_LOCAL: return 2;
    case OP_SET_LOCAL: return 2;
    case OP_DEFINE_CONST: return 2;
    case OP_DEFINE_GLOBAL: return 2;
    case OP_GET_GLOBAL: return 2;
    case OP_SET_GLOBAL: return 2;
    case OP_GET_UPVALUE: return 2;
    case OP_SET_UPVALUE: return 2;
    case OP_GET_PROPERTY: return 2;
    case OP_SET_PROPERTY: return 2;
    case OP_GET_SUBSCRIPT: return 1;
    case OP_SET_SUBSCRIPT: return 1;
    case OP_GET_SUPER: return 2;
    case OP_EQUAL: return 1;
    case OP_GREATER: return 1;
    case OP_LESS: return 1;
    case OP_ADD: return 1;
    case OP_SUBTRACT: return 1;
    case OP_MULTIPLY: return 1;
    case OP_DIVIDE: return 1;
    case OP_MODULO: return 1;
    case OP_NOT: return 1;
    case OP_NEGATE: return 1;
    case OP_JUMP: return 3;
    case OP_JUMP_IF_FALSE: return 3;
    case OP_LOOP: return 3;
    case OP_CALL: return 2;
    case OP_INVOKE: return 3;
    case OP_SUPER_INVOKE: return 3;
    case OP_CLOSURE: {
      int constant = (chunk->code[ip + 1] << 8) | chunk->code[ip + 2];
      ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
      return 2 + (function->upvalueCount * 2);
    }
    case OP_CLOSE_UPVALUE: return 1;
    case OP_CLASS: return 2;
    case OP_INHERIT: return 1;
    case OP_ARRAY: return 2;
    case OP_THROW: return 1;
    case OP_RETURN: return 1;
    case OP_END: return 1;
    default: return 0;
  }
}
