/* Generated from proto/messages.yaml. Do not edit. */
/* description 356c7b841e583485 */
/* Regenerate with: python tools/gen_msgs.py proto/messages.yaml */

#ifndef JOINT_MSGS_H
#define JOINT_MSGS_H

#include <stdint.h>
#include <stddef.h>

/* The ordering of these codes IS the arbitration order: the identifier is
 * function_code << 7 | node_id, and a lower identifier wins. */
#define FC_EMERGENCY   0x01     /* a node has entered a safe state */
#define FC_COMMAND     0x02     /* master to node, every period */
#define FC_STATE       0x03     /* node to master, every period */
#define FC_DIAGNOSTIC  0x04     /* low rate, anything not needed every period */

#define NODE_ID_BITS   7
#define NODE_ID_MAX    127
#define MSG_ID(fc, node)  (((fc) << 7) | ((node) & NODE_ID_MAX))
#define MSG_FC(id)        ((id) >> 7)
#define MSG_NODE(id)      ((id) & NODE_ID_MAX)

/* The node is built with arm-none-eabi-gcc and the host tests with gcc, so
 * one spelling of the packed attribute is enough here. */
#define JOINT_PACKED __attribute__((packed))

/* state: every period, from the node */
#define STATE_LEN  24
#define STATE_FC   FC_STATE

typedef struct {
    int32_t   position_counts; /*  0: exact, and the encoder's own unit */
    int32_t   velocity_mrad_s; /*  4: milli-radians per second */
    int32_t   effort_mnm;      /*  8: milli-newton-metres, and DERIVED unless effort_source says otherwise */
    uint32_t  stamp_us;        /* 12: the low 32 bits of the node's own clock, wraps about every 71 minutes */
    uint16_t  valid_flags;     /* 16: one bit per value field above */
    uint16_t  fault_flags;     /* 18: chapter 18 assigns the meanings */
    uint8_t   mode;            /* 20: chapter 12's node state */
    uint8_t   effort_source;   /* 21: 0 DERIVED, 1 MEASURED, chapter 8 */
    uint8_t   model_version;   /* 22: which parameter set produced the effort figure */
    uint8_t   seq;             /* 23: wraps, and the master notices gaps */
} state_t;

/* A packed mirror of the layout. Nothing packs through it: it exists so the
 * compiler checks the offsets this file was built from, and fails the
 * build if a field is added, widened or moved.
 *
 * The attribute sits straight after the struct keyword, which is the
 * position the compiler documents. Placed after the closing brace it is
 * accepted in some versions and ignored with a warning in others, and a
 * silently ignored packing attribute makes every assertion below pass for
 * the wrong reason. */
typedef struct JOINT_PACKED {
    int32_t   position_counts;
    int32_t   velocity_mrad_s;
    int32_t   effort_mnm;
    uint32_t  stamp_us;
    uint16_t  valid_flags;
    uint16_t  fault_flags;
    uint8_t   mode;
    uint8_t   effort_source;
    uint8_t   model_version;
    uint8_t   seq;
} state_wire_t;

_Static_assert(sizeof(state_wire_t) == STATE_LEN,
               "state frame must stay 24 bytes");
_Static_assert(offsetof(state_wire_t, position_counts) == 0,
               "state.position_counts moved off offset 0");
_Static_assert(offsetof(state_wire_t, velocity_mrad_s) == 4,
               "state.velocity_mrad_s moved off offset 4");
_Static_assert(offsetof(state_wire_t, effort_mnm) == 8,
               "state.effort_mnm moved off offset 8");
_Static_assert(offsetof(state_wire_t, stamp_us) == 12,
               "state.stamp_us moved off offset 12");
_Static_assert(offsetof(state_wire_t, valid_flags) == 16,
               "state.valid_flags moved off offset 16");
_Static_assert(offsetof(state_wire_t, fault_flags) == 18,
               "state.fault_flags moved off offset 18");
_Static_assert(offsetof(state_wire_t, mode) == 20,
               "state.mode moved off offset 20");
_Static_assert(offsetof(state_wire_t, effort_source) == 21,
               "state.effort_source moved off offset 21");
_Static_assert(offsetof(state_wire_t, model_version) == 22,
               "state.model_version moved off offset 22");
_Static_assert(offsetof(state_wire_t, seq) == 23,
               "state.seq moved off offset 23");

void pack_state(uint8_t *p, const state_t *s);
void unpack_state(const uint8_t *p, state_t *s);
/* The name of the first field that differs, or NULL. Useful in a test,
 * and useful in a log line that has to say what changed. */
const char *diff_state(const state_t *a, const state_t *b);

/* command: every period, to the node */
#define COMMAND_LEN  16
#define COMMAND_FC   FC_COMMAND

typedef struct {
    int32_t   target;         /*  0: the setpoint, in the same unit the encoder reports */
    uint32_t  valid_until_us; /*  4: low 32 bits, the same wrap as stamp_us. A command is valid until a time, not forever */
    uint8_t   mode_req;       /*  8: the mode the master is asking for */
    uint8_t   expiry_policy;  /*  9: what the node does when this command expires. 0 is a controlled stop */
    uint8_t   seq;            /* 10: wraps. A repeat of the last sequence number is stale, not new */
    uint8_t   reserved_11;    /* 11: must be zero. Reserved rather than removed, so the length stays allowed */
    uint32_t  reserved_12;    /* 12: must be zero. Chapter 16 may claim these four bytes for a feed-forward term */
} command_t;

/* A packed mirror of the layout. Nothing packs through it: it exists so the
 * compiler checks the offsets this file was built from, and fails the
 * build if a field is added, widened or moved.
 *
 * The attribute sits straight after the struct keyword, which is the
 * position the compiler documents. Placed after the closing brace it is
 * accepted in some versions and ignored with a warning in others, and a
 * silently ignored packing attribute makes every assertion below pass for
 * the wrong reason. */
typedef struct JOINT_PACKED {
    int32_t   target;
    uint32_t  valid_until_us;
    uint8_t   mode_req;
    uint8_t   expiry_policy;
    uint8_t   seq;
    uint8_t   reserved_11;
    uint32_t  reserved_12;
} command_wire_t;

_Static_assert(sizeof(command_wire_t) == COMMAND_LEN,
               "command frame must stay 16 bytes");
_Static_assert(offsetof(command_wire_t, target) == 0,
               "command.target moved off offset 0");
_Static_assert(offsetof(command_wire_t, valid_until_us) == 4,
               "command.valid_until_us moved off offset 4");
_Static_assert(offsetof(command_wire_t, mode_req) == 8,
               "command.mode_req moved off offset 8");
_Static_assert(offsetof(command_wire_t, expiry_policy) == 9,
               "command.expiry_policy moved off offset 9");
_Static_assert(offsetof(command_wire_t, seq) == 10,
               "command.seq moved off offset 10");
_Static_assert(offsetof(command_wire_t, reserved_11) == 11,
               "command.reserved_11 moved off offset 11");
_Static_assert(offsetof(command_wire_t, reserved_12) == 12,
               "command.reserved_12 moved off offset 12");

void pack_command(uint8_t *p, const command_t *s);
void unpack_command(const uint8_t *p, command_t *s);
/* The name of the first field that differs, or NULL. Useful in a test,
 * and useful in a log line that has to say what changed. */
const char *diff_command(const command_t *a, const command_t *b);

#endif /* JOINT_MSGS_H */
