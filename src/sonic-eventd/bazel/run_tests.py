#!/usr/bin/env python3
"""Run the original C++ suite with private Redis and writable test fixtures."""
import json
import os
from pathlib import Path, PurePosixPath
import posixpath
import shutil
import socket
import subprocess
import sys
import tarfile
import tempfile
import time
import xml.etree.ElementTree as ET


def locate(value):
    from python.runfiles import runfiles

    path = Path(value)
    if path.exists():
        return path.absolute()
    result = runfiles.Create().Rlocation(value) or runfiles.Create().Rlocation('_main/' + value)
    if not result or not Path(result).exists():
        raise ValueError('Missing declared test input: ' + value)
    return Path(result).absolute()


def member_filter(member, destination):
    if member.linkname.startswith('/') and (member.issym() or member.islnk()):
        target = posixpath.normpath('/' + member.linkname.lstrip('/')).lstrip('/')
        if member.issym():
            target = posixpath.relpath(target, str(PurePosixPath(member.name).parent))
        member = member.replace(linkname=target)
    return tarfile.data_filter(member, destination)


def unused_port():
    with socket.socket() as listener:
        listener.bind(('127.0.0.1', 0))
        return listener.getsockname()[1]


def stop_redis(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def start_redis(redis, root, unix_socket, environment, attempts=5):
    """Start our server, retrying only a lost TCP port allocation race."""
    for attempt in range(attempts):
        port = unused_port()
        log = (root / ('redis-' + str(port) + '-' + str(attempt) + '.log')).open('w+')
        process = None
        ready = False
        try:
            process = subprocess.Popen([str(redis), '--bind', '127.0.0.1', '--port', str(port),
                '--unixsocket', unix_socket, '--save', '', '--appendonly', 'no',
                '--databases', '32', '--dir', str(root)], env=environment,
                stdout=log, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 10
            while process.poll() is None and time.monotonic() < deadline:
                try:
                    with socket.create_connection(('127.0.0.1', port), timeout=0.2) as client:
                        client.sendall(b'*2\r\n$4\r\nINFO\r\n$6\r\nserver\r\n')
                        with client.makefile('rb') as stream:
                            header = stream.readline(64)
                            if not header.startswith(b'$'):
                                raise ValueError('Redis did not return its server identity')
                            length = int(header[1:].strip())
                            if not 0 < length < 65536:
                                raise ValueError('Unexpected Redis server response size')
                            identity = stream.read(length).decode()
                        ready = ('process_id:' + str(process.pid) + '\r\n') in identity and process.poll() is None
                    if ready:
                        return process, log, port
                except (OSError, ValueError):
                    pass
                time.sleep(0.05)
            log.flush()
            log.seek(0)
            diagnostic = log.read()
            tcp_collision = '127.0.0.1:' + str(port) + ': bind: Address already in use'
            collision = process.poll() is not None and tcp_collision in diagnostic
            if not collision or attempt + 1 == attempts:
                raise ValueError('Private Redis startup failed: ' + diagnostic)
        finally:
            if not ready:
                if process is not None:
                    stop_redis(process)
                log.close()
    raise ValueError('Private Redis startup requires at least one attempt')


def main():
    binary, archive, fixture = map(locate, sys.argv[1:])
    sources = fixture.parent.parent
    expected = {'eventd_tests_bin': 6, 'eventdb_tests_bin': 4, 'rsyslog_plugin_tests_bin': 15}[binary.name]
    with tempfile.TemporaryDirectory(dir='/tmp', prefix='eventd-') as temp:
        root = Path(temp); runtime = root/'runtime'; work = root/'work'
        runtime.mkdir(); work.mkdir()
        with tarfile.open(archive) as package:
            package.extractall(runtime, filter=member_filter)
        for directory in ['tests', 'rsyslog_plugin_tests']:
            for source in (sources/directory).rglob('*'):
                if source.is_file() and source.suffix in {'.json', '.txt'}:
                    dest = work/source.relative_to(sources)
                    dest.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(source, dest)
        libraries = sorted({str(p.parent) for p in runtime.rglob('lib*.so*') if p.is_file()})
        environment = dict(os.environ, LD_LIBRARY_PATH=':'.join(libraries))
        libyang = {str(p.resolve()) for p in runtime.rglob('libyang.so.3')}
        if len(libyang) == 1:
            environment['LD_PRELOAD'] = libyang.pop()
        redis = runtime/'usr/bin/redis-server'
        if not redis.is_file():
            raise ValueError('Declared test runtime lacks redis-server')
        endpoints = {}
        children = []
        try:
            for config in (work/'tests').rglob('*.json'):
                data = json.loads(config.read_text())
                if not isinstance(data, dict) or 'INSTANCES' not in data:
                    continue
                for instance in data['INSTANCES'].values():
                    key = instance['unix_socket_path']
                    if key not in endpoints:
                        unix_socket = str(root/('redis-' + str(len(endpoints)) + '.sock'))
                        process, log, port = start_redis(redis, root, unix_socket, environment)
                        children.append((process, log))
                        endpoints[key] = (port, unix_socket)
                    instance['hostname'] = '127.0.0.1'
                    instance['port'], instance['unix_socket_path'] = endpoints[key]
                config.write_text(json.dumps(data) + '\n')
            report = root/'gtest.xml'
            result = subprocess.run([str(binary), '--gtest_output=xml:' + str(report)], cwd=work,
                                    env=environment, text=True, capture_output=True, timeout=600)
            print(result.stdout); print(result.stderr, file=sys.stderr)
            if result.returncode:
                return result.returncode
            cases = ET.parse(report).findall('.//testcase')
            if len(cases) != expected or any(case.find('skipped') is not None for case in cases):
                raise ValueError('The original suite was incomplete or skipped cases')
            if 'redis not available' in result.stdout or 'Unable to get DB Connector' in result.stdout:
                raise ValueError('The original suite fell back because Redis was unavailable')
            fixture_rows = {}
            if binary.name == 'rsyslog_plugin_tests_bin':
                for name, count in {'onMessage': 4, 'onMessage_noParams': 3}.items():
                    matches = [case for case in cases if case.get('classname') == 'rsyslog_plugin' and case.get('name') == name]
                    if len(matches) != 1:
                        raise ValueError('Missing message fixture test: ' + name)
                    values = [item.get('value') for item in matches[0].findall('./properties/property')
                              if item.get('name') == 'fixture_rows']
                    if matches[0].get('fixture_rows') is not None:
                        values.append(matches[0].get('fixture_rows'))
                    if values != [str(count)]:
                        raise ValueError('Message fixture rows were not exercised: ' + name)
                    fixture_rows[name] = count
            output = Path(os.environ.get('TEST_UNDECLARED_OUTPUTS_DIR', str(root)))
            output.mkdir(parents=True, exist_ok=True); shutil.copyfile(report, output/'gtest.xml')
            (output/'services.json').write_text(json.dumps({'_generated': {'notice':'AUTO-GENERATED. DO NOT EDIT MANUALLY.', 'generator':'src/sonic-eventd/bazel/run_tests.py'}, 'test_cases':len(cases), 'fixture_rows':fixture_rows, 'redis_instances':len(children), 'host_services_used':False}, indent=2)+'\n')
            return 0
        finally:
            for process, log in children:
                stop_redis(process)
                log.close()


if __name__ == '__main__':
    raise SystemExit(main())
