#ifndef cluminique_loop_h
#define cluminique_loop_h

#include "../common.h"

#include <stdio.h>
#include <sys/stat.h>
#include <uv.h>

#include "../common.h"
#include "../value/value.h"

#define LOOP_PUSH_DATA(data) \
  do { \
    push(OBJ_VAL(data->vm->currentModule->closure)); \
    data->vm->frameCount++; \
  } while (false)

#define LOOP_POP_DATA(data) \
  do { \
    pop(); \
    data->vm->frameCount--; \
    free(data); \
  } while (false)

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
