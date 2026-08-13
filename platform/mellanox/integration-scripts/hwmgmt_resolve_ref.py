#!/usr/bin/env python
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
Resolve MLNX_HW_MANAGEMENT_VERSION to a hw-mgmt git ref and write a
sourceable env file (shlex.quote'd).

Precedence: tag V.<input> wins, else branch <input>. Tag flow is
byte-identical to today (HWMGMT_PACKAGE_VERSION == input); branch flow
appends "-<sha9>" so two runs on the same branch at different commits
produce distinct deb filenames. Branches fetch first and fail if the
fetch fails — a stale local remote-tracking ref must not silently
shadow upstream.

The changelog regex mirrors hw-mgmt's own get_hw_mgmt_ver() in
recipes-kernel/linux/deploy_kernel_patches.py.
"""

import argparse
import os
import re
import shlex
import subprocess
import sys


# Permissive: real branch names like "feature/foo", "V.7.0010.1000_BR".
INPUT_RE = re.compile(r'^[A-Za-z0-9._/+~:-]+$')

# Strict: deb-version-safe AND git-refname-safe. The parsed changelog
# version lands in `git checkout -B "..._<version>"` under CREATE_BRANCH=y,
# so '~', '_', ':', '/' (legal in deb versions but not git refnames or deb
# filenames) are excluded.
PARSED_VERSION_RE = re.compile(r'^[A-Za-z0-9.+-]+$')

# First line: "hw-management (1.mlnx.7.0060.1430) unstable; urgency=low"
CHANGELOG_RE = re.compile(r'^hw-management \(1\.mlnx\.([^)]+)\)')

REMOTE = 'origin'
DISTINCT_ID_LEN = 9  # abbreviated commit SHA suffix on the branch path

ENV_KEYS = (
    'HWMGMT_INPUT',
    'HWMGMT_REF_TYPE',
    'HWMGMT_CHECKOUT_REF',
    'HWMGMT_COMMIT',
    'HWMGMT_BASE_VERSION',
    'HWMGMT_DISTINCT_ID',
    'HWMGMT_PACKAGE_VERSION',
)


def fail(msg):
    print('-> FATAL: ' + msg, file=sys.stderr)
    sys.exit(1)


def info(msg, verbose):
    if verbose:
        print(msg)


def run_git(repo, args):
    cmd = ['git', '-C', repo] + list(args)
    return subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def rev_parse(repo, ref):
    res = run_git(repo, ['rev-parse', '--verify', '--quiet', ref + '^{commit}'])
    if res.returncode == 0:
        return res.stdout.strip()
    return None


def have_origin(repo):
    res = run_git(repo, ['remote', 'get-url', REMOTE])
    return res.returncode == 0


def is_shallow(repo):
    res = run_git(repo, ['rev-parse', '--is-shallow-repository'])
    return res.returncode == 0 and res.stdout.strip() == 'true'


def parse_changelog_version(repo, commit):
    res = run_git(repo, ['show', '{}:debian/changelog'.format(commit)])
    if res.returncode != 0:
        return None
    first_line = res.stdout.splitlines()[0] if res.stdout else ''
    m = CHANGELOG_RE.match(first_line)
    return m.group(1) if m else None


def resolve_tag(repo, value, verbose):
    """Resolve V.<value> as a tag. Tags are immutable, so a local match is
    final; only fetch from origin if the tag is missing locally.
    Returns (checkout_ref, commit, fetch_stderr) or (None, None, stderr).
    """
    tag_full_ref = 'refs/tags/V.{}'.format(value)
    sha = rev_parse(repo, tag_full_ref)
    if sha:
        info('-> hw-mgmt: tag {} found locally'.format(tag_full_ref), verbose)
        return 'V.{}'.format(value), sha, ''
    if not have_origin(repo):
        info('-> hw-mgmt: tag {} not local and no origin remote; cannot fetch'.format(tag_full_ref), verbose)
        return None, None, 'tag missing locally and no origin remote configured'
    info('-> hw-mgmt: tag {} not local, attempting targeted fetch'.format(tag_full_ref), verbose)
    res = run_git(repo, ['fetch', REMOTE, 'tag', 'V.{}'.format(value)])
    info('-> hw-mgmt: fetch tag rc={} stderr={!r}'.format(
        res.returncode, (res.stderr or '').strip()), verbose)
    if res.returncode != 0:
        return None, None, (res.stderr or '').strip()
    sha = rev_parse(repo, tag_full_ref)
    if sha:
        return 'V.{}'.format(value), sha, ''
    return None, None, ''


def resolve_branch(repo, value, verbose):
    """Resolve <value> as a branch. Branches move, so we force-fetch and
    fail loudly if the fetch fails — never silently fall back to a
    possibly-stale local remote-tracking ref. Local refs are only used in
    offline mode (no `origin` configured).
    Returns (checkout_ref, commit, fetch_stderr) or (None, None, stderr).
    """
    # check-ref-format catches inputs like `..` or `.lock` that pass
    # INPUT_RE but would produce cryptic git fetch errors.
    chk = run_git(repo, ['check-ref-format', 'refs/heads/{}'.format(value)])
    if chk.returncode != 0:
        info('-> hw-mgmt: branch input {!r} fails git check-ref-format'.format(value), verbose)
        return None, None, 'invalid git branch refname: {!r}'.format(value)

    remote_ref = 'refs/remotes/{}/{}'.format(REMOTE, value)
    local_ref = 'refs/heads/{}'.format(value)

    if have_origin(repo):
        # '+' forces non-fast-forward updates; without it a force-pushed
        # upstream is rejected and we'd cling to the stale local ref.
        refspec = '+{}:{}'.format(local_ref, remote_ref)
        info('-> hw-mgmt: fetching branch {} from origin (forced refresh)'.format(value), verbose)
        res = run_git(repo, ['fetch', REMOTE, refspec])
        info('-> hw-mgmt: fetch branch rc={} stderr={!r}'.format(
            res.returncode, (res.stderr or '').strip()), verbose)
        if res.returncode == 0:
            sha = rev_parse(repo, remote_ref)
            if sha:
                return remote_ref, sha, ''
            fail('fetch returned 0 but {} did not resolve'.format(remote_ref))
        fetch_stderr = (res.stderr or '').strip()
        if is_shallow(repo):
            print(
                '-> NOTICE: hw-mgmt submodule appears to be a shallow clone. '
                'Run `git -C {} fetch --unshallow` and retry.'.format(repo),
                file=sys.stderr,
            )
        return None, None, fetch_stderr

    # Offline mode: no origin to refresh from, use whatever is local.
    info('-> hw-mgmt: no origin remote; resolving branch from local refs only', verbose)
    sha = rev_parse(repo, remote_ref)
    if sha:
        return remote_ref, sha, ''
    sha = rev_parse(repo, local_ref)
    if sha:
        return local_ref, sha, ''
    return None, None, 'no local branch ref and no origin to fetch from'


def write_env_atomically(env_file, values):
    parent = os.path.dirname(env_file)
    tmp = env_file + '.tmp'
    try:
        if parent:
            os.makedirs(parent, exist_ok=True)
        with open(tmp, 'w') as f:
            for key in ENV_KEYS:
                f.write('{}={}\n'.format(key, shlex.quote(values[key])))
        os.replace(tmp, env_file)
    except OSError as e:
        # Best-effort cleanup of the partial tmp; ignore errors so the
        # original failure surfaces.
        try:
            os.unlink(tmp)
        except OSError:
            pass
        fail('failed to write env file {!r}: {}'.format(env_file, e))


def validate_input(value):
    if not value:
        fail('--input is empty')
    if value.startswith('-'):
        fail('--input must not start with "-": {!r}'.format(value))
    if not INPUT_RE.match(value):
        fail(
            '--input {!r} contains invalid characters; allowed: '
            '[A-Za-z0-9._/+~:-]'.format(value)
        )


def validate_repo(repo):
    """Verify --repo is a git working tree. Origin is checked lazily by the
    resolvers only when needed, so offline tag flows keep working."""
    if not os.path.isdir(repo):
        fail('--repo {!r} does not exist'.format(repo))
    git_marker = os.path.join(repo, '.git')
    if not (os.path.isdir(git_marker) or os.path.isfile(git_marker)):
        fail('--repo {!r} is not a git working tree'.format(repo))


def resolve(repo, value, verbose):
    """Run resolution. Returns dict of env values. Calls fail() on error."""
    tag_ref, tag_commit, tag_err = resolve_tag(repo, value, verbose)
    if tag_ref:
        ref_type = 'tag'
        ref_name = tag_ref
        commit = tag_commit
        base_version = value          # Preserve byte-identical tag flow.
        distinct_id = ''
        package_version = base_version
    else:
        br_ref, br_commit, br_err = resolve_branch(repo, value, verbose)
        if not br_ref:
            details = []
            if tag_err:
                details.append('tag fetch error: {}'.format(tag_err))
            if br_err:
                details.append('branch fetch error: {}'.format(br_err))
            suffix = ' ({})'.format('; '.join(details)) if details else ''
            fail(
                'hw-mgmt: neither tag {!r} nor branch {!r} could be resolved '
                'in {}{}'.format('V.' + value, value, repo, suffix)
            )
        ref_type = 'branch'
        ref_name = br_ref
        commit = br_commit
        base_version = parse_changelog_version(repo, commit)
        if not base_version:
            fail(
                'hw-mgmt: branch {!r} resolved to {} but debian/changelog '
                'version could not be parsed (expected first line '
                '"hw-management (1.mlnx.<version>) ...")'.format(value, commit)
            )
        if not PARSED_VERSION_RE.match(base_version):
            fail(
                'hw-mgmt: parsed base version {!r} contains characters '
                'that are not safe for deb filenames or git refnames; '
                'allowed: [A-Za-z0-9.+-]'.format(base_version)
            )
        distinct_id = commit[:DISTINCT_ID_LEN]
        package_version = '{}-{}'.format(base_version, distinct_id)

    return {
        'HWMGMT_INPUT': value,
        'HWMGMT_REF_TYPE': ref_type,
        'HWMGMT_CHECKOUT_REF': ref_name,
        'HWMGMT_COMMIT': commit,
        'HWMGMT_BASE_VERSION': base_version,
        'HWMGMT_DISTINCT_ID': distinct_id,
        'HWMGMT_PACKAGE_VERSION': package_version,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', required=True, help='Path to the hw-mgmt submodule working tree')
    parser.add_argument('--input', required=True, help='User-provided MLNX_HW_MANAGEMENT_VERSION value')
    parser.add_argument('--env-file', dest='env_file', required=True, help='Output env file path')
    parser.add_argument('--verbose', action='store_true', help='Print full resolution trace')
    args = parser.parse_args()

    validate_input(args.input)
    validate_repo(args.repo)

    env = resolve(args.repo, args.input, args.verbose)
    write_env_atomically(args.env_file, env)

    short = env['HWMGMT_COMMIT'][:DISTINCT_ID_LEN]
    if env['HWMGMT_REF_TYPE'] == 'tag':
        print('-> hw-mgmt: resolved tag {} -> {} (package version {})'.format(
            env['HWMGMT_CHECKOUT_REF'], short, env['HWMGMT_PACKAGE_VERSION']))
    else:
        print('-> hw-mgmt: resolved branch {} -> {} (base {} from changelog, '
              'package version {})'.format(
                  env['HWMGMT_CHECKOUT_REF'], short,
                  env['HWMGMT_BASE_VERSION'], env['HWMGMT_PACKAGE_VERSION']))

    return 0


if __name__ == '__main__':
    sys.exit(main())
