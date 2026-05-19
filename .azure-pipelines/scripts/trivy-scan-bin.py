#!/usr/bin/env python3
"""
Reconstruct docker-save format tarballs from a SONiC .bin installer and
scan each container image with trivy.

Usage:
    python3 scan.py <path-to-sonic-PLATFORM.bin> [results-dir]

Example:
    python3 scan.py ~/sonic-buildimage/target/sonic-broadcom.bin /tmp/broadcom-scan/results
    python3 scan.py ~/sonic-buildimage/target/sonic-mellanox.bin /tmp/mellanox-scan/results
"""
import json, os, tarfile, io, subprocess, sys, shutil, re, zipfile, tempfile

def detect_payload_size(bin_path):
    """Read payload_image_size from the shell script header."""
    with open(bin_path, 'rb') as f:
        header = f.read(4096)
    m = re.search(rb'payload_image_size=(\d+)', header)
    if not m:
        raise RuntimeError("Could not find payload_image_size in .bin header")
    return int(m.group(1))

def find_exit_marker(bin_path):
    """Find byte offset just after the exit_marker line."""
    with open(bin_path, 'rb') as f:
        header = f.read(8192)
    marker = b'\nexit_marker\n'
    pos = header.find(marker)
    if pos < 0:
        raise RuntimeError("Could not find exit_marker in .bin header")
    return pos + len(marker)

def extract_dockerfs(bin_path, workdir):
    """Extract dockerfs.tar.gz from inside the .bin installer."""
    payload_size = detect_payload_size(bin_path)
    payload_start = find_exit_marker(bin_path)
    print(f"  payload_start={payload_start}, payload_size={payload_size}", flush=True)

    print("  Extracting installer/fs.zip from tar payload...", flush=True)
    with open(bin_path, 'rb') as f:
        f.seek(payload_start)
        payload_data = f.read(payload_size)

    tf = tarfile.open(fileobj=io.BytesIO(payload_data))
    fs_zip_member = next(m for m in tf.getnames() if m.endswith('fs.zip'))
    fs_zip_data = tf.extractfile(fs_zip_member).read()

    print("  Extracting dockerfs.tar.gz from fs.zip...", flush=True)
    zf = zipfile.ZipFile(io.BytesIO(fs_zip_data))
    dockerfs_data = zf.read('dockerfs.tar.gz')
    print(f"  dockerfs.tar.gz: {len(dockerfs_data)/1024/1024:.1f} MB", flush=True)

    dockerdata = os.path.join(workdir, 'dockerdata')
    os.makedirs(dockerdata, exist_ok=True)
    print("  Extracting overlay2 data...", flush=True)
    dtf = tarfile.open(fileobj=io.BytesIO(dockerfs_data))
    # ignore device node errors (volumes/backingFsBlockDev etc)
    for member in dtf.getmembers():
        try:
            dtf.extract(member, path=dockerdata)
        except Exception:
            pass

    return dockerdata

def build_diff_to_cache_map(dockerdata):
    layerdb = os.path.join(dockerdata, 'image/overlay2/layerdb/sha256')
    diff_to_cache = {}
    for entry in os.listdir(layerdb):
        cid_file = os.path.join(layerdb, entry, 'cache-id')
        diff_file = os.path.join(layerdb, entry, 'diff')
        if os.path.exists(cid_file) and os.path.exists(diff_file):
            with open(cid_file) as f: cid = f.read().strip()
            with open(diff_file) as f: diff = f.read().strip()
            diff_to_cache[diff] = cid
    return diff_to_cache

def build_docker_save(image_name, image_sha, dockerdata, diff_to_cache, out_path):
    img_id = image_sha.replace('sha256:', '')
    config_path = os.path.join(dockerdata, 'image/overlay2/imagedb/content/sha256', img_id)
    with open(config_path, 'rb') as f:
        config_data = f.read()
    config_obj = json.loads(config_data)
    diff_ids = config_obj['rootfs']['diff_ids']

    layer_entries = []
    for i, diff_id in enumerate(diff_ids):
        cache_id = diff_to_cache.get(diff_id)
        if not cache_id:
            print(f"  WARNING: no cache-id for diff {diff_id[:16]} in {image_name}", file=sys.stderr)
            continue
        diff_dir = os.path.join(dockerdata, 'overlay2', cache_id, 'diff')
        layer_entries.append((f'layer_{i:03d}', diff_dir))

    with tarfile.open(out_path, 'w') as out:
        config_filename = f'{img_id}.json'
        info = tarfile.TarInfo(name=config_filename)
        info.size = len(config_data)
        out.addfile(info, io.BytesIO(config_data))

        layer_paths = []
        for layer_name, diff_dir in layer_entries:
            layer_tar_name = f'{layer_name}/layer.tar'
            layer_paths.append(layer_tar_name)

            layer_buf = io.BytesIO()
            with tarfile.open(fileobj=layer_buf, mode='w') as lt:
                if os.path.isdir(diff_dir):
                    lt.add(diff_dir, arcname='.', recursive=True)
            layer_data = layer_buf.getvalue()

            dir_info = tarfile.TarInfo(name=layer_name)
            dir_info.type = tarfile.DIRTYPE
            dir_info.mode = 0o755
            out.addfile(dir_info)

            lt_info = tarfile.TarInfo(name=layer_tar_name)
            lt_info.size = len(layer_data)
            out.addfile(lt_info, io.BytesIO(layer_data))

        manifest = [{"Config": config_filename,
                     "RepoTags": [f"{image_name}:latest"],
                     "Layers": layer_paths}]
        manifest_data = json.dumps(manifest).encode()
        minfo = tarfile.TarInfo(name='manifest.json')
        minfo.size = len(manifest_data)
        out.addfile(minfo, io.BytesIO(manifest_data))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    bin_path = sys.argv[1]
    platform = os.path.basename(bin_path).replace('sonic-', '').replace('.bin', '')
    workdir = sys.argv[2] if len(sys.argv) > 2 else f'/tmp/{platform}-scan'
    results_dir = os.path.join(workdir, 'results')
    archives_dir = os.path.join(workdir, 'archives')
    os.makedirs(results_dir, exist_ok=True)
    os.makedirs(archives_dir, exist_ok=True)

    print(f"Platform: {platform}")
    print(f"BIN:      {bin_path}")
    print(f"Results:  {results_dir}\n")

    # Check if dockerdata already extracted (allow re-run without re-extraction)
    dockerdata = os.path.join(workdir, 'dockerdata')
    repos_path = os.path.join(dockerdata, 'image/overlay2/repositories.json')
    if os.path.exists(repos_path):
        print("==> dockerdata already extracted, skipping extraction.", flush=True)
    else:
        print("==> Extracting .bin...", flush=True)
        dockerdata = extract_dockerfs(bin_path, workdir)

    with open(repos_path) as f:
        repos = json.load(f)['Repositories']

    diff_to_cache = build_diff_to_cache_map(dockerdata)

    overall_exit = 0
    images = sorted(repos.keys())
    print(f"==> Scanning {len(images)} images\n")

    for image_name in images:
        sha = list(repos[image_name].values())[0]
        archive_path = os.path.join(archives_dir, f'{image_name}.tar')
        result_path = os.path.join(results_dir, f'{image_name}.txt')

        print(f"[BUILD] {image_name}...", flush=True)
        try:
            build_docker_save(image_name, sha, dockerdata, diff_to_cache, archive_path)
        except Exception as e:
            print(f"  ERROR: {e}", file=sys.stderr)
            overall_exit = 1
            continue

        print(f"[SCAN ] {image_name}...", flush=True)
        result = subprocess.run([
            'trivy', 'image',
            '--input', archive_path,
            '--skip-db-update',
            '--severity', 'MEDIUM,HIGH,CRITICAL',
            '--ignore-unfixed',
            '--exit-code', '1',
            '--format', 'table',
            '--no-progress',
            '--timeout', '10m',
            '--output', result_path,
        ], capture_output=True, text=True)

        status = "CLEAN" if result.returncode == 0 else "FINDINGS"
        try:
            with open(result_path) as f:
                content = f.read()
            total_lines = [l for l in content.splitlines() if l.startswith('Total:')]
            summary = total_lines[0] if total_lines else "(no Total line)"
        except:
            content = ""
            summary = "(unreadable)"

        print(f"  -> {status}: {summary}", flush=True)
        print(f"\n=== {image_name} ===", flush=True)
        print(content, flush=True)
        if result.returncode not in (0, 1):
            print(f"  trivy stderr: {result.stderr[:300]}", file=sys.stderr)
            overall_exit = 1

        os.unlink(archive_path)

    print(f"\n=== DONE. Results in {results_dir} ===")
    sys.exit(overall_exit)

if __name__ == '__main__':
    main()
