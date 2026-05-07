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


def resolve_make_var(rules_mk: Path, template: str, extra_mk: Path = None,
                     bldenv: str = None) -> str:
    """
    Expand Make variable references in `template` by including rules_mk
    (and optionally extra_mk) in a throwaway Makefile.
    Returns the resolved string, or the original template if resolution fails.
    If bldenv is None, tries trixie/bookworm/bullseye and returns the first
    non-empty result (needed for rules that gate on BLDENV).
    """
    def _try(env):
        bldenv_line = f"BLDENV := {env}\n" if env else ""
        includes = f"include {rules_mk.resolve()}\n"
        if extra_mk and extra_mk.exists():
            includes += f"include {extra_mk.resolve()}\n"
        makefile = (
            f"{bldenv_line}"
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
            return result.stdout.strip()
        except Exception:
            return ""

    if bldenv is not None:
        resolved = _try(bldenv)
        return resolved if resolved else template

    # Try each BLDENV in order of preference; return first fully-resolved result
    # "Fully resolved" means: no remaining $(VAR) references, non-empty,
    # and no double-separator patterns like `__`, `_.`, `_/` which indicate
    # an empty Make variable was expanded (e.g., net-snmp_.dsc).
    def _looks_resolved(s: str) -> bool:
        if not s or "$(" in s:
            return False
        # Detect adjacent separators that signal an empty variable expansion
        if re.search(r"[_/.-]{2,}", s.replace("//", "")):
            # Allow double slashes in URLs (https://), but flag others
            cleaned = re.sub(r"https?://", "", s)
            if re.search(r"[_.-]{2,}|_/|_\.", cleaned):
                return False
        return True

    for env in ("trixie", "bookworm", "bullseye"):
        resolved = _try(env)
        if _looks_resolved(resolved):
            return resolved
    return template


def find_rules_mk(pkg_dir: Path) -> Path | None:
    """Find the rules .mk for a package by convention."""
    pkg = pkg_dir.name
    # Walk up to find rules/ dir (handles both top-level and nested packages)
    for parent in pkg_dir.parents:
        candidate = parent / "rules" / f"{pkg}.mk"
        if candidate.exists():
            return candidate
        # Also try <parent>/<pkg>.mk (e.g. platform/mellanox/iproute2.mk)
        candidate = pkg_dir.parent / f"{pkg}.mk"
        if candidate.exists():
            return candidate
        # Stop at repo root (has Makefile + rules/)
        if (parent / "rules").is_dir():
            # Also try parent-dir name as rules file (e.g. src/sflow/hsflowd -> rules/sflow.mk)
            parent_name = pkg_dir.parent.name
            candidate = parent / "rules" / f"{parent_name}.mk"
            if candidate.exists():
                return candidate
            break
    return None


def has_patches(pkg_dir: Path, _depth: int = 0) -> bool:
    """Return True if pkg_dir contains any in-tree .patch files (recurse 1 level)."""
    for d in pkg_dir.iterdir():
        if not d.is_dir():
            continue
        name = d.name
        # Match patch/, patches/, patch-<ver>/, patches-<ver>/
        if name in ("patch", "patches") or \
           (name.startswith("patch") and len(name) > 5 and name[5] in ("-", "_")):
            if list(d.glob("*.patch")):
                return True
        # Recurse one more level (e.g. src/radius/pam/freeradius/patches/)
        elif _depth == 0 and d.is_dir() and not name.startswith("."):
            if has_patches(d, _depth=1):
                return True
    return False


def source_subdir(pkg_dir: Path) -> Path | None:
    """
    Find the unpacked source subdirectory inside pkg_dir.
    Excludes patch/, patches/ (and versioned variants like patch-1.2.3+dfsg),
    debian/, .git/, and dirs that look like in-repo utility dirs rather than
    upstream source trees (e.g. src/bash/Files/).

    Heuristics (in order):
    1. Skip known non-source dir names: patch*, patches*, debian, .git, Files
    2. Prefer dirs whose name contains a digit (version number in unpacked source)
    3. Fall back to any dir with actual source files if no versioned dir found
    """
    skip_names = {"patch", "patches", "debian", ".git", "Files"}
    versioned = []
    fallback = []

    for d in sorted(pkg_dir.iterdir()):
        if not d.is_dir():
            continue
        name = d.name
        if name in skip_names:
            continue
        if name.startswith("patch") and (len(name) == 5 or name[5] in ("-", "_")):
            continue
        if name.startswith("patches") and (len(name) == 7 or name[7] in ("-", "_")):
            continue
        # Check has actual source files (not empty)
        has_files = any(True for _ in d.iterdir() if _.is_file())
        if not has_files:
            has_files = any(True for sub in d.iterdir()
                            if sub.is_dir() and any(True for _ in sub.iterdir() if _.is_file()))
        if not has_files:
            continue
        if any(c.isdigit() for c in name):
            versioned.append(d)
        else:
            fallback.append(d)

    if versioned:
        return versioned[0]
    return fallback[0] if fallback else None


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

    # git clone line — find the first one with a URL
    clone_match = re.search(r"(pushd\s+(\S+)\s*\n[^\n]*\n\s*)?git clone\b([^\n]+)", content)
    if not clone_match:
        return None
    pushd_subdir = clone_match.group(2) or ""  # subdir from pushd, if any
    clone_line = clone_match.group(3).strip()

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

    # Prepend pushd subdir to dest if cloning from inside a subdir
    if pushd_subdir and not pushd_subdir.startswith("."):
        raw_dest = pushd_subdir + "/" + raw_dest

    # Ref priority:
    # 1. inline -b <ref> appearing BEFORE the URL in clone line
    inline_b = re.search(r"-b\s+(\S+)(?=.*https?://)", clone_line)
    # 2. plain git checkout <ref> (not -b, not stg); skip leading flag tokens
    plain_co = re.search(r"git checkout\b(?!\s+-b)(?!\s+.*stg)\s+((?:-\S+\s+)*)(\S+)", content)
    # 3. git checkout -b <branch> [-f] <tag> → last non-flag token
    co_b = re.search(r"git checkout -b\s+\S+\s+(?:-\S+\s+)*(\S+)", content)
    # 4. git reset --hard <ref>
    reset = re.search(r"git reset --hard\s+(\S+)", content)

    use_reset_hard = False
    if inline_b:
        raw_ref = inline_b.group(1)
    elif plain_co:
        raw_ref = plain_co.group(2)  # group(1) is flags, group(2) is the ref
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



# ---------------------------------------------------------------------------
# make -n based git clone discovery (SONIC_MAKE_DEPS no-op approach)
# ---------------------------------------------------------------------------

_MAKE_N_CLONE_RE = re.compile(r"git clone\b([^\n]+)")
_MAKE_N_CLONE_URL_RE = re.compile(r"(https?://\S+)")
_MAKE_N_CLONE_B_RE = re.compile(r"\s-b\s+(\S+)")
_MAKE_N_RESET_RE = re.compile(r"git reset --hard\s+(\S+)")
_MAKE_N_CO_DASH_B_RE = re.compile(r"git checkout\s+-b\s+\S+\s+(?:-\S+\s+)*(\S+)")
_MAKE_N_CO_BEFORE_B_RE = re.compile(r"git checkout\s+(?!-[bf])(\S+)")
_MAKE_N_CO_AFTER_FLAGS_RE = re.compile(r"git checkout\s+(-\S+\s+)*(\S+)")
_SKIP_REFS = frozenset({"-f", "-a", "push", "pop", "init", "--hard", "-b", "stg", "stg_temp"})


def _extract_git_info_from_make_n(output: str):
    """Parse git clone URL, dest, ref from make -n dry-run output."""
    cm = _MAKE_N_CLONE_RE.search(output)
    if not cm:
        return None
    clone_line = cm.group(1)
    url_m = _MAKE_N_CLONE_URL_RE.search(clone_line)
    if not url_m:
        return None
    url = url_m.group(1)

    tokens = [t for t in clone_line.split() if not t.startswith("-")]
    dest = tokens[-1] if tokens else Path(url).stem
    if dest.startswith("http"):
        dest = Path(url).stem

    # Check if clone is inside a pushd subdir (e.g. pushd freeradius; git clone ...)
    # Look for "pushd <subdir>" on the line before the git clone
    pushd_m = re.search(r"pushd\s+(\S+)\s*\n[^\n]*\n?[^\n]*git clone", output)
    if pushd_m:
        subdir = pushd_m.group(1)
        if not subdir.startswith(".") and not subdir.startswith("/"):
            dest = subdir + "/" + dest

    # Ref priority: -b in clone line > git checkout -b branch <ref> >
    #               git checkout <ref> [-b ...] > git reset --hard <ref>
    b_m = _MAKE_N_CLONE_B_RE.search(clone_line)
    if b_m:
        ref = b_m.group(1).strip("\"'")
    else:
        dash_b_m = _MAKE_N_CO_DASH_B_RE.search(output)
        before_b_m = _MAKE_N_CO_BEFORE_B_RE.search(output)
        after_flags_m = _MAKE_N_CO_AFTER_FLAGS_RE.search(output)
        reset_m = _MAKE_N_RESET_RE.search(output)
        if dash_b_m and dash_b_m.group(1) not in _SKIP_REFS:
            ref = dash_b_m.group(1)
        elif before_b_m and before_b_m.group(1) not in _SKIP_REFS:
            ref = before_b_m.group(1)
        elif reset_m:
            ref = reset_m.group(1)
        elif after_flags_m and after_flags_m.group(2) not in _SKIP_REFS:
            ref = after_flags_m.group(2)
        else:
            ref = ""

    use_reset_hard = bool(_MAKE_N_RESET_RE.search(output))
    # Strip "tags/" prefix — git clone --branch does not accept it
    if ref.startswith("tags/"):
        ref = ref[len("tags/"):]
    # Skip partial refs ending in / (incomplete variable expansion)
    if ref.endswith("/"):
        ref = ""
    return url, dest.lstrip("./") or Path(url).stem, ref, use_reset_hard


def _find_deb_var_for_pkg(pkg_dir: Path, rules_mk: "Path | None") -> str | None:
    """
    Find the Make variable name for the deb corresponding to pkg_dir.
    First checks for a _SRC_PATH assignment pointing to this package,
    then falls back to name-based matching.
    """
    if not rules_mk or not rules_mk.exists():
        return None
    content = rules_mk.read_text(errors="replace")
    repo_root = rules_mk.parents[1]  # rules_mk is at <root>/rules/pkg.mk
    try:
        rel = str(pkg_dir.relative_to(repo_root / "src"))
    except ValueError:
        rel = pkg_dir.name
    # Prefer _SRC_PATH match — exact package association
    m = re.search(
        r"\$\((\w+)\)_SRC_PATH\s*=\s*\$\(SRC_PATH\)/" + re.escape(rel),
        content,
    )
    if m:
        return m.group(1)
    # Fallback: first .deb var whose name contains pkg name (uppercase)
    pkg_upper = pkg_dir.name.upper().replace("-", "_")
    for m in re.finditer(r"^(\w+)\s*=\s*\S+\.deb", content, re.MULTILINE):
        if pkg_upper in m.group(1):
            return m.group(1)
    # Last resort: first .deb var in file
    m = re.search(r"^(\w+)\s*=\s*\S+\.deb", content, re.MULTILINE)
    return m.group(1) if m else None


def _parse_git_clone_info_via_make_n(pkg_dir: Path, rules_mk: "Path | None"):
    """
    Use make -n (dry-run) to discover git clone info for a SONIC_MAKE_DEBS package.
    This lets Make fully expand all variables rather than fragile regex parsing.
    Falls back to None if make -n cannot produce usable output.
    """
    if not rules_mk or not rules_mk.exists():
        return None
    if not (pkg_dir / "Makefile").exists():
        return None

    deb_var = _find_deb_var_for_pkg(pkg_dir, rules_mk)
    if not deb_var:
        return None

    mk_path = str(rules_mk.resolve())
    pkg_mk_path = str((pkg_dir / "Makefile").resolve())

    for arch in ("amd64", "arm64", ""):
        arch_line = "CONFIGURED_ARCH := " + arch + "\n" if arch else ""
        wrapper_base = (
            "DEST := /tmp/dest\n" + arch_line +
            "include " + mk_path + "\n" +
            "include " + pkg_mk_path + "\n"
        )
        print_mk = wrapper_base + "print_deb:\n\t@echo $(" + deb_var + ")\n"
        try:
            dv = subprocess.run(
                ["make", "-f", "/dev/stdin", "--no-print-directory", "print_deb"],
                input=print_mk, capture_output=True, text=True, timeout=3,
                cwd=str(pkg_dir), check=False,
            )
            deb_val = dv.stdout.strip()
            if not deb_val or "$(" in deb_val:
                continue

            target = "/tmp/dest/" + deb_val
            r = subprocess.run(
                ["make", "-n", "-f", "/dev/stdin", target],
                input=wrapper_base, capture_output=True, text=True, timeout=5,
                cwd=str(pkg_dir), check=False,
            )
            if r.returncode != 0:
                continue
            result = _extract_git_info_from_make_n(r.stdout)
            if result:
                return result
        except Exception:
            pass
    return None


def fetch_git_clone(pkg_dir: Path, rules_mk: Path | None):
    # Use make -n (dry-run) to discover git clone info.
    # This fully expands Make variables rather than fragile regex parsing.
    info = _parse_git_clone_info_via_make_n(pkg_dir, rules_mk)
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

    # Detect if ref is a commit SHA (hex string, 7-40 chars)
    is_sha = ref and re.fullmatch(r"[0-9a-f]{7,40}", ref or "")

    if use_reset_hard:
        run(["git", "clone", "--depth", "50", url, str(dest_path)], check=False)
        run(["git", "reset", "--hard", ref], cwd=dest_path, check=False)
    elif is_sha:
        # Can't use --branch with a commit SHA; clone then fetch+checkout
        run(["git", "clone", "--no-checkout", url, str(dest_path)], check=False)
        run(["git", "fetch", "--depth", "1", "origin", ref],
            cwd=dest_path, check=False)
        run(["git", "checkout", ref], cwd=dest_path, check=False)
    else:
        cmd = ["git", "clone", "--depth", "1"]
        if ref:
            cmd += ["--branch", ref]
        cmd += [url, str(dest_path)]
        run(cmd, check=False)


def _find_package_makefiles(directory: Path) -> list[Path]:
    """
    Walk directory returning Makefiles that belong to SONiC package dirs.
    Does NOT descend into fetched upstream source trees (detected by presence
    of configure / CMakeLists.txt / setup.py / configure.ac).
    """
    _source_tree_indicators = frozenset({
        "configure", "CMakeLists.txt", "setup.py", "configure.ac",
        "Kbuild", "Kconfig", "autogen.sh",
    })
    result = []
    for entry in sorted(directory.iterdir()):
        if not entry.is_dir() or entry.name.startswith("."):
            continue
        mk = entry / "Makefile"
        if mk.exists():
            result.append(mk)
            # If it has source-tree indicators, don't descend further
            if any((entry / f).exists() for f in _source_tree_indicators):
                continue
        if not any((entry / f).exists() for f in _source_tree_indicators):
            result.extend(_find_package_makefiles(entry))
    return result


def fetch_git_clones(repo_root: Path):
    print("\n=== Cloning external source trees ===")
    # Auto-discover: src/ dirs with git clone + in-tree patches.
    # Use _find_package_makefiles to avoid descending into fetched source trees.
    src = repo_root / "src"
    for mk_path in _find_package_makefiles(src):
        pkg_dir = mk_path.parent
        try:
            content = mk_path.read_text(errors="replace")
        except OSError:
            continue
        if "git clone" not in content or not has_patches(pkg_dir):
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
                    subprocess.run(["curl", "-fsSL", "-o", f.name, url], check=True)
                    with tarfile.open(f.name) as tar:
                        # Safe extraction: skip members with absolute paths or '..' traversal
                        members = [m for m in tar.getmembers()
                                   if not os.path.isabs(m.name)
                                   and ".." not in m.name.split("/")]
                        tar.extractall(repo_root / "src/ifupdown2", members=members)
                os.unlink(f.name)

    # Auto-discover dget packages
    search_dirs = [repo_root / "src", repo_root / "platform" / "mellanox"]
    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
        for mk_path in _find_package_makefiles(search_dir):
            pkg_dir = mk_path.parent
            try:
                content = mk_path.read_text(errors="replace")
            except OSError:
                continue
            if not re.search(r"\bdget\b", content):
                continue
            if not has_patches(pkg_dir):
                continue
            rules_mk = find_rules_mk(pkg_dir)
            if not rules_mk:
                print(f"  WARNING {pkg_dir}: no rules .mk found — skipping")
                continue
            url = resolve_dsc_url(pkg_dir, rules_mk)
            if not url:
                print(f"  WARNING {pkg_dir}: could not resolve dget URL — skipping")
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

def apply_patches(repo_root: Path) -> list[str]:
    """Apply patches. Returns list of error strings for any failures."""
    errors = []
    print("\n=== Applying source patches ===")
    search_dirs = [repo_root / "src", repo_root / "platform" / "mellanox"]
    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
        for patch_dir in sorted(search_dir.rglob("*")):
            # Skip patch dirs that are inside .git/ (stgit/quilt internal dirs)
            if ".git" in patch_dir.parts:
                continue
            # Skip patch dirs inside submodule checkouts (any ancestor has .git)
            is_in_submodule = False
            for parent in patch_dir.parents:
                if parent == search_dir:
                    break
                if (parent / ".git").exists():
                    is_in_submodule = True
                    break
            if is_in_submodule:
                continue
            name = patch_dir.name
            is_patch_dir = (
                name in ("patch", "patches") or
                (name.startswith("patch") and len(name) > 5 and name[5] in ("-", "_"))
            )
            if not is_patch_dir:
                continue
            if not patch_dir.is_dir():
                continue
            patches = sorted(patch_dir.glob("*.patch"))
            if not patches:
                continue
            pkg_dir = patch_dir.parent
            src_dir = source_subdir(pkg_dir)
            if not src_dir:
                msg = f"MISSING source dir for {pkg_dir} (has patches but no source)"
                print(f"  ERROR: {msg}")
                errors.append(msg)
                continue
            print(f"  {src_dir}: {len(patches)} patches")
            for p in patches:
                result = subprocess.run(
                    ["git", "apply", "--whitespace=nowarn", str(p)],
                    cwd=src_dir, capture_output=True,
                )
                status = "OK" if result.returncode == 0 else "SKIP (already applied)"
                print(f"    {status}: {p.name}")
    return errors


def validate_sources(repo_root: Path) -> list[str]:
    """
    Verify that every package that fetches external source (dget or git clone)
    has an unpacked source dir after fetching.
    Uses _find_package_makefiles to avoid descending into fetched source trees.
    Returns a list of error strings; empty list means all good.
    """
    errors = []
    print("\n=== Validating source trees ===")
    search_dirs = [repo_root / "src", repo_root / "platform" / "mellanox"]
    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
        for mk_path in _find_package_makefiles(search_dir):
            pkg_dir = mk_path.parent
            if not has_patches(pkg_dir):
                continue
            try:
                content = mk_path.read_text(errors="replace")
            except OSError:
                continue
            # Only validate packages that fetch external source (dget or git clone)
            # Submodule packages (sonic-frr, sonic-gnmi, etc.) manage their own source
            fetches_external = re.search(r"\bdget\b", content) or re.search(r"\bgit clone\b", content)
            if not fetches_external:
                continue
            src_dir = source_subdir(pkg_dir)
            rel = pkg_dir.relative_to(repo_root)
            if src_dir:
                print(f"  OK      {rel} -> {src_dir.name}")
            else:
                msg = f"MISSING source for {rel} (patches exist but no source dir fetched)"
                print(f"  ERROR:  {msg}")
                errors.append(msg)
    return errors


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

    # Validate all patched packages have source before applying patches
    errors = validate_sources(repo_root)

    if not args.skip_patches:
        errors += apply_patches(repo_root)
    if args.summary:
        print_summary(repo_root)

    if errors:
        print(f"\n{'='*60}")
        print(f"FAILED: {len(errors)} error(s):")
        for e in errors:
            print(f"  - {e}")
        print(f"{'='*60}")
        sys.exit(1)

    print("\nDone.")


if __name__ == "__main__":
    main()
