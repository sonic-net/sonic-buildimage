"""Select a pair of reviewed Git commits for the manual base-image workflow."""

import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def verify_branch_commit(repository, revision, directory):
    """A GitHub fork network can serve objects outside the repository's branches."""
    subprocess.run(
        ["git", "clone", "--quiet", "--bare", "--filter=tree:0", "--no-tags", "--",
         repository, str(directory)], check=True,
    )
    branch = subprocess.check_output(
        ["git", "-C", str(directory), "for-each-ref", "--count=1",
         "--contains=" + revision, "--format=%(refname)", "refs/heads/"],
        text=True,
    ).strip()
    if not branch:
        raise ValueError(f"{revision} is outside the published branches of {repository}")


def validate_pair(source, registry, architecture):
    if architecture != "amd64" or not all(
        re.fullmatch(r"[0-9a-f]{40}", revision) for revision in (source, registry)
    ):
        raise ValueError("Select AMD64 and provide both full lowercase Git commit IDs")


def main():
    source = os.environ["SOURCE_REVISION"]
    registry = os.environ["REGISTRY_REVISION"]
    validate_pair(source, registry, os.environ["SELECTED_ARCHITECTURE"])
    repository = os.environ["GITHUB_REPOSITORY"]
    source_url = f"https://github.com/{repository}.git"
    catalog_url = "https://github.com/securely1g/sonic-bazel-registry.git"

    with tempfile.TemporaryDirectory(dir=os.environ["RUNNER_TEMP"]) as temporary:
        verify_branch_commit(source_url, source, Path(temporary) / "source")
        verify_branch_commit(catalog_url, registry, Path(temporary) / "registry")

    # Keep the dispatcher's registry selection and mirror/cache setup when the
    # requested source predates those helpers. Both build phases use this copy.
    control = Path(tempfile.mkdtemp(prefix="base-review-", dir=os.environ["RUNNER_TEMP"]))
    bazel = control / "bazel"
    shutil.copy2(Path(__file__).with_name("libteam_draft_bazel.sh"), bazel)
    subprocess.run(["git", "fetch", "--quiet", "--no-tags", "--depth=1",
                    "--no-recurse-submodules", source_url, source], check=True)
    subprocess.run(["git", "checkout", "--quiet", "--detach", source], check=True)
    actual = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    if actual != source:
        raise ValueError("Base-image checkout differs from the requested source")

    evidence = Path("artifacts/base-images/review-selection.json")
    evidence.parent.mkdir(parents=True, exist_ok=True)
    evidence.write_text(json.dumps({"controller": os.environ["GITHUB_SHA"],
                                    "source": source, "registry": registry}, indent=2) + "\n")
    with Path(os.environ["GITHUB_ENV"]).open("a") as environment:
        print(f"BASE_REVIEW_BAZEL={bazel}", file=environment)
        print(f"SONIC_BASE_REVIEW_REGISTRY={registry}", file=environment)


if __name__ == "__main__":
    main()
