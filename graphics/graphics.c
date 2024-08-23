#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <stdbool.h>
#include <string.h>

#include "graphics.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../value/value.h"
#include "../vm/vm.h"

void fillPolygon(SDL_Renderer* renderer, SDL_Point* vertices, int numVertices) {
  int minY = vertices[0].y;
  int maxY = vertices[0].y;
  
  for (int i = 1; i < numVertices; i++) {
    if (vertices[i].y < minY) minY = vertices[i].y;
    if (vertices[i].y > maxY) maxY = vertices[i].y;
  }

  for (int y = minY; y <= maxY; y++) {
    int nodes = 0;
    int nodeX[numVertices];

    for (int i = 0, j = numVertices - 1; i < numVertices; j = i++) {
      if (vertices[i].y < y && vertices[j].y >= y || vertices[j].y < y && vertices[i].y >= y) {
        nodeX[nodes++] = (vertices[i].x + (y - vertices[i].y) * (vertices[j].x - vertices[i].x) / (vertices[j].y - vertices[i].y));
      }
    }

    for (int i = 0; i < nodes - 1; i++) {
      for (int j = i + 1; j < nodes; j++) {
        if (nodeX[i] > nodeX[j]) {
          int temp = nodeX[i];
          nodeX[i] = nodeX[j];
          nodeX[j] = temp;
        }
      }
    }

    for (int i = 0; i < nodes; i += 2) {
      if (nodeX[i] >= 0 && nodeX[i + 1] > nodeX[i]) {
        SDL_RenderDrawLine(renderer, nodeX[i], y, nodeX[i + 1], y);
      }
    }
  }
}


NATIVE_METHOD(Window, __init__) {
  assertArgCount("Window::__init__(title, width, height)", 3, argCount);
  assertArgIsString("Window::__init__(title, width, height)", args, 0);
  assertArgIsInt("Window::__init__(title, width, height)", args, 1);
  assertArgIsInt("Window::__init__(title, width, height)", args, 2);

  const char* title = AS_CSTRING(args[0]);
  int width = AS_INT(args[1]);
  int height = AS_INT(args[2]);

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
  ObjWindow* window = AS_WINDOW(receiver);
  SDL_ShowWindow(window->window);
  RETURN_NIL;
}

NATIVE_METHOD(Window, hide) {
  assertArgCount("Window::hide()", 0, argCount);
  ObjWindow* window = AS_WINDOW(receiver);
  SDL_HideWindow(window->window);
  RETURN_NIL;
}

NATIVE_METHOD(Window, close) {
  assertArgCount("Window::close()", 0, argCount);
  ObjWindow* window = AS_WINDOW(receiver);
  SDL_DestroyWindow(window->window);
  RETURN_NIL;
}

NATIVE_METHOD(Window, pollEvent) {
  assertArgCount("Window::pollEvent()", 0, argCount);

  SDL_Event sdlEvent;
  if (SDL_PollEvent(&sdlEvent)) {
    ObjEvent* event = newEvent(&sdlEvent);
    RETURN_OBJ(event);
  } else {
    RETURN_NIL;
  }
}

NATIVE_METHOD(Window, waitEvent) {
  assertArgCount("Window::waitEvent()", 0, argCount);

  SDL_Event sdlEvent;
  if (SDL_WaitEvent(&sdlEvent)) {
    ObjEvent* event = newEvent(&sdlEvent);
    RETURN_OBJ(event);
  } else {
    RETURN_NIL;
  }
}

NATIVE_METHOD(Window, setDrawColor) {
  assertArgCount("Window::setDrawColor(r, g, b, a)", 4, argCount);
  assertArgIsInt("Window::setDrawColor(r, g, b, a)", args, 0);
  assertArgIsInt("Window::setDrawColor(r, g, b, a)", args, 1);
  assertArgIsInt("Window::setDrawColor(r, g, b, a)", args, 2);
  assertArgIsInt("Window::setDrawColor(r, g, b, a)", args, 3);

  ObjWindow* window = AS_WINDOW(receiver);
  int r = AS_INT(args[0]);
  int g = AS_INT(args[1]);
  int b = AS_INT(args[2]);
  int a = AS_INT(args[3]);

  SDL_SetRenderDrawColor(window->renderer, r, g, b, a);
  RETURN_NIL;
}

NATIVE_METHOD(Window, fillRect) {
  assertArgCount("Window::fillRect(x, y, w, h)", 4, argCount);
  assertArgIsInt("Window::fillRect(x, y, w, h)", args, 0);
  assertArgIsInt("Window::fillRect(x, y, w, h)", args, 1);
  assertArgIsInt("Window::fillRect(x, y, w, h)", args, 2);
  assertArgIsInt("Window::fillRect(x, y, w, h)", args, 3);

  ObjWindow* window = AS_WINDOW(receiver);
  SDL_Rect rect;
  rect.x = AS_INT(args[0]);
  rect.y = AS_INT(args[1]);
  rect.w = AS_INT(args[2]);
  rect.h = AS_INT(args[3]);

  SDL_RenderFillRect(window->renderer, &rect);
  RETURN_NIL;
}

NATIVE_METHOD(Window, drawLine) {
  assertArgCount("Window::drawLine(x1, y1, x2, y2)", 4, argCount);
  assertArgIsInt("Window::drawLine(x1, y1, x2, y2)", args, 0);
  assertArgIsInt("Window::drawLine(x1, y1, x2, y2)", args, 1);
  assertArgIsInt("Window::drawLine(x1, y1, x2, y2)", args, 2);
  assertArgIsInt("Window::drawLine(x1, y1, x2, y2)", args, 3);

  ObjWindow* window = AS_WINDOW(receiver);
  int x1 = AS_INT(args[0]);
  int y1 = AS_INT(args[1]);
  int x2 = AS_INT(args[2]);
  int y2 = AS_INT(args[3]);

  SDL_RenderDrawLine(window->renderer, x1, y1, x2, y2);
  RETURN_NIL;
}

NATIVE_METHOD(Window, fillCircle) {
  assertArgCount("Window::drawFilledCircle(x, y, radius)", 3, argCount);
  assertArgIsNumber("Window::drawFilledCircle(x, y, radius)", args, 0);
  assertArgIsNumber("Window::drawFilledCircle(x, y, radius)", args, 1);
  assertArgIsNumber("Window::drawFilledCircle(x, y, radius)", args, 2);

  ObjWindow* window = AS_WINDOW(receiver);
  int centerX = AS_NUMBER(args[0]);
  int centerY = AS_NUMBER(args[1]);
  int radius = AS_NUMBER(args[2]);

  int x = radius;
  int y = 0;
  int decisionOver2 = 1 - x;

  while (x >= y) {
    SDL_RenderDrawLine(window->renderer, centerX - x, centerY + y, centerX + x, centerY + y);
    SDL_RenderDrawLine(window->renderer, centerX - x, centerY - y, centerX + x, centerY - y);

    if (y != 0) {
      SDL_RenderDrawLine(window->renderer, centerX - y, centerY + x, centerX + y, centerY + x);
      SDL_RenderDrawLine(window->renderer, centerX - y, centerY - x, centerX + y, centerY - x);
    }

    y++;
    if (decisionOver2 <= 0) {
      decisionOver2 += 2 * y + 1;
    } else {
      x--;
      decisionOver2 += 2 * (y - x) + 1;
    }
  }

  RETURN_NIL;
}

NATIVE_METHOD(Window, drawCircle) {
  assertArgCount("Window::drawCircle(x, y, radius)", 3, argCount);
  assertArgIsInt("Window::drawCircle(x, y, radius)", args, 0);
  assertArgIsInt("Window::drawCircle(x, y, radius)", args, 1);
  assertArgIsInt("Window::drawCircle(x, y, radius)", args, 2);

  ObjWindow* window = AS_WINDOW(receiver);
  int x0 = AS_INT(args[0]);
  int y0 = AS_INT(args[1]);
  int radius = AS_INT(args[2]);

  int x = radius;
  int y = 0;
  int radiusError = 1 - x;

  while (x >= y) {
    SDL_RenderDrawPoint(window->renderer, x0 + x, y0 + y);
    SDL_RenderDrawPoint(window->renderer, x0 - x, y0 + y);
    SDL_RenderDrawPoint(window->renderer, x0 + x, y0 - y);
    SDL_RenderDrawPoint(window->renderer, x0 - x, y0 - y);
    SDL_RenderDrawPoint(window->renderer, x0 + y, y0 + x);
    SDL_RenderDrawPoint(window->renderer, x0 - y, y0 + x);
    SDL_RenderDrawPoint(window->renderer, x0 + y, y0 - x);
    SDL_RenderDrawPoint(window->renderer, x0 - y, y0 - x);
    y++;
    if (radiusError < 0) {
      radiusError += 2 * y + 1;
    } else {
      x--;
      radiusError += 2 * (y - x + 1);
    }
  }
  RETURN_NIL;
}

NATIVE_METHOD(Window, drawPolygon) {
  assertArgCount("Window::drawPolygon(vertices)", 1, argCount);
  assertArgIsArray("Window::drawPolygon(vertices)", args, 0);

  ObjArray* vertices = AS_ARRAY(args[0]);
  int numVertices = vertices->elements.count;

  if (numVertices < 3) {
    THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "A polygon must have at least 3 vertices.");
  }

  ObjWindow* window = AS_WINDOW(receiver);

  for (int i = 0; i < numVertices; i++) {
    if (!IS_ARRAY(vertices->elements.values[i]) || AS_ARRAY(vertices->elements.values[i])->elements.count != 2) {
      THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Each vertex must be a list of two numbers [x, y].");
    }
    ObjArray* vertex = AS_ARRAY(vertices->elements.values[i]);
    int x1 = AS_INT(vertex->elements.values[0]);
    int y1 = AS_INT(vertex->elements.values[1]);

    ObjArray* nextVertex = AS_ARRAY(vertices->elements.values[(i + 1) % numVertices]);
    int x2 = AS_INT(nextVertex->elements.values[0]);
    int y2 = AS_INT(nextVertex->elements.values[1]);

    SDL_RenderDrawLine(window->renderer, x1, y1, x2, y2);
  }

  RETURN_NIL;
}

NATIVE_METHOD(Window, fillPolygon) {
  assertArgCount("Window::fillPolygon(vertices)", 1, argCount);
  assertArgIsArray("Window::fillPolygon(vertices)", args, 0);

  ObjArray* vertices = AS_ARRAY(args[0]);
  int numVertices = vertices->elements.count;

  if (numVertices < 3) {
    THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "A polygon must have at least 3 vertices.");
  }

  SDL_Point points[numVertices];

  for (int i = 0; i < numVertices; i++) {
    if (!IS_ARRAY(vertices->elements.values[i]) || AS_ARRAY(vertices->elements.values[i])->elements.count != 2) {
      THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Each vertex must be a list of two numbers [x, y].");
    }
    ObjArray* vertex = AS_ARRAY(vertices->elements.values[i]);
    points[i].x = AS_INT(vertex->elements.values[0]);
    points[i].y = AS_INT(vertex->elements.values[1]);
  }

  ObjWindow* window = AS_WINDOW(receiver);
  fillPolygon(window->renderer, points, numVertices);

  RETURN_NIL;
}

NATIVE_METHOD(Window, clear) {
  assertArgCount("Window::clear()", 0, argCount);
  ObjWindow* window = AS_WINDOW(receiver);
  SDL_RenderClear(window->renderer);
  RETURN_NIL;
}

NATIVE_METHOD(Window, present) {
  assertArgCount("Window::present()", 0, argCount);
  ObjWindow* window = AS_WINDOW(receiver);
  SDL_RenderPresent(window->renderer);
  RETURN_NIL;
}

NATIVE_FUNCTION(createWindow) {
  assertArgCount("createWindow(title, width, height)", 3, argCount);
  assertArgIsString("createWindow(title, width, height)", args, 0);
  assertArgIsInt("createWindow(title, width, height)", args, 1);
  assertArgIsInt("createWindow(title, width, height)", args, 2);

  const char* title = AS_CSTRING(args[0]);
  int width = AS_INT(args[1]);
  int height = AS_INT(args[2]);

  ObjWindow* window = newWindow(title, width, height);

  RETURN_OBJ(window);
}

NATIVE_METHOD(Event, __str__) {
  assertArgCount("Event::__str__()", 0, argCount);
  RETURN_STRING_FMT(eventToString(AS_EVENT(receiver)));
}

NATIVE_METHOD(Event, __format__) {
  assertArgCount("Event::__format__()", 0, argCount);
  RETURN_STRING_FMT(eventToString(AS_EVENT(receiver)));
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
  DEF_METHOD(vm.windowClass, Window, hide, 0);
  DEF_METHOD(vm.windowClass, Window, close, 0);
  DEF_METHOD(vm.windowClass, Window, pollEvent, 0);
  DEF_METHOD(vm.windowClass, Window, waitEvent, 0);
  DEF_METHOD(vm.windowClass, Window, setDrawColor, 4);
  DEF_METHOD(vm.windowClass, Window, drawLine, 4);
  DEF_METHOD(vm.windowClass, Window, fillRect, 4);
  DEF_METHOD(vm.windowClass, Window, fillCircle, 3);
  DEF_METHOD(vm.windowClass, Window, drawCircle, 3);
  DEF_METHOD(vm.windowClass, Window, drawPolygon, 1);
  DEF_METHOD(vm.windowClass, Window, fillPolygon, 1);
  DEF_METHOD(vm.windowClass, Window, clear, 0);
  DEF_METHOD(vm.windowClass, Window, present, 0);

  vm.eventClass = defineNativeClass("Event", true);
  bindSuperclass(vm.eventClass, vm.objectClass);
  DEF_METHOD(vm.eventClass, Event, __str__, 0);
  DEF_METHOD(vm.eventClass, Event, __format__, 0);

  ObjEnum* eventTypeEnum = defineNativeEnum("EventType");
  defineNativeArtificialEnumElement(eventTypeEnum, "QUIT", INT_VAL(SDL_QUIT));
  defineNativeArtificialEnumElement(eventTypeEnum, "APP_TERMINATING", INT_VAL(SDL_APP_TERMINATING));
  defineNativeArtificialEnumElement(eventTypeEnum, "APP_LOWMEMORY", INT_VAL(SDL_APP_LOWMEMORY));
  defineNativeArtificialEnumElement(eventTypeEnum, "APP_WILLENTERBACKGROUND", INT_VAL(SDL_APP_WILLENTERBACKGROUND));
  defineNativeArtificialEnumElement(eventTypeEnum, "APP_DIDENTERBACKGROUND", INT_VAL(SDL_APP_DIDENTERBACKGROUND));
  defineNativeArtificialEnumElement(eventTypeEnum, "APP_WILLENTERFOREGROUND", INT_VAL(SDL_APP_WILLENTERFOREGROUND));
  defineNativeArtificialEnumElement(eventTypeEnum, "APP_DIDENTERFOREGROUND", INT_VAL(SDL_APP_DIDENTERFOREGROUND));
  defineNativeArtificialEnumElement(eventTypeEnum, "LOCALECHANGED", INT_VAL(SDL_LOCALECHANGED));
  defineNativeArtificialEnumElement(eventTypeEnum, "DISPLAYEVENT", INT_VAL(SDL_DISPLAYEVENT));
  defineNativeArtificialEnumElement(eventTypeEnum, "WINDOWEVENT", INT_VAL(SDL_WINDOWEVENT));
  defineNativeArtificialEnumElement(eventTypeEnum, "SYSWMEVENT", INT_VAL(SDL_SYSWMEVENT));
  defineNativeArtificialEnumElement(eventTypeEnum, "KEYDOWN", INT_VAL(SDL_KEYDOWN));
  defineNativeArtificialEnumElement(eventTypeEnum, "KEYUP", INT_VAL(SDL_KEYUP));
  defineNativeArtificialEnumElement(eventTypeEnum, "TEXTEDITING", INT_VAL(SDL_TEXTEDITING));
  defineNativeArtificialEnumElement(eventTypeEnum, "TEXTINPUT", INT_VAL(SDL_TEXTINPUT));
  defineNativeArtificialEnumElement(eventTypeEnum, "KEYMAPCHANGED", INT_VAL(SDL_KEYMAPCHANGED));
  defineNativeArtificialEnumElement(eventTypeEnum, "MOUSEMOTION", INT_VAL(SDL_MOUSEMOTION));
  defineNativeArtificialEnumElement(eventTypeEnum, "MOUSEBUTTONDOWN", INT_VAL(SDL_MOUSEBUTTONDOWN));
  defineNativeArtificialEnumElement(eventTypeEnum, "MOUSEBUTTONUP", INT_VAL(SDL_MOUSEBUTTONUP));
  defineNativeArtificialEnumElement(eventTypeEnum, "MOUSEWHEEL", INT_VAL(SDL_MOUSEWHEEL));
  defineNativeArtificialEnumElement(eventTypeEnum, "JOYAXISMOTION", INT_VAL(SDL_JOYAXISMOTION));
  defineNativeArtificialEnumElement(eventTypeEnum, "JOYBALLMOTION", INT_VAL(SDL_JOYBALLMOTION));
  defineNativeArtificialEnumElement(eventTypeEnum, "JOYHATMOTION", INT_VAL(SDL_JOYHATMOTION));
  defineNativeArtificialEnumElement(eventTypeEnum, "JOYBUTTONDOWN", INT_VAL(SDL_JOYBUTTONDOWN));
  defineNativeArtificialEnumElement(eventTypeEnum, "JOYBUTTONUP", INT_VAL(SDL_JOYBUTTONUP));
  defineNativeArtificialEnumElement(eventTypeEnum, "JOYDEVICEADDED", INT_VAL(SDL_JOYDEVICEADDED));
  defineNativeArtificialEnumElement(eventTypeEnum, "JOYDEVICEREMOVED", INT_VAL(SDL_JOYDEVICEREMOVED));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERAXISMOTION", INT_VAL(SDL_CONTROLLERAXISMOTION));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERBUTTONDOWN", INT_VAL(SDL_CONTROLLERBUTTONDOWN));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERBUTTONUP", INT_VAL(SDL_CONTROLLERBUTTONUP));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERDEVICEADDED", INT_VAL(SDL_CONTROLLERDEVICEADDED));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERDEVICEREMOVED", INT_VAL(SDL_CONTROLLERDEVICEREMOVED));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERDEVICEREMAPPED", INT_VAL(SDL_CONTROLLERDEVICEREMAPPED));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERTOUCHPADDOWN", INT_VAL(SDL_CONTROLLERTOUCHPADDOWN));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERTOUCHPADMOTION", INT_VAL(SDL_CONTROLLERTOUCHPADMOTION));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERTOUCHPADUP", INT_VAL(SDL_CONTROLLERTOUCHPADUP));
  defineNativeArtificialEnumElement(eventTypeEnum, "CONTROLLERSENSORUPDATE", INT_VAL(SDL_CONTROLLERSENSORUPDATE));
  defineNativeArtificialEnumElement(eventTypeEnum, "FINGERDOWN", INT_VAL(SDL_FINGERDOWN));
  defineNativeArtificialEnumElement(eventTypeEnum, "FINGERUP", INT_VAL(SDL_FINGERUP));
  defineNativeArtificialEnumElement(eventTypeEnum, "FINGERMOTION", INT_VAL(SDL_FINGERMOTION));
  defineNativeArtificialEnumElement(eventTypeEnum, "DOLLARGESTURE", INT_VAL(SDL_DOLLARGESTURE));
  defineNativeArtificialEnumElement(eventTypeEnum, "DOLLARRECORD", INT_VAL(SDL_DOLLARRECORD));
  defineNativeArtificialEnumElement(eventTypeEnum, "MULTIGESTURE", INT_VAL(SDL_MULTIGESTURE));
  defineNativeArtificialEnumElement(eventTypeEnum, "CLIPBOARDUPDATE", INT_VAL(SDL_CLIPBOARDUPDATE));
  defineNativeArtificialEnumElement(eventTypeEnum, "DROPFILE", INT_VAL(SDL_DROPFILE));
  defineNativeArtificialEnumElement(eventTypeEnum, "DROPTEXT", INT_VAL(SDL_DROPTEXT));
  defineNativeArtificialEnumElement(eventTypeEnum, "DROPBEGIN", INT_VAL(SDL_DROPBEGIN));
  defineNativeArtificialEnumElement(eventTypeEnum, "DROPCOMPLETE", INT_VAL(SDL_DROPCOMPLETE));
  defineNativeArtificialEnumElement(eventTypeEnum, "AUDIODEVICEADDED", INT_VAL(SDL_AUDIODEVICEADDED));
  defineNativeArtificialEnumElement(eventTypeEnum, "AUDIODEVICEREMOVED", INT_VAL(SDL_AUDIODEVICEREMOVED));
  defineNativeArtificialEnumElement(eventTypeEnum, "SENSORUPDATE", INT_VAL(SDL_SENSORUPDATE));
  defineNativeArtificialEnumElement(eventTypeEnum, "RENDER_TARGETS_RESET", INT_VAL(SDL_RENDER_TARGETS_RESET));
  defineNativeArtificialEnumElement(eventTypeEnum, "RENDER_DEVICE_RESET", INT_VAL(SDL_RENDER_DEVICE_RESET));
  defineNativeArtificialEnumElement(eventTypeEnum, "POLLSENTINEL", INT_VAL(SDL_POLLSENTINEL));
  defineNativeArtificialEnumElement(eventTypeEnum, "USEREVENT", INT_VAL(SDL_USEREVENT));

  DEF_FUNCTION(createWindow, 3);

  vm.currentNamespace = vm.rootNamespace;
}
