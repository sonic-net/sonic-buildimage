"""Use synthetic keys and local tools to check signing-material ownership."""

import base64
import json
from pathlib import Path
import signal
import subprocess
import textwrap

import pytest
import yaml


ROOT = Path(__file__).resolve().parents[1]


def template_script(name, display_name):
    template = yaml.safe_load((ROOT / ".azure-pipelines" / name).read_text())
    return next(
        step["script"] for step in template["steps"]
        if step.get("displayName") == display_name
    )


@pytest.fixture(scope="module")
def synthetic_pfx(tmp_path_factory):
    directory = tmp_path_factory.mktemp("synthetic-signing-input")
    key, cert, pfx = [directory / name for name in ("key.pem", "cert.pem", "cert.pfx")]
    subprocess.run(
        ["openssl", "req", "-new", "-x509", "-newkey", "rsa:2048", "-nodes",
         "-keyout", str(key), "-out", str(cert), "-subj", "/CN=synthetic-fixture", "-days", "1"],
        check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
    )
    subprocess.run(
        ["openssl", "pkcs12", "-export", "-inkey", str(key), "-in", str(cert),
         "-out", str(pfx), "-passout", "pass:"],
        check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
    )
    return base64.b64encode(pfx.read_bytes()).decode()


@pytest.fixture
def certificate(tmp_path, synthetic_pfx):
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    temporary = tmp_path / "temporary"
    temporary.mkdir()
    env = {
        "PATH": f"{bin_dir}:/usr/bin:/bin",
        "HOME": str(tmp_path),
        "TMPDIR": str(temporary),
        "AGENT_TEMPDIRECTORY": str(temporary),
        "CERTIFICATE": synthetic_pfx,
        "PUBLIC_CERTIFICATE_PATH": str(tmp_path / "artifacts/public.pem"),
        "SIGNING_TEST_ROOT": str(tmp_path),
        "SIGNING_TEST_LOG": str(tmp_path / "openssl.jsonl"),
    }
    sudo = bin_dir / "sudo"
    sudo.write_text(textwrap.dedent("""\
        #!/usr/bin/python3
        import os
        from pathlib import Path
        import sys

        args = sys.argv[1:]
        # Emulate only the old template's operations; never touch /etc.
        if args == ["mkdir", "-p", "/etc/certificates"]:
            sys.exit(0)
        if len(args) == 2 and args[0] == "tee":
            path = Path(args[1])
            if path.parent == Path(os.environ["SIGNING_TEST_ROOT"]):
                path.write_bytes(sys.stdin.buffer.read())
                sys.exit(0)
        sys.exit("Unexpected privileged operation in signing fixture")
        """))
    sudo.chmod(0o755)
    openssl = bin_dir / "openssl"
    openssl.write_text(textwrap.dedent("""\
        #!/usr/bin/python3
        import json
        import os
        import signal
        import subprocess
        import sys

        args = sys.argv[1:]
        with open(os.environ["SIGNING_TEST_LOG"], "a") as log:
            log.write(json.dumps({
                "args": args,
                "certificate_exported": any(
                    key in os.environ for key in ("CERTIFICATE", "certificate")
                ),
            }) + "\\n")
        if args[0] == "pkcs12" and os.environ.get("SIGNING_TEST_SIGNAL"):
            os.kill(os.getppid(), int(os.environ["SIGNING_TEST_SIGNAL"]))
        result = subprocess.run(["/usr/bin/openssl", *args])
        if os.environ.get("SIGNING_TEST_FAIL") in args:
            sys.exit(37)
        sys.exit(result.returncode)
        """))
    openssl.chmod(0o755)

    def run(**extra_env):
        script = template_script("azure-pipelines-download-certificate.yml", "Save certificate")
        script = script.replace("$(Build.StagingDirectory)", str(tmp_path / "artifacts"))
        for name in ("SIGNING_KEY", "SIGNING_CERT"):
            script = script.replace(f"$({name})", str(tmp_path / f"old-{name}.pem"))
        result = subprocess.run(
            ["bash", "-c", script], cwd=tmp_path, env={**env, **extra_env},
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=15,
        )
        variables = {}
        for line in result.stdout.splitlines():
            prefix = "##vso[task.setvariable variable="
            if line.startswith(prefix):
                name, value = line[len(prefix):].split("]", 1)
                variables[name.rstrip(";")] = value
        assert synthetic_pfx not in result.stdout + result.stderr
        return result, variables

    return run, env, temporary


def test_certificate_is_valid_private_and_job_owned(certificate):
    run, env, temporary = certificate
    result, variables = run()
    assert result.returncode == 0, result.stderr
    directory = Path(variables["SONIC_SIGNING_DIR"])
    key = Path(variables["SIGNING_KEY"])
    cert = Path(variables["SIGNING_CERT"])
    assert directory.parent == temporary
    assert directory.stat().st_mode & 0o777 == 0o700
    assert key.parent == directory
    assert str(cert) == env["PUBLIC_CERTIFICATE_PATH"]
    assert key.stat().st_mode & 0o777 == 0o600
    assert cert.stat().st_mode & 0o777 == 0o644
    assert not list(directory.glob("*.pfx"))
    records = [json.loads(line) for line in Path(env["SIGNING_TEST_LOG"]).read_text().splitlines()]
    assert not any(record["certificate_exported"] for record in records)
    public_key = subprocess.check_output(["openssl", "pkey", "-in", str(key), "-pubout"])
    cert_key = subprocess.check_output(["openssl", "x509", "-in", str(cert), "-pubkey", "-noout"])
    assert public_key == cert_key
    cleanup = template_script("azure-pipelines-cleanup-signing.yml", "Remove signing material")
    result = subprocess.run(
        ["bash", "-c", cleanup], env={**env, "SIGNING_MATERIAL_DIR": str(directory)},
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
    )
    assert result.returncode == 0, result.stderr
    assert not directory.exists()
    assert cert.is_file()


@pytest.mark.parametrize("value", ["not-valid-base64!", base64.b64encode(b"not a PFX").decode()])
def test_invalid_certificate_material_is_removed(certificate, value):
    run, _, temporary = certificate
    result, variables = run(CERTIFICATE=value)
    assert result.returncode != 0
    assert "SIGNING_KEY" not in variables
    assert not list(temporary.iterdir())


def test_upstream_openssl_failure_is_not_hidden_by_a_pipeline(certificate):
    run, _, temporary = certificate
    result, variables = run(SIGNING_TEST_FAIL="-clcerts")
    assert result.returncode == 37, result.stderr
    assert "SIGNING_KEY" not in variables
    assert not list(temporary.iterdir())


def test_cancelled_certificate_extraction_is_cleaned(certificate):
    run, _, temporary = certificate
    result, _ = run(SIGNING_TEST_SIGNAL=str(int(signal.SIGTERM)))
    assert result.returncode == 143, result.stderr
    assert not list(temporary.iterdir())


def test_job_cleanup_failure_is_visible_and_can_be_retried(certificate):
    run, env, _ = certificate
    result, variables = run()
    assert result.returncode == 0, result.stderr
    directory = Path(variables["SONIC_SIGNING_DIR"])
    remove = Path(env["HOME"]) / "bin/rm"
    remove.write_text("#!/bin/bash\nexit 44\n")
    remove.chmod(0o755)
    script = template_script("azure-pipelines-cleanup-signing.yml", "Remove signing material")
    cleanup_env = {**env, "SIGNING_MATERIAL_DIR": str(directory)}
    failed = subprocess.run(
        ["bash", "-c", script], env=cleanup_env, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
    )
    assert failed.returncode == 44
    assert directory.is_dir()
    remove.unlink()
    retried = subprocess.run(
        ["bash", "-c", script], env=cleanup_env, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
    )
    assert retried.returncode == 0, retried.stderr
    assert not directory.exists()
    assert Path(variables["SIGNING_CERT"]).is_file()


@pytest.mark.parametrize("symlink", [False, True])
def test_signing_cleanup_refuses_unowned_paths(certificate, tmp_path, symlink):
    _, env, temporary = certificate
    unrelated = tmp_path / "unrelated"
    unrelated.mkdir()
    marker = unrelated / "keep"
    marker.write_text("unrelated fixture")
    directory = unrelated
    if symlink:
        directory = temporary / "sonic-signing.link"
        directory.symlink_to(unrelated, target_is_directory=True)
    script = template_script("azure-pipelines-cleanup-signing.yml", "Remove signing material")
    result = subprocess.run(
        ["bash", "-c", script], env={**env, "SIGNING_MATERIAL_DIR": str(directory)},
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
    )
    assert result.returncode != 0
    assert marker.read_text() == "unrelated fixture"


@pytest.fixture
def module_signer(tmp_path):
    temporary = tmp_path / "temporary"
    temporary.mkdir()
    modules = tmp_path / "modules"
    modules.mkdir()
    (modules / "driver.ko").write_text("fixture\n~Module signature appended~\n")
    key, cert = tmp_path / "input.key", tmp_path / "input.crt"
    key.write_text("synthetic private module key")
    key.chmod(0o600)
    cert.write_text("synthetic public module certificate")
    log = tmp_path / "module-tools.jsonl"
    tool_source = textwrap.dedent("""\
        #!/usr/bin/python3
        import json
        import os
        from pathlib import Path
        import signal
        import sys

        tool = Path(sys.argv[0]).name
        args = sys.argv[1:]
        key = Path(args[0] if tool == "extract-cert" else args[1])
        with open(os.environ["SIGNING_TEST_LOG"], "a") as log:
            log.write(json.dumps({
                "tool": tool, "key": str(key),
                "key_mode": key.stat().st_mode & 0o777,
                "directory_mode": key.parent.stat().st_mode & 0o777,
            }) + "\\n")
        if os.environ.get("SIGNING_TEST_SIGNAL") and tool == "sign-file":
            os.kill(os.getppid(), int(os.environ["SIGNING_TEST_SIGNAL"]))
        if os.environ.get("SIGNING_TEST_FAIL") == tool:
            sys.exit(37)
        if tool == "extract-cert":
            Path(args[1]).write_text("synthetic extracted certificate")
        else:
            with open(args[-1], "a") as module:
                module.write("~Module signature appended~\\n")
        """)
    for tool in ("sign-file", "extract-cert"):
        path = tmp_path / tool
        path.write_text(tool_source)
        path.chmod(0o755)
    env = {
        "PATH": "/usr/bin:/bin", "HOME": str(tmp_path), "TMPDIR": str(temporary),
        "SIGNING_TEST_LOG": str(log),
    }

    def run(**extra_env):
        source = (ROOT / "scripts/signing_kernel_modules.sh").read_text()
        # Redirect only the legacy shared path when comparing the old source.
        source = source.replace(
            'dev_certs_tmp_folder="/tmp/dev_kmod_sign"',
            'dev_certs_tmp_folder="$TMPDIR/legacy-shared-signing"',
        )
        result = subprocess.run(
            ["bash", "-s", "--", "-l", "fixture", "-c", str(cert), "-p", str(key),
             "-k", str(modules), "-s", str(tmp_path / "sign-file"),
             "-e", str(tmp_path / "extract-cert")],
            input=source, cwd=tmp_path, env={**env, **extra_env}, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
        )
        records = [json.loads(line) for line in log.read_text().splitlines()] if log.exists() else []
        assert key.read_text() == "synthetic private module key"
        assert key.read_text() not in result.stdout + result.stderr
        return result, records

    return run, temporary, modules


@pytest.mark.parametrize("failure", ["", "extract-cert", "sign-file"])
def test_module_key_copies_are_private_cleaned_and_fail_closed(module_signer, failure):
    run, temporary, _ = module_signer
    result, records = run(SIGNING_TEST_FAIL=failure)
    assert result.returncode == (37 if failure else 0), result.stdout + result.stderr
    assert records
    for record in records:
        assert record["key_mode"] == 0o600
        assert record["directory_mode"] == 0o700
        assert not Path(record["key"]).exists()
    assert not list(temporary.iterdir())
    if failure:
        assert "SIGNED OK" not in result.stdout


def test_module_signing_cancellation_removes_key_copies(module_signer):
    run, temporary, _ = module_signer
    result, _ = run(SIGNING_TEST_SIGNAL=str(int(signal.SIGTERM)))
    assert result.returncode == 143, result.stderr
    assert not list(temporary.iterdir())


def test_module_signing_preserves_paths_with_spaces(module_signer):
    run, _, modules = module_signer
    (modules / "driver.ko").rename(modules / "driver with spaces.ko")
    result, records = run()
    assert result.returncode == 0, result.stderr
    assert [record["tool"] for record in records] == ["extract-cert", "sign-file"]


@pytest.fixture
def image_signer(tmp_path):
    temporary = tmp_path / "temporary"
    temporary.mkdir()
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    log = tmp_path / "image-tools.jsonl"
    for name in ("openssl", "swi-signature"):
        tool = bin_dir / name
        tool.write_text(textwrap.dedent("""\
            #!/usr/bin/python3
            import json
            import os
            from pathlib import Path
            import signal
            import sys

            name, args = Path(sys.argv[0]).name, sys.argv[1:]
            record = {"tool": name, "command": args[0]}
            if name == "openssl":
                output = Path(args[args.index("-out") + 1])
                output.write_text("synthetic generated signing material")
                record["output"] = str(output)
                record["mode"] = output.stat().st_mode & 0o777
            elif args[0] == "sign":
                key = Path(args[args.index("--key") + 1])
                record["key"] = str(key)
                record["mode"] = key.stat().st_mode & 0o777
            with open(os.environ["SIGNING_TEST_LOG"], "a") as log:
                log.write(json.dumps(record) + "\\n")
            if name == "swi-signature" and os.environ.get("SIGNING_TEST_SIGNAL"):
                os.kill(os.getppid(), int(os.environ["SIGNING_TEST_SIGNAL"]))
            if os.environ.get("SIGNING_TEST_FAIL") in (name, args[0]):
                sys.exit(37)
            """))
        tool.chmod(0o755)
    env = {
        "PATH": f"{bin_dir}:/usr/bin:/bin", "HOME": str(tmp_path),
        "TMPDIR": str(temporary), "SIGNING_TEST_LOG": str(log),
    }

    def run(existing=False, **extra_env):
        key, cert, ca = [tmp_path / name for name in ("caller.key", "caller.crt", "ca.crt")]
        if existing:
            for path in (key, cert, ca):
                path.write_text("synthetic caller-owned signing material")
                path.chmod(0o600)
        result = subprocess.run(
            [str(ROOT / "scripts/sign_image.sh"), "-i", str(tmp_path / "image.swi"),
             "-k", str(key), "-c", str(cert), "-a", str(ca)],
            cwd=tmp_path, env={**env, **extra_env}, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
        )
        records = [json.loads(line) for line in log.read_text().splitlines()]
        assert "synthetic generated signing material" not in result.stdout + result.stderr
        if existing:
            assert key.read_text() == "synthetic caller-owned signing material"
        return result, records

    return run, temporary


@pytest.mark.parametrize("failure", ["", "openssl", "prepare", "sign"])
def test_development_image_keys_are_private_and_always_removed(image_signer, failure):
    run, temporary = image_signer
    result, records = run(SIGNING_TEST_FAIL=failure)
    assert result.returncode == (37 if failure else 0), result.stderr
    private = [
        record for record in records
        if record.get("output", "").endswith(("signing.key", "ca.key"))
    ]
    assert private
    assert all(record["mode"] == 0o600 for record in private)
    assert not list(temporary.iterdir())


def test_image_signing_keeps_caller_owned_keys(image_signer):
    run, temporary = image_signer
    result, records = run(existing=True)
    assert result.returncode == 0, result.stderr
    assert all(record["tool"] != "openssl" for record in records)
    assert not list(temporary.iterdir())


def test_cancelled_image_signing_removes_generated_keys(image_signer):
    run, temporary = image_signer
    result, _ = run(SIGNING_TEST_SIGNAL=str(int(signal.SIGTERM)))
    assert result.returncode == 143, result.stderr
    assert not list(temporary.iterdir())
