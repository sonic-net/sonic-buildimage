#!/bin/bash
exec python3 "$(dirname "$0")/generate_secureboot_vs_artifacts.py" "$@"
