"""Check hardware watchdog wiring in complete native installer renders."""

import unittest

from tools.bazel.tests import final_image_template_test


class WatchdogImageTemplateTest(unittest.TestCase):
    def render_platform(self, platform):
        # Reuse the real Make recipe/environment and StrictUndefined renderer;
        # this renders the whole installer and checks its shell syntax without
        # executing installation or touching a watchdog.
        renderer = final_image_template_test.FinalImageTemplateTest()
        # Hardware platform makefiles normally export their own API wheel
        # paths. Supply harmless fixture paths for those non-VS render branches.
        settings = (f"CONFIGURED_PLATFORM := {platform}\n"
                    "export mlnx_platform_api_py3_wheel_path := target/python-wheels/mlnx_fixture.whl\n"
                    "export platform_api_py3_wheel_path := target/python-wheels/dpu_fixture.whl\n")
        rendered, _ = renderer.render(settings)
        return rendered

    def test_vs_omits_hardware_watchdog_but_retains_other_boot_services(self):
        rendered = self.render_platform("vs")
        self.assertNotIn("watchdog-control", rendered)
        self.assertIn('echo "warmboot-finalizer.service" | sudo tee -a $GENERATED_SERVICE_FILE', rendered)
        self.assertIn('echo "serial-config.service" | sudo tee -a $GENERATED_SERVICE_FILE', rendered)

    def test_hardware_and_dpu_images_retain_watchdog_installation_and_registration(self):
        commands = (
            "sudo LANG=C cp $IMAGE_CONFIGS/watchdog-control/watchdog-control.sh "
            "$FILESYSTEM_ROOT/usr/local/bin/watchdog-control.sh",
            "sudo LANG=C cp $IMAGE_CONFIGS/watchdog-control/watchdog-control.service "
            "$FILESYSTEM_ROOT_USR_LIB_SYSTEMD_SYSTEM",
            'echo "watchdog-control.service" | sudo tee -a $GENERATED_SERVICE_FILE',
        )
        for platform in ("broadcom", "mellanox", "nvidia-bluefield", "aspeed"):
            with self.subTest(platform=platform):
                rendered = self.render_platform(platform)
                for command in commands:
                    self.assertEqual(rendered.splitlines().count(command), 1)


if __name__ == "__main__":
    unittest.main()
