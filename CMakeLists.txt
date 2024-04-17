cmake_minimum_required(VERSION 3.0)
project(Luminique)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Source files
set(SOURCES
  main.c
  chunk/chunk.c
  debug/debug.c
  memory/memory.c
  value/value.c
  vm/vm.c
  compiler/compiler.c
  scanner/scanner.c
  object/object.c
  table/table.c
  native/native.c
  std/std.c
  assert/assert.c
  util/util.c
  lang/lang.c
  hash/hash.c
  string/string.c
  pcg/pcg.c
  io/io.c
)

# Add an executable target
add_executable(luminique ${SOURCES})

# Link libraries
target_link_libraries(luminique m)
