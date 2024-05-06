#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "graphics.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../string/string.h"
#include "../hash/hash.h"
#include "../value/value.h"
#include "../vm/vm.h"

NATIVE_FUNCTION(createWindow) {
  assertArgCount("createWindow(title, width, height)", 3, argCount);
  assertArgIsString("createWindow(title, width, height)", args, 0);
  assertArgIsNumber("createWindow(title, width, height)", args, 1);
  assertArgIsNumber("createWindow(title, width, height)", args, 2);

  const char* title = AS_CSTRING(args[0]);
  int width = AS_NUMBER(args[1]);
  int height = AS_NUMBER(args[2]);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    RETURN_NIL;
  }

  SDL_Window* window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    RETURN_NIL;
  }

  SDL_Event e;
  int quit = 0;

  while (!quit) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = 1;
      }
    }
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  RETURN_NIL;
}

void registerGraphicsPackage() {
  ObjNamespace* graphicsNamespace = defineNativeNamespace("graphics", vm.stdNamespace);
  vm.currentNamespace = graphicsNamespace;

  DEF_FUNCTION(createWindow, 3);

  vm.currentNamespace = vm.rootNamespace;
}
