/* Generated from proto/messages.yaml. Do not edit. */
/* description 356c7b841e583485 */
/* Regenerate with: python tools/gen_msgs.py proto/messages.yaml */

#include "joint_msgs.h"

/* Fixed offsets, fixed byte order, never a structure copy. Dull on purpose:
 * it is the reason a frame recorded today still decodes in two years. */

static void put_u8 (uint8_t *p, uint8_t  v) { p[0] = v; }
static void put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v); p[1] = (uint8_t)(v >> 8); }
static void put_u32(uint8_t *p, uint32_t v) { put_u16(p, (uint16_t)v); put_u16(p + 2, (uint16_t)(v >> 16)); }
static void put_u64(uint8_t *p, uint64_t v) { put_u32(p, (uint32_t)v); put_u32(p + 4, (uint32_t)(v >> 32)); }
static void put_i8 (uint8_t *p, int8_t  v) { put_u8 (p, (uint8_t) v); }
static void put_i16(uint8_t *p, int16_t v) { put_u16(p, (uint16_t)v); }
static void put_i32(uint8_t *p, int32_t v) { put_u32(p, (uint32_t)v); }
static void put_i64(uint8_t *p, int64_t v) { put_u64(p, (uint64_t)v); }

static uint8_t  get_u8 (const uint8_t *p) { return p[0]; }
static uint16_t get_u16(const uint8_t *p) { return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8); }
static uint32_t get_u32(const uint8_t *p) { return (uint32_t)get_u16(p) | ((uint32_t)get_u16(p + 2) << 16); }
static uint64_t get_u64(const uint8_t *p) { return (uint64_t)get_u32(p) | ((uint64_t)get_u32(p + 4) << 32); }
static int8_t   get_i8 (const uint8_t *p) { return (int8_t)  get_u8 (p); }
static int16_t  get_i16(const uint8_t *p) { return (int16_t) get_u16(p); }
static int32_t  get_i32(const uint8_t *p) { return (int32_t) get_u32(p); }
static int64_t  get_i64(const uint8_t *p) { return (int64_t) get_u64(p); }

void pack_state(uint8_t *p, const state_t *s)
{
    put_i32(p +  0, s->position_counts);
    put_i32(p +  4, s->velocity_mrad_s);
    put_i32(p +  8, s->effort_mnm);
    put_u32(p + 12, s->stamp_us);
    put_u16(p + 16, s->valid_flags);
    put_u16(p + 18, s->fault_flags);
    put_u8(p + 20, s->mode);
    put_u8(p + 21, s->effort_source);
    put_u8(p + 22, s->model_version);
    put_u8(p + 23, s->seq);
}

void unpack_state(const uint8_t *p, state_t *s)
{
    s->position_counts = get_i32(p +  0);
    s->velocity_mrad_s = get_i32(p +  4);
    s->effort_mnm = get_i32(p +  8);
    s->stamp_us = get_u32(p + 12);
    s->valid_flags = get_u16(p + 16);
    s->fault_flags = get_u16(p + 18);
    s->mode = get_u8(p + 20);
    s->effort_source = get_u8(p + 21);
    s->model_version = get_u8(p + 22);
    s->seq = get_u8(p + 23);
}

const char *diff_state(const state_t *a, const state_t *b)
{
    if (a->position_counts != b->position_counts) return "position_counts";
    if (a->velocity_mrad_s != b->velocity_mrad_s) return "velocity_mrad_s";
    if (a->effort_mnm != b->effort_mnm) return "effort_mnm";
    if (a->stamp_us != b->stamp_us) return "stamp_us";
    if (a->valid_flags != b->valid_flags) return "valid_flags";
    if (a->fault_flags != b->fault_flags) return "fault_flags";
    if (a->mode != b->mode) return "mode";
    if (a->effort_source != b->effort_source) return "effort_source";
    if (a->model_version != b->model_version) return "model_version";
    if (a->seq != b->seq) return "seq";
    return NULL;
}

void pack_command(uint8_t *p, const command_t *s)
{
    put_i32(p +  0, s->target);
    put_u32(p +  4, s->valid_until_us);
    put_u8(p +  8, s->mode_req);
    put_u8(p +  9, s->expiry_policy);
    put_u8(p + 10, s->seq);
    put_u8(p + 11, s->reserved_11);
    put_u32(p + 12, s->reserved_12);
}

void unpack_command(const uint8_t *p, command_t *s)
{
    s->target = get_i32(p +  0);
    s->valid_until_us = get_u32(p +  4);
    s->mode_req = get_u8(p +  8);
    s->expiry_policy = get_u8(p +  9);
    s->seq = get_u8(p + 10);
    s->reserved_11 = get_u8(p + 11);
    s->reserved_12 = get_u32(p + 12);
}

const char *diff_command(const command_t *a, const command_t *b)
{
    if (a->target != b->target) return "target";
    if (a->valid_until_us != b->valid_until_us) return "valid_until_us";
    if (a->mode_req != b->mode_req) return "mode_req";
    if (a->expiry_policy != b->expiry_policy) return "expiry_policy";
    if (a->seq != b->seq) return "seq";
    if (a->reserved_11 != b->reserved_11) return "reserved_11";
    if (a->reserved_12 != b->reserved_12) return "reserved_12";
    return NULL;
}
