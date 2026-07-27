#!/usr/bin/env python3

import os
import re
import sys
import json
import argparse
import yaml
import syslog
import stat
import subprocess
import time
import threading
import re
import swsssdk
from distutils.version import LooseVersion, StrictVersion

APPL_PORT_TABLE = "PORT_TABLE"

def wait_port_init_done():
    try:
        appl_db = swsssdk.ConfigDBConnector()
        appl_db.connect(db_name=appl_db.APPL_DB)
    except Exception as e:
        log_error("Failed to connect to APPL_DB: {}".format(e), True)
        return
    retry = 180
    while retry:
        result = appl_db.get_entry(APPL_PORT_TABLE, "PortInitDone")
        if result:
            log_info("Port initialization is done", True)
            return
        time.sleep(1)
        retry -= 1
    log_info("Port initialization did not complete within 180s", True)

def log_info(msg, also_print_to_console=False):
    syslog.syslog(syslog.LOG_INFO, msg)
    if also_print_to_console:
        print(msg)

def log_error(msg, also_print_to_console=False):
    syslog.syslog(syslog.LOG_ERR, msg)
    if also_print_to_console:
        print(msg)

def run_command(command):
    if sys.version_info[0] < 3:  # Python 2
        process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output, error = process.communicate()
        return process.returncode, output
    else:  # Python 3
        process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        return process.returncode, process.stdout

with open('patch_info.yml', 'r') as fp:
    patch_info = yaml.load(fp, Loader=yaml.SafeLoader)

RESTART_FOR_ACTIVE_TMP_FILE='/tmp/.' + patch_info['Patch name'] + '.flag'

# MUST define
SUCCESS=0
FAIL=1

def _check_os_version_expected(version, expect_pattern):
    operators = [">=", "<=", ">", "<", "="]
    for op in operators:
        if expect_pattern[:len(op)] == op:
            expect_version = expect_pattern.replace(op, "").strip()
            return eval('LooseVersion(version) ' + op + ' LooseVersion(expect_version)')
    return LooseVersion(version) == LooseVersion(expect_pattern.strip())


# Background prompt
bk_prompt = False
bg_prompt_running = False
DEF_BG_PROMPT_INTERVAL = 30
def background_prompt(msg, interval):
    time.sleep(1)
    int_interval = 3
    if interval > int_interval:
        sleep_interval = int_interval
    else:
        sleep_interval = interval

    acc_interval = 0
    while bg_prompt_running == True:
        time.sleep(sleep_interval)
        acc_interval += sleep_interval
        if acc_interval > interval:
            log_info(msg)
            print("...")
            acc_interval = 0




def start_background_prompt(msg, interval=DEF_BG_PROMPT_INTERVAL):
    if not bk_prompt:
        return

    global bg_prompt_running
    prompt_thread = threading.Thread(target=background_prompt, args=(msg, interval,))
    if prompt_thread != None:
        bg_prompt_running = True
        prompt_thread.start()
    else:
        log_error("Failed to create background prompt thead")


def stop_background_prompt():
    if not bk_prompt:
        return

    global bg_prompt_running
    bg_prompt_running = False
    time.sleep(3)


# MUST define
def get_patch_name():
    return patch_info['Patch name']

# MAY define
def get_patch_extern_name():
    return patch_info['Patch extern name']

# MUST define
def get_patch_desc():
    return patch_info['Patch description']

# SHOULD define
def get_patch_type():
    return patch_info['Patch type']

# SHOULD define
def check_patch_inactive():
    inactive_packages = []
    for package in patch_info['Packages']:
        if 'RestartForActive' in package and package['RestartForActive'] == True:
            package_name = package['Name'] if 'Name' in package else os.path.basename(package['Package'])
            # check whether system had rebooted since patch installed
            if os.path.exists(RESTART_FOR_ACTIVE_TMP_FILE):
                inactive_packages.append(package_name)
    if inactive_packages:
        return True, inactive_packages
    else:
        return False, []

# MUST define
def do_patch_install():
    output = ''
    recovery = False
    recovery_index = 0
    with open('/etc/sonic/sonic_version.yml', 'r') as fp:
        sonic_version_info = yaml.load(fp, Loader=yaml.SafeLoader)
    os_version = sonic_version_info['build_version']
    if patch_info.get('SONiC version'):
        if not _check_os_version_expected(os_version, patch_info['SONiC version']):
            log_error('Patch only available for version {}\n'.format(patch_info['SONiC version']), True)
            return FAIL, output

    # prev_patch = ""
    # if 'patches' in sonic_version_info and len(sonic_version_info['patches']) > 0:
    #     prev_patch = sonic_version_info['patches'][-1].get('name', '')

    # if patch_info.get('Previous patch') and patch_info['Previous patch'] != prev_patch:
    #     log_error('Patch only available based on patch {}\n'.format(sonic_version_info['patches'][-1].get('name')), True)
    #     return FAIL, output

    # Wait for PORT_INIT_DONE if this is a hotpatch type
    if patch_info.get('Patch type') == 'hotpatch':
        log_info("Patch type is hotpatch, waiting for PORT_INIT_DONE...", True)
        wait_port_init_done()

    # stop apt-daily and apt-daily-upgrade
    run_command('systemctl stop apt-daily{,-upgrade}.{timer,service}')

    for package in patch_info['Packages']:
        _, md5sum = run_command('md5sum {}'.format(package['Package']))
        if md5sum:
            md5sum = md5sum.split()[0]
        record_md5sum = package['Md5sum']
        if md5sum != record_md5sum:
            raise Exception('md5sum {} of {} is incorrect'.format(md5sum, package['Package']))

        # Background prompt
        global bk_prompt
        if package.get('Background prompt') and package['Background prompt']:
            bk_prompt = True

        pkg_type = package['Type']
        if pkg_type == 'docker':
            upgrade = UpgradeDocker(package['Package'], package['Name'], package['Version'], package['Service'], 
                                    True if package.get('RestartForActive') and package['RestartForActive'] == True else False)
        elif pkg_type == 'debian':
            upgrade = UpgradeDebian(package['Package'], package['Name'])
        elif pkg_type == 'script':
            upgrade = UpgradeScript(package['Package'], package['Target directory'])
        elif pkg_type == 'hook_script':
            upgrade = ExecHookScript(package['Package'])
        elif pkg_type == 'dockerfile':
            upgrade = BuildDockerImage(package['Package'], package['Name'], package['Service'], package['Version'])
        elif pkg_type == 'func_hotpatch':
            upgrade = FuncHotPatch(package['Package'], package['ProcessName'], package['PatchId'])
        else:
            # ignore
            continue

        start_background_prompt("PkgType %s starts running" % pkg_type)
        rv, out = upgrade.run()
        stop_background_prompt()
        output += out
        if rv != SUCCESS:
            recovery = True
            recovery_index = patch_info['Packages'].index(package)
            break
    if recovery:
        rv, out = patch_uninstall(reversed(patch_info['Packages'][0:recovery_index]))
        output += out

        # restart apt-daily and apt-daily-upgrade
        run_command('systemctl start apt-daily{,-upgrade}.timer')
        return FAIL, output

    # restart apt-daily and apt-daily-upgrade
    run_command('systemctl start apt-daily{,-upgrade}.timer')
    return SUCCESS, output

def patch_uninstall(packages):
    output = ''

    # stop apt-daily and apt-daily-upgrade
    run_command('systemctl stop apt-daily{,-upgrade}.{timer,service}')

    for package in packages:
        pkg_type = package['Type']
        if pkg_type == 'docker':
            upgrade = UpgradeDocker(package['Package'], package['Name'], package['Version'], package['Service'],
                                    True if package.get('RestartForActive') and package['RestartForActive'] == True else False)

        elif pkg_type == 'debian':
            upgrade = UpgradeDebian(package['Package'], package['Name'])
        elif pkg_type == 'script':
            upgrade = UpgradeScript(package['Package'], package['Target directory'])
        elif pkg_type == 'hook_script':
            upgrade = ExecHookScript(package['Package'])
        elif pkg_type == 'dockerfile':
            upgrade = BuildDockerImage(package['Package'], package['Name'], package['Service'], package['Version'])
        elif pkg_type == 'func_hotpatch':
            upgrade = FuncHotPatch(package['Package'], package['ProcessName'], package['PatchId'])
        else:
            # ignore
            continue
        rv, out = upgrade.rollback()
        output += out
        if rv != SUCCESS:
            # restart apt-daily and apt-daily-upgrade
            run_command('systemctl start apt-daily{,-upgrade}.timer')
            return rv, output
    # restart apt-daily and apt-daily-upgrade
    run_command('systemctl start apt-daily{,-upgrade}.timer')
    return SUCCESS, output

# MUST define
def do_patch_uninstall():
    return patch_uninstall(reversed(patch_info['Packages']))

def _execute_command(cmd, print_to_stdout=False):
    status, p = run_command(cmd)
    output = ''
    for line in p.splitlines():
        output += line
        if print_to_stdout:
            print(line)
            sys.stdout.flush()
    return status, output

class Upgrade(object):
    def __init__(self, package, cold=False):
        self.package = package
        self.upgrade_commands= []
        self.recover_commands= []
        self.cold_upgrade_commands = []
        self.cold_recover_commands = []
        self.cold = cold

    def run(self, print_to_stdout=False):
        output = ""
        upgrade_commands = []
        recover_commands = []
        if not self.cold:
            upgrade_commands = self.upgrade_commands
            recover_commands = self.recover_commands
        else:
            upgrade_commands = self.cold_upgrade_commands
            recover_commands = self.cold_recover_commands

        try:
            log_info('Starting upgrading {}...'.format(self.package), True)
            for command in upgrade_commands:
                log_info(command)
                status, out = _execute_command(command, print_to_stdout)
                if status != 0:
                    log_error(out)
                    raise Exception('failed to execute command \'{}\': {}'.format(command, out))

            # set a file flag in /tmp/ to indicate system not reboot yet
            if self.cold and not os.path.exists(RESTART_FOR_ACTIVE_TMP_FILE):
                with open(os.path.join('/tmp', RESTART_FOR_ACTIVE_TMP_FILE), 'w'): pass

            log_info('Upgrade successfully', True)
            return SUCCESS, output
        except Exception as e:
            log_error(str(e), True)
            log_info('Starting recovering {}...'.format(self.package), True)
            for command in recover_commands:
                log_info(command)
                status, out = _execute_command(command, print_to_stdout)
                if status != 0:
                    log_error(out)
                    break
            log_info('Upgrade Failed', True)
            return FAIL, output

    def rollback(self):
        if self.cold and os.path.exists(RESTART_FOR_ACTIVE_TMP_FILE):
            os.remove(RESTART_FOR_ACTIVE_TMP_FILE)
        return SUCCESS, ''

class UpgradeScript(Upgrade):
    def __init__(self, path, target_dir):
        super(UpgradeScript, self).__init__(path)
        target_file = os.path.join(target_dir, os.path.basename(path))
        self.upgrade_commands = [
            "mkdir -p backup/{} && if [ -f {} ]; then sudo cp -p {} backup/{}; fi".format(os.path.dirname(path), target_file, target_file, os.path.dirname(path)),
            "if [ -f {} ]; then sudo chmod --reference={} {} && sudo chown --reference={} {}; fi".format(target_file, target_file, path, target_file, path),
            "sudo cp -p {} {}".format(path, target_file)
        ]
        self.recover_commands = [
            "if [ -f backup/{} ]; then sudo chmod --reference={} backup/{}; fi".format(path, target_file, path),
            "if [ -f backup/{} ]; then sudo chown --reference={} backup/{}; fi".format(path, target_file, path),
            'if [ -f backup/{} ]; then sudo mv backup/{} {}; else rm -f {}; fi'.format(path, path, target_file, target_file)
        ]

    def rollback(self):
        for command in self.recover_commands:
            log_info(command)
            status, out = _execute_command(command)
            if status != 0:
                log_error(out, True)
                return FAIL, ''
        return super(UpgradeScript, self).rollback()

class UpgradeDocker(Upgrade):
    def __init__(self, docker_image, docker_name, version, service, restart_for_active=False):
        super(UpgradeDocker, self).__init__(docker_image, cold=restart_for_active)

        self.docker_repo = os.path.basename(docker_image).rsplit('.', 1)[0]
        self.docker_name = docker_name
        self.version = version
        self.service = service
        self.boot_from_docker_image_file = '/etc/sonic/boot_from_docker_image_{}'.format(service)

        if self.service == 'snmp':
            self.upgrade_commands = [
                'docker rmi {}:latest'.format(self.docker_repo),
                'docker load < {}'.format(docker_image),
                'systemctl stop {}.timer'.format(self.service),
                'systemctl stop {}.service'.format(self.service),
                # when auto install patch after poap, snmp may not startup
                'docker rm {} 2>/dev/null || true'.format(self.docker_name), 
                'systemctl start {}.timer'.format(self.service),
                'systemctl status {0}.timer | grep \'elapsed\'; if [ $? -eq 0 ]; then systemctl start {0}.service; fi'.format(self.service),
                'docker tag {}:latest {}:{}'.format(self.docker_repo, self.docker_repo, self.version)
            ]
            self.recover_commands = [
                'cat backup/{}_orig_image_id | xargs -I [] docker tag [] {}:latest'.format(self.docker_repo, self.docker_repo),
                'systemctl restart {}.timer'.format(self.service),
            ]
        else:
            service_state, _ = _execute_command('systemctl is-active --quiet {0}'.format(self.service))
            self.upgrade_commands = [
                'docker rmi {}:latest'.format(self.docker_repo),
                'docker load < {}'.format(docker_image),
                'systemctl stop {0}.service && docker rm {1} && systemctl start {0}.service'.format(self.service, self.docker_name) if service_state == 0 else ':',
                'docker tag {}:latest {}:{}'.format(self.docker_repo, self.docker_repo, self.version)
            ]
            self.recover_commands = [
                'cat backup/{}_orig_image_id | xargs -I [] docker tag [] {}:latest'.format(self.docker_repo, self.docker_repo),
                'systemctl restart {}.service'.format(self.service) if service_state == 0 else ':',
            ]

        self.cold_upgrade_commands = [
            'docker rmi {}:latest'.format(self.docker_repo),
            'docker load < {}'.format(docker_image),
            'docker tag {}:latest {}:{}'.format(self.docker_repo, self.docker_repo, self.version),
            'touch {}'.format(self.boot_from_docker_image_file)
        ]

        self.cold_recover_commands = [
            'cat backup/{}_orig_image_id | xargs -I [] docker tag [] {}:latest'.format(self.docker_repo, self.docker_repo),

        ]

    @classmethod
    def get_docker_image_version_tags(image_id):
        status, output = _execute_command('docker inspect {}'.format(image_id))
        if status != 0:
            return ''
        docker_info = json.loads(output)[0]
        tags = []
        for tag in docker_info['RepoTags']:
            repo, tag = tag.split(':', 1)
            if tag == 'latest':
                continue
            tags.append(tag)
        return tags

    @classmethod
    def get_docker_image_id(self, docker_repo, docker_tag):
        status, output = _execute_command('docker inspect {}:{}'.format(docker_repo, docker_tag))
        if status != 0:
            return ''
        docker_info = json.loads(output)[0]
        image_id = docker_info['Id'].split(':')[1][:12]
        return image_id

    @classmethod
    def get_docker_image_full_id(self, docker_repo, docker_tag):
        status, output = _execute_command('docker inspect {}:{} --format "{{{{ .Id }}}}"'.format(docker_repo, docker_tag))
        if status != 0:
            return ''
        return output

    def run(self):
        image_id = self.get_docker_image_id(self.docker_repo, 'latest')
        status, output = _execute_command('mkdir -p backup; cd backup; echo -n "{}" > {}_orig_image_id'.format(image_id, self.docker_repo))
        if status != 0:
            return FAIL, output

        return super(UpgradeDocker, self).run()

    def rollback(self):
        with open('backup/{}_orig_image_id'.format(self.docker_repo), 'r') as fp:
            orig_docker_image_id = fp.read()

        if self.get_docker_image_id(self.docker_repo, 'latest') == orig_docker_image_id:
            return SUCCESS, ''

        if self.service == 'snmp':
            cmds = [
                'docker rmi {}:latest'.format(self.docker_repo),
                'docker tag {} {}:latest'.format(orig_docker_image_id, self.docker_repo),
                'systemctl stop {}.timer'.format(self.service),
                'systemctl stop {}.service'.format(self.service),
                # when auto install patch after poap, snmp may not startup
                'docker rm {} 2>/dev/null || true'.format(self.docker_name), 
                'systemctl start {}.timer'.format(self.service),
                'systemctl status {0}.timer | grep \'elapsed\'; if [ $? -eq 0 ]; then systemctl start {0}.service; fi'.format(self.service),
                'docker rmi {}:{}'.format(self.docker_repo, self.version)
            ]
        else:
            cmds = [
                'docker rmi {}:latest'.format(self.docker_repo),
                'docker tag {} {}:latest'.format(orig_docker_image_id, self.docker_repo),
                'systemctl stop {}.service'.format(self.service),
                'docker rm {}'.format(self.docker_name),
                'systemctl start {}.service'.format(self.service),
                'docker rmi {}:{}'.format(self.docker_repo, self.version)
            ]

        if self.cold:
            cmds = [
                'docker rmi {}:latest'.format(self.docker_repo),
                'docker tag {} {}:latest'.format(orig_docker_image_id, self.docker_repo),
                'touch {}'.format(self.boot_from_docker_image_file)
            ]
        for cmd in cmds:
            log_info(cmd)
            status, out = _execute_command(cmd)
            if status != 0:
                log_error('failed to execute command \'{}\': {}'.format(cmd, out))
                return FAIL, ''
        return super(UpgradeDocker, self).rollback()

class UpgradeDebian(Upgrade):
    def __init__(self, debian_pkg, debian_pkg_name):
        super(UpgradeDebian, self).__init__(debian_pkg)

        self.debian_pkg = debian_pkg
        self.debian_pkg_name = debian_pkg_name

        self.upgrade_commands = [
            'dpkg -r {}'.format(self.debian_pkg_name),
            'dpkg -i {}'.format(self.debian_pkg),
        ]
        self.recover_commands = [
        ]

    def run(self):
        status, output = _execute_command('mkdir -p backup; cd backup; LC_ALL="en_US.UTF-8" dpkg-repack {}'.format(self.debian_pkg_name))
        if status != 0:
            return FAIL, output

        return super(UpgradeDebian, self).run()

    def rollback(self):
        old_debian_pkg = ''
        for _, _, files in os.walk('backup'):
            for file in files:
                if file.endswith('.deb') and file.startswith(self.debian_pkg_name):
                    old_debian_pkg = file
                    break

        if not old_debian_pkg:
            return FAIL, ''

        cmds = [
            'dpkg -r {}'.format(self.debian_pkg_name),
            'dpkg -i {}'.format('backup/' + old_debian_pkg)
        ]
        for cmd in cmds:
            log_info(cmd)
            status, out = _execute_command(cmd)
            if status != 0:
                log_error(out, True)
                return FAIL, ''
        return super(UpgradeDebian, self).rollback()

class ExecHookScript(Upgrade):
    def __init__(self, script_name):
        super(ExecHookScript, self).__init__(script_name)
        self.script_name = script_name
        # make sure script has execution permission
        st = os.stat(script_name)
        self.upgrade_commands= [
            './{} run'.format(self.script_name)
        ]
        os.chmod(script_name, st.st_mode | stat.S_IEXEC)

    def run(self):
        return super(ExecHookScript, self).run(True)

    def rollback(self):
        status, out = _execute_command('./{} rollback'.format(self.script_name), True)
        if status != 0:
            log_error('failed to rollback {}: {}'.format(self.script_name, out), True)
            return FAIL, ''
        return super(ExecHookScript, self).rollback()

class BuildDockerImage(Upgrade):
    def __init__(self, dockerfile, image_name, service, version):
        super(BuildDockerImage, self).__init__(dockerfile)
        self.boot_from_docker_image_file = '/etc/sonic/boot_from_docker_image_{}'.format(service)
        self.image_name = image_name
        self.service = service
        self.version = version
        self.upgrade_commands = [
            'docker build --no-cache --label Tag={0} -t {1}:{0} {2}'.format(version, image_name, os.path.dirname(dockerfile)),
            'docker tag {0}:{1} {0}:latest'.format(image_name, version),
            'docker image prune --force --filter label=Tag={}'.format(version),
            'touch {}'.format(self.boot_from_docker_image_file)
        ]

    def run(self):
        image_id = UpgradeDocker.get_docker_image_id(self.image_name, 'latest')
        status, output = _execute_command('mkdir -p backup; cd backup; echo -n "{}" > {}_orig_image_id'.format(image_id, self.image_name))
        if status != 0:
            return FAIL, output

        return super(BuildDockerImage, self).run()

    def rollback(self):
        with open('backup/{}_orig_image_id'.format(self.image_name), 'r') as fp:
            orig_docker_image_id = fp.read()

        latest_image_id = UpgradeDocker.get_docker_image_id(self.image_name, 'latest')
        image_id = UpgradeDocker.get_docker_image_full_id(self.image_name, self.version)
        if latest_image_id == orig_docker_image_id:
            if image_id:
                cmd = 'docker rmi {}:{}'.format(self.image_name, self.version)
                status, out = _execute_command(cmd)
                if status != 0:
                    log_error('failed to execute command \'{}\': {}'.format(cmd, out))
                    return FAIL, ''
                return SUCCESS, ''
        else:
            _, container_image_id = _execute_command('docker inspect {} --format "{{{{ .Image }}}}"'.format(self.service))
            if container_image_id == image_id:
                # image is running
                cmds = [
                    'docker tag {} {}:latest'.format(orig_docker_image_id, self.image_name),
                    'touch {}'.format(self.boot_from_docker_image_file)
                ]
            else:
                # image is not running
                cmds = [
                    'docker tag {} {}:latest'.format(orig_docker_image_id, self.image_name),
                    'docker rmi {}:{}'.format(self.image_name, self.version),
                    'if [ -e {0} ]; then rm {0}; fi'.format(self.boot_from_docker_image_file)
                ]
            for cmd in cmds:
                log_info(cmd)
                status, out = _execute_command(cmd)
                if status != 0:
                    log_error(out, True)
                    return FAIL, ''
        return SUCCESS, ''

class FuncHotPatch(Upgrade):
    def __init__(self, hotpatch_name, process_name, patch_id):
        super(FuncHotPatch, self).__init__(hotpatch_name)
        self.hotpatch_name = hotpatch_name
        self.process_name = process_name
        self.patch_id = str(patch_id).zfill(4)

    def get_pid(self):
        # Execute the `pidof` command
        status, out = _execute_command('pidof {}'.format(self.process_name))
        
        # Check status and handle output
        if status == 0:
            pids = out.split()  # Split the output to get individual PIDs
            log_info('PID(s) of {}: {}'.format(self.process_name, pids))
            return pids
        else:
            log_error('Failed to get PID for {}, error: {}'.format(self.process_name, out))
            return []
        
    def run(self):
        pids = self.get_pid()

        for pid in pids:
            log_info('checking pid {}'.format(pid))
            status, out = _execute_command('libcare-ctl info -p {}'.format(pid))
            if status != 0:
                log_error('failed to get patch info for pid {}: {}'.format(pid, out))
                return FAIL, ''
            log_info('pid {} patch info: {}'.format(pid, out))

            already_patched = False
            match = re.search(r'Patch id:\s*([0-9A-Za-z]+)', out)
            if match:
                cur_id = match.group(1)
                if cur_id == self.patch_id:
                    already_patched = True
                    log_info('pid {} already patched with id {}, skip'.format(pid, cur_id))

            if already_patched:
                continue 

            status, out = _execute_command(
                'libcare-ctl patch -p {} {}'.format(pid, self.hotpatch_name))
            if status != 0:
                log_error('failed to apply patch {} to pid {}: {}'.format(self.hotpatch_name, pid, out))
                return FAIL, ''
            log_info('libcare-ctl patch output: {}'.format(out))

        return SUCCESS, ''


        
    def rollback(self):
        pids = self.get_pid()
        # Iterate over the PIDs and apply the hotpatch
        for pid in pids:
            status, out = _execute_command('libcare-ctl info -p {}'.format(pid))
            if status != 0:
                log_error('failed to get patch info for pid {}: {}'.format(pid, out))
                return FAIL, ''
            if not out.strip():
                log_info('pid {} is not patched'.format(pid))
                continue
            
            status, out = _execute_command('libcare-ctl unpatch -p {} -i {}'.format(pid, self.patch_id))
            if status != 0:
                log_error('failed to unapply patch id {} to pid {}: {}'.format(self.patch_id, pid, out))
                return FAIL, ''
            log_info('libcare-ctl unpatch output: {}'.format(out))
        return SUCCESS, ''



if __name__ == '__main__':
    print('Patch Name: ' + get_patch_name())
    print('Patch Description: ' + get_patch_desc())
    print('Starting install patch...')
    try:
        do_patch_install()
    except Exception as e:
        print(str(e))
        sys.exit(1)

