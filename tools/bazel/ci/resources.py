"""Use Bazel's CPU defaults and reserve JVM/system memory before action scheduling."""

import argparse
import json
import os
from pathlib import Path

JVM_HEAP_MB = 2048


def memory_budget_mb(host_bytes, limits, heap_mb=JVM_HEAP_MB):
    available = min([host_bytes, *limits]) // 1024**2
    budget = int((available - heap_mb) * .8)
    if budget <= 0:
        raise ValueError("Runner memory must exceed the JVM heap reserve")
    return budget


def action_memory_mb(cgroup_root=Path('/sys/fs/cgroup'), proc=Path('/proc')):
    host_bytes = os.sysconf('SC_PHYS_PAGES') * os.sysconf('SC_PAGE_SIZE')
    limits = []
    # Cgroup namespaces may expose the job group directly as the mount root.
    directories = {cgroup_root}
    for line in (proc / 'self/cgroup').read_text().splitlines():
        hierarchy, controllers, group = line.split(':', 2)
        if hierarchy == '0' and not controllers:
            relative = Path(group.lstrip('/'))
            if '..' not in relative.parts:
                directory = cgroup_root / relative
                while directory != cgroup_root:
                    directories.add(directory)
                    directory = directory.parent
    for directory in directories:
        path = directory / 'memory.max'
        if path.is_file():
            value = path.read_text().strip()
            if value != 'max':
                limits.append(int(value))
    return memory_budget_mb(host_bytes, limits)


def build_options():
    # Bazel 6.4 in the P4 runtime checks and Bazel 8.5.1 both accept this flag.
    return ['--local_ram_resources=' + str(action_memory_mb())]


def configure(workspace, home):
    budget = action_memory_mb()
    lines = f'startup --host_jvm_args=-Xmx{JVM_HEAP_MB}m\nbuild --local_ram_resources={budget}\n'
    # Preserve cache settings and all existing non-resource configuration.
    for path in {workspace / '.bazelrc.user', home / '.bazelrc'}:
        current = path.read_text() if path.exists() else ''
        separator = '\n' if current and not current.endswith('\n') else ''
        path.write_text(current + separator + lines)
    print(json.dumps({'jvm_heap_mb': JVM_HEAP_MB, 'action_memory_mb': budget,
                      'cpu_policy': 'bazel_default'}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configure', action='store_true')
    args = parser.parse_args()
    if args.configure:
        configure(Path.cwd(), Path.home())
    else:
        print(build_options()[0])
