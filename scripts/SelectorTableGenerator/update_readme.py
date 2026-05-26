#!/usr/bin/env python3
"""
Auto-update README.md with generated selector tables.

This script extracts the DEFAULT and TALP selector tables from the
SelectorTableGenerator output and replaces the tables in README.md
between the existing comment markers.
"""

import os
import shutil
import subprocess
import sys

DEFAULT_HEADER = "### DEFAULT Selectors"
TALP_HEADER = "### TALP Selectors"

DEFAULT_START_MARKER = "CAPI_DEFAULT_SELECTORS_START"
DEFAULT_END_MARKER = "CAPI_DEFAULT_SELECTORS_END"
TALP_START_MARKER = "CAPI_TALP_SELECTORS_START"
TALP_END_MARKER = "CAPI_TALP_SELECTORS_END"

PRETTIER_WARNING_EMITTED = False


def read_generator_output(source_path):
    if source_path:
        with open(source_path, "r") as f:
            return f.read()

    result = subprocess.run(
        ["./build/scripts/SelectorTableGenerator/generator"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "SelectorTableGenerator failed")
    return result.stdout


def extract_table_section(text, header):
    lines = text.splitlines()
    start_idx = None
    for i, line in enumerate(lines):
        if line.strip() == header:
            start_idx = i
            break

    if start_idx is None:
        raise ValueError(f"Missing header '{header}' in generator output")

    table_lines = []
    in_table = False
    for line in lines[start_idx + 1:]:
        if line.startswith("### "):
            break
        if line.startswith("|"):
            table_lines.append(line.rstrip())
            in_table = True
        elif in_table:
            if line.strip() == "":
                continue
            break

    if not table_lines:
        raise ValueError(f"No table found after '{header}'")

    return table_lines


def format_table_markdown(table_lines):
    prettier = shutil.which("prettier")
    if prettier:
        formatter_cmd = [prettier]
    else:
        npx = shutil.which("npx")
        if npx:
            formatter_cmd = [npx, "--yes", "prettier"]
        else:
            global PRETTIER_WARNING_EMITTED
            if not PRETTIER_WARNING_EMITTED:
                print("WARNING: prettier not found; skipping table formatting.")
                PRETTIER_WARNING_EMITTED = True
            return table_lines

    markdown = "\n".join(table_lines) + "\n"
    result = subprocess.run(
        formatter_cmd + ["--parser", "markdown", "--stdin-filepath", "README.md"],
        input=markdown,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "prettier failed")

    return result.stdout.rstrip("\n").splitlines()


def replace_between_markers(lines, start_marker, end_marker, replacement_lines):
    start_idx = None
    end_idx = None
    for i, line in enumerate(lines):
        if start_marker in line:
            start_idx = i
        if end_marker in line:
            end_idx = i
            break

    if start_idx is None or end_idx is None or end_idx <= start_idx:
        raise ValueError(f"Missing or invalid markers {start_marker}/{end_marker} in README.md")

    formatted = [l + "\n" for l in replacement_lines] + ["\n"]
    return lines[: start_idx + 1] + formatted + lines[end_idx:]


def update_readme(default_table_lines, talp_table_lines):
    with open("README.md", "r") as f:
        lines = f.readlines()

    lines = replace_between_markers(lines, DEFAULT_START_MARKER, DEFAULT_END_MARKER, default_table_lines)
    lines = replace_between_markers(lines, TALP_START_MARKER, TALP_END_MARKER, talp_table_lines)

    with open("README.md", "w") as f:
        f.writelines(lines)


def main():
    source_path = sys.argv[1] if len(sys.argv) > 1 else None
    if not source_path and not os.path.exists("build/scripts/SelectorTableGenerator/generator"):
        print("ERROR: SelectorTableGenerator not found. Build it or pass the generator output file.")
        sys.exit(1)
    if source_path and not os.path.exists(source_path):
        print(f"ERROR: Input file not found: {source_path}")
        sys.exit(1)

    output = read_generator_output(source_path)
    default_table = extract_table_section(output, DEFAULT_HEADER)
    talp_table = extract_table_section(output, TALP_HEADER)

    default_table = format_table_markdown(default_table)
    talp_table = format_table_markdown(talp_table)

    update_readme(default_table, talp_table)
    print("README.md updated successfully!")


if __name__ == "__main__":
    main()
