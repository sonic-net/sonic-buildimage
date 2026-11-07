#!/usr/bin/env python3

import os
import json
import jinja2
import hashlib
import argparse

TEMPLATES_DIR='./templates'
PACKAGES_DIR='./packages'
BUILD_DIR='./build'
PATCH_INFO_TEMPLATE='patch_info.j2'
PATCH_INFO_FILE='patch_info.yml'

def md5sum(filename):
    hash_md5 = hashlib.md5()
    with open(filename, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            hash_md5.update(chunk)
        return hash_md5.hexdigest()

def get_packages(directory):
    packages = []
    for root, dirs, files in os.walk(directory):
        if os.path.basename(directory).startswith('__'):
            continue
        if root == directory:
            for file in files:
                package = {}
                if file.startswith('docker') and file.endswith('.gz'):
                    package['type'] = 'docker'
                elif file.endswith('.deb'):
                    package['type'] = 'debian'
                elif file.startswith('hook_script_'):
                    package['type'] = 'hook_script'
                elif file.endswith('.kpatch'):
                    package['type'] = 'func_hotpatch'
                elif file == 'Dockerfile':
                    package['type'] = 'dockerfile'
                else:
                    # default for script
                    package['type'] = 'script'
                package['name'] = os.path.join(root, file)
                package['md5sum'] = md5sum(os.path.join(root, file))
                packages.append(package)
            for dir in sorted(dirs):
                packages.extend(get_packages(os.path.join(root, dir)))
    return packages

def main():
    parser = argparse.ArgumentParser(description='generate patch info file',
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-n', '--patch-name', type=str, help='patch name', default="")
    parser.add_argument('-e', '--extern-patch-name', type=str, help='patch name displayed outside in `show version`', default="")
    parser.add_argument('-t', '--patch-type', type=str, help='patch type', default="NORMAL")
    parser.add_argument('-a', '--sonic-version', type=str, help='patch version', default="")

    args = parser.parse_args()

    os.chdir(PACKAGES_DIR)
    packages = get_packages('.')
    os.chdir('..')

    patch_name = os.environ.get('PATCH_NAME', '') + ".tar.gz" if os.environ.get('PATCH_NAME', '') else args.patch_name
    extern_patch_name = os.environ.get('PATCH_EXTERN_NAME', '') + ".tar.gz" if os.environ.get('PATCH_EXTERN_NAME', '') else args.extern_patch_name
    patch_type = os.environ.get('PATCH_TYPE', '')  if os.environ.get('PATCH_TYPE', '') else args.patch_type
    sonic_version = os.environ.get('SONIC_VERSION', '') if os.environ.get('SONIC_VERSION', '') else args.sonic_version

    patch_config = {
        'patch_name': patch_name,
        'extern_patch_name': extern_patch_name,
        'patch_description': os.environ.get('PATCH_DESCRIPTION', ''),
        'patch_type': patch_type,
        'sonic_version': sonic_version,
        'previous_patch': os.environ.get('PREVIOUS_PATCH', ''),
        'process_name': os.environ.get('PROCESS_NAME', ''),
        'patch_id': os.environ.get('PATCH_ID', '')
    }

    templateLoader = jinja2.FileSystemLoader(searchpath=TEMPLATES_DIR)
    templateEnv = jinja2.Environment(loader=templateLoader)

    template = templateEnv.get_template(PATCH_INFO_TEMPLATE)
    output = template.render(
        packages = packages,
        **patch_config
    )

    if not os.path.exists(BUILD_DIR):
        os.makedirs(BUILD_DIR)

    with open(BUILD_DIR + '/' + PATCH_INFO_FILE, 'w+') as fp:
        fp.write(output)

    print(BUILD_DIR + '/' + PATCH_INFO_FILE)

if __name__ == '__main__':
    main()
