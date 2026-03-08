#!/usr/bin/env python3
"""Wrapper around j2cli that adds extra Jinja2 include search paths.

Usage: j2_include.py [-I path]... template [data] [-- j2-args...]

The -I paths are added to the Jinja2 search path so that
{% include "Dockerfile.common.j2" %} resolves from feature docker directories.
"""
import sys
import os
import io
import jinja2
import j2cli.cli


class MultiPathLoader(jinja2.BaseLoader):
    """Jinja2 loader that searches multiple directories."""
    def __init__(self, cwd, search_paths=None, encoding='utf-8'):
        self.search_paths = [cwd] + (search_paths or [])
        self.encoding = encoding

    def get_source(self, environment, template):
        for search_path in self.search_paths:
            filename = os.path.join(search_path, template)
            if os.path.isfile(filename):
                with io.open(filename, 'rt', encoding=self.encoding) as f:
                    contents = f.read()
                return contents, filename, lambda: False
        raise jinja2.TemplateNotFound(template)


def main():
    # Parse -I flags
    include_paths = []
    args = sys.argv[1:]
    while args and args[0] == '-I':
        args.pop(0)
        if args:
            include_paths.append(args.pop(0))

    if not include_paths:
        # No extra paths — just run j2 directly
        sys.argv = ['j2'] + args
        j2cli.cli.main()
        return

    # Monkey-patch the renderer to use MultiPathLoader
    _orig_init = j2cli.cli.Jinja2TemplateRenderer.__init__

    def _patched_init(self, cwd, allow_undefined, j2_env_params):
        j2_env_params.setdefault('loader', MultiPathLoader(cwd, include_paths))
        _orig_init(self, cwd, allow_undefined, j2_env_params)

    j2cli.cli.Jinja2TemplateRenderer.__init__ = _patched_init

    sys.argv = ['j2'] + args
    j2cli.cli.main()


if __name__ == '__main__':
    main()
