#!/usr/bin/env python3
"""Fix compile_commands.json for clangd — works both standalone and in PlatformIO.

PlatformIO generates compile_commands.json with bare compiler names
(e.g. 'xtensa-esp32s3-elf-gcc') without full paths. clangd needs
the full path to query the cross-compiler for built-in defines.

Usage:
  Standalone:   python3 scripts/fix_compiledb.py
  PlatformIO:   extra_scripts = post:scripts/fix_compiledb.py
                (auto-runs after every build / compiledb)
"""
import json
import os

TOOLCHAIN_BIN = os.path.expanduser(
    "~/.platformio/packages/toolchain-xtensa-esp-elf/bin"
)

COMPILER = "xtensa-esp32s3-elf-gcc"


def fix_compiledb(project_dir=None):
    """Patch bare compiler names with full toolchain paths."""
    if project_dir is None:
        project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    cc_json = os.path.join(project_dir, "compile_commands.json")
    if not os.path.exists(cc_json):
        return

    with open(cc_json, "r") as f:
        data = json.load(f)

    full_compiler = os.path.join(TOOLCHAIN_BIN, COMPILER)
    changed = 0
    for entry in data:
        cmd = entry.get("command", "")
        if cmd.startswith(COMPILER + " "):
            entry["command"] = full_compiler + cmd[len(COMPILER):]
            changed += 1

    if changed:
        with open(cc_json, "w") as f:
            json.dump(data, f, indent=2)
        print(f"[fix_compiledb] Patched {changed} entries in compile_commands.json")


# ── PlatformIO hook ──────────────────────────────────────────
try:
    Import("env")
    import atexit

    _project_dir = env.subst("$PROJECT_DIR")

    # Use atexit so it fires after ANY target (build, compiledb, etc.)
    atexit.register(fix_compiledb, _project_dir)

except NameError:
    # ── Standalone mode ──────────────────────────────────────
    if __name__ == "__main__":
        fix_compiledb()
