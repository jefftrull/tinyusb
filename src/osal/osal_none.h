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

#ifndef _TUSB_OSAL_NONE_H_
#define _TUSB_OSAL_NONE_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------+
// TASK API
//--------------------------------------------------------------------+


//--------------------------------------------------------------------+
// Binary Semaphore API
//--------------------------------------------------------------------+
typedef struct
{
  volatile uint16_t count;
}osal_semaphore_def_t;

typedef osal_semaphore_def_t* osal_semaphore_t;

osal_semaphore_t osal_semaphore_create(osal_semaphore_def_t* semdef);

bool osal_semaphore_post(osal_semaphore_t sem_hdl, bool in_isr);

// TODO blocking for now
bool osal_semaphore_wait (osal_semaphore_t sem_hdl, uint32_t msec);

void osal_semaphore_reset(osal_semaphore_t sem_hdl);

//--------------------------------------------------------------------+
// MUTEX API
// Within tinyusb, mutex is never used in ISR context
//--------------------------------------------------------------------+
typedef osal_semaphore_def_t osal_mutex_def_t;
typedef osal_semaphore_t osal_mutex_t;

#if OSAL_MUTEX_REQUIRED
osal_mutex_t osal_mutex_create(osal_mutex_def_t* mdef);

bool osal_mutex_lock (osal_mutex_t mutex_hdl, uint32_t msec);

bool osal_mutex_unlock(osal_mutex_t mutex_hdl);

#else

#define osal_mutex_create(_mdef)          (NULL)
#define osal_mutex_lock(_mutex_hdl, _ms)  (true)
#define osal_mutex_unlock(_mutex_hdl)     (true)

#endif

//--------------------------------------------------------------------+
// QUEUE API
//--------------------------------------------------------------------+
#include "common/tusb_fifo.h"

typedef struct
{
  void (*interrupt_set)(bool);
  tu_fifo_t ff;
}osal_queue_def_t;

typedef osal_queue_def_t* osal_queue_t;

// _int_set is used as mutex in OS NONE (disable/enable USB ISR)
#define OSAL_QUEUE_DEF(_int_set, _name, _depth, _type)    \
  __xdata uint8_t _name##_buf[_depth*sizeof(_type)];              \
  __xdata osal_queue_def_t _name = {                              \
    .interrupt_set = _int_set,                            \
    .ff = TU_FIFO_INIT(_name##_buf, _depth, _type, false) \
  }

// lock queue by disable USB interrupt
 void _osal_q_lock(osal_queue_t qhdl);

// unlock queue
void _osal_q_unlock(osal_queue_t qhdl);

osal_queue_t osal_queue_create(osal_queue_def_t* qdef);

bool osal_queue_receive(osal_queue_t qhdl, void* data, uint32_t msec);

bool osal_queue_send(osal_queue_t qhdl, void const * data, bool in_isr);

bool osal_queue_empty(osal_queue_t qhdl);

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_OSAL_NONE_H_ */
