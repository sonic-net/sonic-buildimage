import importlib.util
import os
from pathlib import Path
import stat
import subprocess
import sys

import pytest
import yaml


SCRIPT = Path(__file__).resolve().parents[1] / "generate_secureboot_vs_artifacts.py"
SPEC = importlib.util.spec_from_file_location("generate_secureboot_vs_artifacts", SCRIPT)
artifacts = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(artifacts)


@pytest.fixture
def env(tmp_path, monkeypatch):
    workdir = tmp_path / "keys with spaces"
    template = tmp_path / "clean vars.fd"
    template.write_bytes(b"OVMF template")
    output = tmp_path / "output" / "provisioned vars.fd"
    state = {
        "workdir": workdir, "template": template, "output": output,
        "calls": [], "failure": None, "error": None, "missing_marker": None,
        "build_id": "test-build", "real_openssl": False,
    }
    real_run = subprocess.run

    def run(command, **kwargs):
        assert kwargs["check"] is True
        assert kwargs["cwd"] == workdir
        state["calls"].append(command)
        stage = "inspect" if "--print" in command else command[0]
        if state["failure"] == stage:
            if state["error"] is not None:
                raise state["error"]
            raise subprocess.CalledProcessError(7, command)
        if command[0] == "openssl" and state["real_openssl"]:
            return real_run(command, **kwargs)
        if command[0] == "openssl" and "-out" in command:
            (workdir / command[command.index("-out") + 1]).write_bytes(b"generated")
            if "-keyout" in command:
                (workdir / command[command.index("-keyout") + 1]).write_bytes(b"generated key")
        elif command[0] in ("cert-to-efi-sig-list", "sign-efi-sig-list"):
            (workdir / command[-1]).write_bytes(b"generated")
        elif command[0] == "virt-fw-vars" and "--output" in command:
            Path(command[command.index("--output") + 1]).write_bytes(b"OVMF variables")
        markers = [f"subject CN=SONiC CI {name} build {state['build_id']}" for name in ("PK", "KEK", "DB")]
        markers += ["name=SecureBootEnable", "bool: ON"]
        stdout = "\n".join(marker for marker in markers if marker != state["missing_marker"])
        return subprocess.CompletedProcess(command, 0, stdout)

    monkeypatch.setattr(artifacts.subprocess, "run", run)
    state["argv"] = [
        "--workdir", str(workdir), "--output-vars", str(output),
        "--ovmf-vars", str(template), "--build-id", state["build_id"],
    ]
    return state


def test_generation_preserves_hierarchy_updates_and_permissions(env, capsys):
    cwd = Path.cwd()
    assert artifacts.main(env["argv"]) == 0
    assert Path.cwd() == cwd
    calls = env["calls"]
    for name in ("PK", "KEK", "DB"):
        assert ["cert-to-efi-sig-list", f"{name}.crt", f"{name}.esl"] in calls
        assert [
            "openssl", "verify", "-check_ss_sig", "-CAfile", f"{name}.crt", f"{name}.crt",
        ] in calls
        assert stat.S_IMODE((env["workdir"] / f"{name}.key").stat().st_mode) == 0o600
        for suffix in ("crt", "auth"):
            assert stat.S_IMODE((env["workdir"] / f"{name}.{suffix}").stat().st_mode) == 0o644
    assert stat.S_IMODE(env["output"].stat().st_mode) == 0o644
    assert ["sign-efi-sig-list", "-k", "PK.key", "-c", "PK.crt", "PK", "PK.esl", "PK.auth"] in calls
    assert ["sign-efi-sig-list", "-k", "PK.key", "-c", "PK.crt", "KEK", "KEK.esl", "KEK.auth"] in calls
    assert ["sign-efi-sig-list", "-k", "KEK.key", "-c", "KEK.crt", "db", "DB.esl", "DB.auth"] in calls
    assert calls[-2] == [
        "virt-fw-vars", "--input", str(env["template"]),
        "--set-pk", artifacts.DEFAULT_OWNER_GUID, "PK.crt",
        "--add-kek", artifacts.DEFAULT_OWNER_GUID, "KEK.crt",
        "--add-db", artifacts.DEFAULT_OWNER_GUID, "DB.crt",
        "--secure-boot", "--output", str(env["output"]),
    ]
    assert calls[-1] == ["virt-fw-vars", "--input", str(env["output"]), "--print", "--verbose"]
    assert "Secure Boot build inputs generated" in capsys.readouterr().out


def test_real_openssl_certificates_are_self_signed(env):
    env["real_openssl"] = True
    assert artifacts.main(env["argv"]) == 0
    for name, constraint, usage in (
        ("PK", "CA:TRUE", "Certificate Sign, CRL Sign"),
        ("KEK", "CA:TRUE, pathlen:0", "Certificate Sign, CRL Sign"),
        ("DB", "CA:FALSE", "Digital Signature"),
    ):
        result = artifacts.run([
            "openssl", "x509", "-in", f"{name}.crt", "-noout", "-text",
        ], env["workdir"], capture_output=True, text=True)
        assert f"SONiC CI {name} build test-build" in result.stdout
        assert "Public-Key: (4096 bit)" in result.stdout
        assert constraint in result.stdout
        assert usage in result.stdout
        identity = artifacts.run([
            "openssl", "x509", "-in", f"{name}.crt", "-noout", "-subject", "-issuer",
        ], env["workdir"], capture_output=True, text=True).stdout.splitlines()
        assert identity[0].removeprefix("subject=") == identity[1].removeprefix("issuer=")
        certificate_key = artifacts.run([
            "openssl", "x509", "-in", f"{name}.crt", "-noout", "-pubkey",
        ], env["workdir"], capture_output=True, text=True).stdout
        private_key = artifacts.run([
            "openssl", "pkey", "-in", f"{name}.key", "-pubout",
        ], env["workdir"], capture_output=True, text=True).stdout
        assert certificate_key == private_key


def test_reuse_does_not_require_or_replace_private_keys_or_auth_files(env, capsys):
    env["workdir"].mkdir()
    for name in ("PK", "KEK", "DB"):
        (env["workdir"] / f"{name}.crt").write_bytes(b"existing certificate")
    assert artifacts.main(env["argv"] + ["--reuse-keys"]) == 0
    assert len(env["calls"]) == 5
    for name in ("PK", "KEK", "DB"):
        assert (env["workdir"] / f"{name}.crt").read_bytes() == b"existing certificate"
        assert not (env["workdir"] / f"{name}.key").exists()
        assert not (env["workdir"] / f"{name}.auth").exists()
    assert "Reused Secure Boot certificates" in capsys.readouterr().out


@pytest.mark.parametrize("missing", ["PK", "KEK", "DB"])
def test_reuse_requires_all_three_certificates(env, capsys, missing):
    env["workdir"].mkdir()
    for name in ("PK", "KEK", "DB"):
        if name != missing:
            (env["workdir"] / f"{name}.crt").touch()
    assert artifacts.main(env["argv"] + ["--reuse-keys"]) == 1
    assert f"Missing certificate for --reuse-keys: {env['workdir'] / (missing + '.crt')}" in capsys.readouterr().err
    assert not env["calls"]
    assert not env["output"].exists()


def test_missing_template_fails_before_generating_inputs(env, capsys):
    env["template"].unlink()
    assert artifacts.main(env["argv"]) == 1
    assert "OVMF variable-store template is missing" in capsys.readouterr().err
    assert not env["calls"]
    assert not env["workdir"].exists()


@pytest.mark.parametrize("stage", ["openssl", "cert-to-efi-sig-list", "sign-efi-sig-list", "virt-fw-vars", "inspect"])
def test_external_command_failures_are_fatal_and_reported(env, capsys, stage):
    env["failure"] = stage
    assert artifacts.main(env["argv"]) == 7
    captured = capsys.readouterr()
    assert "ERROR:" in captured.err
    assert "non-zero exit status 7" in captured.err
    assert "Provisioned OVMF variables written" not in captured.out
    assert env["calls"][-1][0] == ("virt-fw-vars" if stage == "inspect" else stage)


def test_missing_tool_is_reported_without_an_availability_preflight(env, capsys, monkeypatch):
    import shutil

    monkeypatch.setattr(shutil, "which", lambda command: pytest.fail("unexpected tool availability check"))
    env["failure"] = "openssl"
    env["error"] = FileNotFoundError("openssl is missing")
    assert artifacts.main(env["argv"]) == 1
    assert "ERROR: openssl is missing" in capsys.readouterr().err
    assert len(env["calls"]) == 1


@pytest.mark.parametrize("marker", [
    "subject CN=SONiC CI PK build test-build",
    "subject CN=SONiC CI KEK build test-build",
    "subject CN=SONiC CI DB build test-build",
    "name=SecureBootEnable", "bool: ON",
])
def test_provisioned_store_must_report_each_certificate_and_secure_boot(env, capsys, marker):
    env["missing_marker"] = marker
    assert artifacts.main(env["argv"]) == 1
    captured = capsys.readouterr()
    assert f"missing expected content: {marker}" in captured.err
    assert "Provisioned OVMF variables written" not in captured.out


@pytest.mark.parametrize("failure", [None, "openssl"])
def test_process_umask_and_directory_are_restored(env, failure):
    cwd = Path.cwd()
    env["failure"] = failure
    previous = os.umask(0o022)
    try:
        assert artifacts.main(env["argv"]) == (7 if failure else 0)
        assert os.umask(0o022) == 0o022
        assert Path.cwd() == cwd
    finally:
        os.umask(previous)


@pytest.mark.parametrize("build_id", [None, "", "environment-build"])
def test_build_id_defaults_match_the_shell_command(env, monkeypatch, build_id):
    if build_id is None:
        monkeypatch.delenv("BUILD_BUILDID", raising=False)
    else:
        monkeypatch.setenv("BUILD_BUILDID", build_id)
    env["build_id"] = build_id or "local"
    assert artifacts.main(env["argv"][:-2]) == 0
    assert f"/CN=SONiC CI PK build {env['build_id']}/" in env["calls"][0]


def test_owner_guid_override_is_passed_to_every_firmware_variable(env):
    owner = "12345678-1234-1234-1234-123456789abc"
    assert artifacts.main(env["argv"] + ["--owner-guid", owner]) == 0
    command = env["calls"][-2]
    for flag in ("--set-pk", "--add-kek", "--add-db"):
        assert command[command.index(flag) + 1] == owner


@pytest.mark.parametrize("args", [
    [], ["--workdir"], ["--workdir", "", "--output-vars", "output"], ["--unknown"],
])
def test_invalid_arguments_keep_exit_code_two(args):
    with pytest.raises(SystemExit) as error:
        artifacts.main(args)
    assert error.value.code == 2


@pytest.mark.parametrize("suffix", ["py", "sh"])
def test_canonical_and_legacy_commands_preserve_help_and_forward_arguments(tmp_path, suffix):
    script = SCRIPT.with_suffix(f".{suffix}")
    command = [sys.executable, str(script)] if suffix == "py" else ["bash", str(script)]
    help_result = subprocess.run(command + ["--help"], capture_output=True, text=True, timeout=10)
    assert help_result.returncode == 0
    assert "--reuse-keys" in help_result.stdout
    missing = tmp_path / "missing template.fd"
    result = subprocess.run(command + [
        "--workdir", str(tmp_path / "work dir"), "--output-vars", str(tmp_path / "out.fd"),
        "--ovmf-vars", str(missing),
    ], capture_output=True, text=True, timeout=10)
    assert result.returncode == 1
    assert str(missing) in result.stderr


@pytest.mark.parametrize("mismatched_commit", [False, True])
def test_pipeline_fetches_and_verifies_triggering_pr_before_signing(tmp_path, mismatched_commit):
    """Fetch PR code from a different repository and reject a moved ref before checkout."""
    pipeline = yaml.safe_load(
        (SCRIPT.parents[1] / ".azure-pipelines/azure-pipelines-build-secureboot-vs.yml").read_text()
    )
    steps = pipeline["stages"][0]["jobs"][0]["parameters"]["preSteps"]
    source_step = next(
        step for group in steps for value in group.values() if isinstance(value, list)
        for step in value if step.get("displayName") == "Checkout triggering PR revision"
    )

    def git(directory, *args):
        return subprocess.run(
            ["git", "-C", str(directory), *args], check=True, capture_output=True, text=True
        ).stdout.strip()

    source = tmp_path / "source repo"
    target = tmp_path / "pipeline repo"
    for directory, contents in ((source, "PR code"), (target, "pipeline code")):
        directory.mkdir()
        git(directory, "init", "-b", "main")
        (directory / "marker").write_text(contents)
        git(directory, "add", "marker")
        git(directory, "-c", "user.name=Test", "-c", "user.email=test@example.com", "commit", "-m", "fixture")
    source_commit = git(source, "rev-parse", "HEAD")
    original_commit = git(target, "rev-parse", "HEAD")
    env = dict(os.environ, SOURCE_REPOSITORY=str(source), SOURCE_REF="refs/heads/main",
               SOURCE_COMMIT=original_commit if mismatched_commit else source_commit)
    result = subprocess.run(
        ["bash", "-c", source_step["bash"]], cwd=target, env=env, capture_output=True, text=True, timeout=30
    )
    if mismatched_commit:
        assert result.returncode != 0
        assert git(target, "rev-parse", "HEAD") == original_commit
        assert (target / "marker").read_text() == "pipeline code"
    else:
        assert result.returncode == 0, result.stderr
        assert git(target, "rev-parse", "HEAD") == source_commit
        assert (target / "marker").read_text() == "PR code"


def test_pipeline_reuses_workdir_without_preliminary_vars_file(tmp_path):
    pipeline_dir = SCRIPT.parents[1] / ".azure-pipelines"
    pipeline = yaml.safe_load(
        (pipeline_dir / "azure-pipelines-build-secureboot-vs.yml").read_text()
    )
    variables = pipeline["stages"][0]["jobs"][0]["parameters"]["jobGroups"][0]["variables"]
    workdir = variables["secure_boot_workdir"].replace("$(Build.SourcesDirectory)", str(tmp_path))
    Path(workdir).mkdir()
    (tmp_path / "target").mkdir()
    (tmp_path / "target/sonic-vs.img-uefi_vars.fd").touch()
    shared = yaml.safe_load((pipeline_dir / "azure-pipelines-build.yml").read_text())
    steps = shared["jobs"][0]["parameters"]["buildSteps"]
    script = next(step["bash"] for step in steps if "bash" in step)
    for name, value in {
        "GROUP_NAME": "vs", "PLATFORM_ARCH": "amd64", "dbg_image": "no",
        "asan_image": "no", "K8S_MASTER_CHANGED": "NO", "secure_boot_image": "yes",
        "secure_boot_workdir": workdir, "Build.BuildId": "test-build",
        "Build.Reason": "PullRequest",
    }.items():
        script = script.replace(f"$({name})", value)
    result = subprocess.run(
        ["bash", "-c", 'make() { :; }\npython3() { printf "%s\\n" "$@" > invocation; }\n' + script],
        cwd=tmp_path, env=dict(os.environ, BUILD_OPTIONS=""), capture_output=True, text=True, timeout=10,
    )
    assert result.returncode == 0, result.stderr
    assert (tmp_path / "invocation").read_text().splitlines() == [
        "scripts/generate_secureboot_vs_artifacts.py",
        "--workdir", workdir,
        "--ovmf-vars", "target/sonic-vs.img-uefi_vars.fd",
        "--output-vars", "target/sonic-vs_vars.fd",
        "--build-id", "test-build", "--reuse-keys",
    ]
