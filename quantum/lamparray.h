// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "util.h" // PACKED

// 26.2.1 LampArrayKind Values
#define LAMPARRAY_KIND_UNDEFINED 0x00
#define LAMPARRAY_KIND_KEYBOARD 0x01
#define LAMPARRAY_KIND_MOUSE 0x02
#define LAMPARRAY_KIND_GAMECONTROLLER 0x03
#define LAMPARRAY_KIND_PERIPHERAL 0x04
#define LAMPARRAY_KIND_SCENE 0x05
#define LAMPARRAY_KIND_NOTIFICATION 0x06
#define LAMPARRAY_KIND_CHASSIS 0x07
#define LAMPARRAY_KIND_WEARABLE 0x08
#define LAMPARRAY_KIND_FURNITURE 0x09
#define LAMPARRAY_KIND_ART 0x0A

// 26.3.1 LampPurposes Flags
#define LAMP_PURPOSE_CONTROL 0x01
#define LAMP_PURPOSE_ACCENT 0x02
#define LAMP_PURPOSE_BRANDING 0x04
#define LAMP_PURPOSE_STATUS 0x08
#define LAMP_PURPOSE_ILLUMINATION 0x10
#define LAMP_PURPOSE_PRESENTATION 0x20

// 26.4.1 LampUpdate Flags
#define LAMP_UPDATE_FLAG_COMPLETE 0x01

typedef struct PACKED {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t intensity;
} lamp_state_t;

typedef struct PACKED {
    uint16_t lamp_count;
    struct {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    } bounds;
    uint32_t kind;
    uint32_t update_interval;
} lamparray_attributes_t;

typedef struct PACKED {
    uint16_t lamp_id;
    struct {
        int32_t x;
        int32_t y;
        int32_t z;
    } position;
    int32_t      update_latency;
    int32_t      purposes;
    lamp_state_t levels;
    uint8_t      is_programmable;
    uint8_t      input_binding;
} lamparray_attributes_response_t;

typedef struct PACKED {
    uint8_t      flags;
    uint16_t     start;
    uint16_t     end;
    lamp_state_t color;
} lamparray_range_update_t;

#define LAMP_MULTI_UPDATE_LAMP_COUNT 8
typedef struct PACKED {
    uint8_t      count;
    uint8_t      flags;
    uint16_t     ids[LAMP_MULTI_UPDATE_LAMP_COUNT];
    lamp_state_t colors[LAMP_MULTI_UPDATE_LAMP_COUNT];
} lamparray_multi_update_t;

/**
 * \brief Gets LampArrayAttributesReport data
 */
void lamparray_get_attributes(lamparray_attributes_t* data);

/**
 * \brief Sets LampAttributesRequestReport data
 */
void lamparray_set_attributes_response(uint16_t lamp_id);

/**
 * \brief Gets LampAttributesResponseReport data
 */
void lamparray_get_attributes_response(lamparray_attributes_response_t* data);

/**
 * \brief Sets LampRangeUpdateReport data
 */
void lamparray_set_range(lamparray_range_update_t* data);

/**
 * \brief Sets LampMultiUpdateReport data
 */
void lamparray_set_items(lamparray_multi_update_t* data);

/**
 * \brief Sets LampArrayControlReport data
 */
void lamparray_set_control_response(uint8_t autonomous);
