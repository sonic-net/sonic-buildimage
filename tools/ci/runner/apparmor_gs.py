#!/usr/bin/env python3
"""Configure the host Ghostscript profile for owned SONiC PS/PDF build files."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess

PROFILE_DIR = Path("/etc/apparmor.d")
PROFILES = Path("/sys/kernel/security/apparmor/profiles")
FRAGMENT = "local/sonic-vs-gs"
INCLUDE = f"include <{FRAGMENT}>"
RULE = "owner /sonic/**.{ps,pdf} rw,"
CONTENTS = "# Managed by SONiC runner bootstrap; owned build PS/PDF files only.\n" + RULE + "\n"


def has_include(text, name):
    return re.search(r"^\s*(?:include|#include)(?:\s+if\s+exists)?\s+[<\"]"
                     + re.escape(name) + r"[>\"]\s*(?:#.*)?$", text, re.MULTILINE) is not None


def gs_loaded(profiles=PROFILES):
    try:
        text = profiles.read_text()
    except FileNotFoundError:
        return False
    except PermissionError:
        return None
    return any(line.startswith(("gs (", "/usr/bin/gs (")) for line in text.splitlines())


def install_rules(directory=PROFILE_DIR):
    """Preserve administrator rules and add one separately managed include."""
    profile = directory / "gs"
    if not has_include(profile.read_text(), "local/gs"):
        raise RuntimeError(f"{profile} does not include local/gs; review its layout before applying the SONiC allowance")
    local = directory / "local/gs"
    fragment = directory / FRAGMENT
    local_directory_existed = local.parent.exists()
    local.parent.mkdir(parents=True, exist_ok=True)
    if not local_directory_existed:
        local.parent.chmod(0o755)
        os.chown(local.parent, 0, 0)
    local_existed = local.exists()
    original = local.read_text() if local_existed else ""
    if fragment.exists():
        existing_rules = [line.strip() for line in fragment.read_text().splitlines()
                          if line.strip() and not line.lstrip().startswith("#")]
        if existing_rules != [RULE]:
            raise RuntimeError(f"{fragment} contains other rules; review before replacing this managed file")
    if not fragment.exists() or fragment.read_text() != CONTENTS:
        fragment.write_text(CONTENTS)
    fragment.chmod(0o644)
    os.chown(fragment, 0, 0)
    if not has_include(original, FRAGMENT):
        with local.open("a") as stream:
            if original and not original.endswith("\n"):
                stream.write("\n")
            stream.write(INCLUDE + "\n")
    if not local_existed:
        local.chmod(0o644)
        os.chown(local, 0, 0)


def check_ghostscript(directory=PROFILE_DIR, profiles=PROFILES):
    loaded = gs_loaded(profiles)
    if loaded is False:
        print("AppArmor: gs profile is not loaded")
        return
    profile = directory / "gs"
    if loaded is None and not profile.exists():
        print("NOTE: AppArmor loaded policies are unreadable; no standard gs profile is installed")
        return
    try:
        configured = (has_include(profile.read_text(), "local/gs")
                      and has_include((directory / "local/gs").read_text(), FRAGMENT)
                      and RULE in [line.strip() for line in (directory / FRAGMENT).read_text().splitlines()])
    except PermissionError as error:
        raise RuntimeError(f"Cannot inspect AppArmor gs configuration {error.filename}; have the host administrator "
                           "review readability for sonic-runner. Bootstrap preserves existing local file permissions.") from error
    except FileNotFoundError:
        configured = False
    if not configured:
        raise RuntimeError("AppArmor gs may block Bash PDF generation: run the runner bootstrap to install "
                           "the scoped owner /sonic/**.{ps,pdf} allowance and reload only the gs profile")
    print("AppArmor: managed gs allowance is configured; bootstrap reloads the profile. "
          "This file check does not validate the effective kernel policy.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", action="store_true", help="Install the scoped rules and reload gs only (root)")
    args = parser.parse_args()
    if not args.install:
        check_ghostscript()
        return
    if os.geteuid() != 0:
        parser.error("Installation requires root")
    profile = PROFILE_DIR / "gs"
    if not profile.exists() or (PROFILE_DIR / "disable/gs").is_symlink():
        print("AppArmor: no enabled standard gs profile; no change needed")
        return
    apparmor_parser = shutil.which("apparmor_parser")
    if apparmor_parser is None:
        raise RuntimeError("gs profile exists but apparmor_parser is unavailable; install the apparmor package")
    install_rules(PROFILE_DIR)
    subprocess.run([apparmor_parser, "-Q", "-K", str(profile)], check=True)
    if PROFILES.exists():
        subprocess.run([apparmor_parser, "-r", "-T", str(profile)], check=True)
        print("AppArmor: installed scoped PS/PDF allowance and reloaded gs only")
    else:
        print("AppArmor: allowance configured on disk; kernel policy interface is unavailable")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"Ghostscript AppArmor setup failed: {error}")
