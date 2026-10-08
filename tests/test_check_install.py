import importlib.util
import io
import sys
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest import mock

import unittest

MODULE_PATH = Path(__file__).parents[1] / "check_install.py"
SPEC = importlib.util.spec_from_file_location("check_install", MODULE_PATH)
check_install = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check_install)

# Sentinel values only -- never real credentials.
LOGIN_SENTINEL = "login-S3cret!"
NEW_SENTINEL = "new-S3cret!"


def parse_and_resolve(argv, env):
    """Helper: parse argv with env applied, returning (login, new) or raising
    SystemExit (argparse error) like the real script would.
    """
    parser = check_install.build_arg_parser()
    args = parser.parse_args(argv)
    with mock.patch.dict(check_install.os.environ, env, clear=True):
        return check_install.resolve_credentials(parser, args)


class ResolveCredentialsTest(unittest.TestCase):

    def test_env_vars_only(self):
        login, new = parse_and_resolve(
            [], {"SONIC_PASSWORD": LOGIN_SENTINEL, "SONIC_NEW_PASSWORD": NEW_SENTINEL}
        )
        self.assertEqual(login, LOGIN_SENTINEL)
        self.assertEqual(new, NEW_SENTINEL)

    def test_cli_args_only(self):
        login, new = parse_and_resolve(
            ["-P", LOGIN_SENTINEL, "-N", NEW_SENTINEL], {}
        )
        self.assertEqual(login, LOGIN_SENTINEL)
        self.assertEqual(new, NEW_SENTINEL)

    def test_cli_overrides_env(self):
        login, new = parse_and_resolve(
            ["-P", "cli-login", "-N", "cli-new"],
            {"SONIC_PASSWORD": LOGIN_SENTINEL, "SONIC_NEW_PASSWORD": NEW_SENTINEL},
        )
        self.assertEqual(login, "cli-login")
        self.assertEqual(new, "cli-new")

    def test_credentials_with_shell_metacharacters(self):
        tricky = "p@ss$(id)`whoami`;&|<>"
        login, new = parse_and_resolve(
            ["-P", tricky, "-N", NEW_SENTINEL], {}
        )
        self.assertEqual(login, tricky)

    def test_missing_login_password_fails_before_spawn(self):
        parser = check_install.build_arg_parser()
        args = parser.parse_args(["-N", NEW_SENTINEL])
        with mock.patch.dict(check_install.os.environ, {}, clear=True), \
                mock.patch.object(check_install.pexpect, "spawn") as spawn_mock:
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                with self.assertRaises(SystemExit):
                    check_install.resolve_credentials(parser, args)
            spawn_mock.assert_not_called()
            self.assertIn("login password", stderr.getvalue())

    def test_missing_new_password_fails_before_spawn(self):
        parser = check_install.build_arg_parser()
        args = parser.parse_args(["-P", LOGIN_SENTINEL])
        with mock.patch.dict(check_install.os.environ, {}, clear=True), \
                mock.patch.object(check_install.pexpect, "spawn") as spawn_mock:
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                with self.assertRaises(SystemExit):
                    check_install.resolve_credentials(parser, args)
            spawn_mock.assert_not_called()
            self.assertIn("new password", stderr.getvalue())

    def test_empty_string_values_are_treated_as_missing(self):
        parser = check_install.build_arg_parser()
        args = parser.parse_args(["-P", "", "-N", NEW_SENTINEL])
        with mock.patch.dict(check_install.os.environ, {}, clear=True):
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                with self.assertRaises(SystemExit):
                    check_install.resolve_credentials(parser, args)

    def test_credentials_not_printed(self):
        parser = check_install.build_arg_parser()
        args = parser.parse_args(["-P", LOGIN_SENTINEL, "-N", NEW_SENTINEL])
        stdout = io.StringIO()
        stderr = io.StringIO()
        with mock.patch.dict(check_install.os.environ, {}, clear=True), \
                redirect_stdout(stdout), redirect_stderr(stderr):
            check_install.resolve_credentials(parser, args)
        self.assertNotIn(LOGIN_SENTINEL, stdout.getvalue())
        self.assertNotIn(LOGIN_SENTINEL, stderr.getvalue())
        self.assertNotIn(NEW_SENTINEL, stdout.getvalue())
        self.assertNotIn(NEW_SENTINEL, stderr.getvalue())


class ForcedPasswordChangeFlowTest(unittest.TestCase):
    """Exercise main()'s pexpect-driven flow with a fake pexpect child to
    confirm the resolved login/new passwords (not raw args.P/args.N) are
    used consistently in the forced-password-change sequence, and the
    original password is restored afterward.
    """

    def test_forced_password_change_restores_original(self):
        sent = []

        fake_child = mock.Mock()
        fake_child.sendline.side_effect = lambda s=None: sent.append(s)

        # Sequence of p.expect(...) return values, in call order:
        # 1) grub_selection (implicit via p.expect(grub_selection) -> index unused)
        # 2) login_prompt index (0) from the [login_prompt, passwd_prompt, firsttime_prompt, cmd_prompt] list
        # 3) passwd_prompt index (1)
        # 4) 'Current password:' (no TIMEOUT -> enters forced change branch)
        # 5) 'New password:' prompt
        # 6) 'Retype new password:' prompt
        # 7) 'Current password:' (restore sequence)
        # 8) 'New password:' (restore sequence)
        # 9) 'Retype new password:' (restore sequence)
        # 10) cmd_prompt reached, loop breaks
        expect_results = iter([
            None,  # grub_selection expect (value unused by caller)
            0,     # login_prompt
            1,     # passwd_prompt
            None,  # 'Current password:' succeeds (no TIMEOUT) -> forced change path
            None,  # passwd_change_prompt[1] 'New password:'
            None,  # passwd_change_prompt[2] 'Retype new password:'
            None,  # restore: passwd_change_prompt[0] 'Current password:'
            None,  # restore: passwd_change_prompt[1] 'New password:'
            None,  # restore: passwd_change_prompt[2] 'Retype new password:'
            3,     # cmd_prompt reached -> else branch breaks outer while
            None, None, None, None,  # uptime/show version/show bgp/sync expects
        ])

        def expect_side_effect(*args, **kwargs):
            return next(expect_results)

        fake_child.expect.side_effect = expect_side_effect

        with mock.patch.object(check_install.pexpect, "spawn", return_value=fake_child), \
                mock.patch.object(check_install.time, "sleep", return_value=None), \
                mock.patch.object(
                    sys, "argv",
                    ["check_install.py", "-P", LOGIN_SENTINEL, "-N", NEW_SENTINEL],
                ), \
                mock.patch.dict(check_install.os.environ, {}, clear=True):
            check_install.main()

        # Login password sent first, then on forced-change: old password,
        # new password (twice), then restore flow sends new->old->old.
        self.assertIn(LOGIN_SENTINEL, sent)
        self.assertIn(NEW_SENTINEL, sent)
        # The very last password-bearing sendline in the restore sequence
        # must be the original login password (password restored).
        password_sends = [s for s in sent if s in (LOGIN_SENTINEL, NEW_SENTINEL)]
        self.assertEqual(password_sends[-1], LOGIN_SENTINEL)


if __name__ == '__main__':
    unittest.main()
