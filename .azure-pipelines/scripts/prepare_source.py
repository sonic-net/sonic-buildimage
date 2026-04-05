#!/usr/bin/env python3
"""
prepare_source.py — Prepare a sonic-buildimage workspace for static analysis.

Fetches all externally-obtained source trees (git clone, dget, wget) and
applies in-tree SONiC patches. Discovers packages automatically from
Makefiles and rules/*.mk — no hardcoded versions, URLs, or paths.

Usage:
    python3 prepare_source.py [--repo-root DIR] [--output-summary]
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
from pathlib import Path


BUILD_PUBLIC_URL = "https://packages.trafficmanager.net/public"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def run(cmd, *, cwd=None, check=True, capture=False):
    """Run a shell command, streaming output unless capture=True."""
    kwargs = dict(cwd=cwd, check=check)
    if capture:
        kwargs.update(stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    else:
        print(f"  + {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    return subprocess.run(cmd, shell=isinstance(cmd, str), **kwargs)


def resolve_make_var(rules_mk: Path, template: str, extra_mk: Path = None) -> str:
    """
    Expand Make variable references in `template` by including rules_mk
    (and optionally extra_mk) in a throwaway Makefile.
    Returns the resolved string, or the original template if resolution fails.
    """
    includes = f"include {rules_mk}\n"
    if extra_mk and extra_mk.exists():
        includes += f"include {extra_mk}\n"
    makefile = (
        f"{includes}"
        f"BUILD_PUBLIC_URL ?= {BUILD_PUBLIC_URL}\n"
        f"_x := {template}\n"
        f"resolve: ; @echo $(_x)\n"
    )
    try:
        result = subprocess.run(
            ["make", "-f", "-", "--no-print-directory", "resolve"],
            input=makefile, capture_output=True, text=True, check=False,
        )
        resolved = result.stdout.strip()
        return resolved if resolved else template
    except Exception:
        return template


def find_rules_mk(pkg_dir: Path) -> Path | None:
    """Find the rules .mk for a package by convention."""
    pkg = pkg_dir.name
    # rules/<pkg>.mk
    candidate = pkg_dir.parents[1] / "rules" / f"{pkg}.mk"
    if candidate.exists():
        return candidate
    # <parent>/<pkg>.mk  (e.g. platform/mellanox/iproute2.mk)
    candidate = pkg_dir.parent / f"{pkg}.mk"
    if candidate.exists():
        return candidate
    return None


def has_patches(pkg_dir: Path) -> bool:
    """Return True if pkg_dir contains any in-tree .patch files."""
    for subdir in ("patch", "patches"):
        d = pkg_dir / subdir
        if d.is_dir() and list(d.glob("*.patch")):
            return True
    return False


def source_subdir(pkg_dir: Path) -> Path | None:
    """
    Find the unpacked source subdirectory inside pkg_dir.
    Excludes patch/, patches/, debian/, .git/.
    """
    skip = {"patch", "patches", "debian", ".git"}
    candidates = sorted(
        d for d in pkg_dir.iterdir()
        if d.is_dir() and d.name not in skip
    )
    return candidates[0] if candidates else None


# ---------------------------------------------------------------------------
# Step 1: Sync submodules
# ---------------------------------------------------------------------------

def sync_submodules(repo_root: Path):
    print("\n=== Syncing submodules ===")
    run(["git", "submodule", "sync", "--recursive"], cwd=repo_root)
    run(["git", "submodule", "update", "--init", "--recursive"], cwd=repo_root)


# ---------------------------------------------------------------------------
# Step 2: Git-clone packages
# ---------------------------------------------------------------------------

def _parse_git_clone_info(pkg_dir: Path, rules_mk: Path | None):
    """
    Parse git clone URL, dest subdir, and checkout ref from a package Makefile.
    Returns (url, dest, ref, use_reset_hard) or None if not applicable.
    """
    mk = pkg_dir / "Makefile"
    if not mk.exists():
        return None

    # Strip comment lines (including tab-indented ones)
    lines = []
    for line in mk.read_text(errors="replace").splitlines():
        stripped = line.lstrip("\t ")
        if not stripped.startswith("#"):
            lines.append(line)
    content = "\n".join(lines)

    # git clone line
    clone_match = re.search(r"git clone\b([^\n]+)", content)
    if not clone_match:
        return None
    clone_line = clone_match.group(1).strip()

    # URL
    url_match = re.search(r"https?://\S+", clone_line)
    if not url_match:
        return None
    raw_url = url_match.group(0)

    # Dest: last non-flag token after URL; fall back to repo basename
    tokens = [t for t in clone_line.split() if not t.startswith("-")]
    raw_dest = tokens[-1] if tokens else ""
    if not raw_dest or raw_dest.startswith("http"):
        raw_dest = Path(raw_url).stem  # strip .git

    # Ref priority:
    # 1. inline -b <ref> appearing BEFORE the URL in clone line
    inline_b = re.search(r"-b\s+(\S+)(?=.*https?://)", clone_line)
    # 2. plain git checkout <ref> (not -b, not stg)
    plain_co = re.search(r"git checkout\b(?!\s+-b)(?!\s+.*stg)\s+(\S+)", content)
    # 3. git checkout -b <branch> <tag> → last token
    co_b = re.search(r"git checkout -b\s+\S+\s+(\S+)", content)
    # 4. git reset --hard <ref>
    reset = re.search(r"git reset --hard\s+(\S+)", content)

    use_reset_hard = False
    if inline_b:
        raw_ref = inline_b.group(1)
    elif plain_co:
        raw_ref = plain_co.group(1)
    elif co_b:
        raw_ref = co_b.group(1)
    elif reset:
        raw_ref = reset.group(1).replace("\\%", "%")
        use_reset_hard = True
    else:
        raw_ref = ""

    # Strip "tags/" prefix — git clone --branch doesn't accept it
    if raw_ref.startswith("tags/"):
        raw_ref = raw_ref[len("tags/"):]

    # Resolve Make variables
    url = resolve_make_var(rules_mk, raw_url) if rules_mk else raw_url
    dest = resolve_make_var(rules_mk, raw_dest) if rules_mk else raw_dest
    ref = resolve_make_var(rules_mk, raw_ref) if rules_mk and raw_ref else raw_ref

    return url, dest.lstrip("./") or Path(url).stem, ref, use_reset_hard


def fetch_git_clone(pkg_dir: Path, rules_mk: Path | None):
    info = _parse_git_clone_info(pkg_dir, rules_mk)
    if not info:
        print(f"  SKIP {pkg_dir}: no git clone found")
        return
    url, dest, ref, use_reset_hard = info
    dest_path = pkg_dir / dest
    if dest_path.exists():
        print(f"  EXISTS: {dest_path}")
        return
    ref_str = f" @ {ref}" if ref else ""
    print(f"  Cloning {pkg_dir.name} (dest={dest}{ref_str}) from {url}")
    if use_reset_hard:
        run(["git", "clone", "--depth", "50", url, str(dest_path)], check=False)
        run(["git", "reset", "--hard", ref], cwd=dest_path, check=False)
    else:
        cmd = ["git", "clone", "--depth", "1"]
        if ref:
            cmd += ["--branch", ref]
        cmd += [url, str(dest_path)]
        run(cmd, check=False)


def fetch_git_clones(repo_root: Path):
    print("\n=== Cloning external source trees ===")
    # Auto-discover: src/ dirs with git clone in Makefile + in-tree patches
    src = repo_root / "src"
    for pkg_dir in sorted(src.iterdir()):
        if not pkg_dir.is_dir():
            continue
        mk = pkg_dir / "Makefile"
        if not mk.exists():
            continue
        if "git clone" not in mk.read_text(errors="replace"):
            continue
        if not has_patches(pkg_dir):
            continue
        rules_mk = find_rules_mk(pkg_dir)
        fetch_git_clone(pkg_dir, rules_mk)

    # No-patch clones: not discoverable by patch-dir heuristic
    no_patch_clones = [
        ("https://github.com/Mellanox/libpsample.git", repo_root / "src/sflow/libpsample"),
        ("https://github.com/redis/librdb.git",         repo_root / "src/rdb-cli/librdb"),
        ("https://github.com/openconfig/oc-pyang.git",
         repo_root / "src/sonic-mgmt-common/models/yang/oc-pyang"),
    ]
    for url, dest in no_patch_clones:
        if dest.exists():
            print(f"  EXISTS: {dest}")
            continue
        print(f"  Cloning {dest.name} from {url}")
        run(["git", "clone", "--depth", "1", url, str(dest)], check=False)


# ---------------------------------------------------------------------------
# Step 3: dget / wget-dsc packages
# ---------------------------------------------------------------------------

def resolve_dsc_url(pkg_dir: Path, rules_mk: Path) -> str | None:
    """
    Extract and resolve the .dsc URL from a package Makefile that uses dget.
    Falls back to including the package Makefile for packages that define URL
    vars locally (e.g. lldpd DSC_FILE_URL, libyang3 LIBYANG_URL).
    """
    mk = pkg_dir / "Makefile"
    # Find dget line
    dget_match = re.search(r"dget\s+(?:-u\s+)?(\S+)", mk.read_text(errors="replace"))
    if not dget_match:
        return None
    template = dget_match.group(1)

    url = resolve_make_var(rules_mk, template)
    if not re.search(r"https?://.*\.dsc", url):
        # Retry with package Makefile included
        url = resolve_make_var(rules_mk, template, extra_mk=mk)
    return url if re.search(r"https?://.*\.dsc", url) else None


def fetch_dsc(pkg_dir: Path, url: str):
    """Fetch a Debian source package via dget if not already present."""
    existing = source_subdir(pkg_dir)
    if existing:
        print(f"  EXISTS: {existing}")
        return
    print(f"  Fetching {pkg_dir.name} from {url}")
    run(["dget", "-u", url], cwd=pkg_dir, check=False)


def fetch_dsc_packages(repo_root: Path):
    print("\n=== Fetching dget/tarball source trees ===")

    # ifupdown2: GitHub tarball (no .dsc)
    ifupdown2_mk = repo_root / "rules/ifupdown2.mk"
    if ifupdown2_mk.exists():
        ver_match = re.search(r"^IFUPDOWN2_VERSION\s*=\s*(\S+)",
                              ifupdown2_mk.read_text(), re.MULTILINE)
        if ver_match:
            ver = ver_match.group(1)
            dest = repo_root / f"src/ifupdown2/ifupdown2-{ver}"
            if not dest.exists():
                print(f"  Fetching ifupdown2 {ver}...")
                url = f"https://github.com/CumulusNetworks/ifupdown2/archive/{ver}.tar.gz"
                with tempfile.NamedTemporaryFile(suffix=".tar.gz", delete=False) as f:
                    urllib.request.urlretrieve(url, f.name)
                    with tarfile.open(f.name) as tar:
                        tar.extractall(repo_root / "src/ifupdown2")
                os.unlink(f.name)

    # Auto-discover dget packages
    search_dirs = [repo_root / "src", repo_root / "platform" / "mellanox"]
    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
        for mk_path in sorted(search_dir.rglob("Makefile")):
            pkg_dir = mk_path.parent
            # Skip nested Makefiles (depth > search_dir/<pkg>)
            rel = pkg_dir.relative_to(search_dir)
            if len(rel.parts) > 1:
                continue
            content = mk_path.read_text(errors="replace")
            if not re.search(r"\bdget\b", content):
                continue
            if not has_patches(pkg_dir):
                continue
            rules_mk = find_rules_mk(pkg_dir)
            if not rules_mk:
                print(f"  SKIP {pkg_dir}: no rules .mk found")
                continue
            url = resolve_dsc_url(pkg_dir, rules_mk)
            if not url:
                print(f"  SKIP {pkg_dir}: could not resolve dget URL")
                continue
            fetch_dsc(pkg_dir, url)

    # wget-dsc packages (have in-tree patches, use wget instead of dget)
    _fetch_wget_dsc(repo_root)


def _fetch_wget_dsc(repo_root: Path):
    """Handle iptables, socat, thrift which use wget but have .dsc URLs."""
    # iptables
    iptables_mk = repo_root / "src/iptables/Makefile"
    iptables_rules = repo_root / "rules/iptables.mk"
    if iptables_mk.exists() and iptables_rules.exists():
        url_base = re.search(r"^IPTABLES_URL\s*=\s*(\S+)",
                             iptables_mk.read_text(), re.MULTILINE)
        ver = re.search(r"^IPTABLES_VERSION\s*=\s*(\S+)",
                        iptables_rules.read_text(), re.MULTILINE)
        suffix = re.search(r"^IPTABLES_VERSION_SUFFIX\s*=\s*(\S+)",
                           iptables_rules.read_text(), re.MULTILINE)
        if url_base and ver and suffix:
            url = f"{url_base.group(1)}/iptables_{ver.group(1)}-{suffix.group(1)}.dsc"
            fetch_dsc(repo_root / "src/iptables", url)

    # socat
    socat_mk = repo_root / "src/socat/Makefile"
    socat_rules = repo_root / "rules/socat.mk"
    if socat_mk.exists() and socat_rules.exists():
        wget_line = next((l for l in socat_mk.read_text().splitlines()
                          if "wget" in l and ".dsc" in l), None)
        if wget_line:
            tmpl_match = re.search(r'"([^"]+\.dsc)"', wget_line)
            if tmpl_match:
                url = resolve_make_var(socat_rules, tmpl_match.group(1))
                if re.search(r"https?://.*\.dsc", url):
                    fetch_dsc(repo_root / "src/socat", url)

    # thrift
    thrift_mk = repo_root / "src/thrift/Makefile"
    if thrift_mk.exists():
        content = thrift_mk.read_text()
        ver = re.search(r"^THRIFT_VERSION\s*=\s*(\S+)", content, re.MULTILINE)
        full = re.search(r"^THRIFT_VERSION_FULL\s*=\s*(.+)", content, re.MULTILINE)
        if ver and full:
            full_ver = full.group(1).strip().replace("$(THRIFT_VERSION)", ver.group(1))
            url = f"{BUILD_PUBLIC_URL}/debian/thrift_{full_ver}.dsc"
            fetch_dsc(repo_root / "src/thrift", url)


# ---------------------------------------------------------------------------
# Step 4: Apply patches
# ---------------------------------------------------------------------------

def apply_patches(repo_root: Path):
    print("\n=== Applying source patches ===")
    search_dirs = [repo_root / "src", repo_root / "platform" / "mellanox"]
    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
        for patch_dir in sorted(search_dir.rglob("*")):
            if patch_dir.name not in ("patch", "patches"):
                continue
            if not patch_dir.is_dir():
                continue
            patches = sorted(patch_dir.glob("*.patch"))
            if not patches:
                continue
            pkg_dir = patch_dir.parent
            src_dir = source_subdir(pkg_dir)
            if not src_dir:
                print(f"  SKIP {pkg_dir}: no source dir")
                continue
            print(f"  {src_dir}: {len(patches)} patches")
            for p in patches:
                result = subprocess.run(
                    ["git", "apply", "--whitespace=nowarn", str(p)],
                    cwd=src_dir, capture_output=True,
                )
                status = "OK" if result.returncode == 0 else "SKIP (already applied)"
                print(f"    {status}: {p.name}")


# ---------------------------------------------------------------------------
# Step 5: Summary
# ---------------------------------------------------------------------------

def print_summary(repo_root: Path):
    print("\n=== Workspace summary ===")
    result = run(["git", "submodule", "status", "--recursive"],
                 cwd=repo_root, capture=True, check=False)
    print(f"Submodule count: {len(result.stdout.splitlines())}")

    excludes = {".git", "target", "fsroot", "dpkg",
                "src/p4lang", "src/sonic-linux-kernel", "src/grub2", "src/sonic-device-data"}

    def excluded(p: Path) -> bool:
        parts = p.relative_to(repo_root).parts
        return any(
            str(Path(*parts[:i+1])) in excludes or parts[i] in excludes
            for i in range(len(parts))
        )

    py = sum(1 for p in repo_root.rglob("*.py") if not excluded(p))
    c_cpp = sum(1 for p in repo_root.rglob("*")
                if p.suffix in (".c", ".h", ".cpp", ".hpp") and not excluded(p))
    go = sum(1 for p in repo_root.rglob("*.go") if not excluded(p))
    print(f"Python files:    {py}")
    print(f"C/C++ files:     {c_cpp}")
    print(f"Go files:        {go}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=".", help="Path to sonic-buildimage repo root")
    parser.add_argument("--skip-submodules", action="store_true")
    parser.add_argument("--skip-git-clones", action="store_true")
    parser.add_argument("--skip-dget", action="store_true")
    parser.add_argument("--skip-patches", action="store_true")
    parser.add_argument("--summary", action="store_true")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    os.chdir(repo_root)

    if not args.skip_submodules:
        sync_submodules(repo_root)
    if not args.skip_git_clones:
        fetch_git_clones(repo_root)
    if not args.skip_dget:
        fetch_dsc_packages(repo_root)
    if not args.skip_patches:
        apply_patches(repo_root)
    if args.summary:
        print_summary(repo_root)

    print("\nDone.")


if __name__ == "__main__":
    main()
