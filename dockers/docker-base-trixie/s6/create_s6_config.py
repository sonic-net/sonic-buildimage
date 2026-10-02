#!/usr/bin/env python3

import os
import sys
import configparser
import glob

LOGGER='logger'
EVENT='event_listener'
LONGRUN_SERVICES=[]
EVENT_FIFO_DIR='/var/run/event_listener'
EVENT_FIFO=os.path.join(EVENT_FIFO_DIR, 'input')

def parse_supervisor_conf(conf_path):
    config = configparser.ConfigParser()
    config.read(conf_path)
    programs = {}
    event_listener = False
    for section in config.sections():
        if section.startswith('program:'):
            name = section.split(':', 1)[1]
            options = dict(config.items(section))
            programs[name] = options
        elif section.startswith('eventlistener:'):
            name = section.split(':', 1)[1]
            if name == 'supervisor-proc-exit-listener':
                event_listener = True
    return (programs, event_listener)

def load_critical_services(critical_services_file):
    """Load critical services from file with format 'program:<service_name>'"""
    critical_services = set()
    if os.path.exists(critical_services_file):
        with open(critical_services_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('program:'):
                    service_name = line.split(':', 1)[1]
                    critical_services.add(service_name)
    return critical_services

def is_critical_service(service_name, critical_services):
    """Check if a service name is in the critical services set"""
    return service_name in critical_services

def write_event_service(container_name, output_dir):
    event_dir = os.path.join(output_dir, EVENT)
    os.makedirs(event_dir, exist_ok=True)

    # Write the run script
    run_path = os.path.join(event_dir, 'run')

    with open(run_path, 'w') as f:
        f.write(f'#!/bin/bash\n\n')
        f.write(f'# Open the fifo for both reading and writing.\n')
        f.write(f'# This prevents blocking and prevents EOF for the reader.\n')
        f.write(f'exec 999<> {EVENT_FIFO}\n')
        f.write(f'# Redirect stdin from the fifo\n')
        f.write(f'exec <{EVENT_FIFO}\n')
        f.write(f'# Tell the exit-listener it runs under s6 supervision: it then\n')
        f.write(f'# drains event bursts from the FIFO and stops the container\n')
        f.write(f'# through PID 1 (s6-svscan). Without this variable the listener\n')
        f.write(f'# behaves exactly as under supervisord.\n')
        f.write(f'export S6_SUPERVISED=1\n')
        # Prefer the compiled Rust exit-listener over the Python one: it is a
        # drop-in (same --container-name arg, reads the same event stream
        # from stdin) at a fraction of the resident memory of a Python
        # interpreter per container. Fall back to the Python script if the
        # Rust binary is not present in this container.
        f.write(f'if [ -x /usr/bin/supervisor-proc-exit-listener-rs ]; then\n')
        f.write(f'    exec /usr/bin/supervisor-proc-exit-listener-rs --container-name {container_name}\n')
        f.write(f'fi\n')
        f.write(f'exec /usr/local/bin/supervisor-proc-exit-listener --container-name {container_name}\n')

    os.chmod(run_path, 0o755)
    with open(os.path.join(event_dir, 'type'), 'w') as f:
        f.write('longrun\n')

    open(os.path.join(output_dir + '/user/contents.d', EVENT), 'a').close()

def write_log_service(logger_name, output_dir):
    logger_dir = os.path.join(output_dir, logger_name)
    os.makedirs(logger_dir, exist_ok=True)

    # Write the run script
    run_path = os.path.join(logger_dir, 'run')
    with open(run_path, 'w') as f:
        f.write('#!/bin/sh\n\nexec logger \n')
    os.chmod(run_path, 0o755)
    with open(os.path.join(logger_dir, 'type'), 'w') as f:
        f.write('longrun\n')
    with open(os.path.join(logger_dir, 'pipeline-name'), 'w') as f:
        f.write(LOGGER + '-pipeline\n')
    open(os.path.join(logger_dir, 'consumer-for'), 'w').close()

    open(os.path.join(output_dir + '/user/contents.d', logger_name + '-pipeline'), 'a').close()

def write_log_consumers(logger_name, output_dir):
    """Add a log consumer for the given service"""
    logger_dir = os.path.join(output_dir, logger_name)
    os.makedirs(logger_dir, exist_ok=True)

    with open(os.path.join(logger_dir, 'consumer-for'), 'a') as f:
        for service_name in LONGRUN_SERVICES:
            f.write(service_name + '\n')
            # Write the log script
            service_dir = os.path.join(output_dir, service_name)
            with open(os.path.join(service_dir, 'producer-for'), 'w') as ff:
                ff.write(LOGGER + '\n')
            with open(os.path.join(service_dir, 'type'), 'w') as ff:
                ff.write('longrun\n')

def write_s6_service(service_name, options, output_dir, is_critical=False, event_listener=False, oneshot=False):
    service_dir = os.path.join(output_dir, service_name)
    os.makedirs(service_dir, exist_ok=True)

    # Write the run script
    run_path = os.path.join(service_dir, 'run')
    command = options.get('command')
    if not command:
        print(f"Warning: No command found for {service_name}, skipping.")
        return

    event_payload=f"processname:{service_name} groupname:{service_name} from_state:STARTING"
    event_header=f"ver:3.0 server:supervisor eventname:PROCESS_STATE_RUNNING len:{len(event_payload)}"

    if oneshot:
        # A run-to-completion task that other programs wait for with
        # dependent_startup_wait_for=<name>:exited. As an s6-rc ONESHOT its
        # `up` script runs to completion BEFORE dependents start - which is
        # the supervisord ":exited" contract. As a longrun (the old
        # behaviour) s6-rc considers the service up the moment the process
        # is spawned, so dependents raced the script: e.g. snmpd read
        # snmpd.conf before start.sh had rendered it.
        # The `up` file is parsed by execline, which does NOT understand
        # single quotes (sh -c '...' gets split on whitespace and the task
        # fails with "Unterminated quoted string", taking every dependent
        # service down with it). Keep `up` to a single unquoted word: the
        # absolute path of a real shell script that carries the command.
        script_path = os.path.join(service_dir, 'up.sh')
        with open(script_path, 'w') as f:
            f.write('#!/command/with-contenv sh\n\n')
            f.write('exec 2>&1\n')
            f.write('exec ' + command + '\n')
        os.chmod(script_path, 0o755)
        with open(os.path.join(service_dir, 'up'), 'w') as f:
            f.write(script_path + '\n')
        with open(os.path.join(service_dir, 'type'), 'w') as f:
            f.write('oneshot\n')
    else:
        with open(run_path, 'w') as f:
            f.write('#!/command/with-contenv sh\n\n')
            if event_listener:
                f.write(f'MSG="{event_header}\n{event_payload}"\n')
                f.write(f'printf "%s" "$MSG" > {EVENT_FIFO}\n')
            f.write('exec 2>&1\n')
            f.write('exec ' + command + '\n')
        os.chmod(run_path, 0o755)

        # Persistent daemons stay longrun; autorestart only controls the
        # restart-on-exit policy for them.
        with open(os.path.join(service_dir, 'type'), 'w') as f:
            f.write('longrun\n')

        LONGRUN_SERVICES.append(service_name)

    # Write dependency file
    dependency = options.get('dependent_startup_wait_for')
    if dependency:
        dep_dir = os.path.join(service_dir, 'dependencies.d')
        os.makedirs(dep_dir, exist_ok=True)
        (dep_service_name, dep_service_options) = dependency.split(':', 1)
        open(os.path.join(dep_dir, dep_service_name), 'a').close()

    # Write finish script.
    # For non-critical services with autorestart=false: call s6-svc -D to keep
    # the service down after it exits (mirrors supervisord autorestart=false).
    # For critical or autorestart!=false: omit s6-svc -D so s6 auto-restarts.
    autorestart = options.get('autorestart', 'false')
    should_restart = is_critical or (autorestart and autorestart != 'false')

    if oneshot:
        # oneshots have no supervised process: no finish script, no
        # auto-restart semantics. Just enable it and return.
        open(os.path.join(output_dir + '/user/contents.d', service_name), 'a').close()
        return

    finish_path = os.path.join(service_dir, 'finish')
    event_payload=f"processname:{service_name} groupname:{service_name} from_state:RUNNING expected:"
    event_header=f"ver:3.0 server:supervisor eventname:PROCESS_STATE_EXITED len:{len(event_payload)+1}"
    with open(finish_path, 'w') as f:
        f.write('#!/command/with-contenv sh\n\n')
        f.write('if [ "$1" -eq 0 ]; then\n')
        f.write(f'  MSG="{event_header}\n{event_payload}1"\n')
        f.write('elif [ "$1" -eq 256 ]; then\n')
        f.write('  e=$((128 + $2))\n')
        f.write(f'  MSG="{event_header}\n{event_payload}0"\n')
        f.write('else\n')
        f.write('  e="$1"\n')
        f.write(f'  MSG="{event_header}\n{event_payload}0"\n')
        f.write('fi\n\n')
        if event_listener:
            f.write(f'printf "%s" "$MSG" > {EVENT_FIFO}\n')
        if not should_restart:
            # Keep the service down after exit (no auto-restart)
            f.write(f'/command/s6-svc -D -T 0 -- /run/s6-rc/servicedirs/{service_name}\n')
        f.write('exit 0\n')
    os.chmod(finish_path, 0o755)

    # enable service
    open(os.path.join(output_dir + '/user/contents.d', service_name), 'a').close()

def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <supervisor_configs_dir> <output_dir> <critical_services_file>")
        sys.exit(1)

    configs_dir = sys.argv[1]
    output_dir = sys.argv[2]
    critical_services_file = sys.argv[3]

    if not os.path.isdir(configs_dir):
        print(f"Error: {configs_dir} is not a directory")
        sys.exit(1)

    # Load critical services
    critical_services = load_critical_services(critical_services_file)
    print(f"Loaded {len(critical_services)} critical services from {critical_services_file}")

    # Find all .conf files in the directory
    conf_files = glob.glob(os.path.join(configs_dir, "*.conf"))

    if not conf_files:
        print(f"Warning: No .conf files found in {configs_dir}")
        sys.exit(0)

    total_programs = 0
    critical_count = 0

    service_dir = os.path.join(output_dir, 'user')
    os.makedirs(service_dir + '/contents.d', exist_ok=True)
    with open(os.path.join(service_dir, 'type'), 'w') as f:
        f.write('bundle\n')

    service_dir = os.path.join(output_dir, 'user2')
    os.makedirs(service_dir + '/contents.d', exist_ok=True)
    with open(os.path.join(service_dir, 'type'), 'w') as f:
        f.write('bundle\n')

    write_log_service (LOGGER, output_dir)
    container_name = os.environ.get("CONTAINER_NAME", "unknown")

    # First pass: a program that anyone waits for with
    # dependent_startup_wait_for=<name>:exited is a run-to-completion task
    # and must become an s6-rc oneshot, or dependents race it.
    parsed = []
    oneshot_names = set()
    for conf_file in conf_files:
        print(f"Processing {conf_file}...")
        (programs, event_listener) = parse_supervisor_conf(conf_file)
        parsed.append((conf_file, programs, event_listener))
        for name, options in programs.items():
            dependency = options.get('dependent_startup_wait_for')
            if dependency and ':' in dependency:
                dep_name, dep_state = dependency.split(':', 1)
                if dep_state.strip().startswith('exited'):
                    oneshot_names.add(dep_name.strip())
    if oneshot_names:
        print(f"  Oneshot (\":exited\"-waited) tasks: {sorted(oneshot_names)}")

    for conf_file, programs, event_listener in parsed:
        if event_listener:
            print(f"  Found event listener 'supervisor-proc-exit-listener'")
            write_event_service(container_name, output_dir)
        for name, options in programs.items():
            is_critical = is_critical_service(name, critical_services)
            if is_critical:
                critical_count += 1
                print(f"  {name} is marked as critical")
            write_s6_service(name, options, output_dir, is_critical,
                             event_listener, oneshot=(name in oneshot_names))
        total_programs += len(programs)
        print(f"  Found {len(programs)} programs")

    write_log_consumers(LOGGER, output_dir)
    print(f"Converted {total_programs} supervisor programs ({critical_count} critical) from {len(conf_files)} config files to s6 services in {output_dir}")

if __name__ == '__main__':
    main()
