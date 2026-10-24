#!/usr/bin/env python3

# Copyright 2026 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Jinja2 rendering for the generated platform files."""

import os
import tempfile

import jinja2


def render(template_filepath: str, variables: dict[str, str]) -> str:
    """Return the template at `template_filepath` filled in with `variables`."""
    loader = jinja2.FileSystemLoader(os.path.dirname(template_filepath))
    # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
    j2_env = jinja2.Environment(loader=loader, keep_trailing_newline=True)
    template = j2_env.get_template(os.path.basename(template_filepath))
    # nosemgrep: python.flask.security.xss.audit.direct-use-of-jinja2.direct-use-of-jinja2
    return template.render(variables)


def write_atomically(text: str, output_filepath: str) -> None:
    """Write `text` to `output_filepath` atomically."""
    fd, tmp = tempfile.mkstemp(
        prefix=f".{os.path.basename(output_filepath)}.",
        dir=os.path.dirname(output_filepath) or ".",
    )
    try:
        with os.fdopen(fd, "w") as f:
            f.write(text)
        os.chmod(tmp, 0o644)
        os.replace(tmp, output_filepath)
    except Exception:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise
