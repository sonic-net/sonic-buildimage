"""Check Make-built children can collect buildinfo from source-built base images."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

import jinja2


ROOT = Path(__file__).resolve().parents[3]
APT_OVERRIDE = "zzzz-sonic-ci-acquire-retries"
APT_OVERRIDE_CONTENT = (
    "// Temporary SONiC native Docker CI override; Debian bug #1117797.\n"
    'Acquire::Retries "0";\n')
APT_MOUNTS = (
    " --mount=type=secret,id=sonic_apt_proxy,target=/etc/apt/apt.conf.d/zzzz-sonic-native-proxy,mode=0444,required=true"
    " --mount=type=secret,id=sonic_apt_ca,target=/run/secrets/sonic_apt_ca,mode=0444,required=true"
)


class DockerBuildinfoTest(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.workspace = Path(temporary.name)
        # Keep the real generator and Dockerfile templates. Stub only unrelated
        # source/version preparation; this test must not pull Docker images.
        fixtures = {
            "src/sonic-build-hooks/scripts/utils.sh": "",
            "src/sonic-build-hooks/buildinfo/fixture": "buildinfo fixture\n",
            "files/apt/apt.conf.d/fixture": "apt fixture\n",
            "scripts/docker_version_control.sh": "#!/bin/sh\nexit 0\n",
            "scripts/build_mirror_config.sh": "#!/bin/sh\nexit 0\n",
            "scripts/versions_manager.py": "#!/bin/sh\nexit 0\n",
        }
        for name, content in fixtures.items():
            path = self.workspace / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)
            path.chmod(0o755)

    def render(self, image):
        environment = jinja2.Environment(loader=jinja2.FileSystemLoader(ROOT))
        packages = {image.replace("-", "_") + "_" + kind: ""
                    for kind in ("debs", "pydebs", "whls", "dbgs", "pkgs", "files")}
        return environment.get_template("dockers/" + image + "/Dockerfile.j2").render(
            DOCKER_USERNAME="sonic", DOCKER_USERTAG="latest", DOCKER_BASE_ARCH="amd64",
            CONFIGURED_ARCH="amd64", **packages)

    def prepare(self, image, original=None, apt_retries=None, apt_proxy=None):
        dockerfile = self.workspace / "dockers" / image / "Dockerfile"
        if original is not None:
            dockerfile.parent.mkdir(parents=True, exist_ok=True)
            dockerfile.write_text(original)
        environment = dict(os.environ, SKIP_BUILD_HOOK="y")
        environment.pop("SONIC_DOCKER_APT_RETRIES", None)
        if apt_retries is not None:
            environment["SONIC_DOCKER_APT_RETRIES"] = apt_retries
        environment.pop("SONIC_DOCKER_APT_PROXY", None)
        if apt_proxy is not None:
            environment["SONIC_DOCKER_APT_PROXY"] = apt_proxy
        subprocess.run(
            ["bash", str(ROOT / "scripts/prepare_docker_buildinfo.sh"), image,
             str(dockerfile.relative_to(self.workspace)), "amd64", "", "trixie"],
            cwd=self.workspace, env=environment,
            check=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return dockerfile.read_text()

    @staticmethod
    def instructions(stage):
        """Keep multiline shell RUNs intact, as Docker's line continuations do."""
        logical = []
        pending = ""
        for line in stage:
            if line.endswith("\\"):
                pending += line[:-1] + " "
            else:
                logical.append(pending + line)
                pending = ""
        if pending:
            raise AssertionError("unterminated Dockerfile continuation")
        return logical

    def override_commands(self, generated, stage=0):
        commands = [line[4:] for line in self.instructions(self.stages(generated)[stage])
                    if line.startswith("RUN ") and APT_OVERRIDE in line]
        self.assertEqual(len(commands), 2)
        return commands

    @staticmethod
    def stages(dockerfile):
        stages = []
        for line in dockerfile.splitlines():
            line = line.rstrip()
            if line.startswith("FROM "):
                stages.append([])
            if stages:
                stages[-1].append(line)
        return stages

    def assert_hooks(self, image, initialized, final_stage, original=None):
        if original is None:
            original = self.render(image)
        generated = self.prepare(image, original)
        before, after = self.stages(original), self.stages(generated)
        self.assertEqual(len(after), len(before))
        install = "RUN dpkg -i /usr/local/share/buildinfo/sonic-build-hooks_1.0_all.deb"
        pre = "RUN pre_run_buildinfo " + image
        post = "RUN post_run_buildinfo " + image
        cleanup = "RUN post_run_cleanup " + image
        for index, stage in enumerate(after):
            with self.subTest(image=image, stage=index):
                if index in initialized:
                    self.assertEqual(stage.count(install), 1)
                    self.assertEqual(stage.count(pre), 1)
                    first_run = next((line for line in before[index] if line.startswith("RUN ")), None)
                    self.assertLess(stage.index(install), stage.index(pre))
                    if first_run:
                        self.assertLess(stage.index(pre), stage.index(first_run))
                else:
                    self.assertNotIn(install, stage)
                    self.assertNotIn(pre, stage)
                if index == final_stage:
                    self.assertIn(index, initialized)
                    self.assertEqual(stage.count(post), 1)
                    self.assertEqual(stage.count(cleanup), 1)
                    self.assertLess(stage.index(pre), stage.index(post))
                    self.assertLess(stage.index(post), stage.index(cleanup))
                else:
                    self.assertNotIn(post, stage)
                    self.assertNotIn(cleanup, stage)
        self.assertEqual(self.prepare(image), generated, "preparation must be idempotent")

    def test_watchdog_builders_and_runtime_stages_have_independent_hooks(self):
        for image in ("docker-bmp-watchdog", "docker-telemetry-watchdog"):
            with self.subTest(image=image):
                self.assert_hooks(image, initialized={0, 1}, final_stage=1)
        for image in ("docker-restapi-watchdog", "docker-gnmi-watchdog"):
            with self.subTest(image=image):
                self.assert_hooks(image, initialized={0, 1}, final_stage=1)

    def test_flattening_stages_copy_the_finalized_filesystem_without_new_hooks(self):
        for image in ("docker-base-trixie", "docker-config-engine-trixie"):
            with self.subTest(image=image):
                self.assert_hooks(image, initialized={0}, final_stage=0)

    def test_single_stage_image_retains_its_hooks(self):
        self.assert_hooks("docker-config-engine-stretch", initialized={0}, final_stage=0)

    def test_copy_only_runtime_stage_initializes_before_finalizing(self):
        for flatten in (False, True):
            with self.subTest(flatten=flatten):
                original = (
                    "ARG BASE=docker-config-engine-trixie-sonic:latest\n"
                    "FROM $BASE AS builder\nRUN true\n"
                    "FROM $BASE AS base\n"
                    "COPY --from=builder /usr/bin/true /usr/bin/true\n"
                    "ENV DEBIAN_FRONTEND=noninteractive\n")
                if flatten:
                    original += "FROM $BASE\nCOPY --from=base / /\n"
                self.assert_hooks("docker-copy-only", initialized={0, 1}, final_stage=1,
                                  original=original)

    def test_unset_and_empty_preserve_default_policy(self):
        for value in (None, ""):
            with self.subTest(value=value):
                image = "docker-bmp-watchdog"
                generated = self.prepare(image, self.render(image), apt_retries=value)
                self.assertNotIn(APT_OVERRIDE, generated)
                self.assertNotIn("SONIC_DOCKER_APT_RETRIES=", generated)

    def test_proxy_secrets_cover_real_runs_in_all_watchdog_stages_and_hooks(self):
        image = "docker-bmp-watchdog"
        original = self.render(image)
        for retries in (None, "0"):
            with self.subTest(retries=retries):
                default = self.prepare(image, original, apt_retries=retries)
                for off in ("", "0"):
                    self.assertEqual(self.prepare(image, original, apt_retries=retries, apt_proxy=off), default)
                generated = self.prepare(image, original, apt_retries=retries, apt_proxy="1")
                expected = default.replace("\nRUN ", "\n# SONIC_DOCKER_APT_PROXY=1\nRUN ", 1)
                expected = "\n".join("RUN" + APT_MOUNTS + line[3:] if line.startswith("RUN ") else line
                                     for line in expected.split("\n"))
                self.assertEqual(generated, expected)
                self.assertEqual(self.prepare(image, apt_retries=retries, apt_proxy="1"), generated)

    def test_mounts_preserve_json_shell_mounts_continuations_and_heredocs(self):
        original = (
            "FROM debian:trixie AS builder\n"
            'RUN --mount=type=cache,target=/var/cache/apt ["apt-get", "update"]\n'
            "  rUn\tprintf \\\n# continuation comment\nRUN\n"
            "COPY <<'SCRIPT' /usr/local/bin/sample\nRUN data inside COPY\nSCRIPT\n"
            "RUN <<-\"SCRIPT\"\n\tRUN data inside heredoc\n\tSCRIPT\n"
            "RUN echo 'literal <<SCRIPT'\n"
            "FROM scratch\nCOPY --from=builder /bin/sh /bin/sh\n")
        generated = self.prepare("docker-instructions", original, apt_proxy="1")
        expected_runs = (
            "RUN" + APT_MOUNTS + ' --mount=type=cache,target=/var/cache/apt ["apt-get", "update"]',
            "  rUn" + APT_MOUNTS + "\tprintf \\\n# continuation comment\nRUN\n",
            "COPY <<'SCRIPT' /usr/local/bin/sample\nRUN data inside COPY\nSCRIPT\n",
            "RUN" + APT_MOUNTS + ' <<-"SCRIPT"\n\tRUN data inside heredoc\n\tSCRIPT\n',
            "RUN" + APT_MOUNTS + " echo 'literal <<SCRIPT'",
        )
        for expected in expected_runs:
            self.assertIn(expected, generated)
        self.assertNotIn("sonic_apt_", generated.split("FROM scratch\n")[1])

    def test_proxy_can_switch_modes_when_reusing_generated_input(self):
        original = "FROM debian:trixie\nRUN true\n"
        for prefix in ("", "# SKIP_HOOK\n"):
            with self.subTest(prefix=prefix):
                default = self.prepare("docker-proxy-mode", prefix + original)
                enabled = self.prepare("docker-proxy-mode", apt_proxy="1")
                self.assertIn("RUN" + APT_MOUNTS + " true", enabled)
                self.assertEqual(self.prepare("docker-proxy-mode", apt_proxy="1"), enabled)
                self.assertEqual(self.prepare("docker-proxy-mode", apt_proxy="0"), default)
                self.assertEqual(self.prepare("docker-proxy-mode", apt_proxy="1"), enabled)
                self.assertEqual(self.prepare("docker-proxy-mode"), default)
        with self.assertRaises(subprocess.CalledProcessError):
            self.prepare("docker-proxy-mode", original, apt_proxy="yes")

    def test_opt_in_covers_builders_runtime_and_flattening_boundaries(self):
        images = {
            "docker-bmp-watchdog": {0, 1},
            "docker-telemetry-watchdog": {0, 1},
            "docker-restapi-watchdog": {0, 1},
            "docker-gnmi-watchdog": {0, 1},
            "docker-base-trixie": {0},
            "docker-config-engine-trixie": {0},
            "docker-config-engine-stretch": {0},
        }
        for image, independent in images.items():
            with self.subTest(image=image):
                generated = self.prepare(image, self.render(image), apt_retries="0")
                for index, stage in enumerate(self.stages(generated)):
                    instructions = self.instructions(stage)
                    if index not in independent:
                        self.assertNotIn(APT_OVERRIDE, "\n".join(stage))
                        continue
                    setup, cleanup = self.override_commands(generated, index)
                    runs = [line[4:] for line in instructions if line.startswith("RUN ")]
                    self.assertEqual(runs[0], setup, "policy must precede hooks and APT")
                    self.assertEqual(runs[-1], cleanup, "policy must not escape the stage")
                    self.assertEqual(stage.count("# SONIC_DOCKER_APT_RETRIES=0"), 1)
                self.assertEqual(self.prepare(image, apt_retries="0"), generated)

    def test_copy_only_intermediate_and_runtime_stages_are_clean_before_copy(self):
        original = (
            "FROM debian:trixie AS builder\nCOPY input /input\n"
            "FROM debian:trixie AS base\nCOPY --from=builder / /\n"
            "FROM scratch\nCOPY --from=base / /\n")
        generated = self.prepare("docker-copy-only", original, apt_retries="0")
        for index in (0, 1):
            setup, cleanup = self.override_commands(generated, index)
            self.assertIn("Refusing to overwrite", setup)
            self.assertIn('rm -- "$conf"', cleanup)
        self.assertNotIn(APT_OVERRIDE, "\n".join(self.stages(generated)[2]))

    def test_invalid_policy_is_rejected_before_rewriting(self):
        original = "FROM debian:trixie\nRUN true\n"
        for value in ("1", "8", "-1", "00", " 0", "0\n", "0; touch injected"):
            with self.subTest(value=value):
                with self.assertRaises(subprocess.CalledProcessError) as caught:
                    self.prepare("docker-invalid", original, apt_retries=value)
                self.assertIn("must be unset, empty, or 0", caught.exception.stderr)
                path = self.workspace / "dockers/docker-invalid/Dockerfile"
                self.assertEqual(path.read_text(), original)
                self.assertFalse((self.workspace / "injected").exists())

    def test_generated_policy_cannot_be_silently_changed(self):
        original = "FROM debian:trixie\nRUN true\n"
        for before, after in ((None, "0"), ("0", None)):
            with self.subTest(before=before, after=after):
                generated = self.prepare("docker-policy", original, apt_retries=before)
                with self.assertRaises(subprocess.CalledProcessError) as caught:
                    self.prepare("docker-policy", apt_retries=after)
                self.assertIn("Regenerate the Dockerfile", caught.exception.stderr)
                self.assertEqual((self.workspace / "dockers/docker-policy/Dockerfile").read_text(), generated)

    def test_skipped_hooks_cannot_silently_skip_requested_finalization(self):
        original = "# SKIP_HOOK\nFROM debian:trixie\nRUN true\n"
        self.assertEqual(self.prepare("docker-skip", original), original)
        with self.assertRaises(subprocess.CalledProcessError) as caught:
            self.prepare("docker-skip", original, apt_retries="0")
        self.assertIn("requires buildinfo finalization", caught.exception.stderr)

    def private_apt(self):
        parts = self.workspace / "private-apt"
        parts.mkdir()
        runtime_policy = ROOT / "files/apt/apt.conf.d/apt-timeout-n-retries"
        (parts / runtime_policy.name).write_bytes(runtime_policy.read_bytes())
        config = self.workspace / "apt.conf"
        config.write_text(f'Dir::Etc::parts "{parts}";\nDir::Etc::main "/dev/null";\n')
        generated = self.prepare("docker-private-apt", "FROM debian:trixie\nRUN true\n", apt_retries="0")
        commands = [command.replace("/etc/apt/apt.conf.d", str(parts))
                    for command in self.override_commands(generated)]
        return parts, dict(os.environ, APT_CONFIG=str(config)), commands

    @unittest.skipUnless(shutil.which("apt-config"), "requires local apt-config, no network")
    def test_real_shell_override_restores_original_runtime_policy(self):
        parts, environment, (setup, cleanup) = self.private_apt()
        original = {path.name: path.read_bytes() for path in parts.iterdir()}
        for _ in range(2):
            subprocess.run(["/bin/sh", "-c", setup], env=environment, check=True)
            override = parts / APT_OVERRIDE
            self.assertEqual(override.read_text(), APT_OVERRIDE_CONTENT)
            self.assertEqual(override.stat().st_mode & 0o777, 0o644)
            effective = subprocess.check_output(["apt-config", "shell", "retries", "Acquire::Retries"], env=environment, text=True)
            self.assertEqual(effective.strip(), "retries='0'")
            subprocess.run(["/bin/sh", "-c", cleanup], env=environment, check=True)
            self.assertEqual({path.name: path.read_bytes() for path in parts.iterdir()}, original)
            effective = subprocess.check_output(["apt-config", "shell", "retries", "Acquire::Retries"], env=environment, text=True)
            self.assertEqual(effective.strip(), "retries='8'")

    @unittest.skipUnless(shutil.which("apt-config"), "requires local apt-config, no network")
    def test_override_refuses_existing_file_or_symlink(self):
        parts, environment, (setup, _) = self.private_apt()
        override = parts / APT_OVERRIDE
        for symlink in (False, True):
            with self.subTest(symlink=symlink):
                other = self.workspace / "unrelated"
                other.write_text("preserve me\n")
                if symlink:
                    override.symlink_to(other)
                else:
                    override.write_text("pre-existing policy\n")
                before = other.read_bytes() if symlink else override.read_bytes()
                result = subprocess.run(["/bin/sh", "-c", setup], env=environment, capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Refusing to overwrite", result.stderr)
                self.assertEqual(other.read_bytes() if symlink else override.read_bytes(), before)
                override.unlink()

    @unittest.skipUnless(shutil.which("apt-config"), "requires local apt-config, no network")
    def test_cleanup_refuses_changed_policy(self):
        parts, environment, (setup, cleanup) = self.private_apt()
        subprocess.run(["/bin/sh", "-c", setup], env=environment, check=True)
        override = parts / APT_OVERRIDE
        override.write_text('Acquire::Retries "7";\n')
        result = subprocess.run(["/bin/sh", "-c", cleanup], env=environment, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("changed unexpectedly", result.stderr)
        self.assertEqual(override.read_text(), 'Acquire::Retries "7";\n')

    @unittest.skipUnless(shutil.which("apt-config"), "requires local apt-config, no network")
    def test_override_refuses_later_active_policy(self):
        parts, environment, (setup, _) = self.private_apt()
        later = parts / "zzzzz-other-policy"
        later.write_text('Acquire::Retries "8";\n')
        result = subprocess.run(["/bin/sh", "-c", setup], env=environment, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("did not set effective Acquire::Retries to 0", result.stderr)
        self.assertEqual(later.read_text(), 'Acquire::Retries "8";\n')

    @unittest.skipUnless(shutil.which("apt-config"), "requires local apt-config, no network")
    def test_override_allows_later_ignored_fragments(self):
        parts, environment, (setup, cleanup) = self.private_apt()
        ignored = [parts / ("zzzzz-other-policy" + suffix)
                   for suffix in (".dpkg-old", ".disabled")]
        for path in ignored:
            path.write_text('Acquire::Retries "8";\n')
        subprocess.run(["/bin/sh", "-c", setup], env=environment, check=True)
        effective = subprocess.check_output(["apt-config", "shell", "retries", "Acquire::Retries"], env=environment, text=True)
        self.assertEqual(effective.strip(), "retries='0'")
        subprocess.run(["/bin/sh", "-c", cleanup], env=environment, check=True)
        for path in ignored:
            self.assertEqual(path.read_text(), 'Acquire::Retries "8";\n')


if __name__ == "__main__":
    unittest.main()
