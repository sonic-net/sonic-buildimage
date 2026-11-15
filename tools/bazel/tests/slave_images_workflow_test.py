"""Exercise slave publication, managed-identity login and Make's pull fallback."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[3]
PUBLISH = ROOT / ".github/workflows/sonic-slave-images.yml"
CONSUME = ROOT / ".github/workflows/bazel-oci.yml"


def assignment(source, name):
    lines = source.splitlines(keepends=True)
    start = next(index for index, line in enumerate(lines)
                 if any(line.startswith(name + suffix) for suffix in (" =", "=", " :=")))
    end = start
    while lines[end].rstrip().endswith("\\"):
        end += 1
    return "".join(lines[start:end + 1])


class SlaveImagesWorkflowTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="slave-images-")
        self.addCleanup(temporary.cleanup)
        self.directory = Path(temporary.name)
        self.commands = self.directory / "commands"
        self.commands.mkdir()
        self.calls = self.directory / "calls.jsonl"
        shim = "#!" + sys.executable + "\n" + r'''
import json, os, pathlib, sys
name = pathlib.Path(sys.argv[0]).name
args = sys.argv[1:]
with pathlib.Path(os.environ["CALLS"]).open("a") as output:
    output.write(json.dumps([name, *args]) + "\n")
operation = ("showtag" if "showtag" in args else
             "sonic-slave-build" if "sonic-slave-build" in args else args[0])
if name == "docker" and args[:2] == ["image", "rm"]:
    operation = "image rm"
if os.environ.get("FAIL_COMMAND") == name + " " + operation:
    raise SystemExit(1)
if name == "make" and operation == "showtag":
    release = next(arg.split("=", 1)[1] for arg in args if arg.startswith("BLDENV="))
    print(os.environ.get("COMPUTED_IMAGE", "sonic-slave-" + release + ":0123456789a"))
elif name == "docker" and operation == "inspect":
    raise SystemExit(0 if os.environ.get("LOCAL_PRESENT") == "1" else 1)
elif name == "docker" and operation == "pull":
    raise SystemExit(0 if os.environ.get("PULL_HIT") == "1" else 1)
elif name == "docker" and operation == "image":
    print("{}")
elif name == "az" and operation == "acr":
    print(os.environ.get("ACR_TOKEN", "fixture-acr-token"))
elif name == "docker" and operation == "login":
    if sys.stdin.read() != os.environ.get("ACR_TOKEN", "fixture-acr-token"):
        raise SystemExit(1)
elif name == "sudo" and args[:3] == ["-n", "test", "-d"]:
    exists = (os.environ["CACHE_DIRECTORY_EXISTS"] == "1"
              if "CACHE_DIRECTORY_EXISTS" in os.environ else os.path.isdir(args[3]))
    raise SystemExit(0 if exists else 1)
elif name == "sudo" and args[:2] == ["-n", "stat"]:
    os.execvp("stat", args[1:])
elif name == "stat":
    print(os.environ.get("CACHE_AUTOMOUNT_FILESYSTEM",
                         os.environ.get("CACHE_FILESYSTEM", "nfs")))
'''
        for name in ("make", "docker", "az", "curl", "sudo", "stat"):
            executable = self.commands / name
            executable.write_text(shim)
            executable.chmod(0o755)
        self.environment = dict(
            PATH=str(self.commands) + os.pathsep + os.defpath,
            CALLS=str(self.calls), SONIC_CI_CONTAINER_LABEL="fixture=slave-images",
            REGISTRY_SERVER="sonicdev-microsoft.azurecr.io", REGISTRY_PORT="443",
            ACR_REGISTRY_NAME="sonicdev", AZURE_SUBSCRIPTION_ID="fixture-subscription",
            DOCKER_CONFIG=str(self.directory / "docker-config"))
        self.publisher = yaml.safe_load(PUBLISH.read_text())["jobs"]["publish"]
        self.consumer = yaml.safe_load(CONSUME.read_text())["jobs"]["vs_container"]

    def run_shell(self, script, **environment):
        self.calls.unlink(missing_ok=True)
        return subprocess.run(
            ["bash", "-c", script], cwd=self.directory,
            env=dict(self.environment, **environment), text=True, capture_output=True)

    def recorded(self):
        return [json.loads(line) for line in self.calls.read_text().splitlines()]

    def test_publication_allows_pr_branch_pushes_and_uses_matching_profile(self):
        workflow = yaml.safe_load(PUBLISH.read_text())
        # PyYAML's YAML 1.1 parser treats the unquoted Actions "on" key as True.
        triggers = workflow.get("on", workflow.get(True))
        self.assertEqual(triggers["push"]["branches"], ["master", "vs-runner"])
        self.assertNotIn("pull_request", triggers)
        self.assertIn("workflow_dispatch", triggers)
        self.assertEqual(
            " ".join(self.publisher["if"].split()),
            "github.ref == 'refs/heads/master' || "
            "(github.event_name == 'push' && github.ref == 'refs/heads/vs-runner')")
        self.assertEqual(workflow["permissions"], {"contents": "read"})
        self.assertEqual(self.publisher["runs-on"], self.consumer["runs-on"])
        self.assertNotIn("concurrency", self.publisher)
        self.assertNotIn("concurrency", self.consumer)
        for name, value in self.publisher["env"].items():
            if name != "SONIC_CI_CONTAINER_LABEL":
                self.assertEqual(value, self.consumer["env"][name], name)
        variables = yaml.safe_load(
            (ROOT / ".azure-pipelines/azure-pipelines-repd-build-variables.yml").read_text())
        self.assertEqual(
            variables["variables"]["VERSION_CONTROL_OPTIONS"],
            "SONIC_VERSION_CONTROL_COMPONENTS=" + self.publisher["env"]["SONIC_VERSION_CONTROL_COMPONENTS"])
        for job in (self.publisher, self.consumer):
            for name in ("DEFAULT_CONTAINER_REGISTRY", "SONIC_DOCKER_APT_RETRIES",
                         "SONIC_SLAVE_DEFAULT_CONTAINER_REGISTRY",
                         "SONIC_SLAVE_DOCKER_APT_RETRIES", "DOCKER_BUILD",
                         "SONIC_BUILD_RETRY_COUNT", "SONIC_BUILD_RETRY_INTERVAL"):
                self.assertNotIn(name, job["env"])
        self.assertEqual(self.consumer["env"]["ENABLE_DOCKER_BASE_PULL"], "y")
        self.assertEqual(
            " ".join(self.consumer["env"]["SONIC_DPKG_CACHE_METHOD"].split()),
            "${{ github.event_name == 'push' && github.ref == 'refs/heads/master' "
            "&& 'rwcache' || 'rcache' }}")
        self.assertEqual(self.consumer["env"]["SONIC_DPKG_CACHE_SOURCE"], "/nfs/dpkg_cache/vs")
        self.assertEqual(self.publisher["env"]["ACR_REGISTRY_NAME"], "sonicdev")
        self.assertEqual(self.publisher["env"]["REGISTRY_SERVER"], "sonicdev-microsoft.azurecr.io")
        self.assertEqual(self.publisher["env"]["AZURE_SUBSCRIPTION_ID"],
                         "${{ vars.ACR_SUBSCRIPTION_ID }}")
        self.assertEqual(self.publisher["env"]["MIRROR_SNAPSHOT"], "y")
        for job in (self.publisher, self.consumer):
            self.assertFalse(any(name.startswith("SONIC_SLAVE_REUSE_") for name in job["env"]))
            names = [step.get("name") for step in job["steps"]]
            login = names.index("Authenticate to the slave image registry")
            build = ("Build and publish computed slave tags" if job is self.publisher
                     else "Prepare the host-driven SONiC slave")
            self.assertLess(login, names.index(build))
            self.assertEqual(job["steps"][login]["run"],
                             'bash tools/ci/runner/azure_acr_login.sh "${ACR_REGISTRY_NAME:?}"')
        scripts = "\n".join(step.get("run", "") for step in self.publisher["steps"])
        self.assertNotIn("git commit", scripts)
        self.assertNotIn("git push", scripts)
        self.assertNotIn("docker system prune", scripts)
        cleanup = self.publisher["steps"][-1]
        self.assertIn("steps.evidence.outcome == 'success'", cleanup["if"])
        self.assertIn("cleanup_vs_workspace.py", cleanup["run"])
        checkout = self.publisher["steps"][0]["with"]["path"]
        self.assertEqual(checkout, self.publisher["defaults"]["run"]["working-directory"])

    def test_publisher_builds_and_pushes_both_computed_tags(self):
        step = next(step for step in self.publisher["steps"]
                    if step.get("name") == "Build and publish computed slave tags")
        result = self.run_shell(step["run"])
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = self.recorded()
        self.assertEqual(calls[0], ["make", "init"])
        for index, release in enumerate(("bookworm", "trixie")):
            selected = calls[1 + index * 6:7 + index * 6]
            image = "sonic-slave-" + release + ":0123456789a"
            remote = "sonicdev-microsoft.azurecr.io:443/" + image
            for command, target in zip(selected[:2], ("showtag", "sonic-slave-build")):
                self.assertEqual(command[0], "make")
                for argument in ("Makefile.work", "BLDENV=" + release, target,
                                 "PLATFORM=generic", "PLATFORM_ARCH=amd64"):
                    self.assertIn(argument, command)
            self.assertEqual(selected[2:], [
                ["docker", "image", "inspect", image],
                ["docker", "tag", image, remote],
                ["docker", "push", remote],
                ["docker", "image", "rm", remote, image],
            ])
        self.assertEqual(len(calls), 13)

    def test_publisher_uses_explicit_registry_port(self):
        script = next(step["run"] for step in self.publisher["steps"]
                      if step.get("name") == "Build and publish computed slave tags")
        for port in ("443", "5000"):
            with self.subTest(port=port):
                result = self.run_shell(script, REGISTRY_PORT=port)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                pushes = [call for call in self.recorded() if call[:2] == ["docker", "push"]]
                self.assertEqual(pushes, [
                    ["docker", "push", "sonicdev-microsoft.azurecr.io:" + port
                     + "/sonic-slave-" + release + ":0123456789a"]
                    for release in ("bookworm", "trixie")
                ])

    def test_unused_image_cleanup_runs_before_space_check(self):
        expected = [
            ["docker", "system", "df"],
            ["docker", "image", "prune", "--all", "--force"],
            ["docker", "builder", "prune", "--all", "--force"],
            ["docker", "system", "df"],
        ]
        for job in (self.publisher, self.consumer):
            with self.subTest(job=job["name"]):
                steps = job["steps"]
                names = [step["name"] for step in steps]
                index = names.index("Reclaim unused images and build cache")
                self.assertLess(index, names.index("Check host-agent build prerequisites"))
                script = steps[index]["run"]
                result = self.run_shell(script)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(self.recorded(), expected)
                for failure, count in (("docker system", 1), ("docker image", 2),
                                       ("docker builder", 3)):
                    with self.subTest(failure=failure):
                        result = self.run_shell(script, FAIL_COMMAND=failure)
                        self.assertNotEqual(result.returncode, 0)
                        self.assertEqual(self.recorded(), expected[:count])

    def test_bazel_cache_cleanup_runs_before_space_check(self):
        for job in (self.publisher, self.consumer):
            with self.subTest(job=job["name"]):
                steps = job["steps"]
                cleanup = next(step for step in steps
                               if step.get("name") in (
                                   "Reclaim local Bazel caches", "Reclaim local build caches"))
                preflight = next(step for step in steps
                                 if step.get("name") == "Check host-agent build prerequisites")
                self.assertLess(steps.index(cleanup), steps.index(preflight))
                self.assertEqual(cleanup["run"],
                                 "python3 -B -m tools.bazel.ci.cleanup_runner_bazel_cache"
                                 ' --runner-workspace "${{ runner.workspace }}"' +
                                 (" --remove-package-cache" if job is self.consumer else ""))
                self.assertNotIn("continue-on-error", cleanup)

    def test_nfs_validation_precedes_local_cleanup_and_build(self):
        steps = self.consumer["steps"]
        validation = next(step for step in steps
                          if step["name"] == "Validate shared NFS package cache")
        cleanup = next(step for step in steps if step["name"] == "Reclaim local build caches")
        configure = next(step for step in steps if step["name"] == "Configure persistent caches")
        self.assertEqual(validation["run"], "bash tools/ci/runner/nfs_package_cache.sh")
        self.assertLess(steps.index(validation), steps.index(cleanup))
        self.assertLess(steps.index(cleanup), steps.index(configure))
        self.assertNotIn("continue-on-error", validation)
        self.assertNotIn("SONIC_DPKG_CACHE_SOURCE", configure["env"])
        self.assertIn('install -d -m 0700 "$SONIC_BAZEL_CACHE_SOURCE"', configure["run"])
        self.assertNotIn('install -d -m 0700 "$SONIC_BAZEL_CACHE_SOURCE" "$SONIC_DPKG_CACHE_SOURCE"',
                         configure["run"])
        self.assertNotIn('test -w "$SONIC_DPKG_CACHE_SOURCE"', configure["run"])
        self.assertIn("SONIC_DPKG_CACHE_SOURCE=%s", configure["run"])
        self.assertFalse(any(step.get("name") == validation["name"]
                             for step in self.publisher["steps"]))

    def test_nfs_validation_accepts_only_existing_nfs_caches(self):
        script = (ROOT / "tools/ci/runner/nfs_package_cache.sh").read_text()
        cache = self.directory / "shared packages"
        cache.mkdir()
        for mode in ("rcache", "rwcache"):
            for filesystem in ("nfs", "nfs4", "autofs", "ext4", "overlay", ""):
                with self.subTest(mode=mode, filesystem=filesystem):
                    result = self.run_shell(
                        script, SONIC_DPKG_CACHE_SOURCE=str(cache),
                        SONIC_DPKG_CACHE_METHOD=mode, CACHE_FILESYSTEM=filesystem)
                    self.assertEqual(result.returncode == 0, filesystem in ("nfs", "nfs4"),
                                     result.stdout + result.stderr)
                    self.assertEqual(self.recorded(), [
                        ["sudo", "-n", "test", "-d", str(cache)],
                        ["sudo", "-n", "stat", "--file-system", "--format=%T",
                         "--", str(cache)],
                        ["stat", "--file-system", "--format=%T", "--", str(cache)]])
        for mode, source in (("rcache", cache / "missing"), ("none", cache)):
            result = self.run_shell(script, SONIC_DPKG_CACHE_SOURCE=str(source),
                                    SONIC_DPKG_CACHE_METHOD=mode)
            self.assertNotEqual(result.returncode, 0)
        result = self.run_shell(
            script, SONIC_DPKG_CACHE_SOURCE=str(cache), SONIC_DPKG_CACHE_METHOD="rcache",
            FAIL_COMMAND="stat --file-system")
        self.assertNotEqual(result.returncode, 0)
        result = self.run_shell(
            script, SONIC_DPKG_CACHE_SOURCE=str(cache), SONIC_DPKG_CACHE_METHOD="rcache",
            FAIL_COMMAND="sudo -n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("privileged host access", result.stderr)
        self.assertEqual(self.recorded(), [["sudo", "-n", "test", "-d", str(cache)]])

    def test_nfs_validation_checks_the_filesystem_reached_through_autofs(self):
        script = (ROOT / "tools/ci/runner/nfs_package_cache.sh").read_text()
        cache = self.directory / "automounted packages"
        cache.mkdir()
        for mode in ("rcache", "rwcache"):
            for filesystem in ("nfs", "nfs4", "autofs", "ext4", "overlay", ""):
                with self.subTest(mode=mode, filesystem=filesystem):
                    result = self.run_shell(
                        script, SONIC_DPKG_CACHE_SOURCE=str(cache),
                        SONIC_DPKG_CACHE_METHOD=mode, CACHE_FILESYSTEM="autofs",
                        CACHE_AUTOMOUNT_FILESYSTEM=filesystem)
                    self.assertEqual(result.returncode == 0, filesystem in ("nfs", "nfs4"),
                                     result.stdout + result.stderr)
                    self.assertEqual(self.recorded()[-1], [
                        "stat", "--file-system", "--format=%T", "--", str(cache)])

    def test_nfs_validation_leaves_read_write_permissions_to_the_slave(self):
        script = (ROOT / "tools/ci/runner/nfs_package_cache.sh").read_text()
        cache = self.directory / "read-only packages"
        cache.mkdir(mode=0o555)
        original = cache.stat()
        for mode in ("rcache", "rwcache"):
            result = self.run_shell(script, SONIC_DPKG_CACHE_SOURCE=str(cache),
                                    SONIC_DPKG_CACHE_METHOD=mode)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("cache access handled by the slave", result.stdout)
        current = cache.stat()
        self.assertEqual(
            (current.st_mode, current.st_uid, current.st_gid, current.st_mtime_ns),
            (original.st_mode, original.st_uid, original.st_gid, original.st_mtime_ns))

    @unittest.skipIf(os.geteuid() == 0, "root bypasses local directory mode restrictions")
    def test_nfs_validation_does_not_require_host_user_parent_traversal(self):
        script = (ROOT / "tools/ci/runner/nfs_package_cache.sh").read_text()
        parent = self.directory / "restricted nfs"
        parent.mkdir()
        cache = parent / "packages"
        cache.mkdir()
        original = cache.stat()
        parent.chmod(0o000)
        try:
            self.assertFalse(os.path.isdir(cache))
            self.assertFalse(os.access(cache, os.R_OK | os.X_OK))
            for mode in ("rcache", "rwcache"):
                result = self.run_shell(
                    script, SONIC_DPKG_CACHE_SOURCE=str(cache),
                    SONIC_DPKG_CACHE_METHOD=mode, CACHE_DIRECTORY_EXISTS="1")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(self.recorded(), [
                    ["sudo", "-n", "test", "-d", str(cache)],
                    ["sudo", "-n", "stat", "--file-system", "--format=%T",
                     "--", str(cache)],
                    ["stat", "--file-system", "--format=%T", "--", str(cache)]])
        finally:
            parent.chmod(0o755)
        current = cache.stat()
        self.assertEqual(
            (current.st_mode, current.st_uid, current.st_gid, current.st_mtime_ns),
            (original.st_mode, original.st_uid, original.st_gid, original.st_mtime_ns))

    def test_vs_build_forwards_four_make_jobs_to_slave(self):
        self.assertEqual(self.consumer["env"]["SONIC_BUILD_JOBS"], "4")
        self.assertNotIn("SONIC_BUILD_JOBS", self.publisher["env"])
        source = (ROOT / "Makefile.work").read_text()
        fragment = ("SHELL := /bin/bash\n" + assignment(source, "SONIC_BUILD_INSTRUCTION") +
                    "\ncheck:\n\t@$(SONIC_BUILD_INSTRUCTION) target/sonic-vs.bin\n")
        result = subprocess.run(
            ["/usr/bin/make", "--no-print-directory", "-f", "-", "check",
             "MAKE=" + str(self.commands / "make")],
            input=fragment, cwd=self.directory,
            env=dict(self.environment, SONIC_BUILD_JOBS=self.consumer["env"]["SONIC_BUILD_JOBS"]),
            text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = self.recorded()
        self.assertEqual(len(calls), 1)
        self.assertEqual(calls[0][:3], ["make", "-f", "slave.mk"])
        self.assertIn("SONIC_BUILD_JOBS=4", calls[0])
        self.assertIn("target/sonic-vs.bin", calls[0])

    def test_private_docker_configuration_is_exported_before_docker_operations(self):
        expected = ("${{ runner.temp }}/docker-config-${{ github.run_id }}-"
                    "${{ github.run_attempt }}-${{ github.job }}")
        for index, job in enumerate((self.publisher, self.consumer)):
            with self.subTest(job=job["name"]):
                names = [step["name"] for step in job["steps"]]
                initialize = job["steps"][names.index("Initialize private Docker configuration")]
                self.assertEqual(initialize["env"]["DOCKER_CONFIG"], expected)
                self.assertLess(names.index(initialize["name"]),
                                names.index("Reclaim unused images and build cache"))
                self.assertLess(names.index(initialize["name"]),
                                names.index("Authenticate to the slave image registry"))
                config = self.directory / ("docker-config-" + str(index))
                environment_file = self.directory / ("github-env-" + str(index))
                result = self.run_shell(initialize["run"], DOCKER_CONFIG=str(config),
                                        GITHUB_ENV=str(environment_file))
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(config.stat().st_mode & 0o777, 0o700)
                self.assertEqual(environment_file.read_text(), "DOCKER_CONFIG=" + str(config) + "\n")
                logout = job["steps"][names.index("Log out of the slave image registry")]
                self.assertEqual(logout["if"], "always()")
                self.assertEqual(logout["working-directory"], "${{ runner.temp }}")
                self.assertEqual(logout["env"]["DOCKER_CONFIG"], expected)
                self.assertGreater(names.index(logout["name"]),
                                   names.index("Authenticate to the slave image registry"))
                self.assertLess(names.index(logout["name"]),
                                names.index("Remove only this run's checkout"))
                for step in job["steps"]:
                    if step not in (initialize, logout):
                        self.assertNotIn("DOCKER_CONFIG", step.get("env", {}))

    def test_logout_removes_only_private_credentials_even_on_failure(self):
        home = self.directory / "home"
        shared = home / ".docker"
        shared.mkdir(parents=True)
        shared_config = shared / "config.json"
        shared_config.write_text("shared credentials\n")
        config = Path(self.environment["DOCKER_CONFIG"])
        for job in (self.publisher, self.consumer):
            script = next(step["run"] for step in job["steps"]
                          if step.get("name") == "Log out of the slave image registry")
            for failure in ("", "docker logout"):
                with self.subTest(job=job["name"], failure=failure):
                    config.mkdir(exist_ok=True)
                    credentials = config / "config.json"
                    credentials.write_text("private credentials\n")
                    retained = config / "buildx-metadata"
                    retained.write_text("retain\n")
                    result = self.run_shell(script, HOME=str(home), FAIL_COMMAND=failure)
                    self.assertEqual(result.returncode, 1 if failure else 0,
                                     result.stdout + result.stderr)
                    self.assertEqual(self.recorded(),
                                     [["docker", "logout", "sonicdev-microsoft.azurecr.io:443"]])
                    self.assertFalse(credentials.exists())
                    self.assertTrue(retained.exists())
                    self.assertEqual(shared_config.read_text(), "shared credentials\n")
            result = self.run_shell(script, HOME=str(home))
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_publication_stops_on_invalid_tags_and_failed_commands(self):
        script = next(step["run"] for step in self.publisher["steps"]
                      if step.get("name") == "Build and publish computed slave tags")
        for failure in ("make init", "make showtag", "make sonic-slave-build",
                        "docker image", "docker tag", "docker push"):
            with self.subTest(failure=failure):
                result = self.run_shell(script, FAIL_COMMAND=failure)
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertFalse(any("BLDENV=trixie" in call for call in self.recorded()))
                self.assertFalse(any(call[:3] == ["docker", "image", "rm"]
                                     for call in self.recorded()))
        for image in ("sonic-slave-bookworm:latest", "sonic-slave-trixie:0123456789a", ""):
            with self.subTest(image=image):
                result = self.run_shell(script, COMPUTED_IMAGE=image)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Invalid computed slave image reference", result.stderr)
                self.assertFalse(any(call[0] == "docker" for call in self.recorded()))

    def test_published_image_cleanup_failure_is_reported_without_force_or_pruning(self):
        script = next(step["run"] for step in self.publisher["steps"]
                      if step.get("name") == "Build and publish computed slave tags")
        result = self.run_shell(script, FAIL_COMMAND="docker image rm")
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = self.recorded()
        image = "sonic-slave-bookworm:0123456789a"
        remote = "sonicdev-microsoft.azurecr.io:443/" + image
        self.assertEqual(calls[-2:], [
            ["docker", "push", remote],
            ["docker", "image", "rm", remote, image],
        ])
        self.assertFalse(any("BLDENV=trixie" in call for call in calls))
        scripts = "\n".join(step.get("run", "") for step in self.publisher["steps"])
        self.assertNotIn("docker system prune", scripts)
        self.assertNotIn("docker container prune", scripts)
        self.assertNotIn("docker volume prune", scripts)
        self.assertNotIn("--all", script)

    def test_identity_authentication_and_failures(self):
        helper = ROOT / "tools/ci/runner/azure_acr_login.sh"
        login = next(step for step in self.publisher["steps"]
                     if step["name"] == "Authenticate to the slave image registry")
        script = login["run"].replace("tools/ci/runner/azure_acr_login.sh", str(helper))
        expected = [
            ["az", "version", "--output", "none"],
            ["az", "login", "--identity", "--output", "none"],
            ["az", "account", "set", "--subscription", "fixture-subscription"],
            ["az", "acr", "login", "--name", "sonicdev", "--expose-token",
             "--query", "accessToken", "--output", "tsv"],
            ["docker", "login", "sonicdev-microsoft.azurecr.io:443",
             "--username", "00000000-0000-0000-0000-000000000000", "--password-stdin"],
        ]
        for index, failure in enumerate(("", "az version", "az login", "az account",
                                         "az acr", "docker login")):
            with self.subTest(failure=failure):
                result = self.run_shell(script, FAIL_COMMAND=failure)
                self.assertEqual(result.returncode, 1 if failure else 0, result.stderr)
                count = len(expected) if not failure else index
                self.assertEqual(self.recorded(), expected[:count])
                self.assertNotIn("fixture-acr-token", result.stdout + result.stderr)
                self.assertNotIn("fixture-acr-token", self.calls.read_text())
        result = self.run_shell(script, ACR_TOKEN="")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ACR returned an empty access token", result.stderr)
        self.assertEqual(self.recorded(), expected[:-1])
        result = self.run_shell(script, REGISTRY_PORT="5000")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(self.recorded()[-1][2], "sonicdev-microsoft.azurecr.io:5000")
        environment = dict(self.environment)
        del environment["ACR_REGISTRY_NAME"]
        result = subprocess.run(
            ["bash", "-c", script], cwd=self.directory, env=environment,
            text=True, capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ACR_REGISTRY_NAME", result.stderr)

    def test_missing_registry_configuration_fails_before_azure_commands(self):
        helper = ROOT / "tools/ci/runner/azure_acr_login.sh"
        for name in ("AZURE_SUBSCRIPTION_ID", "REGISTRY_SERVER", "REGISTRY_PORT", "DOCKER_CONFIG"):
            for value in ("", None):
                with self.subTest(name=name, value=value):
                    environment = dict(self.environment)
                    if value is None:
                        del environment[name]
                    else:
                        environment[name] = value
                    result = subprocess.run(
                        ["bash", str(helper), "sonicdev"], cwd=self.directory,
                        env=environment, text=True, capture_output=True)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(name, result.stderr)
                    self.assertFalse(self.calls.exists())

    def test_missing_azure_cli_installs_and_cleans_up_on_failure(self):
        helper = ROOT / "tools/ci/runner/azure_acr_login.sh"
        # Export a controlled lookup so the production helper takes its install path.
        script = (
            'command() { if [[ $1 == -v && $2 == az ]]; then return 1; fi; '
            'builtin command "$@"; }; export -f command\n'
            'bash "' + str(helper) + '" sonicdev')
        for failure in ("", "curl --fail", "sudo -n"):
            with self.subTest(failure=failure):
                result = self.run_shell(script, FAIL_COMMAND=failure)
                self.assertEqual(result.returncode, 1 if failure else 0, result.stderr)
                calls = self.recorded()
                self.assertEqual(calls[0][0], "curl")
                self.assertIn("https://aka.ms/InstallAzureCLIDeb", calls[0])
                installer = calls[0][-1]
                self.assertFalse(Path(installer).exists())
                if failure != "curl --fail":
                    self.assertEqual(calls[1], ["sudo", "-n", "bash", installer])
                if failure:
                    self.assertFalse(any(call[0] == "az" for call in calls))
                else:
                    self.assertEqual(calls[-1][:3],
                                     ["docker", "login", "sonicdev-microsoft.azurecr.io:443"])

    def test_make_reuses_local_then_pulls_then_builds(self):
        source = (ROOT / "Makefile.work").read_text()
        names = ("DOCKER_SLAVE_BASE_BUILD", "DOCKER_BASE_PULL",
                 "DOCKER_SLAVE_BASE_INSPECT", "DOCKER_SLAVE_BASE_PULL_REGISTRY")
        condition = "ifeq ($(SONIC_SLAVE_REUSE_SLAVE),y)\n"
        selection = condition + source.rsplit(condition, 1)[1].split("\nendif\n", 1)[0] + "\nendif\n"
        makefile = self.directory / "fixture.mk"
        makefile.write_text(
            "SHELL := /bin/bash\n.SHELLFLAGS := -e -c\n"
            "SLAVE_BASE_IMAGE := sonic-slave-trixie\nSLAVE_BASE_TAG := 0123456789a\n"
            "SLAVE_DIR := sonic-slave-trixie\nENABLE_DOCKER_BASE_PULL := y\n"
            "SONIC_DPKG_CACHE_METHOD := rwcache\nREGISTRY_SERVER := sonicdev-microsoft.azurecr.io\n"
            "REGISTRY_PORT ?= 443\nPREPARE_DOCKER := echo prepared\n"
            "COLLECT_DOCKER := echo collected\n"
            + "".join(assignment(source, name) for name in names) + selection
            + "\nall:\n\t@$(SONIC_SLAVE_BASE_BUILD)\n")
        image = "sonic-slave-trixie:0123456789a"
        cases = [
            ("1", "0", "", 0, ["inspect"]),
            ("0", "1", "", 0, ["inspect", "pull", "tag"]),
            ("0", "0", "", 0, ["inspect", "pull", "build"]),
            ("0", "0", "docker build", 1, ["inspect", "pull", "build"]),
        ]
        for local, hit, failure, status, expected in cases:
            with self.subTest(local=local, hit=hit, failure=failure):
                self.calls.unlink(missing_ok=True)
                result = subprocess.run(
                    ["/usr/bin/make", "-s", "-f", str(makefile), "all"],
                    cwd=self.directory, env=dict(self.environment, LOCAL_PRESENT=local,
                                                PULL_HIT=hit, FAIL_COMMAND=failure),
                    text=True, capture_output=True)
                self.assertEqual(result.returncode == 0, status == 0, result.stderr)
                calls = self.recorded()
                self.assertEqual([call[1] for call in calls], expected)
                if "pull" in expected:
                    self.assertIn("sonicdev-microsoft.azurecr.io:443/" + image, calls[1])
                    if hit == "1":
                        self.assertEqual(calls[2], ["docker", "tag", calls[1][2], image])
                if "build" in expected:
                    self.assertIn(image, calls[-1])
        for port in ("443", "5000"):
            with self.subTest(port=port):
                self.calls.unlink(missing_ok=True)
                result = subprocess.run(
                    ["/usr/bin/make", "-s", "-f", str(makefile), "all",
                     "REGISTRY_PORT=" + port, "REGISTRY_SERVER_PATH=/slaves"],
                    cwd=self.directory, env=dict(self.environment, PULL_HIT="1"),
                    text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                remote = "sonicdev-microsoft.azurecr.io:" + port + "/slaves/" + image
                self.assertEqual(self.recorded(), [
                    ["docker", "inspect", "--type", "image", image],
                    ["docker", "pull", remote],
                    ["docker", "tag", remote, image],
                ])

    def test_generic_and_vs_slave_tags_match_with_repository_defaults(self):
        source = (ROOT / "Makefile.work").read_text()
        registry = ("ifneq ($(DEFAULT_CONTAINER_REGISTRY),)"
                    + source.split("ifneq ($(DEFAULT_CONTAINER_REGISTRY),)", 1)[1]
                    .split("\nifeq ($(ENABLE_DOCKER_BASE_PULL),)", 1)[0])
        render = ("$(shell CONFIGURED_ARCH="
                  + source.split("$(shell CONFIGURED_ARCH=", 1)[1]
                  .split("\n\nifeq ($(CROSS_BUILD_ENVIRON), y)", 1)[0])
        prepare = assignment(source, "PREPARE_DOCKER")
        collect = assignment(source, "COLLECT_DOCKER")
        tag = assignment(source, "SLAVE_BASE_TAG_COMPUTED")
        (self.directory / "scripts").mkdir()
        slave = self.directory / "sonic-slave-trixie"
        (slave / "buildinfo/versions").mkdir(parents=True)
        (slave / "sources.list.fixture").write_text("deb fixture trixie main\n")
        (slave / "buildinfo/versions/versions-fixture").write_text("fixture==1\n")
        renderer = self.commands / "j2"
        renderer.write_text(
            "#!" + sys.executable + "\nimport os\n"
            "print('FROM ' + os.environ['DEFAULT_CONTAINER_REGISTRY'] + 'debian:fixture')\n")
        renderer.chmod(0o755)
        shim = "#!" + sys.executable + "\n" + r'''
import json, os, pathlib, sys
name = pathlib.Path(sys.argv[0]).name
settings = {"command": name, "registry": os.environ.get("DEFAULT_CONTAINER_REGISTRY", ""),
            "retries": os.environ.get("SONIC_DOCKER_APT_RETRIES", "")}
with pathlib.Path(os.environ["PROFILE_CALLS"]).open("a") as output:
    output.write(json.dumps(settings) + "\n")
if name == "prepare_docker_buildinfo.sh":
    dockerfile = pathlib.Path(sys.argv[2])
    dockerfile.write_text(dockerfile.read_text() + "policy=" + settings["retries"] + "\n")
'''
        for name in ("prepare_docker_buildinfo.sh", "collect_docker_version_files.sh"):
            script = self.directory / "scripts" / name
            script.write_text(shim)
            script.chmod(0o755)
        default_registry = next(
            line for line in (ROOT / "rules/config").read_text().splitlines()
            if line.startswith("DEFAULT_CONTAINER_REGISTRY ?="))
        makefile = self.directory / "profile.mk"
        makefile.write_text(
            "SHELL := /bin/bash\n.DEFAULT_GOAL := all\n"
            + default_registry + "\n"
            "SLAVE_DIR := sonic-slave-trixie\nSLAVE_BASE_IMAGE := sonic-slave-trixie\n"
            "CONFIGURED_ARCH := amd64\nBLDENV := trixie\nSONIC_BUILD_HOOKS_HASH := fixture\n"
            + registry + "\n" + render + "\n" + prepare
            + "\n$(shell $(PREPARE_DOCKER))\n" + tag
            + "\nSLAVE_BASE_TAG := $(SLAVE_BASE_TAG_COMPUTED)\n" + collect
            + "\nall:\n\t@$(COLLECT_DOCKER)\n"
            "\t@printf '%s\\n' 'tag=$(SLAVE_BASE_TAG)' "
            "'registry=$(DEFAULT_CONTAINER_REGISTRY)' 'retries=$(SONIC_DOCKER_APT_RETRIES)'\n")
        reports = {}
        profiles = [
            ("generic", {"PLATFORM": "generic"}),
            ("vs", {"PLATFORM": "vs"}),
            ("inherited", {"DEFAULT_CONTAINER_REGISTRY": "registry.example",
                           "SONIC_DOCKER_APT_RETRIES": "0"}),
        ]
        for name, environment in profiles:
            with self.subTest(profile=name):
                capture = self.directory / (name + ".jsonl")
                result = subprocess.run(
                    ["/usr/bin/make", "-s", "-f", str(makefile)],
                    cwd=self.directory,
                    env=dict(self.environment, **environment, PROFILE_CALLS=str(capture)),
                    text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                reports[name] = dict(line.split("=", 1) for line in result.stdout.splitlines())
                settings = [json.loads(line) for line in capture.read_text().splitlines()]
                prefix = "registry.example/" if name == "inherited" else "publicmirror.azurecr.io/"
                self.assertEqual([entry["registry"] for entry in settings], [prefix, prefix])
                self.assertEqual(settings[0]["retries"], "0" if name == "inherited" else "")
        self.assertEqual(reports["generic"]["tag"], reports["vs"]["tag"])
        self.assertNotEqual(reports["generic"]["tag"], reports["inherited"]["tag"])
        self.assertEqual(reports["vs"]["registry"], "publicmirror.azurecr.io/")
        self.assertEqual(reports["vs"]["retries"], "")


if __name__ == "__main__":
    unittest.main()
