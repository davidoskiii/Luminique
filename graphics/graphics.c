#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <stdbool.h>

#include "graphics.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../value/value.h"
#include "../vm/vm.h"

NATIVE_METHOD(Window, __init__) {
  assertArgCount("Window::__init__(title, width, height)", 3, argCount);
  assertArgIsString("Window::__init__(title, width, height)", args, 0);
  assertArgIsNumber("Window::__init__(title, width, height)", args, 1);
  assertArgIsNumber("Window::__init__(title, width, height)", args, 2);

  const char* title = AS_CSTRING(args[0]);
  int width = AS_NUMBER(args[1]);
  int height = AS_NUMBER(args[2]);

  ObjWindow* window = newWindow(title, width, height);

  RETURN_OBJ(window);
}

NATIVE_METHOD(Window, __str__) {
  assertArgCount("Window::__str__()", 0, argCount);
  RETURN_STRING_FMT("<%s window>", AS_WINDOW(receiver)->title);
}

NATIVE_METHOD(Window, __format__) {
  assertArgCount("Window::__format__()", 0, argCount);
  RETURN_STRING_FMT("<%s window>", AS_WINDOW(receiver)->title);
}

NATIVE_METHOD(Window, show) {
  assertArgCount("Window::show()", 0, argCount);
  SDL_ShowWindow(AS_WINDOW(receiver)->window);

  SDL_Event e;
  bool quit = false;

  while (!quit) {
    while (SDL_PollEvent(&e) != 0 && !quit) {
      switch (e.type) {
        case SDL_QUIT: quit = true; break;
        case SDL_WINDOWEVENT: {
          if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
            quit = true;
          }
          break;
        }
        default: break;
      }
    }
  }

  SDL_DestroyWindow(AS_WINDOW(receiver)->window);
  SDL_Quit();

  RETURN_NIL;
}

NATIVE_FUNCTION(createWindow) {
  assertArgCount("createWindow(title, width, height)", 3, argCount);
  assertArgIsString("createWindow(title, width, height)", args, 0);
  assertArgIsNumber("createWindow(title, width, height)", args, 1);
  assertArgIsNumber("createWindow(title, width, height)", args, 2);

  const char* title = AS_CSTRING(args[0]);
  int width = AS_NUMBER(args[1]);
  int height = AS_NUMBER(args[2]);

  ObjWindow* window = newWindow(title, width, height);

  RETURN_OBJ(window);
}

void registerGraphicsPackage() {
  ObjNamespace* graphicsNamespace = defineNativeNamespace("graphics", vm.stdNamespace);
  vm.currentNamespace = graphicsNamespace;

	vm.windowClass = defineNativeClass("Window", false);
	bindSuperclass(vm.windowClass, vm.objectClass);
	DEF_METHOD(vm.windowClass, Window, __init__, 3);
	DEF_METHOD(vm.windowClass, Window, __str__, 0);
	DEF_METHOD(vm.windowClass, Window, __format__, 0);
  DEF_METHOD(vm.windowClass, Window, show, 0);

  DEF_FUNCTION(createWindow, 3);

  vm.currentNamespace = vm.rootNamespace;
}
