"""Exercise registry publishing without accessing Docker or real credentials."""

import hashlib
import json
from pathlib import Path
import secrets
import signal
import subprocess
import textwrap

import pytest
import yaml


ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture
def registry(tmp_path):
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    temporary = tmp_path / "temporary"
    temporary.mkdir()
    caller_config = tmp_path / "caller-config"
    caller_config.mkdir()
    (caller_config / "config.json").write_text("caller authentication")
    image = tmp_path / "docker-fixture.gz"
    image.write_bytes(b"synthetic image archive")
    log = tmp_path / "docker.jsonl"
    password = secrets.token_hex(24)
    docker = bin_dir / "docker"
    docker.write_text(textwrap.dedent("""\
        #!/usr/bin/python3
        import hashlib
        import json
        import os
        from pathlib import Path
        import signal
        import sys

        args = sys.argv[1:]
        config = Path(os.environ["DOCKER_CONFIG"])
        record = {
            "args": args,
            "config": str(config),
            "mode": config.stat().st_mode & 0o777,
            "password_exported": any(
                name in os.environ for name in ("REGISTRY_PASSWD", "registry_password")
            ),
        }
        if args[0] == "login":
            record["password_digest"] = hashlib.sha256(sys.stdin.buffer.read()).hexdigest()
            (config / "config.json").write_text("synthetic authentication")
        with open(os.environ["DOCKER_TEST_LOG"], "a") as output:
            output.write(json.dumps(record) + "\\n")
        if args[0] == os.environ.get("DOCKER_TEST_SIGNAL_COMMAND"):
            os.kill(os.getppid(), int(os.environ["DOCKER_TEST_SIGNAL"]))
        if args[0] == os.environ.get("DOCKER_TEST_FAIL"):
            if os.environ.get("DOCKER_TEST_FAIL_MATCH", "") in " ".join(args):
                sys.exit(37)
        if args[0] == "push":
            print("latest: digest: sha256:abcd size: 123")
        """))
    docker.chmod(0o755)
    for name in ("cat", "grep", "ls", "mkdir", "mv"):
        tool = bin_dir / name
        tool.write_text(textwrap.dedent("""\
            #!/usr/bin/python3
            import json
            import os
            from pathlib import Path
            import sys

            name = Path(sys.argv[0]).name
            with open(os.environ["CREDENTIAL_SCOPE_LOG"], "a") as log:
                log.write(json.dumps({
                    "password_exported": any(
                        name in os.environ for name in ("REGISTRY_PASSWD", "registry_password")
                    ),
                }) + "\\n")
            os.execv("/usr/bin/" + name, [name, *sys.argv[1:]])
            """))
        tool.chmod(0o755)
    env = {
        "PATH": f"{bin_dir}:/usr/bin:/bin",
        "HOME": str(tmp_path),
        "TMPDIR": str(temporary),
        "DOCKER_CONFIG": str(caller_config),
        "REGISTRY_PASSWD": password,
        "DOCKER_TEST_LOG": str(log),
        "CREDENTIAL_SCOPE_LOG": str(tmp_path / "scope.jsonl"),
        "BUILD_NUMBER": "42",
    }

    def run(arguments=None, **extra_env):
        if log.exists():
            log.unlink()
        result = subprocess.run(
            ["bash", "-x", str(ROOT / "push_docker.sh")]
            + (arguments if arguments is not None else
               [str(image), "registry.example", "", "fixture-user", "first second"]),
            cwd=tmp_path, env={**env, **extra_env}, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
        )
        records = [json.loads(line) for line in log.read_text().splitlines()] if log.exists() else []
        assert password not in result.stdout + result.stderr
        assert (caller_config / "config.json").read_text() == "caller authentication"
        return result, records

    return run, env, image, temporary, bin_dir


def assert_private_configuration(records):
    assert records
    assert len({record["config"] for record in records}) == 1
    for record in records:
        assert record["mode"] == 0o700
        assert not record["password_exported"]
        assert not Path(record["config"]).exists()


def test_tags_empty_port_and_private_configuration(registry):
    run, env, _, temporary, _ = registry
    result, records = run(registry_password="previously exported")
    assert result.returncode == 0, result.stderr
    assert_private_configuration(records)
    login = next(record for record in records if record["args"][0] == "login")
    assert login["args"] == [
        "login", "--username", "fixture-user", "--password-stdin", "registry.example",
    ]
    assert login["password_digest"] == hashlib.sha256(env["REGISTRY_PASSWD"].encode()).hexdigest()
    pushes = [record["args"][1] for record in records if record["args"][0] == "push"]
    assert pushes[:2] == ["registry.example/docker-fixture:first", "registry.example/docker-fixture:second"]
    assert pushes[2].endswith(".bld-42")
    assert not list(temporary.iterdir())


def test_version_and_platform_without_optional_tags(registry):
    run, _, image, _, _ = registry
    result, records = run([
        "-v", "20261106.1", "-p", "vs", str(image), "registry.example", "443", "fixture-user",
    ])
    assert result.returncode == 0, result.stderr
    assert [record["args"] for record in records if record["args"][0] == "push"] == [
        ["push", "registry.example:443/sonic-dockers/vs/docker-fixture:20261106.1"],
    ]
    assert_private_configuration(records)


@pytest.mark.parametrize("command", ["load", "login", "tag", "push"])
def test_docker_failures_propagate_and_remove_authentication(registry, command):
    run, _, _, temporary, _ = registry
    result, records = run(DOCKER_TEST_FAIL=command)
    assert result.returncode == 37, result.stdout + result.stderr
    assert "Job completed" not in result.stdout
    assert_private_configuration(records)
    assert not list(temporary.iterdir())


@pytest.mark.parametrize("signum", [signal.SIGINT, signal.SIGTERM])
def test_interruption_removes_private_configuration(registry, signum):
    run, _, _, temporary, _ = registry
    result, records = run(
        DOCKER_TEST_SIGNAL_COMMAND="load", DOCKER_TEST_SIGNAL=str(int(signum)),
    )
    assert result.returncode == 128 + signum, result.stderr
    assert_private_configuration(records)
    assert not list(temporary.iterdir())


@pytest.mark.parametrize("arguments", [
    ["IMAGE", "server", "", "user", "OLD_PASSWORD", "tags"],
    ["-z", "IMAGE", "server", "", "user"],
    ["-v"],
    ["IMAGE", "server", "user"],
])
def test_ambiguous_or_incomplete_arguments_fail_before_docker(registry, arguments):
    run, env, image, temporary, _ = registry
    arguments = [
        {"IMAGE": str(image), "OLD_PASSWORD": env["REGISTRY_PASSWD"]}.get(arg, arg)
        for arg in arguments
    ]
    result, records = run(arguments)
    assert result.returncode != 0
    assert not records
    assert not list(temporary.iterdir())


def test_cleanup_failure_is_reported_without_hiding_original_failure(registry):
    run, _, _, _, bin_dir = registry
    remove = bin_dir / "rm"
    remove.write_text("#!/bin/bash\nexit 44\n")
    remove.chmod(0o755)
    for failure, expected in [("", 1), ("push", 37)]:
        result, _ = run(DOCKER_TEST_FAIL=failure)
        assert result.returncode == expected
        assert "Failed to remove temporary Docker configuration" in result.stderr


def test_logout_failure_warns_and_still_removes_authentication(registry):
    run, _, _, temporary, _ = registry
    result, records = run(DOCKER_TEST_FAIL="logout")
    assert result.returncode == 0, result.stderr
    assert "Warning: Docker logout failed" in result.stderr
    assert_private_configuration(records)
    assert not list(temporary.iterdir())


@pytest.mark.parametrize("failure", ["", "push"])
def test_build_caller_limits_credentials_to_publishing(registry, tmp_path, failure):
    _, env, _, _, _ = registry
    (tmp_path / "functions.sh").write_text(
        'trap_push() { :; }\ndocker_try_rmi() { docker rmi "$1"; }\n',
    )
    (tmp_path / "dockers/fixture").mkdir(parents=True)
    (tmp_path / "dockers/fixture/Dockerfile").write_text("FROM scratch\n")
    (tmp_path / "files").mkdir()
    helper = tmp_path / "push_docker.sh"
    helper.write_text((ROOT / "push_docker.sh").read_text())
    helper.chmod(0o755)
    result = subprocess.run(
        ["bash", "-x", str(ROOT / "build_docker.sh"), "-i", "fixture", "-t", "requested",
         "fixture", "registry.example", "", "fixture-user"],
        cwd=tmp_path, env={**env, "DOCKER_TEST_FAIL": failure,
                           "registry_password": "previously exported"},
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
    )
    assert result.returncode == (37 if failure else 0), result.stdout + result.stderr
    assert env["REGISTRY_PASSWD"] not in result.stdout + result.stderr
    records = [json.loads(line) for line in Path(env["DOCKER_TEST_LOG"]).read_text().splitlines()]
    assert not any(record["password_exported"] for record in records)
    login = next(record for record in records if record["args"][0] == "login")
    assert login["password_digest"] == hashlib.sha256(env["REGISTRY_PASSWD"].encode()).hexdigest()


@pytest.mark.parametrize("modified, password_present, expected_status", [
    (False, False, 0), (True, True, 37), (True, False, 1),
])
def test_ptf_caller_preserves_artifacts_and_publishing_status(
    registry, tmp_path, modified, password_present, expected_status,
):
    _, env, _, _, _ = registry
    target = tmp_path / "target"
    target.mkdir()
    (target / "docker-ptf.gz").write_bytes(b"synthetic image archive")
    if modified:
        (target / "docker-ptf.gz.log").write_text("synthetic uncached build")
    (tmp_path / ".arch").write_text("amd64")
    helper = tmp_path / "push_docker.sh"
    helper.write_text((ROOT / "push_docker.sh").read_text())
    helper.chmod(0o755)
    template = yaml.safe_load(
        (ROOT / ".azure-pipelines/azure-pipelines-image-template.yml").read_text(),
    )
    script = next(
        step["script"] for step in template["jobs"][0]["parameters"]["postSteps"]
        if step.get("displayName") == "Publish to Docker Registry and Copy Artifacts"
    )
    for macro, value in {
        "Build.Reason": "PullRequest",
        "Build.SourceBranchName": "master",
        "Build.ArtifactStagingDirectory": str(tmp_path / "artifacts"),
    }.items():
        script = script.replace(f"$({macro})", value)
    result = subprocess.run(
        ["bash", "-c", script], cwd=tmp_path, text=True,
        env={**env, "REGISTRY_SERVER_PUBLIC": "registry.example",
             "REGISTRY_USERNAME": "fixture-user", "DOCKER_TEST_FAIL": "push",
             "REGISTRY_PASSWD": env["REGISTRY_PASSWD"] if password_present else "",
             "registry_password": "previously exported",
             "BUILD_DEFINITIONNAME": "synthetic",
             "SYSTEM_PULLREQUEST_PULLREQUESTNUMBER": "123"},
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10,
    )
    assert result.returncode == expected_status, result.stdout + result.stderr
    assert (tmp_path / "artifacts/target/docker-ptf.gz").is_file()
    assert env["REGISTRY_PASSWD"] not in result.stdout + result.stderr
    records = [json.loads(line) for line in Path(env["CREDENTIAL_SCOPE_LOG"]).read_text().splitlines()]
    assert records
    assert not any(record["password_exported"] for record in records)
    if modified and not password_present:
        assert not Path(env["DOCKER_TEST_LOG"]).exists()
