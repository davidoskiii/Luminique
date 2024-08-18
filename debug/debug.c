#include <stdio.h>

#include "debug.h"
#include "../object/object.h"
#include "../value/value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;) {
    offset = disassembleInstruction(chunk, offset);
  }
}

static int constantValueInstruction16(const char* name, Chunk* chunk, int offset) {
  uint16_t constant = (uint16_t)(chunk->code[offset + 1] << 8);
  constant |= chunk->code[offset + 2];
  printf("%-16s %4d '", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 3;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s   %4d\n", name, constant);
  return offset + 2;
}

static int constantInstruction16(const char* name, Chunk* chunk, int offset) {
  uint16_t constant = (uint16_t)(chunk->code[offset + 1] << 8);
  constant |= chunk->code[offset + 2];
  printf("%-16s   %4d\n", name, constant);
  return offset + 3;
}

static int exceptionHandlerInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t exceptionType = chunk->code[offset + 1];
  uint16_t handlerAddress = (uint16_t)(chunk->code[offset + 2] << 8);
  handlerAddress |= chunk->code[offset + 3];
  uint16_t finallyAddress = (uint16_t)(chunk->code[offset + 4] << 8);
  finallyAddress |= chunk->code[offset + 5];
  printf("%-16s %4d -> %d, %d\n", name, exceptionType, handlerAddress, finallyAddress);
  return offset + 6;
}

static int invokeInstruction(const char* name, Chunk* chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  uint8_t argCount = chunk->code[offset + 2];
  printf("%-16s (%d args) %4d '", name, argCount, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 4;
}

static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}

static int byteInstruction(const char* name, Chunk* chunk,
                           int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2; 
}

static int jumpInstruction(const char* name, int sign,
                           Chunk* chunk, int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset,
         offset + 3 + sign * jump);
  return offset + 3;
}

int disassembleInstruction(Chunk* chunk, int offset) {
  printf("%04d ", offset);

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_NIL:
      return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
      return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
      return simpleInstruction("OP_FALSE", offset);
    case OP_ARRAY:
      return byteInstruction("OP_ARRAY", chunk, offset);
    case OP_DICTIONARY:
      return byteInstruction("OP_DICTIONARY", chunk, offset);
    case OP_RANGE:
      return simpleInstruction("OP_RANGE", offset);
    case OP_USING:
      return byteInstruction("OP_USING", chunk, offset);
    case OP_SUBNAMESPACE: 
      return byteInstruction("OP_SUBNAMESPACE", chunk, offset);
    case OP_POP:
      return simpleInstruction("OP_POP", offset);
    case OP_DUP:
      return simpleInstruction("OP_DUP", offset);
    case OP_INCREMENT_LOCAL:
      return byteInstruction("OP_INCREMENT_LOCAL", chunk, offset);
    case OP_INCREMENT_GLOBAL:
      return constantValueInstruction16("OP_INCREMENT_GLOBAL", chunk, offset);
    case OP_INCREMENT_UPVALUE:
      return byteInstruction("OP_INCREMENT_UPVALUE", chunk, offset);
    case OP_DECREMENT_LOCAL:
      return byteInstruction("OP_DECREMENT_LOCAL", chunk, offset);
    case OP_DECREMENT_GLOBAL:
      return constantValueInstruction16("OP_DECREMENT_GLOBAL", chunk, offset);
    case OP_DECREMENT_UPVALUE:
      return byteInstruction("OP_DECREMENT_UPVALUE", chunk, offset);
    case OP_GET_LOCAL:
      return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byteInstruction("OP_SET_LOCAL", chunk, offset);
    case OP_GET_GLOBAL:
      return constantValueInstruction16("OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL:
      return constantValueInstruction16("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_DEFINE_CONST:
      return constantValueInstruction16("OP_DEFINE_CONST", chunk, offset);
    case OP_CLASS_PROPRETY:
      return constantValueInstruction16("OP_CLASS_PROPRETY", chunk, offset);
    case OP_SET_GLOBAL:
      return constantValueInstruction16("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_UPVALUE:
      return byteInstruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
      return byteInstruction("OP_SET_UPVALUE", chunk, offset);
    case OP_EQUAL:
      return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER:
      return simpleInstruction("OP_GREATER", offset);
    case OP_LESS:
      return simpleInstruction("OP_LESS", offset);
    case OP_ADD:
      return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
    case OP_MODULO:
      return simpleInstruction("OP_MODULO", offset);
    case OP_POWER:
      return simpleInstruction("OP_POWER", offset);
    case OP_NOT:
      return simpleInstruction("OP_NOT", offset);
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
    case OP_AFFERM:
      return simpleInstruction("OP_AFFERM", offset);
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_CONSTANT_16:
      return constantInstruction16("OP_CONSTANT_16", chunk, offset);           
    case OP_JUMP:
      return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_JUMP_IF_EMPTY:
      return jumpInstruction("OP_JUMP_IF_EMPTY", 1, chunk, offset);
    case OP_LOOP:
      return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_CALL:
      return byteInstruction("OP_CALL", chunk, offset);
    case OP_GET_PROPERTY:
      return constantValueInstruction16("OP_GET_PROPERTY", chunk, offset);
    case OP_GET_COLON_PROPERTY:
      return constantValueInstruction16("OP_GET_COLON_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY:
      return constantValueInstruction16("OP_SET_PROPERTY", chunk, offset);
    case OP_GET_SUBSCRIPT:
      return simpleInstruction("OP_GET_SUBSCRIPT", offset);
    case OP_SET_SUBSCRIPT:
      return simpleInstruction("OP_SET_SUBSCRIPT", offset);
    case OP_REQUIRE:
      return simpleInstruction("OP_REQUIRE", offset);
    case OP_BEGIN_NAMESPACE:
      return constantValueInstruction16("OP_BEGIN_NAMESPACE", chunk, offset);
    case OP_END_NAMESPACE:
      return simpleInstruction("OP_END_NAMESPACE", offset);
    case OP_TYPEOF:
      return simpleInstruction("OP_TYPEOF", offset);
    case OP_INSTANCEOF:
      return simpleInstruction("OP_INSTANCEOF", offset);
    case OP_GET_SUPER:
      return constantValueInstruction16("OP_GET_SUPER", chunk, offset);
    case OP_CLOSURE: {
      offset++;
      uint16_t constant = (uint16_t)(chunk->code[offset++] << 8);
      constant |= chunk->code[offset++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      printValue(chunk->constants.values[constant]);
      printf("\n");

      ObjFunction* function = AS_FUNCTION(
          chunk->constants.values[constant]);
      for (int j = 0; j < function->upvalueCount; j++) {
        int isLocal = chunk->code[offset++];
        int index = chunk->code[offset++];
        printf("%04d      |                     %s %d\n",
               offset - 2, isLocal ? "local" : "upvalue", index);
      }

      return offset;
    }
    case OP_CLOSE_UPVALUE:
      return simpleInstruction("OP_CLOSE_UPVALUE", offset);
    case OP_ENUM:
      return constantValueInstruction16("OP_ENUM", chunk, offset);
    case OP_ENUM_ELEMENT:
      return constantValueInstruction16("OP_ENUM_ELEMENT", chunk, offset);
    case OP_CLASS:
      return constantValueInstruction16("OP_CLASS", chunk, offset);
    case OP_ABSTRACT_CLASS:
      return constantValueInstruction16("OP_ABSTRACT_CLASS", chunk, offset);
    case OP_METHOD:
      return constantValueInstruction16("OP_METHOD", chunk, offset);
    case OP_ABSTRACT_METHOD:
      return constantValueInstruction16("OP_ABSTRACT_METHOD", chunk, offset);
    case OP_GETTER:
      return constantValueInstruction16("OP_GETTER", chunk, offset);
    case OP_SETTER:
      return constantValueInstruction16("OP_SETTER", chunk, offset);
    case OP_INVOKE:
      return invokeInstruction("OP_INVOKE", chunk, offset);
    case OP_SUPER_INVOKE:
      return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
    case OP_THROW:
      return simpleInstruction("OP_THROW", offset);
    case OP_TRY:
      return exceptionHandlerInstruction("OP_TRY", chunk, offset);
    case OP_CATCH:
      return simpleInstruction("OP_CATCH", offset);
    case OP_FINALLY:
      return simpleInstruction("OP_FINALLY", offset);
    case OP_INHERIT:
      return simpleInstruction("OP_INHERIT", offset);
    case OP_ASSERT:
      return simpleInstruction("OP_ASSERT", offset);
    case OP_BITAND:
      return simpleInstruction("OP_BITAND", offset);
    case OP_BITXOR:
      return simpleInstruction("OP_BITXOR", offset);
    case OP_BITOR:
      return simpleInstruction("OP_BITOR", offset);
    case OP_BITNOT:
      return simpleInstruction("OP_BITNOT", offset);
    case OP_SHOWEL_L:
      return simpleInstruction("OP_SHOWEL_L", offset);
    case OP_SHOWEL_R:
      return simpleInstruction("OP_SHOWEL_R", offset);
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    case OP_RETURN_NONLOCAL:
      return byteInstruction("OP_RETURN_NONLOCAL", chunk, offset);
    case OP_YIELD:
      return simpleInstruction("OP_YIELD", offset);
    case OP_YIELD_FROM:
      return simpleInstruction("OP_YIELD_FROM", offset);
    case OP_AWAIT:
      return simpleInstruction("OP_AWAIT", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}
