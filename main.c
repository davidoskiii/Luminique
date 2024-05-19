#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "common.h"
#include "string/string.h"
#include "./vm/vm.h"

static void repl() {
  char *line;
  vm.currentModule = newModule(newString("<repl>"));
  vm.repl = true;

  for (;;) {
    line = readline("> ");

    if (!line) {
      printf("\n");
      break;
    }

    if (*line) add_history(line);

    vm.currentModule->source = line;
    interpret(vm.currentModule->source);
  }
}

char* readFile(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file \"%s\".\n", path);
    fprintf(stderr, "Usage: luminique [path]\n");
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
    exit(74);
  }

  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file \"%s\".\n", path);
    exit(74);
  }

  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

static void runFile(const char* filePath) {
  ObjString* path = newString(filePath);
  vm.currentModule = newModule(path);

  char* source = readFile(filePath);
  vm.currentModule->source = source;
  InterpretResult result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, char *argv[]) {
  initVM(argc, argv);

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    runFile(argv[1]);
  }

  freeVM();
  return 0;
}
