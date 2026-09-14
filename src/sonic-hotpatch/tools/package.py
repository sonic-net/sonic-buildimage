#!/usr/bin/env python3

import os
import argparse
import yaml
import sys
import subprocess
import re

def is_macos():
    if sys.platform == 'darwin':
        return True
    return False

def run_command(command):
    proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, text='utf-8')
    (out, err) = proc.communicate()

    print('+ ' + command)
    if proc.returncode != 0:
        sys.exit(proc.returncode)
    return out

def main():
    parser = argparse.ArgumentParser(description='packaging patches',
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--version', action='version', version='%(prog)s 1.0.0')
    parser.add_argument('-o', '--old', type=str, help='old patch', default=None)
    parser.add_argument('-s', '--new-sub-patches', nargs='+', help='sub patches to append', default=[])
    parser.add_argument('-r', '--replace-sub-patches', nargs='+',
                        help='sub patches to replace existing ones (matched by basename); requires --old',
                        default=[])
    parser.add_argument('-n', '--patch-name', type=str, help='new patch name', required=True)
    parser.add_argument('-a', '--sonic-version', type=str, help='patch version', default=None)
    parser.add_argument('--non-interactive', action='store_true', help='automatic script uses non-interactive mode')
    parser.add_argument('--patch-type', type=str, help='patch type', default="normal")
    args = parser.parse_args()
    if not args.old and not args.sonic_version:
        args.print_help()

    run_command("mkdir -p %s"%(args.patch_name))
    summary = {}
    CI=False
    if args.non_interactive:
        CI=True
    if args.old:
        if is_macos():
            run_command("tar -xvz --no-xattrs -f %s -C %s/"%(args.old, args.patch_name))
        else:
            run_command("tar -xvz -f %s -C %s/"%(args.old, args.patch_name))
        with open('%s/summary.yml'%(args.patch_name), 'r') as fp:
            summary = yaml.load(fp, Loader=yaml.SafeLoader)
    else:
        summary['os_version'] = args.sonic_version
        summary['patches'] = []

    if args.replace_sub_patches and not args.old:
        print("!!! --replace-sub-patches requires --old")
        sys.exit(1)

    if args.new_sub_patches:
        new_sub_patches = args.new_sub_patches
    elif args.replace_sub_patches:
        # only replacing existing sub patches, no new sub patch to append/build
        new_sub_patches = []
    else:
        # try to make sub patch from packages
        build_script = os.path.join(os.path.dirname(__file__), 'build.sh')
        run_command("chmod +x %s"%(build_script))
        if build_script == "build.sh":
            build_script = "./build.sh"
        if CI is True:
            build_script = "CI=true ./build.sh"
        if summary['patches']:
            last_sub_patch = summary['patches'][-1]['name']
            match = re.search(r'(\d+).tar.gz', last_sub_patch)
            if not match:
                print("!!! last sub patch name invalid: %s"%(last_sub_patch))
                sys.exit(1)
            new_sub_patch = last_sub_patch[:match.start()] + str(int(match.group(1)) + 1) + '.tar.gz'
        else:
            new_sub_patch = 'sub-Hotfix1.tar.gz'
        cmd = build_script
        cmd += " --patch-name " + new_sub_patch
        cmd += " --extern-patch-name " + args.patch_name + ".tar.gz"
        if args.sonic_version:
            cmd += " --sonic-version " + args.sonic_version
        cmd += " --patch-type " + args.patch_type
        print(cmd)
        os.system(cmd)
        new_sub_patches = [new_sub_patch]

    for sub in new_sub_patches:
        if is_macos():
            md5sum = run_command("md5sum --quiet %s"%(sub)).strip()
        else:
            md5sum = run_command("md5sum %s"%(sub)).strip()
            md5sum = md5sum.split()[0]
        summary['patches'].append({'name': sub, 'md5sum': str(md5sum)})
        run_command("cp %s %s/"%(sub, args.patch_name))

    for sub in args.replace_sub_patches:
        sub_name = os.path.basename(sub)
        found_idx = None
        for i, p in enumerate(summary['patches']):
            if p['name'] == sub_name:
                found_idx = i
                break
        if found_idx is None:
            existing = [p['name'] for p in summary['patches']]
            print("!!! sub patch %s not found in old patch, cannot replace. existing: %s"%(sub_name, existing))
            sys.exit(1)
        if is_macos():
            md5sum = run_command("md5sum --quiet %s"%(sub)).strip()
        else:
            md5sum = run_command("md5sum %s"%(sub)).strip()
            md5sum = md5sum.split()[0]
        old_md5 = summary['patches'][found_idx].get('md5sum')
        summary['patches'][found_idx] = {'name': sub_name, 'md5sum': str(md5sum)}
        print("replaced %s: %s -> %s"%(sub_name, old_md5, md5sum))
        run_command("cp %s %s/"%(sub, args.patch_name))

    print(summary)

    with open('%s/summary.yml'%(args.patch_name), 'w') as fp:
        yaml.dump(summary, fp)

    if is_macos():
        run_command('cd %s; tar -cvz --no-xattrs -f ../%s.tar.gz *'%(args.patch_name, args.patch_name))
        run_command('md5sum --quiet %s.tar.gz > %s.tar.gz.md5'%(args.patch_name, args.patch_name))
    else:
        run_command('cd %s; tar -cvz -f ../%s.tar.gz *'%(args.patch_name, args.patch_name))
        run_command('md5sum %s.tar.gz > %s.tar.gz.md5'%(args.patch_name, args.patch_name))

if __name__ == '__main__':
    main()
