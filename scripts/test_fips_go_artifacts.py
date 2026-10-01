#!/usr/bin/env python3

import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from urllib.parse import parse_qs, urlsplit

ROOT = Path(__file__).resolve().parent.parent
PUBLIC_URL = "https://packages.trafficmanager.net/public"


def rule_values(distro, arch, include_fips="y"):
    names = (
        "SYMCRYPT_OPENSSL",
        "FIPS_VERSION",
        "FIPS_GOLANG_MAIN_VERSION",
        "FIPS_GOLANG_VERSION",
        "FIPS_GOLANG_URL_PREFIX",
        "FIPS_PACKAGE_ALL",
        "SONIC_MAKE_DEBS",
    )
    source = "include {}\nprint:\n".format(ROOT / "rules/sonic-fips.mk")
    source += "\t@printf '%s\\n' " + " ".join(
        "'$({})'".format(name) for name in names
    ) + "\n"
    result = subprocess.run(
        [
            "make", "--no-print-directory", "-f", "-", "print",
            "BLDENV=" + distro, "CONFIGURED_ARCH=" + arch,
            "BUILD_PUBLIC_URL=" + PUBLIC_URL, "INCLUDE_FIPS=" + include_fips,
        ],
        input=source, text=True, capture_output=True, check=True,
    )
    return dict(zip(names, result.stdout.splitlines()))


class FipsGoArtifactsTest(unittest.TestCase):
    def assert_artifact_url(self, url, arch, filename):
        parsed = urlsplit(url)
        self.assertEqual(parsed.scheme, "https")
        self.assertEqual(parsed.netloc, "sonic-build.azurewebsites.net")
        self.assertEqual(parsed.path, "/api/sonic/artifacts")
        self.assertEqual(parse_qs(parsed.query), {
            "buildId": ["1235906"],
            "definitionId": ["412"],
            "artifactName": ["fips-symcrypt-" + arch],
            "target": ["/" + filename],
        })
        self.assertIn("%2B", url)

    def test_package_downloads(self):
        for distro in ("trixie", "bookworm", "bullseye"):
            for arch in ("amd64", "arm64", "armhf"):
                with self.subTest(distro=distro, arch=arch):
                    values = rule_values(distro, arch)
                    with tempfile.TemporaryDirectory() as directory:
                        sandbox = Path(directory)
                        source = sandbox / "src/sonic-fips"
                        source.mkdir(parents=True)
                        (sandbox / "rules").mkdir()
                        shutil.copy(
                            ROOT / "rules/sonic-fips.mk",
                            sandbox / "rules/sonic-fips.mk",
                        )
                        shutil.copy(ROOT / "src/sonic-fips/Makefile", source)
                        binary = sandbox / "bin"
                        binary.mkdir()
                        curl = binary / "curl"
                        curl.write_text(
                            "#!/usr/bin/env python3\n"
                            "import json, os, pathlib, sys\n"
                            "args = sys.argv[1:]\n"
                            "output = args[args.index('-o') + 1]\n"
                            "with open(os.environ['CURL_LOG'], 'a') as log:\n"
                            "    log.write(json.dumps({'output': output, "
                            "'url': args[-1]}) + '\\n')\n"
                            "pathlib.Path(output).write_text('test package\\n')\n"
                        )
                        curl.chmod(0o755)
                        log = sandbox / "curl.jsonl"
                        destination = sandbox / "debs"
                        environment = dict(os.environ)
                        environment["PATH"] = (
                            str(binary) + os.pathsep + os.environ["PATH"]
                        )
                        environment["CURL_LOG"] = str(log)
                        subprocess.run(
                            [
                                "make", "--no-print-directory", "-C", str(source),
                                "BLDENV=" + distro, "CONFIGURED_ARCH=" + arch,
                                "BUILD_PUBLIC_URL=" + PUBLIC_URL, "INCLUDE_FIPS=y",
                                "SONIC_FIPS_BUILD_FROM_SOURCE=n",
                                "DEST=" + str(destination),
                                str(destination / values["SYMCRYPT_OPENSSL"]),
                            ],
                            env=environment, text=True,
                            capture_output=True, check=True,
                        )
                        records = [
                            json.loads(line)
                            for line in log.read_text().splitlines()
                        ]
                        packages = values["FIPS_PACKAGE_ALL"].split()
                        self.assertEqual(
                            [
                                Path(record["output"]).name
                                for record in records
                            ],
                            packages,
                        )
                        for record in records:
                            filename = Path(record["output"]).name
                            self.assertTrue(Path(record["output"]).is_file())
                            if (
                                distro == "trixie"
                                and filename.startswith("golang-")
                            ):
                                self.assert_artifact_url(
                                    record["url"], arch, filename,
                                )
                            else:
                                self.assertEqual(
                                    record["url"],
                                    "{}/fips/{}/{}/{}/{}".format(
                                        PUBLIC_URL, distro, values["FIPS_VERSION"],
                                        arch, filename,
                                    ),
                                )
                    if distro != "trixie":
                        self.assertEqual(values["FIPS_GOLANG_URL_PREFIX"], "")

    def test_slave_template(self):
        for arch in ("amd64", "arm64", "armhf"):
            for cross_build in ("n", "y"):
                for include_fips in ("n", "y"):
                    with self.subTest(
                        arch=arch, cross=cross_build, fips=include_fips,
                    ):
                        values = rule_values("trixie", arch, include_fips)
                        context = dict(values)
                        context.update(
                            BUILD_PUBLIC_URL=PUBLIC_URL,
                            CONFIGURED_ARCH=arch,
                            CROSS_BUILD_ENVIRON=cross_build,
                            DEFAULT_CONTAINER_REGISTRY="",
                            DOCKER_EXTRA_OPTS="",
                            INCLUDE_FIPS=include_fips,
                            MULTIARCH_QEMU_ENVIRON="n",
                        )
                        rendered = subprocess.run(
                            ["j2", str(ROOT / "sonic-slave-trixie/Dockerfile.j2")],
                            env=dict(os.environ, **context), cwd=ROOT,
                            text=True, capture_output=True, check=True,
                        ).stdout
                        downloads = dict(re.findall(
                            r"wget -O golang-(go|src)\.deb '([^']+)'", rendered,
                        ))
                        if include_fips == "n":
                            self.assertEqual(downloads, {})
                            self.assertNotIn("FIPS Go toolchain:", rendered)
                            self.assertNotIn("1235906", rendered)
                            self.assertEqual(values["SONIC_MAKE_DEBS"], "")
                            continue
                        self.assertEqual(set(downloads), {"go", "src"})
                        self.assert_artifact_url(
                            downloads["go"], arch,
                            "golang-1.26-go_1.26.7-1+fips_{}.deb".format(arch),
                        )
                        self.assert_artifact_url(
                            downloads["src"], arch,
                            "golang-1.26-src_1.26.7-1+fips_all.deb",
                        )
                        self.assertIn('test "$go_version" = go1.26.7', rendered)
                        self.assertIn('test "$go_fips140" = certified', rendered)
                        block = rendered.split("RUN wget -O golang-go.deb", 1)[1]
                        command = (
                            "wget -O golang-go.deb"
                            + block.split("\nENV PATH=", 1)[0]
                        )
                        subprocess.run(
                            ["/bin/sh", "-n", "-c", command],
                            text=True, capture_output=True, check=True,
                        )


if __name__ == "__main__":
    unittest.main()
