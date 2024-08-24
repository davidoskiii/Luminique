#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <string.h>

#include "graphics.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../value/value.h"
#include "../vm/vm.h"

typedef enum {
  MIDDLE, 
  LEFT, 
  RIGHT, 
  TOP, 
  BOTTOM
} AlignDirection;

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

SDL_Surface* renderTextToSurface(const char* text, SDL_Color color, TTF_Font* font) {
  SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
  return textSurface;
}

SDL_Texture* renderTextToTexture(SDL_Renderer* renderer, const char* text, SDL_Color color, TTF_Font* font) {
  SDL_Surface* textSurface = renderTextToSurface(text, color, font);
  if (!textSurface) return NULL;

  SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
  SDL_FreeSurface(textSurface);

  return textTexture;
}

NATIVE_METHOD(Window, __init__) {
  assertArgCount("Window::__init__(title, width, height, isResizable)", 4, argCount);
  assertArgIsString("Window::__init__(title, width, height, isResizable)", args, 0);
  assertArgIsInt("Window::__init__(title, width, height, isResizable)", args, 1);
  assertArgIsInt("Window::__init__(title, width, height, isResizable)", args, 2);
  assertArgIsBool("Window::__init__(title, width, height, isResizable)", args, 3);

  const char* title = AS_CSTRING(args[0]);
  int width = AS_INT(args[1]);
  int height = AS_INT(args[2]);
  bool isResizable = AS_BOOL(args[3]);

  ObjWindow* window = newWindow(title, width, height, isResizable);

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
  assertArgIsInt("Window::drawFilledCircle(x, y, radius)", args, 0);
  assertArgIsInt("Window::drawFilledCircle(x, y, radius)", args, 1);
  assertArgIsInt("Window::drawFilledCircle(x, y, radius)", args, 2);

  ObjWindow* window = AS_WINDOW(receiver);
  int centerX = AS_INT(args[0]);
  int centerY = AS_INT(args[1]);
  int radius = AS_INT(args[2]);

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
    if (!IS_ARRAY(vertices->elements.values[i]) || AS_ARRAY(vertices->elements.values[i])->elements.count != 2 || 
      !IS_INT(AS_ARRAY(vertices->elements.values[i])->elements.values[0]) || !IS_INT(AS_ARRAY(vertices->elements.values[i])->elements.values[1])) {
      THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Each vertex must be a list of two integers [x, y].");
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

NATIVE_METHOD(Window, loadFont) {
  assertArgCount("Window::loadFont(fontPath, size)", 2, argCount);
  assertArgIsString("Window::loadFont(fontPath, size)", args, 0);
  assertArgIsInt("Window::loadFont(fontPath, size)", args, 1);

  const char* fontPath = AS_CSTRING(args[0]);
  int size = AS_INT(args[1]);

  TTF_Font* font = TTF_OpenFont(fontPath, size);
  if (!font) {
    THROW_EXCEPTION_FMT(luminique::std::io, FileNotFoundException, "%s", TTF_GetError());
  }

  ObjWindow* window = AS_WINDOW(receiver);
  window->font = font;

  RETURN_NIL;
}

NATIVE_METHOD(Window, drawText) {
  assertArgCount("Window::drawText(text, x, y, color, alignX, alignY)", 6, argCount);
  assertArgIsString("Window::drawText(text, x, y, color, alignX, alignY)", args, 0);
  assertArgIsInt("Window::drawText(text, x, y, color, alignX, alignY)", args, 1);
  assertArgIsInt("Window::drawText(text, x, y, color, alignX, alignY)", args, 2);
  assertArgIsArray("Window::drawText(text, x, y, color, alignX, alignY)", args, 3);
  assertArgIsInt("Window::drawText(text, x, y, color, alignX, alignY)", args, 4);
  assertArgIsInt("Window::drawText(text, x, y, color, alignX, alignY)", args, 5);

  const char* text = AS_CSTRING(args[0]);
  int x = AS_INT(args[1]);
  int y = AS_INT(args[2]);

  SDL_Color color = { AS_INT(AS_ARRAY(args[3])->elements.values[0]), 
                      AS_INT(AS_ARRAY(args[3])->elements.values[1]), 
                      AS_INT(AS_ARRAY(args[3])->elements.values[2]), 
                      255 };

  int alignXDirection = AS_INT(args[4]);
  int alignYDirection = AS_INT(args[5]);

  ObjWindow* window = AS_WINDOW(receiver);
  if (!window->font) {
    THROW_EXCEPTION(luminique::std::graphics, RenderException, "Font not loaded.");
  }

  SDL_Renderer* renderer = SDL_GetRenderer(window->window);
  SDL_Texture* textTexture = renderTextToTexture(renderer, text, color, window->font);
  if (!textTexture) {
    RETURN_NIL;
  }

  int textWidth, textHeight;
  SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);

  SDL_Rect renderQuad;
  renderQuad.w = textWidth;
  renderQuad.h = textHeight;

  renderQuad.x = x;
  renderQuad.y = y;

  switch (alignXDirection) {
    case MIDDLE:
      renderQuad.x -= textWidth / 2;
      break;
    case LEFT:
      renderQuad.x -= textWidth;
      break;
    case RIGHT:
      // No change needed for RIGHT
      break;
    default:
      THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Unknown alignment direction.");
      break;
  }

  switch (alignYDirection) {
    case MIDDLE:
      renderQuad.y -= textHeight / 2;
      break;
    case TOP:
      renderQuad.y -= textHeight;
      break;
    case BOTTOM:
      // No change needed for BOTTOM
      break;
    default:
      THROW_EXCEPTION(luminique::std::lang, IllegalArgumentException, "Unknown alignment direction.");
      break;
  }

  SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
  SDL_DestroyTexture(textTexture);

  RETURN_NIL;
}

NATIVE_FUNCTION(createWindow) {
  assertArgCount("createWindow(title, width, height, isResizable)", 4, argCount);
  assertArgIsString("createWindow(title, width, height, isResizable)", args, 0);
  assertArgIsInt("createWindow(title, width, height, isResizable)", args, 1);
  assertArgIsInt("createWindow(title, width, height, isResizable)", args, 2);
  assertArgIsBool("createWindow(title, width, height, isResizable)", args, 3);

  const char* title = AS_CSTRING(args[0]);
  int width = AS_INT(args[1]);
  int height = AS_INT(args[2]);
  bool isResizable = AS_BOOL(args[3]);

  ObjWindow* window = newWindow(title, width, height, isResizable);

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

  ObjClass* graphicsExceptionClass = defineNativeException("GraphicsException", vm.exceptionClass);
  defineNativeException("RenderException", graphicsExceptionClass);

  vm.windowClass = defineNativeClass("Window", false);
  bindSuperclass(vm.windowClass, vm.objectClass);
  DEF_METHOD(vm.windowClass, Window, __init__, 3);
  DEF_METHOD(vm.windowClass, Window, __str__, 0);
  DEF_METHOD(vm.windowClass, Window, __format__, 0);
  DEF_METHOD(vm.windowClass, Window, show, 0);
  DEF_METHOD(vm.windowClass, Window, hide, 0);
  DEF_METHOD(vm.windowClass, Window, close, 0);
  DEF_METHOD_ASYNC(vm.windowClass, Window, pollEvent, 0);
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
  DEF_METHOD(vm.windowClass, Window, loadFont, 2);
  DEF_METHOD(vm.windowClass, Window, drawText, 6);

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

  ObjEnum* keyCodeTypeEnum = defineNativeEnum("KeyCodeType");
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "UNKNOWN", INT_VAL(SDLK_UNKNOWN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "RETURN", INT_VAL(SDLK_RETURN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "ESCAPE", INT_VAL(SDLK_ESCAPE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "BACKSPACE", INT_VAL(SDLK_BACKSPACE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "TAB", INT_VAL(SDLK_TAB));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "SPACE", INT_VAL(SDLK_SPACE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "EXCLAIM", INT_VAL(SDLK_EXCLAIM));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "QUOTEDBL", INT_VAL(SDLK_QUOTEDBL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "HASH", INT_VAL(SDLK_HASH));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "PERCENT", INT_VAL(SDLK_PERCENT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "DOLLAR", INT_VAL(SDLK_DOLLAR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AMPERSAND", INT_VAL(SDLK_AMPERSAND));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "QUOTE", INT_VAL(SDLK_QUOTE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "LEFTPAREN", INT_VAL(SDLK_LEFTPAREN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "RIGHTPAREN", INT_VAL(SDLK_RIGHTPAREN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "ASTERISK", INT_VAL(SDLK_ASTERISK));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "PLUS", INT_VAL(SDLK_PLUS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "COMMA", INT_VAL(SDLK_COMMA));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "MINUS", INT_VAL(SDLK_MINUS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "PERIOD", INT_VAL(SDLK_PERIOD));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "SLASH", INT_VAL(SDLK_SLASH));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_0", INT_VAL(SDLK_0));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_1", INT_VAL(SDLK_1));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_2", INT_VAL(SDLK_2));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_3", INT_VAL(SDLK_3));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_4", INT_VAL(SDLK_4));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_5", INT_VAL(SDLK_5));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_6", INT_VAL(SDLK_6));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_7", INT_VAL(SDLK_7));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_8", INT_VAL(SDLK_8));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUM_9", INT_VAL(SDLK_9));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "COLON", INT_VAL(SDLK_COLON));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "SEMICOLON", INT_VAL(SDLK_SEMICOLON));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "LESS", INT_VAL(SDLK_LESS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "EQUALS", INT_VAL(SDLK_EQUALS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "GREATER", INT_VAL(SDLK_GREATER));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "QUESTION", INT_VAL(SDLK_QUESTION));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AT", INT_VAL(SDLK_AT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "LEFTBRACKET", INT_VAL(SDLK_LEFTBRACKET));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "BACKSLASH", INT_VAL(SDLK_BACKSLASH));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "RIGHTBRACKET", INT_VAL(SDLK_RIGHTBRACKET));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CARET", INT_VAL(SDLK_CARET));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "UNDERSCORE", INT_VAL(SDLK_UNDERSCORE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "BACKQUOTE", INT_VAL(SDLK_BACKQUOTE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "A", INT_VAL(SDLK_a));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "B", INT_VAL(SDLK_b));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "C", INT_VAL(SDLK_c));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "D", INT_VAL(SDLK_d));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "E", INT_VAL(SDLK_e));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F", INT_VAL(SDLK_f));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "G", INT_VAL(SDLK_g));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "H", INT_VAL(SDLK_h));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "I", INT_VAL(SDLK_i));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "J", INT_VAL(SDLK_j));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "K", INT_VAL(SDLK_k));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "L", INT_VAL(SDLK_l));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "M", INT_VAL(SDLK_m));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "N", INT_VAL(SDLK_n));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "O", INT_VAL(SDLK_o));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "P", INT_VAL(SDLK_p));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "Q", INT_VAL(SDLK_q));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "R", INT_VAL(SDLK_r));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "S", INT_VAL(SDLK_s));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "T", INT_VAL(SDLK_t));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "U", INT_VAL(SDLK_u));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "V", INT_VAL(SDLK_v));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "W", INT_VAL(SDLK_w));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "X", INT_VAL(SDLK_x));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "Y", INT_VAL(SDLK_y));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "Z", INT_VAL(SDLK_z));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CAPSLOCK", INT_VAL(SDLK_CAPSLOCK));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F1", INT_VAL(SDLK_F1));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F2", INT_VAL(SDLK_F2));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F3", INT_VAL(SDLK_F3));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F4", INT_VAL(SDLK_F4));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F5", INT_VAL(SDLK_F5));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F6", INT_VAL(SDLK_F6));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F7", INT_VAL(SDLK_F7));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F8", INT_VAL(SDLK_F8));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F9", INT_VAL(SDLK_F9));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F10", INT_VAL(SDLK_F10));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F11", INT_VAL(SDLK_F11));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F12", INT_VAL(SDLK_F12));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "PRINTSCREEN", INT_VAL(SDLK_PRINTSCREEN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "SCROLLLOCK", INT_VAL(SDLK_SCROLLLOCK));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "PAUSE", INT_VAL(SDLK_PAUSE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "INSERT", INT_VAL(SDLK_INSERT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "HOME", INT_VAL(SDLK_HOME));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "PAGEUP", INT_VAL(SDLK_PAGEUP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "DELETE", INT_VAL(SDLK_DELETE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "END", INT_VAL(SDLK_END));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "PAGEDOWN", INT_VAL(SDLK_PAGEDOWN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "RIGHT", INT_VAL(SDLK_RIGHT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "LEFT", INT_VAL(SDLK_LEFT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "DOWN", INT_VAL(SDLK_DOWN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "UP", INT_VAL(SDLK_UP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "NUMLOCKCLEAR", INT_VAL(SDLK_NUMLOCKCLEAR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_DIVIDE", INT_VAL(SDLK_KP_DIVIDE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_MULTIPLY", INT_VAL(SDLK_KP_MULTIPLY));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_MINUS", INT_VAL(SDLK_KP_MINUS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_PLUS", INT_VAL(SDLK_KP_PLUS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_ENTER", INT_VAL(SDLK_KP_ENTER));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_1", INT_VAL(SDLK_KP_1));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_2", INT_VAL(SDLK_KP_2));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_3", INT_VAL(SDLK_KP_3));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_4", INT_VAL(SDLK_KP_4));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_5", INT_VAL(SDLK_KP_5));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_6", INT_VAL(SDLK_KP_6));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_7", INT_VAL(SDLK_KP_7));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_8", INT_VAL(SDLK_KP_8));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_9", INT_VAL(SDLK_KP_9));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_0", INT_VAL(SDLK_KP_0));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_PERIOD", INT_VAL(SDLK_KP_PERIOD));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "APPLICATION", INT_VAL(SDLK_APPLICATION));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "POWER", INT_VAL(SDLK_POWER));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_EQUALS", INT_VAL(SDLK_KP_EQUALS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F13", INT_VAL(SDLK_F13));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F14", INT_VAL(SDLK_F14));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F15", INT_VAL(SDLK_F15));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F16", INT_VAL(SDLK_F16));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F17", INT_VAL(SDLK_F17));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F18", INT_VAL(SDLK_F18));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F19", INT_VAL(SDLK_F19));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F20", INT_VAL(SDLK_F20));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F21", INT_VAL(SDLK_F21));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F22", INT_VAL(SDLK_F22));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F23", INT_VAL(SDLK_F23));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "F24", INT_VAL(SDLK_F24));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "EXECUTE", INT_VAL(SDLK_EXECUTE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "HELP", INT_VAL(SDLK_HELP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "MENU", INT_VAL(SDLK_MENU));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "SELECT", INT_VAL(SDLK_SELECT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "STOP", INT_VAL(SDLK_STOP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AGAIN", INT_VAL(SDLK_AGAIN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "UNDO", INT_VAL(SDLK_UNDO));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CUT", INT_VAL(SDLK_CUT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "COPY", INT_VAL(SDLK_COPY));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "PASTE", INT_VAL(SDLK_PASTE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "FIND", INT_VAL(SDLK_FIND));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "MUTE", INT_VAL(SDLK_MUTE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "VOLUMEUP", INT_VAL(SDLK_VOLUMEUP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "VOLUMEDOWN", INT_VAL(SDLK_VOLUMEDOWN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_COMMA", INT_VAL(SDLK_KP_COMMA));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_EQUALSAS400", INT_VAL(SDLK_KP_EQUALSAS400));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "ALTERASE", INT_VAL(SDLK_ALTERASE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "SYSREQ", INT_VAL(SDLK_SYSREQ));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CANCEL", INT_VAL(SDLK_CANCEL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CLEAR", INT_VAL(SDLK_CLEAR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "PRIOR", INT_VAL(SDLK_PRIOR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "RETURN2", INT_VAL(SDLK_RETURN2));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "SEPARATOR", INT_VAL(SDLK_SEPARATOR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "OUT", INT_VAL(SDLK_OUT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "OPER", INT_VAL(SDLK_OPER));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CLEARAGAIN", INT_VAL(SDLK_CLEARAGAIN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CRSEL", INT_VAL(SDLK_CRSEL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "EXSEL", INT_VAL(SDLK_EXSEL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_00", INT_VAL(SDLK_KP_00));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_000", INT_VAL(SDLK_KP_000));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "THOUSANDSSEPARATOR", INT_VAL(SDLK_THOUSANDSSEPARATOR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "DECIMALSEPARATOR", INT_VAL(SDLK_DECIMALSEPARATOR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CURRENCYUNIT", INT_VAL(SDLK_CURRENCYUNIT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CURRENCYSUBUNIT", INT_VAL(SDLK_CURRENCYSUBUNIT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_LEFTPAREN", INT_VAL(SDLK_KP_LEFTPAREN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_RIGHTPAREN", INT_VAL(SDLK_KP_RIGHTPAREN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_LEFTBRACE", INT_VAL(SDLK_KP_LEFTBRACE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_RIGHTBRACE", INT_VAL(SDLK_KP_RIGHTBRACE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_TAB", INT_VAL(SDLK_KP_TAB));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_BACKSPACE", INT_VAL(SDLK_KP_BACKSPACE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_A", INT_VAL(SDLK_KP_A));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_B", INT_VAL(SDLK_KP_B));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_C", INT_VAL(SDLK_KP_C));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_D", INT_VAL(SDLK_KP_D));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_E", INT_VAL(SDLK_KP_E));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_F", INT_VAL(SDLK_KP_F));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_XOR", INT_VAL(SDLK_KP_XOR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_POWER", INT_VAL(SDLK_KP_POWER));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_PERCENT", INT_VAL(SDLK_KP_PERCENT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_LESS", INT_VAL(SDLK_KP_LESS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_GREATER", INT_VAL(SDLK_KP_GREATER));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_AMPERSAND", INT_VAL(SDLK_KP_AMPERSAND));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_DBLAMPERSAND", INT_VAL(SDLK_KP_DBLAMPERSAND));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_VERTICALBAR", INT_VAL(SDLK_KP_VERTICALBAR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_DBLVERTICALBAR", INT_VAL(SDLK_KP_DBLVERTICALBAR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_COLON", INT_VAL(SDLK_KP_COLON));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_HASH", INT_VAL(SDLK_KP_HASH));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_SPACE", INT_VAL(SDLK_KP_SPACE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_AT", INT_VAL(SDLK_KP_AT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_EXCLAM", INT_VAL(SDLK_KP_EXCLAM));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_MEMSTORE", INT_VAL(SDLK_KP_MEMSTORE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_MEMRECALL", INT_VAL(SDLK_KP_MEMRECALL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_MEMCLEAR", INT_VAL(SDLK_KP_MEMCLEAR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_MEMADD", INT_VAL(SDLK_KP_MEMADD));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_MEMSUBTRACT", INT_VAL(SDLK_KP_MEMSUBTRACT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_MEMMULTIPLY", INT_VAL(SDLK_KP_MEMMULTIPLY));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_MEMDIVIDE", INT_VAL(SDLK_KP_MEMDIVIDE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_PLUSMINUS", INT_VAL(SDLK_KP_PLUSMINUS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_CLEAR", INT_VAL(SDLK_KP_CLEAR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_CLEARENTRY", INT_VAL(SDLK_KP_CLEARENTRY));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_BINARY", INT_VAL(SDLK_KP_BINARY));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_OCTAL", INT_VAL(SDLK_KP_OCTAL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_DECIMAL", INT_VAL(SDLK_KP_DECIMAL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KP_HEXADECIMAL", INT_VAL(SDLK_KP_HEXADECIMAL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "LCTRL", INT_VAL(SDLK_LCTRL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "LSHIFT", INT_VAL(SDLK_LSHIFT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "LALT", INT_VAL(SDLK_LALT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "LGUI", INT_VAL(SDLK_LGUI));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "RCTRL", INT_VAL(SDLK_RCTRL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "RSHIFT", INT_VAL(SDLK_RSHIFT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "RALT", INT_VAL(SDLK_RALT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "RGUI", INT_VAL(SDLK_RGUI));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "MODE", INT_VAL(SDLK_MODE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AUDIONEXT", INT_VAL(SDLK_AUDIONEXT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AUDIOPREV", INT_VAL(SDLK_AUDIOPREV));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AUDIOSTOP", INT_VAL(SDLK_AUDIOSTOP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AUDIOPLAY", INT_VAL(SDLK_AUDIOPLAY));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AUDIOMUTE", INT_VAL(SDLK_AUDIOMUTE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "MEDIASELECT", INT_VAL(SDLK_MEDIASELECT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "WWW", INT_VAL(SDLK_WWW));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "MAIL", INT_VAL(SDLK_MAIL));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "CALCULATOR", INT_VAL(SDLK_CALCULATOR));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "COMPUTER", INT_VAL(SDLK_COMPUTER));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AC_SEARCH", INT_VAL(SDLK_AC_SEARCH));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AC_HOME", INT_VAL(SDLK_AC_HOME));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AC_BACK", INT_VAL(SDLK_AC_BACK));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AC_FORWARD", INT_VAL(SDLK_AC_FORWARD));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AC_STOP", INT_VAL(SDLK_AC_STOP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AC_REFRESH", INT_VAL(SDLK_AC_REFRESH));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AC_BOOKMARKS", INT_VAL(SDLK_AC_BOOKMARKS));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "BRIGHTNESSDOWN", INT_VAL(SDLK_BRIGHTNESSDOWN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "BRIGHTNESSUP", INT_VAL(SDLK_BRIGHTNESSUP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "DISPLAYSWITCH", INT_VAL(SDLK_DISPLAYSWITCH));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KBDILLUMTOGGLE", INT_VAL(SDLK_KBDILLUMTOGGLE));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KBDILLUMDOWN", INT_VAL(SDLK_KBDILLUMDOWN));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "KBDILLUMUP", INT_VAL(SDLK_KBDILLUMUP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "EJECT", INT_VAL(SDLK_EJECT));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "SLEEP", INT_VAL(SDLK_SLEEP));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "APP1", INT_VAL(SDLK_APP1));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "APP2", INT_VAL(SDLK_APP2));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AUDIOREWIND", INT_VAL(SDLK_AUDIOREWIND));
  defineNativeArtificialEnumElement(keyCodeTypeEnum, "AUDIOFASTFORWARD", INT_VAL(SDLK_AUDIOFASTFORWARD));


  ObjEnum* alignDirectionEnum = defineNativeEnum("AlignDirection");
  defineNativeEnumElement(alignDirectionEnum, "MIDDLE");
  defineNativeEnumElement(alignDirectionEnum, "LEFT");
  defineNativeEnumElement(alignDirectionEnum, "RIGHT");
  defineNativeEnumElement(alignDirectionEnum, "TOP");
  defineNativeEnumElement(alignDirectionEnum, "BOTTOM");

  DEF_FUNCTION(createWindow, 3);

  vm.currentNamespace = vm.rootNamespace;
}
