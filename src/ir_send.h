/*
 * Copyright (c) 2022, Daniel Gorbea
 * All rights reserved.
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Library for pin capture timer for RP2040
 */

#ifndef IR_SEND
#define IR_SEND

#ifdef __cplusplus
extern "C" {
#endif

#include "ir_send.pio.h"
#include "hardware/pio.h"

void ir_send_init(PIO pio, uint pin, float clk_div);
void ir_send_push(bool carrier, uint32_t count);
void ir_send_remove(void);

#ifdef __cplusplus
}
#endif

#endif
