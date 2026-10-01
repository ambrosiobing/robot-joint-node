/* Generated from proto/messages.yaml. Do not edit. */
/* description 356c7b841e583485 */
/* Regenerate with: python tools/gen_msgs.py proto/messages.yaml */

#ifndef JOINT_VECTORS_H
#define JOINT_VECTORS_H

#include "joint_msgs.h"

typedef struct { const char *label; state_t f; } state_case_t;

static const state_case_t state_cases[] = {
    { "all zero", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "all minimum", { .position_counts = INT32_MIN, .velocity_mrad_s = INT32_MIN, .effort_mnm = INT32_MIN, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "all maximum", { .position_counts = INT32_MAX, .velocity_mrad_s = INT32_MAX, .effort_mnm = INT32_MAX, .stamp_us = UINT32_MAX, .valid_flags = UINT16_MAX, .fault_flags = UINT16_MAX, .mode = UINT8_MAX, .effort_source = UINT8_MAX, .model_version = UINT8_MAX, .seq = UINT8_MAX } },
    { "alternating bits", { .position_counts = 1431655765, .velocity_mrad_s = 1431655765, .effort_mnm = 1431655765, .stamp_us = 1431655765, .valid_flags = 21845, .fault_flags = 21845, .mode = 85, .effort_source = 85, .model_version = 85, .seq = 85 } },
    { "position_counts at minimum", { .position_counts = INT32_MIN, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "position_counts at maximum", { .position_counts = INT32_MAX, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "velocity_mrad_s at minimum", { .position_counts = 0, .velocity_mrad_s = INT32_MIN, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "velocity_mrad_s at maximum", { .position_counts = 0, .velocity_mrad_s = INT32_MAX, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "effort_mnm at minimum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = INT32_MIN, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "effort_mnm at maximum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = INT32_MAX, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "stamp_us at minimum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "stamp_us at maximum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = UINT32_MAX, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "valid_flags at minimum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "valid_flags at maximum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = UINT16_MAX, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "fault_flags at minimum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "fault_flags at maximum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = UINT16_MAX, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "mode at minimum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "mode at maximum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = UINT8_MAX, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "effort_source at minimum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "effort_source at maximum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = UINT8_MAX, .model_version = 0, .seq = 0 } },
    { "model_version at minimum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "model_version at maximum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = UINT8_MAX, .seq = 0 } },
    { "seq at minimum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = 0 } },
    { "seq at maximum", { .position_counts = 0, .velocity_mrad_s = 0, .effort_mnm = 0, .stamp_us = 0, .valid_flags = 0, .fault_flags = 0, .mode = 0, .effort_source = 0, .model_version = 0, .seq = UINT8_MAX } },
};
#define STATE_CASE_COUNT (sizeof(state_cases) / sizeof(state_cases[0]))

typedef struct { const char *label; command_t f; } command_case_t;

static const command_case_t command_cases[] = {
    { "all zero", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "all minimum", { .target = INT32_MIN, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "all maximum", { .target = INT32_MAX, .valid_until_us = UINT32_MAX, .mode_req = UINT8_MAX, .expiry_policy = UINT8_MAX, .seq = UINT8_MAX, .reserved_11 = UINT8_MAX, .reserved_12 = UINT32_MAX } },
    { "alternating bits", { .target = 1431655765, .valid_until_us = 1431655765, .mode_req = 85, .expiry_policy = 85, .seq = 85, .reserved_11 = 85, .reserved_12 = 1431655765 } },
    { "target at minimum", { .target = INT32_MIN, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "target at maximum", { .target = INT32_MAX, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "valid_until_us at minimum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "valid_until_us at maximum", { .target = 0, .valid_until_us = UINT32_MAX, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "mode_req at minimum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "mode_req at maximum", { .target = 0, .valid_until_us = 0, .mode_req = UINT8_MAX, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "expiry_policy at minimum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "expiry_policy at maximum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = UINT8_MAX, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "seq at minimum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "seq at maximum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = UINT8_MAX, .reserved_11 = 0, .reserved_12 = 0 } },
    { "reserved_11 at minimum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "reserved_11 at maximum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = UINT8_MAX, .reserved_12 = 0 } },
    { "reserved_12 at minimum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = 0 } },
    { "reserved_12 at maximum", { .target = 0, .valid_until_us = 0, .mode_req = 0, .expiry_policy = 0, .seq = 0, .reserved_11 = 0, .reserved_12 = UINT32_MAX } },
};
#define COMMAND_CASE_COUNT (sizeof(command_cases) / sizeof(command_cases[0]))

#endif /* JOINT_VECTORS_H */
