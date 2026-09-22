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
static uint pin_, irq_, sm_, offset_;

void ir_send_init(PIO pio, uint pin, float clk_div, uint irq) {
    if (initialized_) {
        ir_send_remove();
    }
    pio_ = pio;
    irq_ = irq;
    pin_ = pin;

    sm_ = pio_claim_unused_sm(pio_, true);
    offset_ = pio_add_program(pio_, &ir_send_program);
    pio_sm_set_consecutive_pindirs(pio_, sm_, pin_, 1, false);
    pio_sm_config c = ir_send_program_get_default_config(offset_);
    sm_config_set_clkdiv(&c, clk_div);
    sm_config_set_in_pins(&c, pin_);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    /*if (irq_ == PIO0_IRQ_0 || irq_ == PIO1_IRQ_0) {
        pio_set_irq0_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + IR_SEND_IRQ_NUM), true);
    } else {
        pio_set_irq1_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + IR_SEND_IRQ_NUM), true);
    }
    pio_interrupt_clear(pio_, IR_SEND_IRQ_NUM);*/
    pio_sm_init(pio_, sm_, offset_ + offset_, &c);
    //irq_set_exclusive_handler(irq_, handler_pio);
    //irq_set_enabled(irq_, true);
    pio_sm_set_enabled(pio_, sm_, true);
    initialized_ = true;
}

void ir_send_remove(void) {
    if (!initialized_) {
        return;
    }

    // Disable IRQ generation from PIO
    if (irq_ == PIO0_IRQ_0 || irq_ == PIO1_IRQ_0) {
        pio_set_irq0_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + IR_SEND_IRQ_NUM), false);
    } else {
        pio_set_irq1_source_enabled(pio_, (enum pio_interrupt_source)(pis_interrupt0 + IR_SEND_IRQ_NUM), false);
    }

    //irq_set_enabled(irq_, false);
    //irq_remove_handler(irq_, handler_pio);
    //pio_interrupt_clear(pio_, CAPTURE_EDGE_IRQ_NUM);

    // Stop state machine
    pio_sm_set_enabled(pio_, sm_, false);
    pio_sm_clear_fifos(pio_, sm_);
    pio_remove_program(pio_, &ir_send_program, offset_);
    pio_sm_unclaim(pio_, sm_);
    
    initialized_ = false;
}
