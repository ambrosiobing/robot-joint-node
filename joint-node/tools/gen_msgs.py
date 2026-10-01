#!/usr/bin/env python3
"""One description, four outputs: the node's packer, the host's decoder, a
database file for standard tooling, and the test vectors both sides are
checked against.

    python tools/gen_msgs.py proto/messages.yaml \
           --c src/bus --py host --dbc proto/joint.dbc --vectors test/vectors.json

    python tools/gen_msgs.py proto/messages.yaml --check     # for the build

The generated files are not editable. Each carries the description's hash in
its banner, and --check regenerates everything in memory and compares, so a
hand edit fails the build rather than surviving to surprise somebody later.

The validation here is the point as much as the code generation. A layout whose
fields overlap, whose offsets leave a hole, or whose total is a length the frame
format does not allow, is rejected before a single byte is emitted.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import yaml

# CAN FD payload lengths. Above eight bytes the length jumps, so a layout is
# designed to land on one of these rather than to be as small as possible.
ALLOWED_LENGTHS = (0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64)

TYPES = {
    "i8":  dict(size=1, c="int8_t",   py="b", signed=True),
    "u8":  dict(size=1, c="uint8_t",  py="B", signed=False),
    "i16": dict(size=2, c="int16_t",  py="h", signed=True),
    "u16": dict(size=2, c="uint16_t", py="H", signed=False),
    "i32": dict(size=4, c="int32_t",  py="i", signed=True),
    "u32": dict(size=4, c="uint32_t", py="I", signed=False),
    "i64": dict(size=8, c="int64_t",  py="q", signed=True),
    "u64": dict(size=8, c="uint64_t", py="Q", signed=False),
}


def limits(t):
    info = TYPES[t]
    bits = info["size"] * 8
    if info["signed"]:
        return -(1 << (bits - 1)), (1 << (bits - 1)) - 1
    return 0, (1 << bits) - 1


def c_literal(t, value):
    """A C initialiser for a value of this type.

    The extremes go out as the stdint macros rather than as digits. A signed
    minimum written in full is not the constant it looks like: -2147483648 is
    the unary minus of 2147483648, which does not fit in an int, so a strict
    compiler is within its rights to complain or to promote it. INT32_MIN is
    the spelling that means what it says.
    """
    lo, hi = limits(t)
    name = t.upper().replace("I", "INT").replace("U", "UINT")
    if value == lo and TYPES[t]["signed"]:
        return f"{name}_MIN"
    if value == hi:
        return f"{name}_MAX"
    if not TYPES[t]["signed"] and value > 0x7FFFFFFF:
        return f"{value}u"
    return str(value)


class SpecError(Exception):
    pass


# ----------------------------------------------------------------- validation

def validate(spec):
    """Reject a layout before generating from it. Every check here exists
    because the failure it catches is silent at run time."""
    problems = []

    proto = spec["protocol"]
    id_bits, node_bits = proto["id_bits"], proto["node_id_bits"]
    fc_bits = id_bits - node_bits
    if fc_bits < 1:
        problems.append(f"id_bits {id_bits} leaves no room above node_id_bits {node_bits}")

    seen_codes, seen_names = {}, set()
    for fc in spec["function_codes"]:
        if fc["name"] in seen_names:
            problems.append(f"duplicate function code name {fc['name']}")
        seen_names.add(fc["name"])
        if fc["code"] in seen_codes:
            problems.append(f"function codes {seen_codes[fc['code']]} and {fc['name']} "
                            f"share value {fc['code']:#x}")
        seen_codes[fc["code"]] = fc["name"]
        if fc["code"] >= (1 << fc_bits):
            problems.append(f"function code {fc['name']} = {fc['code']:#x} does not fit "
                            f"in {fc_bits} bits")

    for mname, m in spec["messages"].items():
        if m["function_code"] not in seen_names:
            problems.append(f"{mname}: unknown function code {m['function_code']}")
        length = m["length"]
        if length not in ALLOWED_LENGTHS:
            problems.append(f"{mname}: length {length} is not a length the frame format "
                            f"allows. Allowed: {', '.join(map(str, ALLOWED_LENGTHS))}")

        cursor, names = 0, set()
        for f in m["fields"]:
            if f["type"] not in TYPES:
                problems.append(f"{mname}.{f['name']}: unknown type {f['type']}")
                continue
            if f["name"] in names:
                problems.append(f"{mname}: duplicate field name {f['name']}")
            names.add(f["name"])
            size = TYPES[f["type"]]["size"]
            if f["offset"] < cursor:
                problems.append(f"{mname}.{f['name']} at offset {f['offset']} overlaps the "
                                f"field before it, which ends at {cursor}")
            elif f["offset"] > cursor:
                problems.append(f"{mname}.{f['name']} at offset {f['offset']} leaves bytes "
                                f"{cursor} to {f['offset'] - 1} unaccounted for. Name them "
                                f"reserved rather than leaving a hole")
            cursor = max(cursor, f["offset"] + size)
        if cursor != length:
            problems.append(f"{mname}: fields total {cursor} bytes but length says {length}")

    if problems:
        raise SpecError("\n".join("  " + p for p in problems))


# ------------------------------------------------------------------- emitters

def banner(kind, digest, comment):
    o, c = comment
    return (f"{o} Generated from proto/messages.yaml. Do not edit.{c}\n"
            f"{o} description {digest}{c}\n"
            f"{o} Regenerate with: python tools/gen_msgs.py proto/messages.yaml{c}\n")


def emit_header(spec, digest):
    proto = spec["protocol"]
    node_bits = proto["node_id_bits"]
    out = [banner("h", digest, ("/*", " */")),
           "#ifndef JOINT_MSGS_H",
           "#define JOINT_MSGS_H",
           "",
           "#include <stdint.h>",
           "#include <stddef.h>",
           "",
           "/* The ordering of these codes IS the arbitration order: the identifier is",
           f" * function_code << {node_bits} | node_id, and a lower identifier wins. */"]
    for fc in spec["function_codes"]:
        out.append(f"#define FC_{fc['name']:<11} {fc['code']:#04x}     /* {fc['note']} */")
    out += ["",
            f"#define NODE_ID_BITS   {node_bits}",
            f"#define NODE_ID_MAX    {(1 << node_bits) - 1}",
            f"#define MSG_ID(fc, node)  (((fc) << {node_bits}) | ((node) & NODE_ID_MAX))",
            f"#define MSG_FC(id)        ((id) >> {node_bits})",
            f"#define MSG_NODE(id)      ((id) & NODE_ID_MAX)",
            ""]

    out += ["/* The node is built with arm-none-eabi-gcc and the host tests with gcc, so",
            " * one spelling of the packed attribute is enough here. */",
            "#define JOINT_PACKED __attribute__((packed))",
            ""]

    for mname, m in spec["messages"].items():
        up = mname.upper()
        width = max(len(f["name"]) for f in m["fields"]) + 1
        out += [f"/* {mname}: {m['note']} */",
                f"#define {up}_LEN  {m['length']}",
                f"#define {up}_FC   FC_{m['function_code']}",
                "",
                "typedef struct {"]
        for f in m["fields"]:
            decl = f"{TYPES[f['type']]['c']:<9} {f['name'] + ';':<{width}}"
            out.append(f"    {decl} /* {f['offset']:>2}: {f['note']} */")
        out += [f"}} {mname}_t;",
                "",
                "/* A packed mirror of the layout. Nothing packs through it: it exists so the",
                " * compiler checks the offsets this file was built from, and fails the",
                " * build if a field is added, widened or moved. */",
                "typedef struct {"]
        for f in m["fields"]:
            out.append(f"    {TYPES[f['type']]['c']:<9} {f['name']};")
        out += [f"}} JOINT_PACKED {mname}_wire_t;",
                "",
                f'_Static_assert(sizeof({mname}_wire_t) == {up}_LEN,',
                f'               "{mname} frame must stay {m["length"]} bytes");']
        for f in m["fields"]:
            out.append(f'_Static_assert(offsetof({mname}_wire_t, {f["name"]}) == {f["offset"]},'
                       f'\n               "{mname}.{f["name"]} moved off offset {f["offset"]}");')
        out += ["",
                f"void pack_{mname}(uint8_t *p, const {mname}_t *s);",
                f"void unpack_{mname}(const uint8_t *p, {mname}_t *s);",
                "/* The name of the first field that differs, or NULL. Useful in a test,",
                " * and useful in a log line that has to say what changed. */",
                f"const char *diff_{mname}(const {mname}_t *a, const {mname}_t *b);",
                ""]

    out += ["#endif /* JOINT_MSGS_H */", ""]
    return "\n".join(out)


PUT_GET = """
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
"""


def emit_source(spec, digest):
    out = [banner("c", digest, ("/*", " */")),
           '#include "joint_msgs.h"',
           "",
           "/* Fixed offsets, fixed byte order, never a structure copy. Dull on purpose:",
           " * it is the reason a frame recorded today still decodes in two years. */",
           PUT_GET]

    for mname, m in spec["messages"].items():
        out.append(f"void pack_{mname}(uint8_t *p, const {mname}_t *s)\n{{")
        for f in m["fields"]:
            out.append(f"    put_{f['type']}(p + {f['offset']:>2}, s->{f['name']});")
        out += ["}", ""]
        out.append(f"void unpack_{mname}(const uint8_t *p, {mname}_t *s)\n{{")
        for f in m["fields"]:
            out.append(f"    s->{f['name']} = get_{f['type']}(p + {f['offset']:>2});")
        out += ["}", ""]

        out.append(f"const char *diff_{mname}(const {mname}_t *a, const {mname}_t *b)\n{{")
        for f in m["fields"]:
            out.append(f'    if (a->{f["name"]} != b->{f["name"]}) return "{f["name"]}";')
        out += ["    return NULL;", "}", ""]

    return "\n".join(out)


def emit_vectors_h(spec, digest):
    """The same cases as the JSON, as C initialisers, so the C test needs no
    parser and both tests are driven from one description."""
    out = [banner("h", digest, ("/*", " */")),
           "#ifndef JOINT_VECTORS_H",
           "#define JOINT_VECTORS_H",
           "",
           '#include "joint_msgs.h"',
           ""]
    vectors = json.loads(emit_vectors(spec, digest))
    for mname, m in vectors["messages"].items():
        types = {f["name"]: f["type"] for f in spec["messages"][mname]["fields"]}
        out += [f"typedef struct {{ const char *label; {mname}_t f; }} {mname}_case_t;",
                "",
                f"static const {mname}_case_t {mname}_cases[] = {{"]
        for case in m["cases"]:
            fields = ", ".join(f".{k} = {c_literal(types[k], v)}"
                               for k, v in case["fields"].items())
            out.append(f'    {{ "{case["label"]}", {{ {fields} }} }},')
        out += ["};",
                f"#define {mname.upper()}_CASE_COUNT "
                f"(sizeof({mname}_cases) / sizeof({mname}_cases[0]))",
                ""]
    out += ["#endif /* JOINT_VECTORS_H */", ""]
    return "\n".join(out)


def emit_python(spec, digest):
    out = [banner("#", digest, ("#", "")),
           '"""The host twin of src/bus/joint_msgs.c, from the same description."""',
           "import struct",
           "",
           f"NODE_ID_BITS = {spec['protocol']['node_id_bits']}",
           f"NODE_ID_MAX = {(1 << spec['protocol']['node_id_bits']) - 1}",
           "",
           "",
           "def msg_id(fc, node):",
           "    return (fc << NODE_ID_BITS) | (node & NODE_ID_MAX)",
           "",
           "",
           "def msg_fc(mid):",
           "    return mid >> NODE_ID_BITS",
           "",
           "",
           "def msg_node(mid):",
           "    return mid & NODE_ID_MAX",
           "",
           ""]
    for fc in spec["function_codes"]:
        out.append(f"FC_{fc['name']} = {fc['code']:#04x}  # {fc['note']}")
    out.append("")

    for mname, m in spec["messages"].items():
        fmt = "<" + "".join(TYPES[f["type"]]["py"] for f in m["fields"])
        names = [f["name"] for f in m["fields"]]
        out += ["",
                f"{mname.upper()}_LEN = {m['length']}",
                f"{mname.upper()}_FC = FC_{m['function_code']}",
                f"{mname.upper()}_FMT = {fmt!r}",
                f"{mname.upper()}_FIELDS = {names!r}",
                "",
                "",
                f"def pack_{mname}(**kw):",
                f'    """Pack a {mname} frame. Every field is required: a default here would',
                f'    be a value invented by the encoder rather than chosen by the caller."""',
                f"    missing = [n for n in {mname.upper()}_FIELDS if n not in kw]",
                "    if missing:",
                f'        raise ValueError("missing fields: " + ", ".join(missing))',
                f"    return struct.pack({mname.upper()}_FMT, *[kw[n] for n in {mname.upper()}_FIELDS])",
                "",
                "",
                f"def unpack_{mname}(buf):",
                f"    if len(buf) != {mname.upper()}_LEN:",
                f'        raise ValueError(f"{mname} frame is {{len(buf)}} bytes, expected '
                f'{{{mname.upper()}_LEN}}")',
                f"    vals = struct.unpack({mname.upper()}_FMT, buf)",
                f"    return dict(zip({mname.upper()}_FIELDS, vals))",
                ""]
    return "\n".join(out)


def emit_dbc(spec, digest, node_id=1):
    """A database file for the standard tooling. Written for one node so the
    identifiers are concrete; the rule that produces them is in the comment."""
    node_bits = spec["protocol"]["node_id_bits"]
    codes = {fc["name"]: fc["code"] for fc in spec["function_codes"]}
    out = ['VERSION ""', "", "NS_ :", "", "BS_:", "", "BU_: MASTER JOINT", ""]
    comments = []
    for mname, m in spec["messages"].items():
        mid = (codes[m["function_code"]] << node_bits) | node_id
        tx = "JOINT" if m["direction"] == "node_to_master" else "MASTER"
        rx = "MASTER" if m["direction"] == "node_to_master" else "JOINT"
        out.append(f"BO_ {mid} {mname}_{node_id}: {m['length']} {tx}")
        for f in m["fields"]:
            info = TYPES[f["type"]]
            lo, hi = limits(f["type"])
            sign = "-" if info["signed"] else "+"
            start = f["offset"] * 8          # little endian: start bit is the LSB
            out.append(f' SG_ {f["name"]} : {start}|{info["size"] * 8}@1{sign}'
                       f' (1,0) [{lo}|{hi}] "{f["unit"]}" {rx}')
        out.append("")
        comments.append(f'CM_ BO_ {mid} "{m["note"]}";')
    out += comments
    out += ["",
            f'CM_ "Generated from proto/messages.yaml, description {digest}.";',
            f'CM_ "Identifier rule: function_code << {node_bits} | node_id. '
            f'This file is written for node {node_id}; '
            f'every other node repeats it with a different low {node_bits} bits.";',
            'CM_ "Lengths above eight bytes are flexible-data lengths. A tool that '
            'only knows classic frames will reject them, which is correct of it.";',
            ""]
    return "\n".join(out)


def emit_vectors(spec, digest):
    """Boundary values per field, as whole frames, so the C test and the Python
    test are checked against one source rather than against each other."""
    vectors = {"description": digest, "messages": {}}
    for mname, m in spec["messages"].items():
        cases = []

        def frame(label, pick):
            return {"label": label,
                    "fields": {f["name"]: pick(f) for f in m["fields"]}}

        cases.append(frame("all zero", lambda f: 0))
        cases.append(frame("all minimum", lambda f: limits(f["type"])[0]))
        cases.append(frame("all maximum", lambda f: limits(f["type"])[1]))
        cases.append(frame("alternating bits", lambda f: (
            limits(f["type"])[1] & int("55" * TYPES[f["type"]]["size"], 16))))
        # one frame per field at each extreme, everything else zero
        for target in m["fields"]:
            for which, idx in (("minimum", 0), ("maximum", 1)):
                cases.append(frame(
                    f"{target['name']} at {which}",
                    lambda f, t=target["name"], i=idx: limits(f["type"])[i] if f["name"] == t else 0))
        vectors["messages"][mname] = {"length": m["length"], "cases": cases}
    return json.dumps(vectors, indent=2) + "\n"


# ----------------------------------------------------------------------- main

def generate(spec_path):
    raw = Path(spec_path).read_bytes()
    digest = hashlib.sha256(raw).hexdigest()[:16]
    spec = yaml.safe_load(raw)
    validate(spec)
    return spec, digest


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("spec")
    ap.add_argument("--c", default="src/bus")
    ap.add_argument("--py", default="host")
    ap.add_argument("--dbc", default="proto/joint.dbc")
    ap.add_argument("--vectors", default="test/vectors.json")
    ap.add_argument("--node-id", type=int, default=1)
    ap.add_argument("--check", action="store_true",
                    help="regenerate in memory and compare, writing nothing")
    a = ap.parse_args(argv)

    try:
        spec, digest = generate(a.spec)
    except SpecError as e:
        print(f"{a.spec} is not a valid description:\n{e}", file=sys.stderr)
        return 2

    outputs = {
        Path(a.c) / "joint_msgs.h": emit_header(spec, digest),
        Path(a.c) / "joint_msgs.c": emit_source(spec, digest),
        Path(a.py) / "joint_msgs.py": emit_python(spec, digest),
        Path(a.dbc): emit_dbc(spec, digest, a.node_id),
        Path(a.vectors): emit_vectors(spec, digest),
        Path(a.vectors).parent / "vectors.h": emit_vectors_h(spec, digest),
    }

    if a.check:
        stale = []
        for path, text in outputs.items():
            if not path.exists():
                stale.append(f"{path} is missing")
            elif path.read_text(encoding="utf-8") != text:
                stale.append(f"{path} does not match the description")
        if stale:
            print("generated files are out of date or were edited by hand:", file=sys.stderr)
            for s in stale:
                print("  " + s, file=sys.stderr)
            print("\nrun: python tools/gen_msgs.py proto/messages.yaml", file=sys.stderr)
            return 1
        print(f"generated files match the description ({digest})")
        return 0

    for path, text in outputs.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        print(f"wrote {path}")
    print(f"description {digest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
