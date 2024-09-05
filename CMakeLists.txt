cmake_minimum_required(VERSION 3.0)
project(Luminique)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Source files
set(GCC_COVERAGE_COMPILE_FLAGS 
  "-O3 -g")

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
  assert/assert.c
  util/util.c
  collection/collection.c
  lang/lang.c
  hash/hash.c
  string/string.c
  pcg/pcg.c
  io/io.c
  math/math.c
  sys/sys.c
  chrono/chrono.c
  graphics/graphics.c
  random/random.c
  network/network.c
  yuarel/yuarel.c
  json/json.c
  cjson/cjson.c
  cnet/cnet.c
  generator/generator.c
  promise/promise.c
  os/os.c
  date/date.c
  loop/loop.c
  file/file.c
  sonus/sonus.c
  dsa/dsa.c
  exception/exception.c
  interceptor/interceptor.c
  variable/variable.c
  statistics/statistics.c
)

# Add an executable target
add_executable(luminique ${SOURCES})

# Link libraries
target_link_libraries(luminique m SDL2 readline curl uv SDL2_ttf SDL2_image SDL2_mixer)
add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})
