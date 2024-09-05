#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <string.h>

#include "sonus.h"
#include "../assert/assert.h"
#include "../native/native.h"
#include "../object/object.h"
#include "../value/value.h"
#include "../vm/vm.h"

NATIVE_METHOD(Sound, __init__) {
  assertArgCount("Sound::__init__(filePath)", 1, argCount);
  assertArgIsString("Sound::__init__(filePath)", args, 0);

  const char* filePath = AS_CSTRING(args[0]);
  ObjSound* objSound = newSound(filePath);

  if (!objSound) {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Failed to initialize sound from file.");
  }

  RETURN_OBJ(objSound);
}

NATIVE_METHOD(Sound, playSound) {
  assertArgCount("Sound::playSound()", 0, argCount);

  ObjSound* objSound = AS_SOUND(receiver);
  if (!objSound->sound) {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Sound not loaded.");
    RETURN_NIL;
  }

  if (objSound->channel != -1 && Mix_Playing(objSound->channel)) {
    Mix_HaltChannel(objSound->channel);
  }

  objSound->channel = Mix_PlayChannel(-1, objSound->sound, 0);
  if (objSound->channel == -1) {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Failed to play sound.");
  }

  RETURN_NIL;
}

NATIVE_METHOD(Sound, loopSound) {
  assertArgCount("Sound::loopSound(loops)", 1, argCount);
  assertArgIsInt("Sound::loopSound(loops)", args, 0);

  ObjSound* objSound = AS_SOUND(receiver);
  if (!objSound->sound) {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Sound not loaded.");
    RETURN_NIL;
  }

  if (objSound->channel != -1 && Mix_Playing(objSound->channel)) {
    Mix_HaltChannel(objSound->channel);
  }

  int loops = AS_INT(args[0]) - 1;
  objSound->channel = Mix_PlayChannel(-1, objSound->sound, loops);
  if (objSound->channel == -1) {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Failed to loop sound.");
  }

  RETURN_NIL;
}

NATIVE_METHOD(Sound, stopSound) {
  assertArgCount("Sound::stopSound()", 0, argCount);

  ObjSound* objSound = AS_SOUND(receiver);
  if (objSound->channel != -1 && Mix_Playing(objSound->channel)) {
    if (Mix_HaltChannel(objSound->channel) == -1) {
      THROW_EXCEPTION(luminique::std::sonus, AudioException, "Failed to stop sound.");
    }
    objSound->channel = -1;
  }

  RETURN_NIL;
}

NATIVE_METHOD(Sound, pauseSound) {
  assertArgCount("Sound::pauseSound()", 0, argCount);

  ObjSound* objSound = AS_SOUND(receiver);
  if (objSound->channel != -1 && Mix_Playing(objSound->channel)) {
    Mix_Pause(objSound->channel);
  } else {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Sound is not currently playing.");
  }

  RETURN_NIL;
}


NATIVE_METHOD(Sound, resumeSound) {
  assertArgCount("Sound::resumeSound()", 0, argCount);

  ObjSound* objSound = AS_SOUND(receiver);
  if (objSound->channel != -1 && Mix_Paused(objSound->channel)) {
    Mix_Resume(objSound->channel);
  } else {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Sound is not paused.");
  }

  RETURN_NIL;
}

NATIVE_METHOD(Sound, fadeInSound) {
  assertArgCount("Sound::fadeInSound(ms)", 1, argCount);
  assertArgIsInt("Sound::fadeInSound(ms)", args, 0);

  ObjSound* objSound = AS_SOUND(receiver);
  if (!objSound->sound) {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Sound not loaded.");
    RETURN_NIL;
  }

  int fadeInDuration = AS_INT(args[0]);

  if (objSound->channel != -1 && Mix_Playing(objSound->channel)) {
    Mix_HaltChannel(objSound->channel);
  }

  objSound->channel = Mix_FadeInChannel(-1, objSound->sound, 0, fadeInDuration);
  if (objSound->channel == -1) {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Failed to fade in sound.");
  }

  RETURN_NIL;
}

NATIVE_METHOD(Sound, fadeOutSound) {
  assertArgCount("Sound::fadeOutSound(ms)", 1, argCount);
  assertArgIsInt("Sound::fadeOutSound(ms)", args, 0);

  ObjSound* objSound = AS_SOUND(receiver);
  if (objSound->channel != -1 && Mix_Playing(objSound->channel)) {
    int fadeOutDuration = AS_INT(args[0]);

    if (Mix_FadeOutChannel(objSound->channel, fadeOutDuration) == -1) {
      THROW_EXCEPTION(luminique::std::sonus, AudioException, "Failed to fade out sound.");
    }

    objSound->channel = -1;
  } else {
    THROW_EXCEPTION(luminique::std::sonus, AudioException, "Sound is not currently playing.");
  }

  RETURN_NIL;
}

NATIVE_METHOD(Sound, __str__) {
  assertArgCount("Sound::__str__()", 0, argCount);
  RETURN_STRING_FMT("<sound %s | %d ms>", AS_SOUND(receiver)->path->chars, AS_SOUND(receiver)->duration);
}

NATIVE_METHOD(Sound, __format__) {
  assertArgCount("Sound::__format__()", 0, argCount);
  RETURN_STRING_FMT("<sound %s | %d ms>", AS_SOUND(receiver)->path->chars, AS_SOUND(receiver)->duration);
}

void registerSonusPackage() {
  ObjNamespace* sonusNamespace = defineNativeNamespace("sonus", vm.stdNamespace);
  vm.currentNamespace = sonusNamespace;

  ObjClass* graphicsExceptionClass = defineNativeException("AudioException", vm.exceptionClass);

  vm.soundClass = defineNativeClass("Sound", false);
  bindSuperclass(vm.soundClass, vm.objectClass);
  DEF_METHOD(vm.soundClass, Sound, __init__, 1);
  DEF_METHOD(vm.soundClass, Sound, playSound, 0);
  DEF_METHOD(vm.soundClass, Sound, loopSound, 1);
  DEF_METHOD(vm.soundClass, Sound, stopSound, 0);
  DEF_METHOD(vm.soundClass, Sound, pauseSound, 0);
  DEF_METHOD(vm.soundClass, Sound, resumeSound, 0);
  DEF_METHOD(vm.soundClass, Sound, fadeInSound, 1);
  DEF_METHOD(vm.soundClass, Sound, fadeOutSound, 1);
  DEF_METHOD(vm.soundClass, Sound, __str__, 0);
  DEF_METHOD(vm.soundClass, Sound, __format__, 0);

  vm.currentNamespace = vm.rootNamespace;
}
