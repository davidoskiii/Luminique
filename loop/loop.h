#ifndef cluminique_loop_h
#define cluminique_loop_h

#include "../common.h"

#include <stdio.h>
#include <sys/stat.h>
#include <uv.h>

#include "../common.h"
#include "../value/value.h"

typedef struct {
  VM* vm;
  Value receiver;
  ObjClosure* closure;
  int delay;
  int interval;
} TimerData;

void initLoop();
void freeLoop();
void timerClose(uv_handle_t* handle);
void timerRun(uv_timer_t* timer);

#endif
