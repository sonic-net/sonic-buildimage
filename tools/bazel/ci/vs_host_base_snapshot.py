"""Select a VS host-base artifact from the same PR or from master."""

import http.client
import json
import os
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import urlencode, urlsplit


PREFIX = "sonic-vs-host-base-v1-"
MASTER = PREFIX + "master"


def scopes(env, event):
    """Return eligible artifact names in lookup order and an optional write name."""
    kind, ref = env["GITHUB_EVENT_NAME"], env["GITHUB_REF"]
    if kind == "pull_request":
        pr = event["pull_request"]
        number = pr["number"]
        repo = env["GITHUB_REPOSITORY"]
        if (type(number) is int and number > 0 and ref == f"refs/pull/{number}/merge"
                and (pr["head"]["repo"] or {}).get("full_name") == pr["base"]["repo"]["full_name"] == repo):
            name = PREFIX + f"pr-{number}"
            return [name, MASTER], name
    elif kind == "push" and ref == "refs/heads/master":
        return [MASTER], MASTER
    elif kind == "workflow_dispatch":
        return [MASTER], None
    return [], None


def eligible_run(run, name, workflow_id, repo, pr):
    if (run["workflow_id"] != workflow_id or run["repository"]["full_name"] != repo
            or (run["head_repository"] or {}).get("full_name") != repo):
        return False
    if name == MASTER:
        return run["event"] == "push" and run["head_branch"] == "master"
    repo_id = run["repository"]["id"]
    return (run["event"] == "pull_request" and run["head_branch"] == pr["head"]["ref"]
            and any(p["number"] == pr["number"] and (p["head"]["repo"] or {}).get("id") == repo_id
                    and (p["base"]["repo"] or {}).get("id") == repo_id for p in run["pull_requests"]))


def candidates(api, name, run_id, attempt):
    now = datetime.now(timezone.utc)
    artifacts = []
    page = 1
    while True:
        query = urlencode({"name": name, "per_page": 100, "page": page})
        result = api("artifacts?" + query)
        for artifact in result["artifacts"]:
            if (artifact["name"] == name and not artifact["expired"] and artifact.get("workflow_run")
                    and datetime.fromisoformat(artifact["expires_at"].replace("Z", "+00:00")) > now
                    and (artifact["workflow_run"]["id"] != run_id or attempt > 1)):
                artifacts.append(artifact)
        if len(result["artifacts"]) < 100 or page * 100 >= result["total_count"]:
            break
        page += 1
    return sorted(artifacts, key=lambda a: (a["created_at"], a["id"]), reverse=True)


def select(api, names, env, event):
    run_id = int(env["GITHUB_RUN_ID"])
    attempt = int(env["GITHUB_RUN_ATTEMPT"])
    current_run = None
    for name in names:
        for artifact in candidates(api, name, run_id, attempt):
            producer_id = artifact["workflow_run"]["id"]
            if current_run is None:
                current_run = api(f"runs/{run_id}")
            try:
                run = current_run if producer_id == run_id else api(f"runs/{producer_id}")
            except FileNotFoundError:
                # A workflow run and its artifacts can expire during lookup.
                continue
            if eligible_run(run, name, current_run["workflow_id"], env["GITHUB_REPOSITORY"], event.get("pull_request")):
                return {"read_artifact_id": artifact["id"], "read_run_id": producer_id}
    return {}


def github_api(endpoint):
    base = urlsplit(os.environ["GITHUB_API_URL"])
    if base.scheme != "https" or not base.hostname or base.username or base.password or base.query or base.fragment:
        raise ValueError("expected an HTTPS GitHub API URL")
    connection = http.client.HTTPSConnection(base.netloc, timeout=20)
    try:
        path = f"{base.path.rstrip('/')}/repos/{os.environ['GITHUB_REPOSITORY']}/actions/{endpoint}"
        connection.request("GET", path, headers={
            "Authorization": "Bearer " + os.environ["GITHUB_TOKEN"],
            "Accept": "application/vnd.github+json", "User-Agent": "sonic-vs-host-base",
        })
        response = connection.getresponse()
        # http.client never follows redirects. The token stays on the configured API host.
        if response.status == 404:
            raise FileNotFoundError("GitHub Actions resource no longer available")
        if response.status != 200:
            raise RuntimeError(f"GitHub Actions API returned HTTP {response.status}")
        return json.load(response)
    finally:
        connection.close()


def main():
    event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text())
    names, write_name = scopes(os.environ, event)
    with Path(os.environ["GITHUB_OUTPUT"]).open("a") as output:
        if write_name:
            print("write_name=" + write_name, file=output)
        if names:
            for key, value in select(github_api, names, os.environ, event).items():
                print(f"{key}={value}", file=output)


if __name__ == "__main__":
    main()
