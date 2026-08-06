#!/usr/bin/env python3

import argparse
import copy
import os
import sys

import yaml
from jinja2 import Environment, FileSystemLoader, StrictUndefined


REPLACE_KEY = "__replace__"


def render_yaml(path):
    template_path = os.path.abspath(path)
    environment = Environment(
        loader=FileSystemLoader(os.path.dirname(template_path)),
        undefined=StrictUndefined,
    )
    template = environment.get_template(os.path.basename(template_path))
    rendered = template.render(**os.environ)
    return yaml.safe_load(rendered) or {}


def merge(base, override):
    if isinstance(override, dict) and override.get(REPLACE_KEY) is True:
        return {
            key: merge({}, value)
            for key, value in override.items()
            if key != REPLACE_KEY
        }

    if isinstance(base, dict) and isinstance(override, dict):
        result = copy.deepcopy(base)
        for key, value in override.items():
            if key in result:
                result[key] = merge(result[key], value)
            else:
                result[key] = copy.deepcopy(value)
        return result

    return copy.deepcopy(override)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Render constants YAML with optional downstream overlays."
    )
    parser.add_argument("template", help="Base constants Jinja template")
    parser.add_argument(
        "--overlay",
        action="append",
        default=[],
        help="Optional overlay template; may be specified multiple times",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    overlays = list(args.overlay)
    env_overlay = os.environ.get("SONIC_CONSTANTS_OVERLAY")
    if env_overlay:
        overlays.extend(path for path in env_overlay.split(os.pathsep) if path)

    constants = render_yaml(args.template)
    for overlay in overlays:
        constants = merge(constants, render_yaml(overlay))

    yaml.safe_dump(constants, sys.stdout, sort_keys=False)


if __name__ == "__main__":
    main()
