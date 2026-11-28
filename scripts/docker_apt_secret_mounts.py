"""Add build-only APT configuration to Dockerfile RUN instructions."""

from pathlib import Path
import re
import shlex
import sys


MOUNTS = (
    " --mount=type=secret,id=sonic_apt_proxy,target=/etc/apt/apt.conf.d/zzzz-sonic-native-proxy,mode=0444,required=true"
    " --mount=type=secret,id=sonic_apt_ca,target=/run/secrets/sonic_apt_ca,mode=0444,required=true"
)
MARKER = "# SONIC_DOCKER_APT_PROXY=1"
INSTRUCTION = re.compile(r"^[ \t]*([a-z]+)(?=[ \t])", re.I)
# Preserve quoting while identifying Dockerfile heredoc words. A quoted
# shell argument such as 'echo <<END' does not introduce a heredoc.
WORD = re.compile(r'''(?:\\.|'[^']*'|"[^"]*"|[^\s\\'"]+)+''')
HEREDOC = re.compile(r"\d*<<(-?)([^<]+)")


def configure_apt_mounts(dockerfile, enabled):
    lines = dockerfile.splitlines(keepends=True)
    marked = MARKER in dockerfile.splitlines()
    escape = "\\"
    i = 0
    while i < len(lines):
        line = lines[i]
        i += 1
        if not enabled and line.rstrip("\r\n") == MARKER:
            lines[i - 1] = ""
            continue
        directive = re.fullmatch(r"[ \t]*#[ \t]*escape[ \t]*=[ \t]*([\\\x60])", line.rstrip(), re.I)
        if directive:
            escape = directive.group(1)
        instruction = INSTRUCTION.match(line)
        if not instruction:
            continue
        name = instruction.group(1).upper()
        if name == "RUN":
            end = instruction.end()
            mounted = line.startswith(MOUNTS, end)
            if enabled and not mounted:
                lines[i - 1] = line[:end] + MOUNTS + line[end:]
                if not marked:
                    lines[i - 1] = MARKER + "\n" + lines[i - 1]
                    marked = True
            elif not enabled and mounted:
                lines[i - 1] = line[:end] + line[end + len(MOUNTS):]

        # Only the first physical line of a Dockerfile instruction can
        # introduce RUN. Preserve continuation lines and heredoc data as-is.
        command = line.rstrip()
        while command.endswith(escape) and not command.endswith(escape * 2) and i < len(lines):
            next_line = lines[i].strip()
            i += 1
            if next_line and not next_line.startswith("#"):
                command = command[:-1] + next_line
        if name not in {"RUN", "COPY", "ADD", "ONBUILD"} or "<<" not in command:
            continue
        for word in WORD.findall(command):
            heredoc = HEREDOC.fullmatch(word)
            if heredoc:
                delimiter = "".join(shlex.split(heredoc.group(2)))
                while i < len(lines):
                    value = lines[i].rstrip("\r\n")
                    i += 1
                    if (value.lstrip("\t") if heredoc.group(1) else value) == delimiter:
                        break
                else:
                    raise ValueError("unterminated Dockerfile heredoc " + delimiter)
    return "".join(lines)


if __name__ == "__main__":
    path = Path(sys.argv[1])
    before = path.read_text()
    after = configure_apt_mounts(before, sys.argv[2] == "1")
    if before != after:
        path.write_text(after)
