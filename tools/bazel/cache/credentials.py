#!/usr/bin/env python3
"""Supply cache credentials to Bazel without putting secrets in its flags."""

# Reuse Common's reviewed origin-scoped helper (sonic-swss-common commit
# 893c737ad0266ea5ea3f8f22c66ee16f9601bf71). Keep the credential protocol and
# sanitized diagnostics aligned; Make applies its own read-only upload policy.

import argparse
import base64
import binascii
import json
import os
from pathlib import Path
import sys
import time
import urllib.error
import urllib.parse
import urllib.request


HOST = "lgh-bazel-87263989.eastus.cloudapp.azure.com"
ENDPOINT = "https://" + HOST
EMPTY_CAS = ENDPOINT + "/cas/e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
ERROR = "Bazel remote cache access failed."


def authorization():
    """Read the complete header; never include its contents in diagnostics."""
    value = os.environ.get("BAZEL_REMOTE_AUTH", "")
    if not value.startswith("Basic ") or any(c in value for c in "\r\n"):
        raise ValueError(ERROR)
    try:
        decoded = base64.b64decode(value[6:], validate=True)
    except (ValueError, binascii.Error):
        raise ValueError(ERROR) from None
    if b":" not in decoded:
        raise ValueError(ERROR)
    return value


def credentials(request):
    """Implement the credential-helper get request for this HTTPS origin only."""
    if not isinstance(request, dict) or not isinstance(request.get("uri"), str):
        raise ValueError(ERROR)
    uri = urllib.parse.urlsplit(request["uri"])
    if (uri.scheme != "https" or uri.hostname != HOST or uri.port not in (None, 443)
            or uri.username is not None or uri.password is not None):
        raise ValueError(ERROR)
    return {"headers": {"Authorization": [authorization()]}}


class NoRedirects(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        raise urllib.error.HTTPError(req.full_url, code, ERROR, None, None)


def write_receipt(output, receipt):
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")


def transient_failure(error):
    if isinstance(error, urllib.error.HTTPError):
        return error.code in (429, 502, 503, 504)
    if isinstance(error, urllib.error.URLError):
        return isinstance(error.reason, TimeoutError)
    return isinstance(error, TimeoutError)


def probe(output):
    """Check authenticated cache access without uploading data or following redirects."""
    output.unlink(missing_ok=True)
    try:
        request = urllib.request.Request(EMPTY_CAS, method="HEAD",
                                         headers={"Authorization": authorization()})
        opener = urllib.request.build_opener(NoRedirects())
        for attempt in range(3):
            try:
                with opener.open(request, timeout=10) as response:
                    if response.status != 200:
                        raise ValueError(ERROR)
                break
            except (OSError, urllib.error.URLError) as error:
                if attempt == 2 or not transient_failure(error):
                    raise
                if isinstance(error, urllib.error.HTTPError):
                    error.close()
                time.sleep(2 * (attempt + 1))
    except (ValueError, OSError, urllib.error.URLError) as error:
        receipt = {"endpoint": ENDPOINT, "authenticated": False,
                   "error_type": type(error).__name__}
        if isinstance(error, urllib.error.HTTPError):
            receipt["status"] = error.code
        write_receipt(output, receipt)
        raise
    write_receipt(output, {"endpoint": ENDPOINT, "authenticated": True, "status": 200})


class Parser(argparse.ArgumentParser):
    def error(self, message):
        self.exit(2, ERROR + "\n")


def main(argv=None):
    parser = Parser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("get")
    check = commands.add_parser("probe")
    check.add_argument("--output", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        if args.command == "get":
            json.dump(credentials(json.load(sys.stdin)), sys.stdout)
            sys.stdout.write("\n")
        else:
            probe(args.output)
    except (ValueError, OSError, urllib.error.URLError):
        print(ERROR, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
