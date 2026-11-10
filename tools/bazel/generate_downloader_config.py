#!/usr/bin/env python3

import argparse
import re
from pathlib import Path
from urllib.parse import urlsplit


def generate_config(versions, mirror):
    mirror_url = urlsplit(mirror)
    if mirror_url.scheme not in ("http", "https") or not mirror_url.netloc:
        raise ValueError("Bazel mirror must be an HTTP(S) URL")
    if (any(char.isspace() for char in mirror)
            or mirror_url.query or mirror_url.fragment):
        raise ValueError(
            "Bazel mirror must not contain whitespace, a query, or a fragment"
        )

    rules = []
    for line_number, line in enumerate(versions.splitlines(), 1):
        if not line.strip() or line.startswith("#"):
            continue
        url, separator, checksum = line.rpartition("==")
        parsed = urlsplit(url)
        if (not separator or not re.fullmatch(r"[0-9a-fA-F]{32}", checksum)
                or parsed.scheme not in ("http", "https") or not parsed.netloc
                or any(char.isspace() for char in url)):
            raise ValueError(
                "Invalid versions-web entry at line {}".format(line_number)
            )
        filename = parsed.path.rsplit("/", 1)[-1]
        pattern = re.escape(url.split("://", 1)[1])
        target = "{}/{}-{}".format(mirror.rstrip("/"), filename, checksum)
        target = target.replace("\\", "\\\\").replace("$", "\\$")
        rules.append("rewrite {} {}".format(pattern, target))
        # Match the web hook's upstream fallback for unpublished mirrors.
        rules.append("rewrite {} $0".format(pattern))
    return "\n".join(rules) + "\n"


def main():
    parser = argparse.ArgumentParser(
        description="Map pinned web downloads to the SONiC mirror"
    )
    parser.add_argument("--versions", type=Path, required=True)
    parser.add_argument("--mirror", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(
        generate_config(args.versions.read_text(), args.mirror)
    )


if __name__ == "__main__":
    main()
