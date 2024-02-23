#include "common.h"
#include "./chunk/chunk.h"
#include "./debug/debug.h"
#include "./vm/vm.h"

int main(int argc, char *argv[]) {
  initVM();

  Chunk chunk;
  initChunk(&chunk);


  int constant = addConstant(&chunk, 2);

  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);

  constant = addConstant(&chunk, 3);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);

  writeChunk(&chunk, OP_ADD, 123);

  constant = addConstant(&chunk, 2);
  writeChunk(&chunk, OP_CONSTANT, 123);
  writeChunk(&chunk, constant, 123);

  writeChunk(&chunk, OP_POWER, 123);

  writeChunk(&chunk, OP_NEGATE, 123);

  writeChunk(&chunk, OP_RETURN, 123);



  disassembleChunk(&chunk, "test chunk");
  interpret(&chunk);
  freeVM();
  freeChunk(&chunk);
  return 0;
}
