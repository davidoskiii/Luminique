#ifndef cluminique_debug_h
#define cluminique_debug_h

#include "../chunk/chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif
