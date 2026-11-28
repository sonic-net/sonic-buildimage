"""Capture Git install modes and platform names that Bazel file digests omit."""

def _run(repo, argv, wd):
    result = repo.execute(argv, working_directory = str(wd))
    if result.return_code:
        fail(result.stderr)
    return result.stdout

def _executable_files(repo, device):
    root = device.dirname
    # Source distributions already carry their install modes. Git checkouts
    # use the index, including when core.filemode is false on the worker.
    if not root.get_child(".git").exists:
        return _run(repo, ["find", "-L", ".", "-type", "f", "-perm", "/111", "-printf", "%P\\0"], device).split("\000")[:-1]
    git = ["git", "-c", "safe.directory=" + str(root)]
    index = _run(repo, git + ["rev-parse", "--path-format=absolute", "--git-path", "index"], root).strip()
    repo.watch(index)
    entries = _run(repo, git + ["ls-files", "--stage", "-z", "--", "device"], root).split("\000")[:-1]
    executables = {}
    for entry in entries:
        mode, path = entry.split("\t", 1)
        if mode.startswith("100755 "):
            executables[str(root.get_child(path))] = True

    # Make dereferences both file and directory symlinks during install.
    # Project the target's Git mode onto every installed alias.
    paths = _run(repo, ["find", "-L", ".", "-type", "f", "-printf", "%P\\0"], device).split("\000")[:-1]
    return [p for p in paths if str(device.get_child(p).realpath) in executables]

def _metadata(repo):
    device = repo.path(repo.attr.device).dirname
    repo.watch_tree(device)
    # .bazelignore hides several vendor trees. The owner checker only needs
    # the platforms containing a rules.mk, not those files' Make content.
    platforms = []
    for entry in device.dirname.get_child("platform").readdir(watch = "yes"):
        if entry.is_dir:
            rules = entry.get_child("rules.mk")
            repo.watch(rules)
            if rules.exists:
                platforms.append(entry.basename)
    repo.file("metadata.json", json.encode({
        "executables": sorted(_executable_files(repo, device)),
        "platforms": sorted(platforms),
    }))
    repo.file("BUILD.bazel", "exports_files([\"metadata.json\"])\n")

device_data_metadata = repository_rule(
    implementation = _metadata,
    attrs = {"device": attr.label(default = Label("//device:BUILD.bazel"))},
    local = True,
)
