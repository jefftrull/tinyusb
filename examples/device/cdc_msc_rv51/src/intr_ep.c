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
 */

/*
 * Implementation of a driver for a new interface containing only the interrupt
 * endpoint of a M5623, which I intend to use as a side channel for debugging
 */

#include "tusb.h"
#include "device/usbd_pvt.h"
#include "intr_ep.h"

/*
 * Define (member) data required by the driver class
 */

static uint8_t itf_num;

/*
 * Define (member) functions required by the driver class
 */

static void intr_ep_init(void) {}

static void intr_ep_reset(uint8_t) {
    itf_num = 0;
}

static uint16_t
intr_ep_open(uint8_t, tusb_desc_interface_t const *itf_desc, uint16_t max_len) {

    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass &&
              INTR_EP_INTERFACE_SUBCLASS == itf_desc->bInterfaceSubClass &&
              INTR_EP_INTERFACE_PROTOCOL == itf_desc->bInterfaceProtocol, 0);

    // one interface with one endpoint
    uint16_t const drv_len = sizeof(tusb_desc_interface_t) + sizeof(tusb_desc_endpoint_t);
    TU_VERIFY(max_len >= drv_len, 0);

    itf_num = itf_desc->bInterfaceNumber;
    return drv_len;
}

bool intr_ep_control_xfer_cb(uint8_t /* rhport */,
                             uint8_t /* stage */,
                             tusb_control_request_t const * /* request */) {
    return false;   // we don't support these ATM
}

bool intr_ep_xfer_cb(uint8_t /* rhport */,
                     uint8_t /* ep_addr */,
                     xfer_result_t /* result */,
                     uint32_t /* xferred_bytes */) {
    // we probably support these, but I don't know how exactly it works yet
    return false;
}


static usbd_class_driver_t const _intr_ep_driver =
{
#if CFG_TUSB_DEBUG >= 2
    .name = "RESET",
#endif
    .init             = intr_ep_init,
    .reset            = intr_ep_reset,
    .open             = intr_ep_open,
    .control_xfer_cb  = intr_ep_control_xfer_cb,
    .xfer_cb          = intr_ep_xfer_cb,
    .sof              = NULL
};

// define a strong (in the linker sense) usbd_app_driver_get_cb to replace the
// default. This is the magic that makes the main TinyUSB recognize the driver:

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return &_intr_ep_driver;
}
