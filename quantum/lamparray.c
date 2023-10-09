// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
#include "lamparray.h"
#include "keycodes.h"
#include "keymap_introspection.h"
#include "action_layer.h"

// Defaults are generated from info.json layout content
#ifndef LAMPARRAY_WIDTH
#    define LAMPARRAY_WIDTH ESTIMATED_KEYBOARD_WIDTH
#endif
#ifndef LAMPARRAY_HEIGHT
#    define LAMPARRAY_HEIGHT ESTIMATED_KEYBOARD_HEIGHT
#endif
#ifndef LAMPARRAY_DEPTH
#    define LAMPARRAY_DEPTH 30000
#endif
#ifndef LAMPARRAY_KIND
#    define LAMPARRAY_KIND LAMPARRAY_KIND_KEYBOARD
#endif

// TODO: Implement backing API
void lamparray_backing_enable(bool enable);
void lamparray_backing_set_item(uint16_t index, lamp_state_t color);
void lamparray_backing_update_finished(void);

/**
 * \brief Query a HID usage for a given location
 *
 * This can be requested while the user is changing layers. This is mitigated somewhat by assuming the default layer changes less frequently.
 * This is currently accepted as a limitation as there is no method to invalidate the hosts view of the data.
 */
static inline uint16_t binding_at_keymap_location(uint8_t row, uint8_t col) {
    uint16_t keycode = keycode_at_keymap_location(get_highest_layer(default_layer_state), row, col);
#if LAMPARRAY_KIND == LAMPARRAY_KIND_KEYBOARD
    // Basic QMK keycodes currently map directly to Keyboard UsagePage so safe to return without added indirection
    // Mousekeys are ignored due to values overlap Keyboard UsagePage
    if (IS_BASIC_KEYCODE(keycode) || IS_MODIFIER_KEYCODE(keycode)) {
        return keycode;
    }
#elif LAMPARRAY_KIND == LAMPARRAY_KIND_MOUSE
    // Usages from the Button UsagePage (0x09) in the range of Button1 (0x01) to Button5 (0x05) inclusive
    if ((code) >= KC_MS_BTN1 && (code) <= KC_MS_BTN5) {
        return keycode - KC_MS_BTN1;
    }
#endif
    return 0;
}

#ifdef RGB_MATRIX_ENABLE
#    include "rgb_matrix.h"

#    define LAMPARRAY_RED_LEVELS 255
#    define LAMPARRAY_GREEN_LEVELS 255
#    define LAMPARRAY_BLUE_LEVELS 255
#    define LAMPARRAY_INTENSITY_LEVELS 1
#    define LAMPARRAY_LAMP_COUNT RGB_MATRIX_LED_COUNT
#    define LAMPARRAY_UPDATE_INTERVAL (RGB_MATRIX_LED_FLUSH_LIMIT * 1000)

/**
 * \brief Get feature specific lamp info.
 *
 * Scales the LED config with the assumed RGB Matrix dimensions (224x64), for simplicity, as a completely flat device.
 * Assumes all keys are either on the top or bottom of the resulting rectangle.
 */
__attribute__((weak)) void lamparray_get_lamp_info_data(uint16_t lamp_id, lamparray_attributes_response_t* data) {
    data->position.x = (LAMPARRAY_WIDTH / 224) * g_led_config.point[lamp_id].x;
    data->position.y = (LAMPARRAY_HEIGHT / 64) * (64 - g_led_config.point[lamp_id].y);
    data->position.z = (g_led_config.flags[lamp_id] & LED_FLAG_UNDERGLOW) ? LAMPARRAY_DEPTH : 0;
    data->purposes   = (g_led_config.flags[lamp_id] & LED_FLAG_UNDERGLOW) ? LAMP_PURPOSE_ACCENT : LAMP_PURPOSE_CONTROL;
}

/**
 * \brief Query a HID usage for a given lamp
 */
__attribute__((weak)) uint8_t lamparray_get_lamp_binding(uint16_t lamp_id) {
    for (uint8_t i_row = 0; i_row < MATRIX_ROWS; i_row++) {
        for (uint8_t i_col = 0; i_col < MATRIX_COLS; i_col++) {
            if (g_led_config.matrix_co[i_row][i_col] == lamp_id) {
                return binding_at_keymap_location(i_row, i_col);
            }
        }
    }
    return 0;
}

// TODO: temporay binding of storage and render
#    include "rgb_matrix/overlay.c"

void lamparray_backing_enable(bool enable) {
    rgb_matrix_overlay_enable(enable);
}

void lamparray_backing_set_item(uint16_t index, lamp_state_t color) {
    rgb_matrix_overlay_set(index, (rgba_t){color.red, color.green, color.blue, color.intensity ? 0 : 0xFF});
}

void lamparray_backing_update_finished(void) {
    rgb_matrix_overlay_flush();
}
#endif

static uint16_t cur_lamp_id   = 0;
static bool     is_autonomous = true;

void lamparray_get_attributes(lamparray_attributes_t* data) {
    data->lamp_count      = LAMPARRAY_LAMP_COUNT;
    data->update_interval = LAMPARRAY_UPDATE_INTERVAL;
    data->kind            = LAMPARRAY_KIND;
    data->bounds.width    = LAMPARRAY_WIDTH;
    data->bounds.height   = LAMPARRAY_HEIGHT;
    data->bounds.depth    = LAMPARRAY_DEPTH;
}

void lamparray_get_lamp_info(uint16_t lamp_id, lamparray_attributes_response_t* data) {
    data->lamp_id         = lamp_id;
    data->update_latency  = 1000;
    data->is_programmable = 1;
    data->input_binding   = lamparray_get_lamp_binding(lamp_id);

    data->levels.red       = LAMPARRAY_RED_LEVELS;
    data->levels.green     = LAMPARRAY_GREEN_LEVELS;
    data->levels.blue      = LAMPARRAY_BLUE_LEVELS;
    data->levels.intensity = LAMPARRAY_INTENSITY_LEVELS;

    lamparray_get_lamp_info_data(lamp_id, data);
}

void lamparray_get_attributes_response(lamparray_attributes_response_t* data) {
    lamparray_get_lamp_info(cur_lamp_id, data);

    // Automatic address pointer incrementing - 26.8.1 LampAttributesRequestReport
    cur_lamp_id++;
    if (cur_lamp_id >= LAMPARRAY_LAMP_COUNT) cur_lamp_id = 0;
}

void lamparray_set_attributes_response(uint16_t lamp_id) {
    cur_lamp_id = lamp_id;
}

void lamparray_set_control_response(uint8_t autonomous) {
    is_autonomous = !!autonomous;

    lamparray_backing_enable(!autonomous);
}

void lamparray_set_range(lamparray_range_update_t* data) {
    // Any Lamp*UpdateReports can be ignored - 26.10.1 AutonomousMode
    if (is_autonomous) {
        return;
    }

    // Ensure IDs are within bounds
    if ((data->start >= LAMPARRAY_LAMP_COUNT) || (data->end >= LAMPARRAY_LAMP_COUNT)) {
        return;
    }

    for (uint16_t index = data->start; index <= data->end; index++) {
        lamparray_backing_set_item(index, data->color);
    }

    // Batch update complete - 26.11 Updating Lamp State
    if (data->flags & LAMP_UPDATE_FLAG_COMPLETE) {
        lamparray_backing_update_finished();
    }
}

void lamparray_set_items(lamparray_multi_update_t* data) {
    // Any Lamp*UpdateReports can be ignored - 26.10.1 AutonomousMode
    if (is_autonomous) {
        return;
    }

    // Ensure data is within bounds
    if (data->count > LAMP_MULTI_UPDATE_LAMP_COUNT) {
        return;
    }

    for (uint8_t i = 0; i < data->count; i++) {
        // Ensure IDs are within bounds
        if (data->ids[i] >= LAMPARRAY_LAMP_COUNT) {
            continue;
        }
        lamparray_backing_set_item(data->ids[i], data->colors[i]);
    }

    // Batch update complete - 26.11 Updating Lamp State
    if (data->flags & LAMP_UPDATE_FLAG_COMPLETE) {
        lamparray_backing_update_finished();
    }
}
