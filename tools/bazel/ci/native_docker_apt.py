"""Pass the native worker's Debian APT route to nested BuildKit stages."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
from urllib.parse import urlsplit


HOST = "deb.debian.org"
ROUTERS = {"/run/sonic-download-proxy/native_apt_proxy.py", "/opt/sonic-native-bin/native-apt-proxy"}
CA_FILES = {"/run/sonic-native-apt/ca-bundle.pem", "/run/sonic-native-apt/apt-ca-bundle.pem"}


def settings():
    command = ["/usr/bin/apt-config", "shell"]
    for scheme in ("http", "https"):
        for label, key in (("proxy", "Proxy"), ("host", "Proxy::" + HOST),
                           ("detect", "Proxy-Auto-Detect"), ("legacy", "ProxyAutoDetect")):
            command += [scheme + "_" + label, f"Acquire::{scheme}::{key}"]
    command += ["ca", f"Acquire::https::{HOST}::CAInfo", "global_ca", "Acquire::https::CAInfo"]
    result = subprocess.check_output(command, text=True)
    return dict(word.split("=", 1) for word in shlex.split(result))


def lookup(router, url):
    # The manager's helper accounts for NO_PROXY and job-level proxy overrides.
    return subprocess.check_output([router, url], text=True, stderr=subprocess.DEVNULL).strip()


def routes(config, query=lookup):
    def managed_router(scheme):
        router = config.get(scheme + "_detect")
        if router in ROUTERS and not any(config.get(scheme + "_" + key) for key in ("host", "proxy", "legacy")):
            return router

    # APT checks a host's HTTP proxy before a global HTTPS proxy or https_proxy.
    # Only export the HTTP rule alongside an explicit, manager-owned HTTPS rule.
    routers = {scheme: managed_router(scheme) for scheme in ("http", "https")}
    baseline = (None, "", "/etc/ssl/certs/ca-certificates.crt", "/usr/local/share/sonic-build-trust/ca-bundle.pem")
    if not all(routers.values()) or config.get("ca") not in CA_FILES or config.get("global_ca") not in baseline:
        return [], None
    lines = []
    for scheme, router in routers.items():
        endpoint = query(router, f"{scheme}://{HOST}/debian/")
        if not endpoint or endpoint == "DIRECT":
            if scheme == "https":
                return [], None
            continue
        # Native Squid uses a credential-free HTTP endpoint. Never place an
        # arbitrary helper response, account data or control character in APT.
        if not re.fullmatch(r"http://(?:[a-zA-Z0-9.\-]+|\[[a-fA-F0-9:.]+\])(?::[0-9]{1,5})?/?", endpoint):
            raise ValueError("Invalid native worker APT proxy endpoint")
        try:
            port = urlsplit(endpoint).port
        except ValueError:
            port = 0
        if port == 0:
            raise ValueError("Invalid native worker APT proxy port")
        lines.append(f'Acquire::{scheme}::Proxy::{HOST} "{endpoint}";')
    lines.append(f'Acquire::https::{HOST}::CaInfo "/run/secrets/sonic_apt_ca";')
    return lines, config["ca"]


def build_options(temporary, workspace, config):
    lines, ca = routes(config)
    if not lines:
        return ""
    # Make expands DOCKER_BUILD_OPTS in a shell command. These secret source
    # paths are runner-owned and must sit outside the build context and uploads.
    if temporary.is_relative_to(workspace) or not re.fullmatch(r"/[A-Za-z0-9_./-]+", str(temporary)):
        raise ValueError("Docker APT secrets require a separate runner temporary directory")
    if not Path(ca).is_file():
        raise ValueError("Native worker APT CA bundle is missing")
    with tempfile.NamedTemporaryFile(mode="w", dir=temporary, prefix="docker-apt-", delete=False) as secret:
        secret.write("\n".join(lines) + "\n")
    return f"--secret id=sonic_apt_proxy,src={secret.name} --secret id=sonic_apt_ca,src={ca}"


if __name__ == "__main__":
    # The job hook removes this policy for disabled jobs and workers with an
    # explicit route. Stay opt-in even when a worker still projects its CA.
    policy = Path("/etc/apt/apt.conf.d/99sonic-native-squid")
    if policy.is_symlink() and os.readlink(policy) == "/run/sonic-native-apt/policy" and policy.is_file():
        print(build_options(Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve(), settings()))
