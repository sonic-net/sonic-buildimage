#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""
Unit tests for hwmgmt_resolve_ref.

Uses real `git` against tmp directories to exercise the resolver end-to-end,
including the targeted-fetch fallback. No pyfakefs dependency, so these tests
run in any environment with python3 and git.
"""

import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import hwmgmt_resolve_ref as resolver


def run(cmd, cwd=None, check=True, env=None):
    return subprocess.run(
        cmd, cwd=cwd, check=check, capture_output=True, text=True, env=env
    )


def init_repo(path):
    os.makedirs(path, exist_ok=True)
    run(['git', 'init', '-q', '-b', 'master', path])
    run(['git', 'config', 'user.email', 'test@example.com'], cwd=path)
    run(['git', 'config', 'user.name', 'Test'], cwd=path)
    run(['git', 'config', 'commit.gpgsign', 'false'], cwd=path)


def commit_changelog(repo, version, msg='c'):
    """Write debian/changelog with given version and commit. Return commit SHA."""
    cl_dir = os.path.join(repo, 'debian')
    os.makedirs(cl_dir, exist_ok=True)
    with open(os.path.join(cl_dir, 'changelog'), 'w') as f:
        f.write('hw-management (1.mlnx.{}) unstable; urgency=low\n'.format(version))
        f.write('  [ MLNX ]\n\n')
        f.write(' -- NBU BSP <nbu@example.com>  Thu, 30 Apr 2026 17:04:57 +0300\n')
    run(['git', 'add', 'debian/changelog'], cwd=repo)
    run(['git', 'commit', '-q', '-m', msg], cwd=repo)
    return run(['git', 'rev-parse', 'HEAD'], cwd=repo).stdout.strip()


def commit_raw_changelog(repo, content, msg='c'):
    """Write debian/changelog with raw content (used to test malformed parsing)."""
    cl_dir = os.path.join(repo, 'debian')
    os.makedirs(cl_dir, exist_ok=True)
    with open(os.path.join(cl_dir, 'changelog'), 'w') as f:
        f.write(content)
    run(['git', 'add', 'debian/changelog'], cwd=repo)
    run(['git', 'commit', '-q', '-m', msg], cwd=repo)
    return run(['git', 'rev-parse', 'HEAD'], cwd=repo).stdout.strip()


def commit_blob(repo, name='README.md', text='hello', msg='c'):
    """Commit an arbitrary file (used when we don't need a changelog)."""
    with open(os.path.join(repo, name), 'w') as f:
        f.write(text)
    run(['git', 'add', name], cwd=repo)
    run(['git', 'commit', '-q', '-m', msg], cwd=repo)
    return run(['git', 'rev-parse', 'HEAD'], cwd=repo).stdout.strip()


def create_tag(repo, tag, ref='HEAD'):
    run(['git', 'tag', tag, ref], cwd=repo)


def create_branch(repo, branch, ref='HEAD'):
    run(['git', 'branch', branch, ref], cwd=repo)


def add_origin(repo, url):
    run(['git', 'remote', 'add', 'origin', url], cwd=repo)


def read_env(env_file):
    """Source the env file in bash and dump key=value lines for inspection."""
    if not os.path.isfile(env_file):
        return {}
    keys = ['HWMGMT_INPUT', 'HWMGMT_REF_TYPE', 'HWMGMT_CHECKOUT_REF',
            'HWMGMT_COMMIT', 'HWMGMT_BASE_VERSION',
            'HWMGMT_DISTINCT_ID', 'HWMGMT_PACKAGE_VERSION']
    cmd = '. ' + shlex.quote(env_file) + '; ' + '; '.join(
        'printf "%s=%s\\n" {} "${}"'.format(k, '{' + k + '}') for k in keys
    )
    res = subprocess.run(['bash', '-c', cmd], capture_output=True, text=True)
    out = {}
    for line in res.stdout.splitlines():
        if '=' in line:
            k, _, v = line.partition('=')
            out[k] = v
    return out


def call_resolver(repo, input_value, env_file, verbose=False):
    """Invoke main() in-process and capture exit code + stdout/stderr."""
    argv = ['hwmgmt_resolve_ref.py',
            '--repo', repo,
            '--input', input_value,
            '--env-file', env_file]
    if verbose:
        argv.append('--verbose')

    from io import StringIO
    stdout, stderr = StringIO(), StringIO()
    code = 0
    with mock.patch.object(sys, 'argv', argv):
        with mock.patch.object(sys, 'stdout', stdout):
            with mock.patch.object(sys, 'stderr', stderr):
                try:
                    resolver.main()
                except SystemExit as e:
                    code = e.code if isinstance(e.code, int) else 1
    return code, stdout.getvalue(), stderr.getvalue()


class _GitFixture(unittest.TestCase):
    """Base class providing tmp dirs and clean teardown."""

    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix='hwmgmt-resolve-test-')
        self.env_file = os.path.join(self.tmpdir, 'resolved.env')

    def tearDown(self):
        shutil.rmtree(self.tmpdir, ignore_errors=True)


class TestTagPath(_GitFixture):
    """Tag flow regression cases (preserve byte-identical existing behavior)."""

    def test_01_tag_success(self):
        """Case 1: tag V.7.0001.0026 exists locally; package version == input (byte-identical to today)."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        sha = commit_blob(repo)
        create_tag(repo, 'V.7.0001.0026')
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        code, stdout, stderr = call_resolver(repo, '7.0001.0026', self.env_file)
        self.assertEqual(code, 0, msg='stderr={}'.format(stderr))
        env = read_env(self.env_file)
        self.assertEqual(env['HWMGMT_REF_TYPE'], 'tag')
        self.assertEqual(env['HWMGMT_CHECKOUT_REF'], 'V.7.0001.0026')
        self.assertEqual(env['HWMGMT_COMMIT'], sha)
        self.assertEqual(env['HWMGMT_INPUT'], '7.0001.0026')
        # Tag flow: package version is the user input verbatim, no distinct id.
        self.assertEqual(env['HWMGMT_BASE_VERSION'], '7.0001.0026')
        self.assertEqual(env['HWMGMT_DISTINCT_ID'], '')
        self.assertEqual(env['HWMGMT_PACKAGE_VERSION'], '7.0001.0026')

    def test_02_tag_wins_over_branch(self):
        """Case 2: tag V.same-name and branch same-name at different commits — tag wins."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        sha_a = commit_blob(repo, msg='A')
        create_tag(repo, 'V.same-name', sha_a)
        sha_b = commit_blob(repo, name='B.md', msg='B')
        create_branch(repo, 'same-name', sha_b)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        code, _, stderr = call_resolver(repo, 'same-name', self.env_file)
        self.assertEqual(code, 0, msg='stderr={}'.format(stderr))
        env = read_env(self.env_file)
        self.assertEqual(env['HWMGMT_REF_TYPE'], 'tag')
        self.assertEqual(env['HWMGMT_COMMIT'], sha_a)
        # Tag path: BASE = input verbatim, no distinct id, PACKAGE = BASE.
        self.assertEqual(env['HWMGMT_BASE_VERSION'], 'same-name')
        self.assertEqual(env['HWMGMT_DISTINCT_ID'], '')
        self.assertEqual(env['HWMGMT_PACKAGE_VERSION'], 'same-name')

    def test_02b_tag_offline_no_origin(self):
        """Tag flow must work in a repo with no 'origin' remote when the tag is local
        (backward compat with offline / pre-prepared trees)."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        sha = commit_blob(repo)
        create_tag(repo, 'V.1.0.0')
        # Deliberately do NOT add any remote.

        code, stdout, stderr = call_resolver(repo, '1.0.0', self.env_file)
        self.assertEqual(code, 0,
                         msg='offline tag resolution must succeed; stderr={}'.format(stderr))
        env = read_env(self.env_file)
        self.assertEqual(env['HWMGMT_REF_TYPE'], 'tag')
        self.assertEqual(env['HWMGMT_PACKAGE_VERSION'], '1.0.0')

    def test_02c_tag_fetched_from_origin(self):
        """Remote-only tag: not local, present on origin → resolver fetches and resolves."""
        upstream = os.path.join(self.tmpdir, 'upstream')
        init_repo(upstream)
        sha = commit_blob(upstream)
        create_tag(upstream, 'V.7.0001.0026', sha)

        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', upstream, bare])

        # Local repo: init + add origin pointing at bare, do NOT fetch tags.
        local = os.path.join(self.tmpdir, 'local')
        init_repo(local)
        commit_blob(local)
        add_origin(local, bare)

        # Sanity: tag is not yet local.
        self.assertIsNone(resolver.rev_parse(local, 'refs/tags/V.7.0001.0026'))

        code, stdout, stderr = call_resolver(local, '7.0001.0026', self.env_file)
        self.assertEqual(code, 0,
                         msg='remote-only tag resolution must succeed; stderr={}'.format(stderr))
        env = read_env(self.env_file)
        self.assertEqual(env['HWMGMT_REF_TYPE'], 'tag')
        self.assertEqual(env['HWMGMT_COMMIT'], sha)
        self.assertEqual(env['HWMGMT_PACKAGE_VERSION'], '7.0001.0026')
        # Tag is now local after the fetch.
        self.assertEqual(resolver.rev_parse(local, 'refs/tags/V.7.0001.0026'), sha)


class TestBranchPath(_GitFixture):
    """Branch fallback flow with debian/changelog version parsing."""

    def test_03_branch_fallback_changelog_version(self):
        """Case 3: branch resolves; BASE from changelog, PACKAGE = BASE-<sha9>."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_changelog(repo, '7.0050.0000', msg='base')
        sha_branch = commit_changelog(repo, '7.0060.1430', msg='bump')
        # Reset master to the base commit so 'bmc-draft' diverges.
        run(['git', 'reset', '-q', '--hard', 'HEAD~1'], cwd=repo)
        create_branch(repo, 'bmc-draft', sha_branch)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        code, _, stderr = call_resolver(repo, 'bmc-draft', self.env_file)
        self.assertEqual(code, 0, msg='stderr={}'.format(stderr))
        env = read_env(self.env_file)
        self.assertEqual(env['HWMGMT_REF_TYPE'], 'branch')
        self.assertEqual(env['HWMGMT_COMMIT'], sha_branch)
        self.assertEqual(env['HWMGMT_BASE_VERSION'], '7.0060.1430')
        self.assertEqual(env['HWMGMT_DISTINCT_ID'], sha_branch[:9])
        self.assertEqual(env['HWMGMT_PACKAGE_VERSION'],
                         '7.0060.1430-{}'.format(sha_branch[:9]))

    def test_04_branch_with_slash(self):
        """Case 4: branch name 'feature/foo' resolves and produces a sane suffixed package version."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        sha = commit_changelog(repo, '7.0060.1430')
        create_branch(repo, 'feature/foo', sha)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        code, _, stderr = call_resolver(repo, 'feature/foo', self.env_file)
        self.assertEqual(code, 0, msg='stderr={}'.format(stderr))
        env = read_env(self.env_file)
        self.assertEqual(env['HWMGMT_REF_TYPE'], 'branch')
        self.assertEqual(env['HWMGMT_BASE_VERSION'], '7.0060.1430')
        self.assertEqual(env['HWMGMT_PACKAGE_VERSION'],
                         '7.0060.1430-{}'.format(sha[:9]))
        # The unsafe '/' in the input must NOT propagate into the package version.
        self.assertNotIn('/', env['HWMGMT_PACKAGE_VERSION'])

    def test_05_remote_branch_needs_fetch(self):
        """Case 5: local has no remote-tracking branch; resolver fetches from origin first."""
        upstream = os.path.join(self.tmpdir, 'upstream')
        init_repo(upstream)
        sha = commit_changelog(upstream, '7.0060.1430')
        create_branch(upstream, 'bmc-draft', sha)

        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', upstream, bare])

        # Local repo: init + add origin pointing at bare, do NOT fetch.
        local = os.path.join(self.tmpdir, 'local')
        init_repo(local)
        commit_blob(local)
        add_origin(local, bare)

        self.assertIsNone(resolver.rev_parse(local, 'refs/remotes/origin/bmc-draft'))

        code, _, stderr = call_resolver(local, 'bmc-draft', self.env_file)
        self.assertEqual(code, 0, msg='stderr={}'.format(stderr))
        env = read_env(self.env_file)
        self.assertEqual(env['HWMGMT_REF_TYPE'], 'branch')
        self.assertEqual(env['HWMGMT_COMMIT'], sha)
        self.assertEqual(env['HWMGMT_PACKAGE_VERSION'],
                         '7.0060.1430-{}'.format(sha[:9]))

    def test_05b_branch_fetch_first_picks_up_upstream_advance(self):
        """Resolver must fetch before resolving, so an upstream advance is picked up
        even when refs/remotes/origin/<branch> already exists locally pointing at
        the old commit (regression test for fast-forward stale-ref bug)."""
        upstream = os.path.join(self.tmpdir, 'upstream')
        init_repo(upstream)
        sha_old = commit_changelog(upstream, '7.0050.0000', msg='old')
        create_branch(upstream, 'moving', sha_old)

        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', upstream, bare])

        local = os.path.join(self.tmpdir, 'local')
        run(['git', 'clone', '-q', bare, local])
        run(['git', 'config', 'user.email', 'test@example.com'], cwd=local)
        run(['git', 'config', 'user.name', 'Test'], cwd=local)

        # Sanity: local already has the old commit as origin/moving.
        before = resolver.rev_parse(local, 'refs/remotes/origin/moving')
        self.assertEqual(before, sha_old)

        # Upstream advances past the local ref (fast-forward).
        sha_new = commit_changelog(upstream, '7.0060.1430', msg='new')
        run(['git', 'branch', '-f', 'moving', sha_new], cwd=upstream)
        # Push the new commit to the bare so 'origin' has it.
        run(['git', 'push', '-q', '-f', bare, 'moving:moving'], cwd=upstream)

        code, _, stderr = call_resolver(local, 'moving', self.env_file)
        self.assertEqual(code, 0, msg='stderr={}'.format(stderr))
        env = read_env(self.env_file)
        # The fetch must have run and picked up sha_new, NOT sha_old.
        self.assertEqual(env['HWMGMT_COMMIT'], sha_new,
                         msg='resolver used stale local ref instead of fetching')
        self.assertEqual(env['HWMGMT_BASE_VERSION'], '7.0060.1430')

    def test_05c_branch_force_rewind_picks_up_unrelated_tip(self):
        """Force-pushed/rebased upstream branch (non-fast-forward update) must NOT
        leave the resolver pegged on the stale local remote-tracking ref. Regression
        test for the missing '+' on the fetch refspec.
        """
        upstream = os.path.join(self.tmpdir, 'upstream')
        init_repo(upstream)
        # Initial 'moving' tip: a chain of two commits, branch points at HEAD.
        commit_changelog(upstream, '7.0050.0000', msg='base')
        sha_old = commit_changelog(upstream, '7.0060.0000', msg='old-tip')
        run(['git', 'branch', '-f', 'moving', sha_old], cwd=upstream)

        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', upstream, bare])

        local = os.path.join(self.tmpdir, 'local')
        run(['git', 'clone', '-q', bare, local])
        run(['git', 'config', 'user.email', 'test@example.com'], cwd=local)
        run(['git', 'config', 'user.name', 'Test'], cwd=local)

        # Local has origin/moving pointing at sha_old.
        self.assertEqual(
            resolver.rev_parse(local, 'refs/remotes/origin/moving'), sha_old)

        # Upstream rewinds 'moving' to an EARLIER commit (non-fast-forward
        # from the local view). Without '+' in the fetch refspec, git fetch
        # rejects this update.
        run(['git', 'reset', '-q', '--hard', 'HEAD~1'], cwd=upstream)
        sha_new = run(['git', 'rev-parse', 'HEAD'], cwd=upstream).stdout.strip()
        run(['git', 'branch', '-f', 'moving', sha_new], cwd=upstream)
        run(['git', 'push', '-q', '-f', bare, 'moving:moving'], cwd=upstream)
        self.assertNotEqual(sha_new, sha_old)

        code, _, stderr = call_resolver(local, 'moving', self.env_file)
        self.assertEqual(code, 0, msg='stderr={}'.format(stderr))
        env = read_env(self.env_file)
        # If the resolver fetches with '+', it will pick up sha_new. Without
        # '+' it falls back to the stale local ref pointing at sha_old.
        self.assertEqual(
            env['HWMGMT_COMMIT'], sha_new,
            msg='resolver clung to stale local ref after force-update')


class TestFailureCases(_GitFixture):
    """Failure modes — all must exit non-zero with a useful message."""

    def _make_repo_with_branch(self, branch_name, changelog_content=None, version=None):
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        if changelog_content is not None:
            sha = commit_raw_changelog(repo, changelog_content)
        elif version is not None:
            sha = commit_changelog(repo, version)
        else:
            sha = commit_blob(repo)
        create_branch(repo, branch_name, sha)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)
        return repo

    def test_06_branch_changelog_malformed(self):
        """Case 6: branch resolves, changelog first line doesn't match the regex."""
        repo = self._make_repo_with_branch(
            'bar',
            changelog_content='garbage (oops) unstable; urgency=low\n',
        )
        code, _, stderr = call_resolver(repo, 'bar', self.env_file)
        self.assertNotEqual(code, 0)
        self.assertIn('debian/changelog', stderr)
        self.assertFalse(os.path.isfile(self.env_file),
                         'env file must not exist on failure')

    def test_07_branch_parsed_version_unsafe(self):
        """Case 7: changelog version contains '/' — must be rejected."""
        repo = self._make_repo_with_branch(
            'baz',
            changelog_content='hw-management (1.mlnx.7.0060.1430_dev/foo) unstable;\n',
        )
        code, _, stderr = call_resolver(repo, 'baz', self.env_file)
        self.assertNotEqual(code, 0)
        self.assertIn('not safe for deb filenames', stderr)

    def test_08_neither_tag_nor_branch(self):
        """Case 8: input matches no tag and no branch — error mentions both."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_blob(repo)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        code, _, stderr = call_resolver(repo, 'does-not-exist', self.env_file)
        self.assertNotEqual(code, 0)
        self.assertIn('V.does-not-exist', stderr)
        self.assertIn("'does-not-exist'", stderr)
        self.assertIn('neither', stderr)

    def test_09_empty_input(self):
        """Case 9: --input '' fails before any git op."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_blob(repo)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        code, _, stderr = call_resolver(repo, '', self.env_file)
        self.assertNotEqual(code, 0)
        self.assertIn('empty', stderr)

    def test_10_no_origin_remote_and_no_local_match(self):
        """Repo has no 'origin' remote AND no local tag/branch matches the input.

        The resolver must still fail (since there's nothing to resolve), but
        the offline-tag-with-local-match case is covered separately by
        test_02b_tag_offline_no_origin to confirm absence of `origin` is
        no longer a blocking precondition.
        """
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_blob(repo)
        # No tags or matching branches.

        code, _, stderr = call_resolver(repo, '7.0001.0026', self.env_file)
        self.assertNotEqual(code, 0)
        # The error should mention both attempts, and may mention origin in
        # the diagnostic detail since the fetch could not run.
        self.assertIn('neither', stderr)

    def test_11_branch_fetch_fails_when_origin_invalid(self):
        """Case 11: origin URL is bogus, fetch fails, error surfaces."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_blob(repo)
        # Point origin at a path that doesn't exist; fetches will fail.
        add_origin(repo, os.path.join(self.tmpdir, 'nonexistent.git'))

        code, _, stderr = call_resolver(repo, 'never-existed', self.env_file)
        self.assertNotEqual(code, 0)
        self.assertIn('neither', stderr)

    def test_11b_no_silent_fallback_to_stale_remote_tracking_ref(self):
        """When origin exists but fetch fails AND a stale local
        refs/remotes/origin/<branch> exists, the resolver must NOT silently
        use the stale ref. Regression test for the 'fetch failed → use
        whatever's local' anti-pattern.
        """
        # Set up a real remote with the branch, clone locally so the stale
        # remote-tracking ref exists, then break the origin URL so any future
        # fetch fails.
        upstream = os.path.join(self.tmpdir, 'upstream')
        init_repo(upstream)
        sha = commit_changelog(upstream, '7.0050.0000')
        create_branch(upstream, 'mybranch', sha)

        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', upstream, bare])

        local = os.path.join(self.tmpdir, 'local')
        run(['git', 'clone', '-q', bare, local])
        # Sanity: the stale remote-tracking ref is now local.
        self.assertEqual(
            resolver.rev_parse(local, 'refs/remotes/origin/mybranch'), sha)

        # Break the origin URL so subsequent fetches fail.
        run(['git', 'remote', 'set-url', 'origin',
             os.path.join(self.tmpdir, 'gone.git')], cwd=local)

        code, _, stderr = call_resolver(local, 'mybranch', self.env_file)
        self.assertNotEqual(code, 0,
                            msg='resolver must fail rather than use stale ref')
        # No env file should be written on failure.
        self.assertFalse(os.path.isfile(self.env_file))

    def test_12a_invalid_input_chars(self):
        """Whitespace and shell metacharacters are rejected at the input layer."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_blob(repo)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        for bad in ['has space', 'has;semicolon', 'has`tick`', 'has$dollar']:
            code, _, stderr = call_resolver(repo, bad, self.env_file)
            self.assertNotEqual(code, 0,
                                msg='input {!r} should have been rejected'.format(bad))
            self.assertIn('invalid characters', stderr)

    def test_12c_invalid_git_refname_branch(self):
        """Inputs that pass INPUT_RE but fail Git's refname rules (e.g. contain
        '..' or end with '.lock') must be rejected on the branch path by
        git check-ref-format, with a clean diagnostic rather than a cryptic
        git fetch failure."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_blob(repo)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        # These inputs all pass INPUT_RE (only contain chars from the
        # permissive set) but fail Git's stricter refname rules, so the
        # check-ref-format guard is the layer that rejects them.
        for bad in ['has..dotdot', 'ends.lock']:
            code, _, stderr = call_resolver(repo, bad, self.env_file)
            self.assertNotEqual(code, 0,
                                msg='input {!r} should have been rejected'.format(bad))
            self.assertIn('invalid git branch refname', stderr,
                          msg='input {!r} should be flagged by check-ref-format'.format(bad))

    def test_12b_input_starts_with_dash(self):
        """Inputs starting with '-' must be rejected by our own validation.

        Pass via --input=<val> form so argparse doesn't treat the value as a flag;
        this exercises the resolver's own dash-prefix check.
        """
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_blob(repo)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        argv = ['hwmgmt_resolve_ref.py',
                '--repo', repo,
                '--input=-rf',
                '--env-file', self.env_file]
        from io import StringIO
        stdout, stderr = StringIO(), StringIO()
        code = 0
        with mock.patch.object(sys, 'argv', argv):
            with mock.patch.object(sys, 'stdout', stdout):
                with mock.patch.object(sys, 'stderr', stderr):
                    try:
                        resolver.main()
                    except SystemExit as e:
                        code = e.code if isinstance(e.code, int) else 1
        self.assertNotEqual(code, 0)
        self.assertIn('must not start with', stderr.getvalue())


class TestEnvFileFormat(_GitFixture):
    """Verify the env file format and atomicity invariants."""

    def test_14_atomic_write_no_tmp_left_behind(self):
        """After a successful run, no .tmp file should remain alongside the env file."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_blob(repo)
        create_tag(repo, 'V.1.0.0')
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        for _ in range(3):
            code, _, _ = call_resolver(repo, '1.0.0', self.env_file)
            self.assertEqual(code, 0)
            self.assertFalse(os.path.isfile(self.env_file + '.tmp'),
                             'temp file leaked after successful run')
            self.assertTrue(os.path.isfile(self.env_file))

    def test_16_env_file_write_failure_clean_diagnostic(self):
        """Env-file write failures must emit -> FATAL:, not a Python traceback."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        commit_blob(repo)
        create_tag(repo, 'V.1.0.0')

        # Path-is-a-directory case
        bad_dir = os.path.join(self.tmpdir, 'is_a_dir')
        os.makedirs(bad_dir, exist_ok=True)
        code, _, stderr = call_resolver(repo, '1.0.0', bad_dir)
        self.assertNotEqual(code, 0)
        self.assertIn('FATAL', stderr)
        self.assertIn('failed to write env file', stderr)
        self.assertNotIn('Traceback', stderr)
        # No leftover .tmp file alongside the directory.
        self.assertFalse(os.path.isfile(bad_dir + '.tmp'))

    def test_15_values_with_special_chars_quoted_safely(self):
        """Branch names with '/' must round-trip through the env file unchanged.
        With fetch-first behavior in effect (origin present), the resolver must
        select the remote-tracking ref."""
        repo = os.path.join(self.tmpdir, 'repo')
        init_repo(repo)
        sha = commit_changelog(repo, '7.0060.1430')
        create_branch(repo, 'feature/foo', sha)
        bare = os.path.join(self.tmpdir, 'origin.git')
        run(['git', 'clone', '-q', '--bare', repo, bare])
        add_origin(repo, bare)

        code, _, stderr = call_resolver(repo, 'feature/foo', self.env_file)
        self.assertEqual(code, 0, msg='stderr={}'.format(stderr))
        env = read_env(self.env_file)
        self.assertEqual(env['HWMGMT_INPUT'], 'feature/foo')
        # Origin is present and the fetch succeeds, so the checkout ref must
        # be the remote-tracking form.
        self.assertEqual(env['HWMGMT_CHECKOUT_REF'],
                         'refs/remotes/origin/feature/foo')
        self.assertEqual(env['HWMGMT_COMMIT'], sha)


if __name__ == '__main__':
    unittest.main()
