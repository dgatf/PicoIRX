/*
 * -------------------------------------------------------------------------------
 *
 * Copyright (c) 2022, Daniel Gorbea
 * All rights reserved.
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * -------------------------------------------------------------------------------
 *
 *  A pio program to capture signal edges on any pins of the RP2040:
 *
 *  Base pin is 0. CAPTURE_EDGE_pin_rx_count is 2 -> Pins 0 & 1 can be captured
 *
 *  Set the number of pins to capture in capture_edge.pio with CAPTURE_EDGE_pin_rx_count
 *
 *  Connect a signal to pins 0 and/or 1 and check output at 115200
 *
 * -------------------------------------------------------------------------------
 */

#include <stdio.h>

#include "capture_edge.h"
#include "hardware/clocks.h"
#include "ir_send.h"
#include "pico/stdlib.h"

#define MAX_PULSES 200
#define CAPTURE_TIMEOUT_US 100000L

typedef enum pulse_state_t { PULSE_LOW, PULSE_HIGH } pulse_state_t;

typedef struct pulse_t {
    volatile bool state;
    volatile uint cycles;
    volatile float duration;
} pulse_t;

typedef struct command_t {
    uint count;
    pulse_t pulses[MAX_PULSES];
} command_t;

float clk_div = 1.0f;
volatile uint capture_counter = 0;
command_t ir_command = {0};

volatile bool is_captured = false;
volatile edge_type_t edge_type = EDGE_NONE;
volatile alarm_id_t timeout_alarm_id = 0;

static int64_t timeout_callback(alarm_id_t id, void *parameters) {
    ir_command.count = capture_counter;
    is_captured = true;
    return 0;
}

static void capture_pin_0_handler(uint counter, edge_type_t edge) {
    if (timeout_alarm_id) cancel_alarm(timeout_alarm_id);
    if (capture_counter >= MAX_PULSES) {
        is_captured = true;
        ir_command.count = capture_counter;
        return;
    }
    const float tick_seconds = (float)CAPTURE_COUNTER_CYCLES / (float)clock_get_hz(clk_sys);
    static uint counter_prev = 0;
    ir_command.pulses[capture_counter].cycles = counter - counter_prev;
    ir_command.pulses[capture_counter].duration = (float)ir_command.pulses[capture_counter].cycles * tick_seconds;
    if (edge == EDGE_RISING) {
        ir_command.pulses[capture_counter].state = PULSE_LOW;
    } else if (edge == EDGE_FALLING) {
        ir_command.pulses[capture_counter].state = PULSE_HIGH;
    }
    counter_prev = counter;
    capture_counter++;
    timeout_alarm_id = add_alarm_in_us(CAPTURE_TIMEOUT_US, timeout_callback, NULL, true);
}

static void ir_send_command() {
    for (uint i = 1; i < ir_command.count; i++) {
        bool carrier = ir_command.pulses[i].state == PULSE_LOW;
        uint32_t periods =
            (uint32_t)(ir_command.pulses[i].duration * (float)clock_get_hz(clk_sys) / (float)IR_SEND_COUNTER_CYCLES);
        // ir_send_push(carrier, periods);
    }
}

int main() {
    PIO pio = pio0;
    uint pin_rx = 0;
    uint pin_tx = 1;
    uint pin_rx_count = 1;
    uint irq = PIO0_IRQ_0;

    stdio_init_all();

    capture_edge_init(pio, pin_rx, pin_rx_count, clk_div, irq);
    capture_edge_set_handler(0, capture_pin_0_handler);

    ir_send_init(pio, pin_tx, clk_div);

    while (true) {
        if (is_captured) {
            is_captured = false;
            sleep_ms(500);
            ir_send_command();
            printf("\nPulses %d", capture_counter - 1);
            for (uint i = 1; i < capture_counter; i++) {
                ir_command.pulses[i].cycles = 0;
                ir_command.pulses[i].duration = 0.0f;
                ir_command.pulses[i].state = PULSE_LOW;
            }
            capture_counter = 0;
        }
        sleep_ms(100);
    }
}