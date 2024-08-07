#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loop.h"
#include "../memory/memory.h"
#include "../object/object.h"
#include "../vm/vm.h"

void initLoop() {
  vm.eventLoop = ALLOCATE_STRUCT(uv_loop_t);
  ABORT_IFNULL(vm.eventLoop, "Not enough memory to create event loop.");
  uv_loop_init(vm.eventLoop);
}

void freeLoop() {
  if (vm.eventLoop != NULL) {
    uv_loop_close(vm.eventLoop);
    free(vm.eventLoop);
  }
}

void timerClose(uv_handle_t* handle) {
  free(handle->data);
  free(handle);
}

void timerRun(uv_timer_t* timer) {
  TimerData* data = (TimerData*)timer->data;
  if (data->interval == 0) uv_close((uv_handle_t*)timer, timerClose);
  LOOP_PUSH_DATA(data);

  switch (data->closure->function->arity) {
    case 0:
      callReentrantMethod(data->receiver, OBJ_VAL(data->closure));
      break;
    case 1:
      callReentrantMethod(data->receiver, OBJ_VAL(data->closure), data->receiver);
      break;
    default:
      throwNativeException("luminique::std::lang", "IllegalArgumentException", "timer callback closure may accept only 0 or 1 argument");
  }
  pop();
  data->vm->frameCount--;
}
