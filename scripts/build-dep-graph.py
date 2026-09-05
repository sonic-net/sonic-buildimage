#!/usr/bin/env python3
"""build-dep-graph.py — Parse SONiC build dependency graph from rules/*.mk files.

Usage: python3 scripts/build-dep-graph.py [sonic-buildimage-dir]

Outputs:
  - Critical path analysis
  - Fan-out/fan-in stats (bottleneck identification)
  - DOT graph file for visualization
  - JSON adjacency list
"""

import os
import re
import sys
import json
from collections import defaultdict
from pathlib import Path


def parse_rules(rules_dir: str) -> dict:
    """Parse all .mk files and extract dependency relationships."""
    deps = defaultdict(set)       # pkg -> set of packages it depends on
    rdeps = defaultdict(set)      # pkg -> set of packages that depend on it
    after = defaultdict(set)      # pkg -> set of packages it must build after
    all_packages = set()

    dep_patterns = [
        (r'\$\((\w+)\)_DEPENDS\s*[\?\+:]?=\s*(.*)', 'depends'),
        (r'\$\((\w+)\)_DEBS_DEPENDS\s*[\?\+:]?=\s*(.*)', 'depends'),
        (r'\$\((\w+)\)_WHEEL_DEPENDS\s*[\?\+:]?=\s*(.*)', 'depends'),
        (r'\$\((\w+)\)_AFTER\s*[\?\+:]?=\s*(.*)', 'after'),
        (r'\$\((\w+)\)_RDEPENDS\s*[\?\+:]?=\s*(.*)', 'rdeps'),
    ]

    pkg_var_pattern = re.compile(r'\$\((\w+)\)')

    for mk_file in sorted(Path(rules_dir).glob('*.mk')):
        content = mk_file.read_text(errors='replace')
        # Join continuation lines
        content = re.sub(r'\\\n\s*', ' ', content)

        for line in content.splitlines():
            line = line.strip()
            if line.startswith('#'):
                continue

            for pattern, dep_type in dep_patterns:
                m = re.match(pattern, line)
                if not m:
                    continue
                pkg_name = m.group(1)
                dep_str = m.group(2).strip()

                all_packages.add(pkg_name)

                # Extract variable references
                for dep_var in pkg_var_pattern.findall(dep_str):
                    if dep_var.endswith('_DBG'):
                        continue  # Skip debug variants
                    all_packages.add(dep_var)
                    if dep_type == 'depends':
                        deps[pkg_name].add(dep_var)
                        rdeps[dep_var].add(pkg_name)
                    elif dep_type == 'after':
                        after[pkg_name].add(dep_var)
                    elif dep_type == 'rdeps':
                        rdeps[pkg_name].add(dep_var)

    return {
        'deps': {k: sorted(v) for k, v in deps.items()},
        'rdeps': {k: sorted(v) for k, v in rdeps.items()},
        'after': {k: sorted(v) for k, v in after.items()},
        'all_packages': sorted(all_packages),
    }


def find_critical_path(deps: dict, all_packages: list) -> list:
    """Find the longest dependency chain (critical path)."""
    memo = {}
    in_progress = set()  # cycle detection

    def longest_chain(pkg):
        if pkg in memo:
            return memo[pkg]
        if pkg in in_progress:
            return [pkg]  # cycle detected, break recursion
        in_progress.add(pkg)
        dep_list = deps.get(pkg, [])
        if not dep_list:
            memo[pkg] = [pkg]
            in_progress.discard(pkg)
            return memo[pkg]
        best = []
        for d in dep_list:
            chain = longest_chain(d)
            if len(chain) > len(best):
                best = chain
        memo[pkg] = best + [pkg]
        in_progress.discard(pkg)
        return memo[pkg]

    longest = []
    for pkg in all_packages:
        chain = longest_chain(pkg)
        if len(chain) > len(longest):
            longest = chain

    return longest


def compute_fan_stats(deps: dict, rdeps: dict, after: dict = None) -> list:
    """Compute fan-out (dependents) and fan-in (dependencies) per package."""
    stats = []
    all_pkgs = set(deps.keys()) | set(rdeps.keys())
    if after:
        for k, v in after.items():
            all_pkgs.add(k)
            all_pkgs.update(v)
    for pkg in sorted(all_pkgs):
        fan_out = len(rdeps.get(pkg, []))
        fan_in = len(deps.get(pkg, []))
        if fan_out > 0 or fan_in > 0:
            stats.append({
                'package': pkg,
                'fan_out': fan_out,  # how many depend on me
                'fan_in': fan_in,    # how many I depend on
            })
    return sorted(stats, key=lambda x: -x['fan_out'])


def generate_dot(deps: dict, rdeps: dict, output_path: str):
    """Generate Graphviz DOT file."""
    with open(output_path, 'w') as f:
        f.write('digraph sonic_build {\n')
        f.write('  rankdir=LR;\n')
        f.write('  node [shape=box, fontsize=10];\n')

        # Color high fan-out nodes
        for pkg, dependents in rdeps.items():
            if len(dependents) >= 10:
                f.write(f'  "{pkg}" [style=filled, fillcolor=red, fontcolor=white];\n')
            elif len(dependents) >= 5:
                f.write(f'  "{pkg}" [style=filled, fillcolor=orange];\n')

        for pkg, dep_list in deps.items():
            for dep in dep_list:
                f.write(f'  "{dep}" -> "{pkg}";\n')

        f.write('}\n')


def main():
    base_dir = sys.argv[1] if len(sys.argv) > 1 else '.'
    rules_dir = os.path.join(base_dir, 'rules')

    if not os.path.isdir(rules_dir):
        print(f"Error: {rules_dir} not found", file=sys.stderr)
        sys.exit(1)

    print("Parsing rules/*.mk files...")
    graph = parse_rules(rules_dir)

    print(f"\nTotal packages: {len(graph['all_packages'])}")
    print(f"Dependency edges: {sum(len(v) for v in graph['deps'].values())}")
    print(f"After-ordering edges: {sum(len(v) for v in graph['after'].values())}")

    # Critical path
    print("\n" + "=" * 60)
    print("  CRITICAL PATH (longest dependency chain)")
    print("=" * 60)
    crit_path = find_critical_path(graph['deps'], graph['all_packages'])
    for i, pkg in enumerate(crit_path):
        indent = "  " * i
        print(f"  {indent}→ {pkg}")
    print(f"\n  Critical path length: {len(crit_path)} packages")

    # Fan-out analysis
    print("\n" + "=" * 60)
    print("  TOP 20 BOTTLENECK PACKAGES (by fan-out)")
    print("=" * 60)
    print(f"  {'PACKAGE':<40} {'FAN-OUT':>8} {'FAN-IN':>8}")
    print(f"  {'-------':<40} {'--------':>8} {'------':>8}")
    fan_stats = compute_fan_stats(graph['deps'], graph['rdeps'], graph['after'])
    for s in fan_stats[:20]:
        print(f"  {s['package']:<40} {s['fan_out']:>8} {s['fan_in']:>8}")

    # Packages with zero dependencies (can build immediately)
    roots = [p for p in graph['all_packages'] if p not in graph['deps'] or not graph['deps'][p]]
    print(f"\n  Root packages (zero deps, can build first): {len(roots)}")

    # Packages with zero dependents (leaves)
    leaves = [p for p in graph['all_packages'] if p not in graph['rdeps'] or not graph['rdeps'][p]]
    print(f"  Leaf packages (nothing depends on them): {len(leaves)}")

    # Max theoretical parallelism
    # (packages at the same depth in the graph can build concurrently)
    print(f"\n  Max theoretical parallelism: ~{len(roots)} at start, "
          f"fans out to ~{len(leaves)} at end")

    # Generate outputs
    dot_path = os.path.join(base_dir, 'target', 'build-dep-graph.dot')
    json_path = os.path.join(base_dir, 'target', 'build-dep-graph.json')

    os.makedirs(os.path.join(base_dir, 'target'), exist_ok=True)

    generate_dot(graph['deps'], graph['rdeps'], dot_path)
    print(f"\n  DOT graph: {dot_path}")

    with open(json_path, 'w') as f:
        json.dump({
            'deps': graph['deps'],
            'rdeps': {k: sorted(v) for k, v in graph['rdeps'].items()},
            'after': graph['after'],
            'critical_path': crit_path,
            'fan_stats': fan_stats[:30],
            'root_count': len(roots),
            'leaf_count': len(leaves),
        }, f, indent=2)
    print(f"  JSON graph: {json_path}")


if __name__ == '__main__':
    main()
