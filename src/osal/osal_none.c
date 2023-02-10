/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

// another trick to reduce code size: move always inline stuff out of header
// because sdcc doesn't inline anything
#include <stdbool.h>
#include <stdint.h>
#include "common/tusb_compiler.h"
#include "osal_none.h"

//--------------------------------------------------------------------+
// Binary Semaphore API
//--------------------------------------------------------------------+
osal_semaphore_t osal_semaphore_create(osal_semaphore_def_t* semdef)
{
  semdef->count = 0;
  return semdef;
}

bool osal_semaphore_post(osal_semaphore_t sem_hdl, bool in_isr)
{
  (void) in_isr;
  sem_hdl->count++;
  return true;
}

// TODO blocking for now
bool osal_semaphore_wait (osal_semaphore_t sem_hdl, uint32_t msec)
{
  (void) msec;

  while (sem_hdl->count == 0) { }
  sem_hdl->count--;

  return true;
}

void osal_semaphore_reset(osal_semaphore_t sem_hdl)
{
  sem_hdl->count = 0;
}

//--------------------------------------------------------------------+
// MUTEX API
// Within tinyusb, mutex is never used in ISR context
//--------------------------------------------------------------------+
#if OSAL_MUTEX_REQUIRED
// Note: multiple cores MCUs usually do provide IPC API for mutex
// or we can use std atomic function
osal_mutex_t osal_mutex_create(osal_mutex_def_t* mdef)
{
  mdef->count = 1;
  return mdef;
}

bool osal_mutex_lock (osal_mutex_t mutex_hdl, uint32_t msec)
{
  return osal_semaphore_wait(mutex_hdl, msec);
}

bool osal_mutex_unlock(osal_mutex_t mutex_hdl)
{
  return osal_semaphore_post(mutex_hdl, false);
}

#endif

//--------------------------------------------------------------------+
// QUEUE API
//--------------------------------------------------------------------+

// lock queue by disable USB interrupt
 void _osal_q_lock(osal_queue_t qhdl)
{
  // disable dcd/hcd interrupt
  qhdl->interrupt_set(false);
}

// unlock queue
 void _osal_q_unlock(osal_queue_t qhdl)
{
  // enable dcd/hcd interrupt
  qhdl->interrupt_set(true);
}

 osal_queue_t osal_queue_create(osal_queue_def_t* qdef)
{
  tu_fifo_clear(&qdef->ff);
  return (osal_queue_t) qdef;
}

 bool osal_queue_receive(osal_queue_t qhdl, void* data, uint32_t msec)
{
  (void) msec; // not used, always behave as msec = 0

  _osal_q_lock(qhdl);
  bool success = tu_fifo_read(&qhdl->ff, data);
  _osal_q_unlock(qhdl);

  return success;
}

 bool osal_queue_send(osal_queue_t qhdl, void const * data, bool in_isr)
{
  if (!in_isr) {
    _osal_q_lock(qhdl);
  }

  bool success = tu_fifo_write(&qhdl->ff, data);

  if (!in_isr) {
    _osal_q_unlock(qhdl);
  }

  TU_ASSERT(success);

  return success;
}

 bool osal_queue_empty(osal_queue_t qhdl)
{
  // Skip queue lock/unlock since this function is primarily called
  // with interrupt disabled before going into low power mode
  return tu_fifo_empty(&qhdl->ff);
}
