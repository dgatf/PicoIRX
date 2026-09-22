/*
 * Copyright (c) 2022, Daniel Gorbea
 * All rights reserved.
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Library for pin capture timer for RP2040
 */

#include "ir_send.h"

#include <stdio.h>

#include "hardware/irq.h"

static bool initialized_ = false;
static PIO pio_;
static uint pin_, sm_, offset_;

void ir_send_init(PIO pio, uint pin, float clk_div) {
    if (initialized_) ir_send_remove();
    pio_ = pio;
    pin_ = pin;
    sm_ = pio_claim_unused_sm(pio_, true);
    offset_ = pio_add_program(pio_, &ir_send_program);
    pio_gpio_init(pio_, pin_);
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin_, 1, true);
    pio_sm_config c = ir_send_program_get_default_config(offset_);
    sm_config_set_clkdiv(&c, clk_div);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_set_pins(&c, pin_, 1);
    pio_sm_init(pio_, sm_, offset_, &c);
    pio_sm_set_enabled(pio_, sm_, true);
    initialized_ = true;
}

void ir_send_remove(void) {
    if (!initialized_) return;
    pio_sm_set_enabled(pio_, sm_, false);
    pio_sm_clear_fifos(pio_, sm_);
    pio_remove_program(pio_, &ir_send_program, offset_);
    pio_sm_unclaim(pio_, sm_);
    initialized_ = false;
}

void ir_send_push(bool carrier, uint32_t count) {
    uint32_t word = (count << 1) | carrier;
    pio_sm_put_blocking(pio_, sm_, word);
}