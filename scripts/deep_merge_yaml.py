#!/usr/bin/env python3
"""Deep-merge YAML documents and emit the result on stdout.

Usage: deep_merge_yaml.py BASE.yml OVERLAY.yml [OVERLAY2.yml ...]

Merge semantics (each overlay applied in order on top of the accumulated result):
  * mappings are merged recursively;
  * a null overlay value deletes the corresponding key;
  * any other value (scalar or list) replaces the base value.

This lets a downstream/organization overlay carry only its deltas on top of an
upstream base file, instead of forking the whole file.
"""
import sys
import yaml


def deep_merge(base, overlay):
    if not isinstance(base, dict) or not isinstance(overlay, dict):
        return overlay
    result = dict(base)
    for key, value in overlay.items():
        if value is None:
            result.pop(key, None)
        elif isinstance(value, dict) and isinstance(result.get(key), dict):
            result[key] = deep_merge(result[key], value)
        else:
            result[key] = value
    return result


def main(argv):
    if len(argv) < 3:
        sys.stderr.write(__doc__)
        return 2
    with open(argv[1]) as stream:
        merged = yaml.safe_load(stream)
    for path in argv[2:]:
        with open(path) as stream:
            merged = deep_merge(merged, yaml.safe_load(stream))
    yaml.safe_dump(merged, sys.stdout, default_flow_style=False, sort_keys=False)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
