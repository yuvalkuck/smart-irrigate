#!/usr/bin/env python3
"""Generate mod_types_gen.h, the table behind the iSetup JSON.

Reads flash.h (NVS keys, enum entries, bit shifts) and mod_types.yaml (titles, defaults,
ranges). The yaml is kept in sync with flash.h in place: a key or enum entry that is new in
flash.h is added with a derived title, one that is gone from flash.h is removed, and every
other entry, comment and the layout are left as they are.
"""
import argparse
import re
import sys

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq

SAFE_TEXT = re.compile(r"^[A-Za-z0-9 ._()/-]*$")
FLASH_ENUMS = ("SoftwareEnable", "HardwareEnable")
DEFAULT_RANGE = (0, 64)
NEW_SECTION = "other"


def fail(msg):
    sys.exit(f"gen_mod_types: {msg}")


def parse_flash(text):
    keys = re.findall(r"^SETUP_KEY_NAME\((\w+),\s*\w+\);", text, re.M)
    enums = {}
    for name, ctype, body in re.findall(r"enum class (\w+)\s*:\s*(\w+)\s*\{(.*?)\};", text, re.S):
        entries = {}
        for entry, value in re.findall(r"(\w+)\s*=\s*(0[xX][0-9A-Fa-f]+|\d+)", body):
            value = int(value, 0)
            if value and value & (value - 1) == 0:  # Disable (0) and Enable (all ones) are not bits
                entries[entry] = value.bit_length() - 1
        enums[name] = (int(re.search(r"\d+", ctype).group()), entries)
    return keys, enums


def humanize(name):
    if name.isupper() or "_" in name:  # WIFI_SSID -> Wifi ssid
        return name.replace("_", " ").capitalize()
    return re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", name)  # SensorSHT -> Sensor SHT


def flow(node):
    node.fa.set_flow_style()
    return node


def is_mask(sec):
    return sec.get("return", "key") == "mask"


def sync_yaml(cfg, keys, enums):
    """Add what flash.h gained and remove what it lost. Returns the list of changes."""
    changes = []
    sections = cfg["sections"]
    mask_keys = {s.get("key") for s in sections.values() if is_mask(s)}

    for sname, sec in list(sections.items()):
        props = sec["properties"]
        if is_mask(sec):
            if sec.get("enum") not in enums:
                continue  # reported by build_section: it needs a human
            entries = enums[sec["enum"]][1]
            for pname in [p for p in props if p not in entries]:
                del props[pname]
                changes.append(f"removed {sname}.{pname}")
            for pname in entries:
                if pname not in props:
                    props[pname] = flow(CommentedMap([("title", humanize(pname))]))
                    changes.append(f"added {sname}.{pname}")
        else:
            for pname in [p for p in props if p not in keys or p in mask_keys]:
                del props[pname]
                changes.append(f"removed {sname}.{pname}")
            if not props:
                del sections[sname]
                changes.append(f"removed section {sname}")

    present = {p for s in sections.values() if not is_mask(s) for p in s["properties"]}
    for key in keys:
        if key in present or key in mask_keys:
            continue
        if NEW_SECTION not in sections:
            sections[NEW_SECTION] = CommentedMap([("title", "Other"), ("properties", CommentedMap())])
        rng = flow(CommentedSeq(DEFAULT_RANGE))
        sections[NEW_SECTION]["properties"][key] = flow(
            CommentedMap([("title", humanize(key)), ("range", rng)]))
        changes.append(f"added {NEW_SECTION}.{key}")
    return changes


def text(value, where):
    if not isinstance(value, str) or not SAFE_TEXT.match(value):
        fail(f"{where}: '{value}' must be a string of letters, digits and ' ._()/-'")
    return f'"{value}"'


def prop_init(name, title, key, ptype, default, mask, rng, shift):
    key_c = f"SETUP_KEY({key})" if key else "nullptr"
    r = rng or (0, 0)
    return (f"    {{{text(name, 'property')}, {text(title, name)}, {key_c}, Type::{ptype}, "
            f"{default}, {str(mask).lower()}, {str(bool(rng)).lower()}, {r[0]}, {r[1]}, {shift}}},")


def build_section(sname, sec, keys, enums, used_keys):
    mask_mode = is_mask(sec)
    props = sec.get("properties") or {}
    if not props:
        fail(f"section '{sname}' has no properties")
    lines = []
    if mask_mode:
        if sec.get("key") not in keys:
            fail(f"section '{sname}': key '{sec.get('key')}' is not a SETUP_KEY_NAME in flash.h")
        used_keys.add(sec["key"])
        if sec.get("enum") not in enums:
            fail(f"section '{sname}': enum '{sec.get('enum')}' not found in flash.h")
        bits, entries = enums[sec["enum"]]
        default = bool(sec.get("default", True))
        for pname, p in props.items():
            lines.append(prop_init(pname, p["title"], None, "Bool",
                                   '"true"' if p.get("default", default) else '"false"',
                                   False, None, entries[pname]))
        head = f'{{{text(sname, "section")}, {text(sec["title"], sname)}, true, SETUP_KEY({sec["key"]}), {bits}, '
    else:
        for pname, p in props.items():
            if pname in used_keys:
                fail(f"'{pname}' is used twice")
            used_keys.add(pname)
            rng = p.get("range")
            if rng is not None and (len(rng) != 2 or rng[0] > rng[1]):
                fail(f"{pname}: range must be [min, max]")
            lines.append(prop_init(pname, p["title"], pname, "String", text(p.get("default", ""), pname),
                                   bool(p.get("mask", False)), rng, 0))
        head = f'{{{text(sname, "section")}, {text(sec["title"], sname)}, false, nullptr, 0, '
    arr = f"props_{sname}"
    table = f"inline constexpr Property {arr}[] = {{\n" + "\n".join(lines) + "\n};\n"
    return table, f"    {head}{arr}, {len(props)}}},"


def main():
    ap = argparse.ArgumentParser(
        description="Generate the iSetup JSON table header from flash.h and mod_types.yaml; "
                    "the yaml is synced with flash.h in place.")
    ap.add_argument("--flash-h", required=True, help="path to flash.h: NVS keys and enum bit shifts")
    ap.add_argument("--yaml", required=True, help="path to mod_types.yaml: titles, defaults, ranges (updated in place)")
    ap.add_argument("--template", required=True, help="path to the mod_types.in.h template")
    ap.add_argument("--out", required=True, help="path of the generated header")
    a = ap.parse_args()

    keys, enums = parse_flash(open(a.flash_h).read())
    for e in FLASH_ENUMS:
        if e not in enums:
            fail(f"enum {e} not found in {a.flash_h}")

    yaml = YAML()  # round trip: keeps comments and layout
    yaml.width = 4096
    cfg = yaml.load(open(a.yaml))
    if not isinstance(cfg.get("format"), int):
        fail("yaml: 'format' must be an integer")

    changes = sync_yaml(cfg, keys, enums)
    if changes:
        with open(a.yaml, "w") as f:
            yaml.dump(cfg, f)
        for change in changes:
            print(f"gen_mod_types: {a.yaml}: {change}")

    used_keys, tables, sections = set(), [], []
    for sname, sec in cfg["sections"].items():
        table, entry = build_section(sname, sec, keys, enums, used_keys)
        tables.append(table)
        sections.append(entry)

    body = "\n".join(tables) + "\ninline constexpr Section sections[] = {\n" + "\n".join(sections) + "\n};\n" \
           "inline constexpr size_t section_count = sizeof(sections) / sizeof(sections[0]);\n"
    out = open(a.template).read().replace("@FORMAT_VER@", str(cfg["format"])).replace("@TABLES@", body)
    with open(a.out, "w") as f:
        f.write(out)


if __name__ == "__main__":
    main()
