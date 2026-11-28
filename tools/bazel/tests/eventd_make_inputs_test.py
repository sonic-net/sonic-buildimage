#!/usr/bin/env python3
"""Check eventd's Docker context inputs without building packages or images."""

from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[3]


class EventdMakeInputsTest(unittest.TestCase):
    def test_template_is_staged_without_building_the_legacy_config_engine(self):
        # Use the production copy recipe and Docker prerequisites. A source-built
        # config-engine base does not populate Make's shared files directory.
        slave = (ROOT / "slave.mk").read_text()
        copy_start = "$(addprefix $(FILES_PATH)/, $(SONIC_COPY_FILES)) :"
        copy_rule = copy_start + slave.split(copy_start, 1)[1].split(
            "\nSONIC_TARGET_LIST", 1
        )[0]
        docker_start = "$(addprefix $(TARGET_PATH)/, $(DOCKER_IMAGES)) :"
        docker_rule = docker_start + slave.split(docker_start, 1)[1].split(
            "\n\t$(HEADER)", 1
        )[0]

        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary) / "target"
            files = target / "files"
            files.mkdir(parents=True)
            template = files / "rsyslog_plugin.conf.j2"
            self.assertFalse(template.exists())
            makefile = f"""
.SECONDEXPANSION:
TARGET_PATH := {target}
FILES_PATH := $(TARGET_PATH)/files
DOCKERS_PATH := dockers
DOCKER_CONFIG_ENGINE_TRIXIE := docker-config-engine-trixie.gz
include rules/scripts.mk
include rules/docker-eventd.mk
DOCKER_IMAGES := $(DOCKER_EVENTD)
$(DOCKER_EVENTD)_FILES_PATH := $(FILES_PATH)
.PHONY: .platform docker-start $(TARGET_PATH)/$(DOCKER_CONFIG_ENGINE_TRIXIE)-load
.platform docker-start $(TARGET_PATH)/$(DOCKER_CONFIG_ENGINE_TRIXIE)-load:
\t@:
{copy_rule}
{docker_rule}
\t@test -s $(FILES_PATH)/rsyslog_plugin.conf.j2
\t@touch $@
"""
            result = subprocess.run(
                ["make", "--no-print-directory", "-f", "-", str(target / "docker-eventd.gz")],
                input=makefile, cwd=ROOT, text=True, capture_output=True, check=False,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(template.read_bytes(),
                             (ROOT / "files/build_templates/rsyslog_plugin.conf.j2").read_bytes())
            self.assertTrue((target / "docker-eventd.gz").exists())


if __name__ == "__main__":
    unittest.main()
