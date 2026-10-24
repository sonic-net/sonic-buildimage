#!/usr/bin/env python3
"""Generate the ephemeral Secure Boot hierarchy and OVMF variables used by VS CI."""

import argparse
import os
import subprocess
import sys
from pathlib import Path


DEFAULT_OVMF_VARS = Path("/usr/share/OVMF/OVMF_VARS_4M.fd")
DEFAULT_OWNER_GUID = "8c7f3d10-9b67-4e17-8d8a-73e6f9725b1d"
CERTIFICATE_NAMES = ("PK", "KEK", "DB")


class ArtifactError(Exception):
    """Invalid inputs or an unexpected provisioned variable store."""


def run(command, workdir, **kwargs):
    """Run a required tool in the key directory, raising on failure."""
    return subprocess.run(command, cwd=workdir, check=True, **kwargs)


def generate_key_material(workdir, build_id):
    """Create PK -> KEK -> DB certificates and their authenticated EFI updates."""
    run(["openssl", "genrsa", "-out", "PK.key", "4096"], workdir)
    run([
        "openssl", "req", "-new", "-x509", "-sha256", "-days", "3650",
        "-key", "PK.key", "-out", "PK.crt", "-subj", f"/CN=SONiC CI PK build {build_id}/",
        "-addext", "basicConstraints=critical,CA:TRUE",
        "-addext", "keyUsage=critical,keyCertSign,cRLSign",
        "-addext", "subjectKeyIdentifier=hash",
    ], workdir)

    for name, issuer, days, section, constraints, usage, authority in (
        ("KEK", "PK", "3650", "v3_ca", "CA:TRUE,pathlen:0", "keyCertSign,cRLSign", "keyid:always,issuer"),
        ("DB", "KEK", "365", "v3_db", "CA:FALSE", "digitalSignature", "keyid,issuer"),
    ):
        run(["openssl", "genrsa", "-out", f"{name}.key", "4096"], workdir)
        (workdir / f"{name}.cnf").write_text(
            f"[ req ]\nprompt = no\ndistinguished_name = dn\n"
            f"[ dn ]\nCN = SONiC CI {name} build {build_id}\n"
            f"[ {section} ]\nbasicConstraints = critical,{constraints}\n"
            f"keyUsage = critical,{usage}\nsubjectKeyIdentifier = hash\n"
            f"authorityKeyIdentifier = {authority}\n",
            encoding="utf-8",
        )
        run([
            "openssl", "req", "-new", "-sha256", "-key", f"{name}.key",
            "-out", f"{name}.csr", "-config", f"{name}.cnf",
        ], workdir)
        run([
            "openssl", "x509", "-req", "-sha256", "-days", days, "-in", f"{name}.csr",
            "-CA", f"{issuer}.crt", "-CAkey", f"{issuer}.key", "-CAcreateserial",
            "-out", f"{name}.crt", "-extfile", f"{name}.cnf", "-extensions", section,
        ], workdir)

    for name, variable, issuer in (("PK", "PK", "PK"), ("KEK", "KEK", "PK"), ("DB", "db", "KEK")):
        run(["cert-to-efi-sig-list", f"{name}.crt", f"{name}.esl"], workdir)
        run([
            "sign-efi-sig-list", "-k", f"{issuer}.key", "-c", f"{issuer}.crt",
            variable, f"{name}.esl", f"{name}.auth",
        ], workdir)


def provision_variables(workdir, ovmf_vars, output_vars, build_id, owner_guid):
    """Enroll the hierarchy in an offline OVMF store and check its reported state."""
    run([
        "virt-fw-vars", "--input", str(ovmf_vars),
        "--set-pk", owner_guid, "PK.crt", "--add-kek", owner_guid, "KEK.crt",
        "--add-db", owner_guid, "DB.crt", "--secure-boot", "--output", str(output_vars),
    ], workdir)
    result = run([
        "virt-fw-vars", "--input", str(output_vars), "--print", "--verbose",
    ], workdir, stdout=subprocess.PIPE, text=True)
    expected = [f"subject CN=SONiC CI {name} build {build_id}" for name in CERTIFICATE_NAMES]
    expected += ["name=SecureBootEnable", "bool: ON"]
    for marker in expected:
        if marker not in result.stdout:
            raise ArtifactError(f"Provisioned OVMF variables are missing expected content: {marker}")


def generate_artifacts(workdir, output_vars, *, ovmf_vars=DEFAULT_OVMF_VARS,
                       build_id="local", owner_guid=DEFAULT_OWNER_GUID, reuse_keys=False):
    """Generate CI inputs without changing the caller's directory or file-creation mask."""
    workdir = Path(workdir).resolve()
    ovmf_vars = Path(ovmf_vars).resolve()
    output_vars = Path(output_vars).resolve()
    if not ovmf_vars.is_file():
        raise ArtifactError(f"OVMF variable-store template is missing: {ovmf_vars}")
    workdir.mkdir(parents=True, exist_ok=True)
    output_vars.parent.mkdir(parents=True, exist_ok=True)
    previous_umask = os.umask(0o077)
    try:
        if reuse_keys:
            for name in CERTIFICATE_NAMES:
                certificate = workdir / f"{name}.crt"
                if not certificate.is_file():
                    raise ArtifactError(f"Missing certificate for --reuse-keys: {certificate}")
        else:
            generate_key_material(workdir, build_id)

        run(["openssl", "verify", "-CAfile", "PK.crt", "KEK.crt"], workdir)
        run(["openssl", "verify", "-CAfile", "PK.crt", "-untrusted", "KEK.crt", "DB.crt"], workdir)
        provision_variables(workdir, ovmf_vars, output_vars, build_id, owner_guid)

        if not reuse_keys:
            for name in CERTIFICATE_NAMES:
                (workdir / f"{name}.key").chmod(0o600)
                (workdir / f"{name}.auth").chmod(0o644)
        for name in CERTIFICATE_NAMES:
            (workdir / f"{name}.crt").chmod(0o644)
        output_vars.chmod(0o644)
    finally:
        os.umask(previous_umask)

    if reuse_keys:
        print(f"Reused Secure Boot certificates from {workdir}")
    else:
        print(f"Secure Boot build inputs generated in {workdir}")
    print(f"Provisioned OVMF variables written to {output_vars}")


def path_argument(value):
    """Reject empty path options, matching the original command's required inputs."""
    if not value:
        raise argparse.ArgumentTypeError("path must not be empty")
    return Path(value)


def main(argv=None):
    """Parse the existing command-line options and report explicit generation errors."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workdir", required=True, type=path_argument)
    parser.add_argument("--output-vars", required=True, type=path_argument)
    parser.add_argument("--ovmf-vars", type=path_argument, default=DEFAULT_OVMF_VARS,
                        help="clean OVMF variable-store template")
    parser.add_argument("--build-id", default=os.environ.get("BUILD_BUILDID") or "local",
                        help="identifier included in certificate common names")
    parser.add_argument("--owner-guid", default=DEFAULT_OWNER_GUID,
                        help="signature owner GUID stored in UEFI signature lists")
    parser.add_argument("--reuse-keys", action="store_true",
                        help="reuse PK/KEK/DB certificates already in the work directory")
    args = parser.parse_args(argv)
    try:
        generate_artifacts(**vars(args))
        return 0
    except subprocess.CalledProcessError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return error.returncode if error.returncode > 0 else 128 - error.returncode
    except (ArtifactError, OSError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
