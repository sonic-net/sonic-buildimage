"""Keep the image root's build settings and selected cache/downloader options."""

from pathlib import Path
import shlex
import sys


VALUE_OPTIONS = {
    "--disk_cache",
    "--distdir",
    "--downloader_config",
    "--experimental_downloader_config",
    "--experimental_remote_downloader",
    "--remote_cache",
    "--remote_timeout",
    "--repository_cache",
}
BOOLEAN_OPTIONS = {
    "--experimental_remote_downloader_local_fallback",
    "--noexperimental_remote_downloader_local_fallback",
    "--experimental_repository_cache_hardlinks",
    "--noexperimental_repository_cache_hardlinks",
}


def configuration(workspace, home):
    lines = (workspace / "tools/bazel/common.bazelrc").read_text().splitlines()
    local_import = "try-import %workspace%/.bazelrc.user"
    if lines.count(local_import) != 1:
        raise ValueError("common.bazelrc local import changed")
    lines.remove(local_import)
    for line in lines:
        tokens = shlex.split(line, comments=True)
        if tokens and (tokens[0] in ("import", "try-import") or
                       any(token.split("=", 1)[0] == "--registry" for token in tokens)):
            raise ValueError("unexpected registry/import in common.bazelrc")

    for path in (home / ".bazelrc", workspace / ".bazelrc.user"):
        if not path.is_file():
            continue
        for line in path.read_text().splitlines():
            tokens = shlex.split(line, comments=True)
            if not tokens or tokens[0] not in ("common", "build", "test"):
                continue
            options = iter(tokens[1:])
            for token in options:
                name, separator, _ = token.partition("=")
                if name in VALUE_OPTIONS and not separator:
                    value = next(options, None)
                    if value is None or value.startswith("--"):
                        raise ValueError("missing cache option value for " + name)
                    token += "=" + value
                elif name not in VALUE_OPTIONS and name not in BOOLEAN_OPTIONS:
                    continue
                lines.append(tokens[0] + " " + shlex.quote(token))
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    Path(sys.argv[1]).write_text(configuration(Path.cwd(), Path.home()))
