#ifndef cluminique_chunk_h
#define cluminique_chunk_h

#include "../common.h"
#include "../value/value.h"

typedef enum {
  OP_CONSTANT,
  OP_CONSTANT_16,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_DUP,
  OP_INCREMENT_LOCAL,
  OP_INCREMENT_GLOBAL,
  OP_INCREMENT_UPVALUE,
  OP_DECREMENT_LOCAL,
  OP_DECREMENT_GLOBAL,
  OP_DECREMENT_UPVALUE,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_DEFINE_CONST,
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_GET_SUBSCRIPT,
  OP_SET_SUBSCRIPT,
  OP_GET_SUPER,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_MODULO,
  OP_POWER,
  OP_NOT,
  OP_BITAND,
  OP_BITOR,
  OP_BITXOR,
  OP_BITNOT,
  OP_SHOWEL_L,
  OP_SHOWEL_R,
  OP_NEGATE,
  OP_AFFERM,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_JUMP_IF_EMPTY,
  OP_LOOP,
  OP_CALL,
  OP_TYPEOF,
  OP_INVOKE,
  OP_SUPER_INVOKE,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_ENUM_ELEMENT,
  OP_ENUM,
  OP_CLASS,
  OP_INHERIT,
  OP_METHOD,
  OP_STATIC_METHOD,
  OP_GETTER,
  OP_SETTER,
  OP_ARRAY,
  OP_DICTIONARY,
  OP_RANGE,
  OP_REQUIRE,
  OP_BEGIN_NAMESPACE,
  OP_END_NAMESPACE,
  OP_USING,
  OP_SUBNAMESPACE,
  OP_GET_COLON_PROPERTY,
  OP_ASSERT,
  OP_THROW,
  OP_TRY,
  OP_CATCH,
  OP_FINALLY,
  OP_RETURN,
  OP_RETURN_NONLOCAL,
  OP_END
} OpCode;

typedef struct {
  int count;
  int capacity;
  uint8_t* code;
  int* lines;
  ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
int opCodeOffset(Chunk* chunk, int ip);

#endif
