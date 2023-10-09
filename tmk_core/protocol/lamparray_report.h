// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "lamparray.h"

#define LAMPARRAY_REPORT_ID_ATTRIBUTES 0x01
#define LAMPARRAY_REPORT_ID_ATTRIBUTES_REQUEST 0x02
#define LAMPARRAY_REPORT_ID_ATTRIBUTES_RESPONSE 0x03
#define LAMPARRAY_REPORT_ID_MULTI_UPDATE 0x04
#define LAMPARRAY_REPORT_ID_RANGE_UPDATE 0x05
#define LAMPARRAY_REPORT_ID_CONTROL 0x06

typedef struct PACKED universal_lamparray_response_t {
    uint8_t report_id;
    union {
        struct {
            uint16_t lamp_id;
        };
        struct {
            uint8_t autonomous;
        };
        lamparray_range_update_t range_update;
        lamparray_multi_update_t multi_update;
    };
} universal_lamparray_response_t;

typedef struct PACKED lamparray_attributes_report_t {
    uint8_t                report_id;
    lamparray_attributes_t attributes;
} lamparray_attributes_report_t;

typedef struct PACKED lamparray_attributes_response_report_t {
    uint8_t                         report_id;
    lamparray_attributes_response_t attributes_response;
} lamparray_attributes_response_report_t;

static inline void lamparray_handle_set(universal_lamparray_response_t* report) {
    switch (report->report_id) {
        case LAMPARRAY_REPORT_ID_ATTRIBUTES_REQUEST:
            lamparray_set_attributes_response(report->lamp_id);
            break;
        case LAMPARRAY_REPORT_ID_RANGE_UPDATE:
            lamparray_set_range(&report->range_update);
            break;
        case LAMPARRAY_REPORT_ID_MULTI_UPDATE:
            lamparray_set_items(&report->multi_update);
            break;
        case LAMPARRAY_REPORT_ID_CONTROL:
            lamparray_set_control_response(report->autonomous);
            break;
    }
};
