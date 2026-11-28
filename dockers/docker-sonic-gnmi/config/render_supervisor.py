"""Apply Make's ENABLE_DIALOUT=n change without rewriting other sections."""

import argparse
from pathlib import Path


def without_dialout(source: str) -> str:
    lines = []
    skipping = False
    sections = 0
    for line in source.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            skipping = stripped == "[program:dialout]"
            if skipping:
                sections += 1
        if not skipping:
            lines.append(line)
    if sections != 1:
        raise ValueError(f"expected exactly one dialout section, found {sections}")
    while lines and not lines[-1].strip():
        lines.pop()
    return "".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    args.destination.write_text(without_dialout(args.source.read_text()))


if __name__ == "__main__":
    main()
