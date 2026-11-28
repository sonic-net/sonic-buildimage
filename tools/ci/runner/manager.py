#!/usr/bin/env python3
"""Keep one dedicated ephemeral runner available; run once from a systemd timer."""
import argparse
from contextlib import contextmanager
import fcntl
import json
import os
from pathlib import Path
import pwd
import re
import stat
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
import rearm

CONFIG_PATH = Path('/etc/sonic-vs-runner-manager.json')
MANAGER_LOCK = Path('/run/lock/sonic-vs-runner-manager.lock')
REARM_LOCK = Path('/run/lock/sonic-vs-runner.lock')
STATE_DIRECTORY = Path('/var/lib/sonic-vs-runner-manager')
PENDING = STATE_DIRECTORY / 'pending.json'
REARM_PROGRAM = '/opt/sonic-runner-tools/runner/rearm.py'
PREFLIGHT_PROGRAM = '/opt/sonic-runner-tools/runner/preflight.py'
ROOT_ENV = {'HOME': '/root', 'USER': 'root', 'LOGNAME': 'root',
            'PATH': '/usr/local/bin:/usr/bin:/bin', 'LANG': 'C.UTF-8'}


class LockBusy(RuntimeError):
    pass


class GitHubNotFound(rearm.GitHubApiError):
    pass


def validate_config(config):
    required = {'operator', 'github_account', 'repo', 'pr'}
    if not isinstance(config, dict) or not required <= config.keys() or config.keys() - required - {'_generated'}:
        raise RuntimeError('Manager config requires operator, github_account, repo and pr; unknown keys are rejected')
    if not isinstance(config['operator'], str) or not re.fullmatch(r'[a-z_][a-z0-9_-]*', config['operator']):
        raise RuntimeError('operator must name a normal local account')
    if not isinstance(config['github_account'], str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9-]*', config['github_account']):
        raise RuntimeError('github_account must name the expected GitHub login')
    if (not isinstance(config['repo'], str) or '..' in config['repo']
            or not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', config['repo'])):
        raise RuntimeError('repo must be owner/name')
    if config['pr'] is not None and (type(config['pr']) is not int or config['pr'] < 1):
        raise RuntimeError('pr must be null for master, or a positive PR number')
    if config['operator'] in ('root', 'sonic-runner'):
        raise RuntimeError('operator must be a normal account distinct from root and sonic-runner')
    return config


def load_config(path=CONFIG_PATH):
    # Never consume a runner-controlled config as root.
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
    with os.fdopen(fd) as stream:
        info = os.fstat(stream.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o022:
            raise RuntimeError('Manager config must be a root-owned regular file, not writable by group or others')
        return validate_config(json.load(stream))


@contextmanager
def locked(path):
    fd = os.open(path, os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
    with os.fdopen(fd, 'w') as stream:
        info = os.fstat(stream.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o022 or info.st_nlink != 1:
            raise RuntimeError(f'Unsafe runner lock: {path}')
        try:
            fcntl.flock(stream, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise LockBusy(f'Another runner operation holds {path}') from error
        yield


class OperatorGitHub:
    """Use the operator's existing gh login without exporting it to the runner."""
    def __init__(self, config):
        validate_config(config)
        self.account = pwd.getpwnam(config['operator'])
        if self.account.pw_uid == 0 or self.account.pw_name == 'sonic-runner':
            raise RuntimeError('GitHub operator must not be root or the runner account')
        self.expected_login = config['github_account']
        self.environment = {'HOME': self.account.pw_dir, 'USER': self.account.pw_name,
                            'LOGNAME': self.account.pw_name, 'PATH': ROOT_ENV['PATH'],
                            'LANG': 'C.UTF-8', 'GH_HOST': 'github.com', 'GH_PROMPT_DISABLED': '1'}

    def request(self, endpoint, deadline=None, method='GET'):
        remaining = 15 if deadline is None else min(15, deadline - time.monotonic())
        if remaining <= 0:
            raise rearm.GitHubApiError('GitHub request deadline reached')
        try:
            result = subprocess.run(['gh', 'api', '--method', method, endpoint],
                                    capture_output=True, text=True, timeout=remaining,
                                    cwd=self.account.pw_dir, env=self.environment,
                                    user=self.account.pw_uid, group=self.account.pw_gid,
                                    extra_groups=os.getgrouplist(self.account.pw_name, self.account.pw_gid))
        except subprocess.TimeoutExpired as error:
            raise rearm.GitHubApiError('Operator GitHub API request timed out') from error
        if result.returncode:
            # Do not echo response bodies: registration responses contain tokens.
            status = re.search(r'\(HTTP (\d{3})\)', result.stderr or '')
            code = status.group(1) if status else 'unavailable'
            error_type = GitHubNotFound if code == '404' else rearm.GitHubApiError
            raise error_type(f'Operator GitHub API {method} {endpoint} failed (HTTP {code}); check operator gh authentication and repository access')
        try:
            data = json.loads(result.stdout)
        except ValueError as error:
            raise rearm.GitHubApiError('Operator GitHub API returned invalid JSON') from error
        if not isinstance(data, dict):
            raise rearm.GitHubApiError('Operator GitHub API returned an unexpected response')
        return data

    def verify_identity(self):
        if self.request('user').get('login') != self.expected_login:
            raise RuntimeError(f'Operator gh login does not match expected account {self.expected_login}; select that account before enabling the manager')


def root_command(command):
    result = subprocess.run(command, text=True, capture_output=True, timeout=30, env=ROOT_ENV)
    if result.returncode:
        raise RuntimeError(f'{command[0]} could not inspect or update the dedicated runner; check its service journal')
    return result.stdout.strip()


def host_idle():
    text = root_command(['systemctl', 'show', rearm.SERVICE,
                         '--property=LoadState,ActiveState,SubState'])
    state = dict(line.split('=', 1) for line in text.splitlines() if '=' in line)
    if state.get('LoadState') != 'loaded':
        raise RuntimeError('Runner service is not loaded; run bootstrap.sh before enabling the manager')
    if state.get('ActiveState') in ('active', 'activating', 'deactivating', 'reloading', 'refreshing'):
        print('Deferred: runner service is active or transitioning', flush=True)
        return False
    if (state.get('ActiveState'), state.get('SubState')) not in (('inactive', 'dead'), ('failed', 'failed')):
        raise RuntimeError(f'Unknown runner service state: {state}; inspect before rearming')
    running = subprocess.run(['pgrep', '-f', r'Runner\.(Listener|Worker)'],
                             capture_output=True, timeout=15, env=ROOT_ENV)
    if running.returncode == 0:
        print('Deferred: a runner Listener or Worker still exists', flush=True)
        return False
    if running.returncode != 1:
        raise RuntimeError('Cannot inspect runner processes; no runner was started')
    if root_command(['docker', 'ps', '--quiet']):
        print('Deferred: Docker containers are still running; no containers were changed', flush=True)
        return False
    return True


def current_registration(config):
    current = rearm.RUNNER_HOME / 'current'
    if not current.exists() and not current.is_symlink():
        return None
    if not current.is_symlink():
        raise RuntimeError('Runner current must be an attempt symlink; inspect before rearming')
    attempts = rearm.RUNNER_HOME / 'attempts'
    if attempts.is_symlink():
        raise RuntimeError('Runner attempts directory must not be a symlink')
    try:
        attempt = current.resolve(strict=True)
    except OSError as error:
        raise RuntimeError('Runner current points to a missing attempt; inspect before rearming') from error
    if attempt.parent != attempts.resolve(strict=True) or not attempt.is_dir():
        raise RuntimeError('Runner current points outside the attempts directory')
    stamp = attempt.name
    if not re.fullmatch(r'\d{8}T\d{6}Z-[0-9a-f]{8}', stamp):
        raise RuntimeError('Runner current has an unexpected attempt name')
    path = attempt / '.runner'
    if not path.exists() and not path.is_symlink():
        return None
    if path.is_symlink() or not path.is_file():
        raise RuntimeError('Local .runner must be a regular file, not a symlink')
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    name = f"sonic-vs-{config['pr'] or 'master'}-{stamp}"
    if (not isinstance(data, dict) or data.get('ephemeral') is not True
            or type(data.get('agentId')) is not int or data['agentId'] <= 0
            or data.get('agentName') != name or data.get('workFolder') != '_work'
            or not isinstance(data.get('gitHubUrl'), str)
            or data['gitHubUrl'].rstrip('/') != f"https://github.com/{config['repo']}"):
        raise RuntimeError('Local runner identity, ephemeral mode or repository differs from manager config; inspect the attempt before rearming')
    return {'id': data['agentId'], 'name': name, 'attempt': str(attempt)}


def remote_runners(config, api):
    """A successful listing distinguishes an absent runner from lost API access."""
    runners = []
    expected_total = None
    for page in range(1, 101):
        result = api.request(f"repos/{config['repo']}/actions/runners?per_page=100&page={page}")
        batch, total = result.get('runners'), result.get('total_count')
        if not isinstance(batch, list) or type(total) is not int or total < 0:
            raise RuntimeError('Cannot verify repository runner inventory')
        if expected_total is not None and total != expected_total:
            raise RuntimeError('Repository runner inventory changed; retry on the next timer tick')
        expected_total = total
        runners.extend(batch)
        if len(runners) == total:
            if any(not isinstance(r, dict) or type(r.get('id')) is not int or not isinstance(r.get('name'), str) for r in runners):
                raise RuntimeError('Repository runner inventory has invalid identities')
            if len({r['id'] for r in runners}) != len(runners):
                raise RuntimeError('Repository runner inventory contains duplicate identities')
            return runners
        if not batch or len(runners) > total:
            raise RuntimeError('Repository runner inventory changed; retry on the next timer tick')
    raise RuntimeError('Repository runner inventory exceeds the inspection limit')


def check_unfinished_attempts():
    current = (rearm.RUNNER_HOME / 'current').resolve()
    attempts = rearm.RUNNER_HOME / 'attempts'
    if attempts.is_symlink():
        raise RuntimeError('Runner attempts directory must not be a symlink')
    if not attempts.exists():
        return
    for attempt in attempts.iterdir():
        if attempt == current:
            continue
        if (attempt / '.runner').exists() or (attempt / '.runner').is_symlink():
            raise RuntimeError(f'Unfinished registration outside current: {attempt}; inspect it before rearming')


def read_pending(config):
    if not PENDING.exists() and not PENDING.is_symlink():
        return None
    info = STATE_DIRECTORY.lstat()
    if not stat.S_ISDIR(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o077:
        raise RuntimeError('Manager state directory must be root-owned with mode 0700')
    fd = os.open(PENDING, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
    with os.fdopen(fd) as stream:
        info = os.fstat(stream.fileno())
        if not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o077 or info.st_nlink != 1:
            raise RuntimeError('Pending registration must be a root-owned regular file with mode 0600')
        pending = json.load(stream)
    if (not isinstance(pending, dict)
            or not {'operator', 'github_account', 'repo', 'pr'} <= pending.keys()
            or any(pending.get(key) != config[key] or type(pending.get(key)) is not type(config[key])
                   for key in ('operator', 'github_account', 'repo', 'pr'))):
        raise RuntimeError('Previous pending registration config differs from manager config; inspect before rearming')
    return pending


def check_pending():
    if PENDING.exists() or PENDING.is_symlink():
        raise RuntimeError(f'Previous registration outcome is uncertain; inspect the service journal and attempt directories, then explicitly remove {PENDING} after recovery')


def mark_pending(config):
    STATE_DIRECTORY.mkdir(mode=0o700, exist_ok=True)
    info = STATE_DIRECTORY.lstat()
    if not stat.S_ISDIR(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o077:
        raise RuntimeError('Manager state directory must be root-owned with mode 0700')
    fd = os.open(PENDING, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
    with os.fdopen(fd, 'w') as stream:
        json.dump({**{key: config[key] for key in ('operator', 'github_account', 'repo', 'pr')},
                   'started_at': time.time(),
                   '_generated': {'notice': 'AUTO-GENERATED. DO NOT EDIT MANUALLY.',
                                  'generator': 'tools/ci/runner/manager.py'}}, stream)
        stream.write('\n')
        stream.flush()
        os.fsync(stream.fileno())


def preflight(attempt=None):
    command = ['/usr/bin/python3', PREFLIGHT_PROGRAM]
    if attempt:
        command += ['--workspace', attempt]
    rearm.as_runner(command)


def registration_token(config, api):
    response = api.request(f"repos/{config['repo']}/actions/runners/registration-token", method='POST')
    token = response.get('token')
    if not isinstance(token, str) or not token or len(token) > 4096 or any(c.isspace() for c in token):
        raise RuntimeError('GitHub did not return a valid short-lived registration token')
    return token


def reconcile(config, api):
    token = None
    with locked(REARM_LOCK):
        if not host_idle():
            return 'deferred'
        api.verify_identity()
        registration = current_registration(config)
        check_unfinished_attempts()
        pending = read_pending(config)
        if registration:
            try:
                remote = api.request(f"repos/{config['repo']}/actions/runners/{registration['id']}")
            except GitHubNotFound:
                remote = None
            if remote is not None:
                labels = remote.get('labels')
                if (type(remote.get('id')) is not int or remote.get('id') != registration['id'] or remote.get('name') != registration['name']
                        or type(remote.get('busy')) is not bool or remote.get('status') not in ('online', 'offline')
                        or not isinstance(labels, list)
                        or rearm.routing_label(config['pr']) not in [x.get('name') for x in labels if isinstance(x, dict)]):
                    raise RuntimeError('GitHub runner identity, status or routing label differs; no service was started')
                if pending is not None:
                    # A verified local/remote pair proves the earlier child did
                    # register successfully, even if it crashed before reporting.
                    if read_pending(config) != pending:
                        raise RuntimeError('Pending registration changed during recovery; inspect before retrying')
                    PENDING.unlink()
                if remote['busy']:
                    print('Deferred: GitHub reports this runner busy', flush=True)
                    return 'deferred'
                preflight(registration['attempt'])
                if not host_idle():
                    return 'deferred'
                root_command(['systemctl', 'reset-failed', rearm.SERVICE])
                root_command(['systemctl', 'start', rearm.SERVICE])
                rearm.wait_until_ready(config['repo'], registration, api=api.request)
                return 'started'
        # Verify a 404 really means no registration, and avoid duplicate listeners
        # after a crash between remote registration and publishing current.
        check_pending()
        known = remote_runners(config, api)
        prefix = f"sonic-vs-{config['pr'] or 'master'}-"
        if any(r['name'].startswith(prefix) or (registration and r['id'] == registration['id']) for r in known):
            raise RuntimeError('An existing repository runner conflicts with this attempt; inspect it before rearming')
        preflight(registration['attempt'] if registration else None)
        token = registration_token(config, api)
        if not host_idle():
            return 'deferred'
        if registration:
            # --local is supported by the pinned runner and only clears its local
            # registration files. Never remove the remote registration or attempt.
            attempt = Path(registration['attempt'])
            rearm.as_runner([str(attempt / 'config.sh'), 'remove', '--local'], cwd=attempt)
            if (attempt / '.runner').exists() or (attempt / '.runner').is_symlink():
                raise RuntimeError('Local runner removal did not clear .runner; inspect the preserved attempt')
        mark_pending(config)
    # The registration helper takes the same shared lock and repeats its idle
    # checks, so a manual rearm winning this race cannot produce two listeners.
    command = ['/usr/bin/python3', REARM_PROGRAM, '--register', '--repo', config['repo']]
    command += ['--pr', str(config['pr'])] if config['pr'] else ['--master']
    registration = rearm.register_from_operator(command, token, env=ROOT_ENV)
    del token
    if current_registration(config) != registration:
        raise RuntimeError('Registration result does not match current; inspect the pending registration before retrying')
    PENDING.unlink()
    rearm.wait_until_ready(config['repo'], registration, api=api.request)
    return 'registered'


def run_once(config):
    if os.geteuid() != 0:
        raise RuntimeError('Run the manager as root through its systemd service')
    validate_config(config)
    try:
        with locked(MANAGER_LOCK):
            return reconcile(config, OperatorGitHub(config))
    except LockBusy as error:
        print(f'Deferred: {error}', flush=True)
        return 'deferred'


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, default=CONFIG_PATH)
    args = parser.parse_args(argv)
    result = run_once(load_config(args.config))
    print(f'Runner manager: {result}', flush=True)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        raise SystemExit(f'Runner manager failed: {error}')
