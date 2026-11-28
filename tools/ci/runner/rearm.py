#!/usr/bin/env python3
"""Register and start one ephemeral runner from the operator's gh login."""
import argparse
from datetime import datetime, timezone
import fcntl
import hashlib
import json
import os
from pathlib import Path
import pwd
import re
import subprocess
import sys
import time
import uuid

RUNNER_HOME = Path("/data/sonic-runner")
ARCHIVE = Path("/opt/sonic-runner-tools/releases/actions-runner-linux-x64-2.337.0.tar.gz")
SHA256 = "70920811a4f8ad4328818682bca5c6469c1c942fab52448868071d0063816613"
SERVICE = "sonic-vs-runner.service"
REGISTRATION_PREFIX = "SONIC_RUNNER_REGISTRATION="
READY_TIMEOUT = 90


class GitHubApiError(RuntimeError):
    pass


def github_json(endpoint, deadline):
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise GitHubApiError("Readiness deadline reached")
    try:
        result = subprocess.run(["gh", "api", endpoint], text=True, capture_output=True,
                                timeout=min(10, remaining))
    except subprocess.TimeoutExpired as error:
        raise GitHubApiError("GitHub API request timed out") from error
    if result.returncode:
        raise GitHubApiError((result.stderr or result.stdout).strip())
    try:
        return json.loads(result.stdout)
    except ValueError as error:
        raise GitHubApiError("GitHub returned an invalid JSON response") from error


def wait_until_ready(repo, registration, timeout=READY_TIMEOUT, api=None):
    api = github_json if api is None else api
    deadline = time.monotonic() + timeout
    last_status = "No runner status received"
    while time.monotonic() < deadline:
        try:
            runner = api(f"repos/{repo}/actions/runners/{registration['id']}", deadline)
            if runner.get("id") != registration["id"] or runner.get("name") != registration["name"]:
                last_status = "GitHub returned a different runner identity"
                break
            if runner.get("status") == "online" or runner.get("busy") is True:
                state = "busy" if runner.get("busy") else "online"
                print(f"Verified {registration['name']}: GitHub reports {state}")
                return
            last_status = f"GitHub reports {runner.get('status', 'unknown')}"
        except GitHubApiError as error:
            last_status = str(error)
        remaining = deadline - time.monotonic()
        if remaining > 0:
            time.sleep(min(3, remaining))
    raise RuntimeError(
        f"Could not verify runner readiness within {timeout}s: {last_status}. "
        f"Attempt: {registration['attempt']}. "
        f"Inspect sudo journalctl -u {SERVICE} --since today and this attempt's _diag. "
        "The service was not stopped; inspect its state before retrying. "
        "Also check whether this ephemeral runner already consumed a job and unregistered.")


def register_from_operator(command, token, env=None):
    options = {} if env is None else {"env": env}
    result = subprocess.run(command, input=token + "\n", text=True, stdout=subprocess.PIPE, **options)
    registration = None
    for line in result.stdout.splitlines():
        if line.startswith(REGISTRATION_PREFIX):
            registration = json.loads(line[len(REGISTRATION_PREFIX):])
        else:
            print(line, flush=True)
    if result.returncode:
        raise subprocess.CalledProcessError(result.returncode, command)
    if (not isinstance(registration, dict) or not isinstance(registration.get("id"), int)
            or registration["id"] <= 0 or not registration.get("name") or not registration.get("attempt")):
        raise RuntimeError("Registration did not return a runner identity; inspect the service journal before retrying")
    return registration


def routing_label(pr):
    return f"sonic-vs-source-pr-{pr}" if pr else "sonic-vs-source-master"


def runner_environment(token=None):
    # Do not inherit gh, SSH-agent, operator credential, or proxy environments.
    environment = {"HOME": str(RUNNER_HOME), "USER": "sonic-runner", "LOGNAME": "sonic-runner",
                   "PATH": "/usr/local/bin:/usr/bin:/bin", "LANG": "C.UTF-8"}
    if token is not None:
        environment["ACTIONS_RUNNER_INPUT_TOKEN"] = token
    return environment


def archive_digest(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def sudo_command():
    probe = subprocess.run(["sudo", "-n", "--", "/usr/bin/true"], capture_output=True)
    if probe.returncode == 0:
        return ["sudo", "-n", "--"]
    if not sys.stdin.isatty():
        raise RuntimeError("Sudo needs authentication. Run this command in an interactive terminal or configure passwordless sudo for this operator")
    subprocess.run(["sudo", "-v"], check=True)
    return ["sudo", "-n", "--"]


def as_runner(command, *, token=None, cwd=None):
    account = pwd.getpwnam("sonic-runner")
    subprocess.run(command, check=True, cwd=cwd, env=runner_environment(token),
                   user=account.pw_uid, group=account.pw_gid,
                   extra_groups=os.getgrouplist(account.pw_name, account.pw_gid))


def idle_host():
    state = subprocess.run(["systemctl", "is-active", SERVICE], capture_output=True, text=True)
    if state.stdout.strip() in ("active", "activating", "deactivating", "reloading"):
        raise RuntimeError(f"{SERVICE} is active; wait for the job to finish")
    running = subprocess.run(["pgrep", "-u", "sonic-runner", "-f", r"Runner\.(Listener|Worker)"],
                             capture_output=True)
    if running.returncode == 0:
        raise RuntimeError("Another runner for this account is active; wait for it to finish")
    if running.returncode != 1:
        raise RuntimeError("Cannot inspect existing runner processes")


def register(args):
    if os.geteuid() != 0:
        raise RuntimeError("Internal registration requires sudo")
    if not os.path.ismount("/data"):
        raise RuntimeError("/data must be mounted")
    # Serialize operator attempts without trusting a runner-writable lock path.
    with open("/run/lock/sonic-vs-runner.lock", "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        idle_host()
        if subprocess.check_output(["systemctl", "show", SERVICE, "--property=LoadState", "--value"], text=True).strip() != "loaded":
            raise RuntimeError("Run bootstrap.sh first")
        current = RUNNER_HOME / "current"
        if current.exists() and (current / ".runner").exists():
            raise RuntimeError("Previous registration remains. Inspect/remove it using the README recovery steps first")
        as_runner(["/usr/bin/python3", "/opt/sonic-runner-tools/runner/preflight.py"])
        if archive_digest(ARCHIVE) != SHA256:
            raise RuntimeError("Cached runner archive failed SHA-256 verification; rerun bootstrap.sh")
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ") + "-" + uuid.uuid4().hex[:8]
        attempt = RUNNER_HOME / "attempts" / stamp
        account = pwd.getpwnam("sonic-runner")
        attempt.mkdir(mode=0o700)
        os.chown(attempt, account.pw_uid, account.pw_gid)
        print(f"Attempt directory: {attempt}", flush=True)
        as_runner(["tar", "xzf", str(ARCHIVE), "--no-same-owner", "-C", str(attempt)])
        # Extraction consumes workspace capacity. Recheck the actual attempt
        # before registering a runner that could immediately accept a job.
        as_runner(["/usr/bin/python3", "/opt/sonic-runner-tools/runner/preflight.py",
                   "--workspace", str(attempt)])
        # The caller sends only a short-lived registration token through stdin.
        token = sys.stdin.readline().strip()
        if not token or len(token) > 4096:
            raise RuntimeError("Missing or invalid registration token")
        runner_name = f"sonic-vs-{args.pr or 'master'}-{stamp}"
        as_runner([str(attempt / "config.sh"), "--unattended", "--ephemeral",
                   "--url", f"https://github.com/{args.repo}", "--name", runner_name,
                   "--labels", routing_label(args.pr), "--work", "_work"], token=token, cwd=attempt)
        del token
        config = json.loads((attempt / ".runner").read_text(encoding="utf-8-sig"))
        if config.get("agentName") != runner_name or not isinstance(config.get("agentId"), int):
            raise RuntimeError(f"Unexpected local runner identity in {attempt}; inspect before retrying")
        registration = {"id": config["agentId"], "name": runner_name, "attempt": str(attempt)}
        # Keep every prior attempt intact for logs and interrupted checkout diagnosis.
        link = RUNNER_HOME / f".current-{stamp}"
        link.symlink_to(attempt)
        link.replace(current)
        subprocess.run(["systemctl", "reset-failed", SERVICE], check=True)
        subprocess.run(["systemctl", "start", SERVICE], check=True)
        print(f"Service start requested for {runner_name} with label {routing_label(args.pr)}")
        print(REGISTRATION_PREFIX + json.dumps(registration), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default="securely1g/sonic-buildimage")
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument("--pr", type=int)
    target.add_argument("--master", action="store_true", help="Use the push/manual workflow label")
    parser.add_argument("--dry-run", action="store_true", help="Print plan; no sudo, network or writes")
    parser.add_argument("--register", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repo) or ".." in args.repo:
        parser.error("Repository must be owner/name")
    if args.pr is not None and args.pr < 1:
        parser.error("PR number must be positive")
    if args.master:
        args.pr = None
    if args.dry_run:
        print(f"Repository: {args.repo}; label: {routing_label(args.pr)}")
        print("Check idle service and host prerequisites, verify archive, create a fresh attempt,")
        print("register one ephemeral runner using a short-lived token, start sonic-vs-runner.service.")
        print("Keep prior attempt directories; do not store gh credentials in the runner account.")
        return
    if args.register:
        register(args)
        return
    if os.geteuid() == 0:
        parser.error("Run as your normal GitHub-authenticated operator account, without sudo")
    sudo = sudo_command()
    identity = subprocess.check_output(["gh", "api", "user", "--jq", ".login"], text=True).strip()
    print(f"Using operator GitHub account {identity} for {args.repo}", flush=True)
    token_response = subprocess.check_output(
        ["gh", "api", "--method", "POST", f"repos/{args.repo}/actions/runners/registration-token"], text=True)
    token = json.loads(token_response)["token"]
    command = sudo + ["/usr/bin/python3", str(Path(__file__).resolve()), "--register", "--repo", args.repo]
    command += ["--pr", str(args.pr)] if args.pr else ["--master"]
    registration = register_from_operator(command, token)
    del token, token_response
    wait_until_ready(args.repo, registration)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"Rearm failed: {error}")
