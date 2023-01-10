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

#include "dcd.h"

//--------------------------------------------------------------------+
// Event API (implemented by stack)
//--------------------------------------------------------------------+

// helper to send bus signal event
void dcd_event_bus_signal (uint8_t rhport, dcd_eventid_t eid, bool in_isr)
{
  dcd_event_t event = { .rhport = rhport, .event_id = eid };
  dcd_event_handler(&event, in_isr);
}

// helper to send bus reset event
 void dcd_event_bus_reset (uint8_t rhport, tusb_speed_t speed, bool in_isr)
{
  dcd_event_t event = { .rhport = rhport, .event_id = DCD_EVENT_BUS_RESET };
  event.bus_reset.speed = speed;
  dcd_event_handler(&event, in_isr);
}

// helper to send setup received
void dcd_event_setup_received(uint8_t rhport, uint8_t const * setup, bool in_isr)
{
  dcd_event_t event = { .rhport = rhport, .event_id = DCD_EVENT_SETUP_RECEIVED };
  memcpy(&event.setup_received, setup, 8);

  dcd_event_handler(&event, in_isr);
}

// helper to send transfer complete event
void dcd_event_xfer_complete (uint8_t rhport, uint8_t ep_addr, uint32_t xferred_bytes, uint8_t result, bool in_isr)
{
  dcd_event_t event = { .rhport = rhport, .event_id = DCD_EVENT_XFER_COMPLETE };

  event.xfer_complete.ep_addr = ep_addr;
  event.xfer_complete.len     = xferred_bytes;
  event.xfer_complete.result  = result;

  dcd_event_handler(&event, in_isr);
}

void dcd_event_sof(uint8_t rhport, uint32_t frame_count, bool in_isr)
{
  dcd_event_t event = { .rhport = rhport, .event_id = DCD_EVENT_SOF };
  event.sof.frame_count = frame_count;
  dcd_event_handler(&event, in_isr);
}

