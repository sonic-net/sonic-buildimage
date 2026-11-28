"""Attribute overlapping VS batch profile spans to the images that produced them."""

from bisect import bisect_right
from collections import Counter, defaultdict
import math
from operator import itemgetter


COMPILERS = {"CppCompile", "CppModuleCompile", "Rustc", "GoCompilePkg", "GoCompile"}


def subprocess_mnemonic(event):
    """A local process names its tool. A sandbox process takes its enclosing action's mnemonic."""
    if event.get("cat") == "Local execution process wall time":
        return event["name"]
    if event.get("cat") == "general information" and event["name"] == "subprocess.run":
        return ""
    return None


def union_seconds(intervals):
    """Return the observed wall time, counting simultaneously active threads once."""
    return sum(end - start for start, end in merged(intervals)) / 1_000_000


def merged(intervals):
    end = -math.inf
    for a, b in sorted(intervals):
        if a > end:
            if end != -math.inf:
                yield start, end
            start = a
        end = max(b, end)
    if end != -math.inf:
        yield start, end


def cache_fetch(event):
    return (event.get("cat") == "remote output download"
            and event["name"].lower() in {"download outputs", "stage remote inputs"})


def package(label):
    # Bazel may spell a main-repository label with an explicit canonical repo.
    label = label.removeprefix("@@") if label.startswith("@@//") else label
    return label.split(":", 1)[0] if "//" in label else ""


def old_archive_paths(events, archives):
    """Recover final archive producer paths where old profiles omit target labels."""
    outputs = {}
    for event in events:
        if (event.get("cat") != "action processing" or event.get("ph") != "X"
                or event.get("args", {}).get("mnemonic") != "GzipCompress"):
            continue
        _, separator, path = event["name"].rpartition(" to ")
        archive = path.rsplit("/", 1)[-1]
        if separator and archive in archives:
            outputs.setdefault(archive, set()).add(path)
    return {archive: paths.pop() for archive, paths in outputs.items() if len(paths) == 1}


def batch_profile(events, elapsed, archives, *, legacy, owner_bounds=None):
    """archives maps selected archive names to (Make owner, public Bazel label)."""
    if not archives:
        return None
    outputs = old_archive_paths(events, archives)
    # Older receipts have export plans but do not link them to a particular build.
    # Require a matching archive action for every inferred member to establish it.
    if legacy and outputs.keys() != archives.keys():
        return None

    packages = defaultdict(set)
    image_labels = {}
    tar_outputs = {}
    gzip_outputs = {}
    for archive, (owner, label) in archives.items():
        directory = package(label)
        if directory and not directory.endswith("//"):
            packages[directory].add(owner)
        if archive in outputs:
            path = outputs[archive]
            prefix = path.removesuffix(".gz")
            gzip_outputs[path] = owner
            tar_outputs[prefix + ".load/tarball.tar"] = owner
            tar_outputs[prefix + ".load/tarball.spec"] = owner
            if "/" in prefix and not prefix.startswith("external/"):
                parent, name = prefix.rsplit("/", 1)
                image_labels[f"//{parent}:{name}"] = owner

    def owning_image(event):
        args = event.get("args", {})
        if target := args.get("target"):
            # Only the producing package can claim an action. Work performed by
            # external libraries, even for a single image, remains unattributed.
            directory = package(target)
            while directory and not directory.endswith("//"):
                if directory in packages:
                    owners = packages[directory]
                    if len(owners) == 1:
                        return next(iter(owners))
                    break
                directory = directory.rpartition("/")[0]
        # The text of arbitrary action messages may describe inputs. Match only
        # known final-image action formats whose label/output identifies the owner.
        name = event["name"]
        mnemonic = args.get("mnemonic")
        if mnemonic == "GzipCompress":
            return gzip_outputs.get(name.rpartition(" to ")[2])
        if mnemonic in {"OCITarball", "OCITarballManifest"}:
            return tar_outputs.get(name.removeprefix(mnemonic + " "))
        if mnemonic == "OCIImage":
            return image_labels.get(name.removeprefix("OCI Image "))
        return None

    actions = defaultdict(list)
    children = defaultdict(list)
    spans = {owner: defaultdict(list) for owner, _ in archives.values()}
    all_actions = []
    for event in events:
        if event.get("ph") != "X":
            continue
        start = event["ts"]
        stop = start + event["dur"]
        thread = (event["pid"], event["tid"])
        if event.get("cat") == "action processing":
            owner = owning_image(event)
            mnemonic = event.get("args", {}).get("mnemonic")
            actions[thread].append((start, stop, owner, mnemonic))
            all_actions.append((start, stop))
            if owner is not None:
                span = spans[owner]
                span["action"].append((start, stop))
                if mnemonic == "OCITarball":
                    span["archive"].append((start, stop))
                elif mnemonic == "GzipCompress":
                    span["compress"].append((start, stop))
        elif cache_fetch(event):
            children[thread].append((start, stop, "fetch"))
        else:
            process = subprocess_mnemonic(event)
            if process == "" or process in COMPILERS:
                # An unnamed subprocess can be classified only after finding its parent.
                children[thread].append((start, stop, "compile" if process else None))

    for thread, intervals in children.items():
        parents = sorted(actions[thread], key=itemgetter(0))
        for start, end, metric in intervals:
            index = bisect_right(parents, start, key=itemgetter(0)) - 1
            if index >= 0:
                _, parent_end, owner, mnemonic = parents[index]
                if owner is not None and end <= parent_end:
                    if metric is not None:
                        spans[owner][metric].append((start, end))
                    elif mnemonic in COMPILERS:
                        spans[owner]["compile"].append((start, end))

    rows = []
    changes = Counter()
    for owner, kinds in sorted(spans.items()):
        if owner_bounds is not None and kinds["action"]:
            owner_bounds[owner] = (min(start for start, _ in kinds["action"]),
                                   max(end for _, end in kinds["action"]))
        rows.append({"owner": owner, "actions": len(kinds["action"]),
                     **{k + "_seconds": union_seconds(kinds[k])
                        for k in ("action", "compile", "fetch", "archive", "compress")}})
        for start, end in merged(kinds["action"]):
            changes[start] += 1
            changes[end] -= 1
    active = previous = covered = parallel = 0
    for timestamp, delta in sorted(changes.items()):
        if active:
            covered += timestamp - previous
        if active > 1:
            parallel += timestamp - previous
        previous = timestamp
        active += delta
    all_seconds = union_seconds(all_actions)
    return {"rows": rows, "other_action_seconds": max(0, all_seconds - covered / 1_000_000),
            "no_action_seconds": max(0, elapsed - all_seconds), "parallel_seconds": parallel / 1_000_000}


def combine_batches(batches):
    """Independent retries contribute once each, just like the parent recipe."""
    owners = {}
    totals = Counter()
    for batch in batches:
        for row in batch["rows"]:
            owners.setdefault(row["owner"], Counter()).update({k: v for k, v in row.items() if k != "owner"})
        totals.update({k: v for k, v in batch.items() if k != "rows"})
    return {"rows": [{"owner": name, **values} for name, values in sorted(owners.items())], **totals}
