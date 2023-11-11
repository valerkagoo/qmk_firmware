// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
#include "potentiometer.h"
#include "gpio.h"
#include "util.h"
#include "timer.h"
#include "analog.h"

#ifndef POTENTIOMETER_THROTTLE_MS
#    define POTENTIOMETER_THROTTLE_MS 1
#endif

#ifndef POTENTIOMETER_OUTPUT_MIN_VALUE
#    define POTENTIOMETER_OUTPUT_MIN_VALUE 0
#endif
#ifndef POTENTIOMETER_OUTPUT_MAX_VALUE
#    define POTENTIOMETER_OUTPUT_MAX_VALUE 127
#endif
#ifndef POTENTIOMETER_ADC_MIN_VALUE
#    define POTENTIOMETER_ADC_MIN_VALUE 0
#endif
#ifndef POTENTIOMETER_ADC_MAX_VALUE
#    define POTENTIOMETER_ADC_MAX_VALUE (1 << 10)
#endif

static pin_t potentiometer_pads[] = POTENTIOMETER_PINS;
#define NUM_POTENTIOMETERS (ARRAY_SIZE(potentiometer_pads))

__attribute__((weak)) bool potentiometer_update_user(uint8_t index, uint16_t value) {
    return true;
}

__attribute__((weak)) bool potentiometer_update_kb(uint8_t index, uint16_t value) {
    return potentiometer_update_user(index, value);
}

__attribute__((weak)) bool potentiometer_throttle_task(void) {
#if (POTENTIOMETER_THROTTLE_MS > 0)
    static uint32_t last_exec = 0;
    if (timer_elapsed32(last_exec) < POTENTIOMETER_THROTTLE_MS) {
        return false;
    }
    last_exec = timer_read32();
#endif
    return true;
}

__attribute__((weak)) uint16_t potentiometer_map(uint8_t index, uint16_t value) {
    (void)index;

    uint32_t a   = POTENTIOMETER_OUTPUT_MIN_VALUE;
    uint32_t b   = POTENTIOMETER_OUTPUT_MAX_VALUE;
    uint32_t min = POTENTIOMETER_ADC_MIN_VALUE;
    uint32_t max = POTENTIOMETER_ADC_MAX_VALUE;

    // Scale value to min/max using the adc range
    return ((b - a) * (value - min) / (max - min)) + a;
}

__attribute__((weak)) bool potentiometer_filter(uint8_t index, uint16_t value) {
    // ADC currently limited to max of 12 bits - init to max 16 ensures
    // we can correctly capture even a raw first sample at max adc bounds
    static bool potentiometer_state[NUM_POTENTIOMETERS] = {UINT16_MAX};

    if (value == potentiometer_state[index]) {
        return false;
    }

    potentiometer_state[index] = value;
    return true;
}

bool potentiometer_task(void) {
    if (!potentiometer_throttle_task()) {
        return false;
    }

    bool changed = false;
    for (uint8_t index = 0; index < NUM_POTENTIOMETERS; index++) {
        uint16_t raw   = analogReadPin(potentiometer_pads[index]);
        uint16_t value = potentiometer_map(index, raw);
        if (potentiometer_filter(index, value)) {
            changed |= true;

            potentiometer_update_kb(index, value);
        }
    }

    return changed;
}