/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_hw_timers_hal.h"

#include "common/cs_dbg.h"

#include "mgos_hal.h"

#ifndef IRAM
#define IRAM
#endif

static struct mgos_hw_timer_info s_timers[MGOS_NUM_HW_TIMERS];

IRAM void mgos_hw_timers_isr(struct mgos_hw_timer_info *ti) {
  timer_callback cb = ti->cb;
  void *cb_arg = ti->cb_arg;
  /* Release a one-shot timer before invoking the callback so it can be
   * rescheduled from within it. */
  if (!(ti->flags & MGOS_TIMER_REPEAT)) {
    mgos_hw_timers_dev_clear(ti);
    ti->cb_arg = NULL;
    ti->cb = NULL;
  }
  cb(cb_arg);
  mgos_hw_timers_dev_isr_bottom(ti);
}

IRAM mgos_timer_id
mgos_set_hw_timer(int usecs, int flags, timer_callback cb, void *cb_arg) {
  mgos_timer_id id;
  struct mgos_hw_timer_info *ti = NULL;
  mgos_ints_disable();
  for (id = 0; id < MGOS_NUM_HW_TIMERS; id++) {
    if (s_timers[id].cb == NULL) {
      ti = &s_timers[id];
      break;
    }
  }
  if (ti == NULL) {
    mgos_ints_enable();
    LOG(LL_ERROR, ("No HW timers available."));
    return MGOS_INVALID_TIMER_ID;
  }
  ti->cb = cb;
  ti->cb_arg = cb_arg;
  ti->flags = flags;
  mgos_ints_enable();
  if (!mgos_hw_timers_dev_set(ti, usecs, flags)) {
    ti->cb_arg = NULL;
    ti->cb = NULL;
    return MGOS_INVALID_TIMER_ID;
  }
  return id + 1;
}

IRAM void mgos_clear_hw_timer(mgos_timer_id id) {
  if (id < 1 || id > MGOS_NUM_HW_TIMERS) return;
  struct mgos_hw_timer_info *ti = &s_timers[id - 1];
  mgos_hw_timers_dev_clear(ti);
  ti->cb_arg = NULL;
  ti->cb = NULL;
}

enum mgos_init_result mgos_hw_timers_init(void) {
  for (int i = 0; i < MGOS_NUM_HW_TIMERS; i++) {
    s_timers[i].id = i + 1;
    if (!mgos_hw_timers_dev_init(&s_timers[i])) {
      return MGOS_INIT_TIMERS_INIT_FAILED;
    }
  }
  return MGOS_INIT_OK;
}
