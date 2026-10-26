#!/usr/bin/env python3
"""Probe an installed image. Input/output are JSON; no host services are used."""
import argparse
import configparser
import contextlib
import glob
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shlex
import shutil
import socket
import subprocess
import sys
import tempfile
import time


def absolute_path(name):
    path = PurePosixPath(name)
    if '..' in path.parts or '\x00' in name:
        raise ValueError('Unsafe image path: ' + name)
    return '/' + str(path).lstrip('/')


def command(argv, timeout=30):
    try:
        result = subprocess.run(argv, text=True, capture_output=True, timeout=timeout,
                                env=dict(os.environ, LC_ALL='C', DEBUGINFOD_URLS='', LD_BIND_NOW='1'))
        return {'argv': argv, 'returncode': result.returncode,
                'stdout': result.stdout[-16000:], 'stderr': result.stderr[-4000:]}
    except subprocess.TimeoutExpired:
        return {'argv': argv, 'returncode': None, 'stdout': '', 'stderr': 'timeout'}


def loader_result(result, needed):
    output = result['stdout'] + result['stderr']
    return (result['returncode'] == 0 and 'not found' not in output) or (
        not needed and result['returncode'] in (0, 1)
        and ('statically linked' in output or 'not a dynamic executable' in output))


GDB_SCRIPT = r'''python
import gdb, json, re
result = {"found": False}
try:
    requested_symbol = __SONIC_DEBUG_SYMBOL__
    candidates = [requested_symbol] if requested_symbol else []
    for line in ([] if requested_symbol else gdb.execute("info functions", to_string=True).splitlines()):
        match = re.match(r"^\s*\d+:\s+(.+);\s*$", line)
        if not match:
            continue
        name = match.group(1).split("(", 1)[0].split()[-1]
        if re.fullmatch(r"[A-Za-z_~][\w:<>~]*", name):
            candidates.append(name)
    for name in candidates[:200]:
        try:
            locations = gdb.decode_line(name)[1] or []
            for location in locations:
                if not location.symtab or location.line <= 0:
                    continue
                block = gdb.block_for_pc(location.pc)
                while block is not None and block.function is None:
                    block = block.superblock
                symbol = block.function if block is not None else None
                if symbol is None or symbol.type.code != gdb.TYPE_CODE_FUNC:
                    continue
                result = {"found": True, "function": symbol.name,
                          "source": location.symtab.filename, "line": location.line,
                          "type": str(symbol.type),
                          "parameters": [str(field.type) for field in symbol.type.fields()]}
                break
            if result["found"]:
                break
        except Exception:
            continue
except Exception as error:
    result["error"] = str(error)
print("SONIC_DEBUG_LOOKUP=" + json.dumps(result))
end
'''


def gdb_lookup(binary, directory, debug_directory, symbol=None):
    script = directory / 'lookup.gdb'
    script.write_text(GDB_SCRIPT.replace('__SONIC_DEBUG_SYMBOL__', repr(symbol)))
    result = command(['gdb', '-nx', '-nh', '--batch',
                      '-iex', 'set auto-load off', '-iex', 'set debuginfod enabled off',
                      '-iex', 'set pagination off',
                      '-iex', 'set debug-file-directory ' + debug_directory,
                      '-ex', 'file ' + json.dumps(str(binary)), '-x', str(script)], timeout=300)
    records = [line.removeprefix('SONIC_DEBUG_LOOKUP=') for line in result['stdout'].splitlines()
               if line.startswith('SONIC_DEBUG_LOOKUP=')]
    if result['returncode'] != 0 or len(records) != 1:
        raise ValueError('GDB did not return one successful lookup result: ' + json.dumps(result))
    return json.loads(records[0])


def debug_pair(record):
    binary, symbols = Path(absolute_path(record['path'])), Path(absolute_path(record['debug_path']))
    if not binary.is_file() or not symbols.is_file():
        raise ValueError('Missing installed runtime/symbol pair: ' + str(binary))
    forwarding = record.get('debug_forwarding')
    implementation = Path(absolute_path(forwarding['provider'])) if forwarding else binary
    requested_symbol = forwarding['symbol'] if forwarding else None
    if forwarding and (not implementation.is_file() or
                       not Path(absolute_path(forwarding['debug_path'])).is_file()):
        raise ValueError('Missing installed forwarding implementation/symbols: ' + str(implementation))
    with tempfile.TemporaryDirectory(prefix='sonic-gdb-') as name:
        directory = Path(name)
        isolated = directory / implementation.name
        shutil.copyfile(implementation, isolated)
        empty = directory / 'empty'
        empty.mkdir()
        without = gdb_lookup(isolated, directory, str(empty), requested_symbol)
        if without.get('found'):
            raise ValueError('Debug lookup succeeded with packaged symbols unavailable: ' + str(binary))
        with_symbols = gdb_lookup(implementation, directory, '/usr/lib/debug', requested_symbol)
        if not with_symbols.get('found'):
            raise ValueError('Packaged symbols do not provide source/line/type lookup: ' + str(binary))
        if forwarding and (with_symbols.get('function') != requested_symbol or
                           PurePosixPath(with_symbols.get('source', '')).name != forwarding['source']):
            raise ValueError('Forwarded function does not resolve its declared source: ' + str(binary))
        return {'path': str(binary), 'debug_path': str(symbols), 'without_symbols': without,
                'debug_forwarding': forwarding,
                'with_symbols': with_symbols, 'scope': 'Source filename, line and type metadata; source text is not supplied.'}


def service_checks(policy):
    result = {}
    profile = policy.get('startup_profile')
    if profile:
        source, target = Path(absolute_path(profile['input'])), Path(absolute_path(profile['output']))
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_text(profile['contents'])
        run = command([absolute_path(profile['script'])])
        if run['returncode'] != 0 or not target.is_file() or target.read_text() != profile['contents']:
            raise ValueError('Installed startup script did not produce its expected SAI profile: ' + json.dumps(run))
        result['startup_profile'] = {'script': profile['script'], 'output': str(target), 'matched': True}
    config = policy.get('supervisor_config')
    if config:
        parser = configparser.ConfigParser(interpolation=None)
        if not parser.read(absolute_path(config)):
            raise ValueError('Missing installed supervisor configuration')
        programs = {}
        for section in parser.sections():
            if not section.startswith(('program:', 'eventlistener:')):
                continue
            argv = shlex.split(parser.get(section, 'command'))
            executable = shutil.which(argv[0])
            if not executable or not os.access(executable, os.X_OK):
                raise ValueError('Supervisor command is unavailable: ' + section)
            programs[section] = argv
        if not programs:
            raise ValueError('Supervisor configuration has no programs')
        result['supervisor_commands'] = programs
    return result


P4_PROGRAM = """#include <core.p4>
#include <v1model.p4>
struct headers { }
struct metadata { }
parser ParserImpl(packet_in p, out headers h, inout metadata m, inout standard_metadata_t s) {
    state start { transition accept; }
}
control VerifyChecksumImpl(inout headers h, inout metadata m) { apply { } }
control IngressImpl(inout headers h, inout metadata m, inout standard_metadata_t s) {
    apply { mark_to_drop(s); }
}
control EgressImpl(inout headers h, inout metadata m, inout standard_metadata_t s) { apply { } }
control ComputeChecksumImpl(inout headers h, inout metadata m) { apply { } }
control DeparserImpl(packet_out p, in headers h) { apply { } }
V1Switch(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(),
         ComputeChecksumImpl(), DeparserImpl()) main;
"""


def p4_service_check(config):
    # Compile only a small data-plane test program with the installed compiler.
    # This does not build PI, BMv2, p4c or the DASH SAI producer.
    output_mode = config.get('compiler_output_mode', 'file')
    if output_mode not in ('file', 'directory'):
        raise ValueError('Unknown P4 compiler output mode: ' + str(output_mode))
    with tempfile.TemporaryDirectory(prefix='sonic-p4-smoke-') as name:
        directory = Path(name)
        source, output = directory / 'drop.p4', directory / 'drop.json'
        source.write_text(P4_PROGRAM)
        compiler = command([config['compiler'], *config.get('compiler_args', []),
                            '--arch', 'v1model', '--std', 'p4-16',
                            str(source), '-o', str(directory if output_mode == 'directory' else output)], timeout=60)
        if compiler['returncode'] != 0 or not output.is_file():
            raise ValueError('Installed P4 compiler failed: ' + json.dumps(compiler))
        program = json.loads(output.read_text())
        if not program.get('pipelines'):
            raise ValueError('P4 compiler produced no pipeline')
        ports = []
        for unused in range(2):
            with socket.socket() as listener:
                listener.bind(('127.0.0.1', 0))
                ports.append(listener.getsockname()[1])
        argv = [config['switch'], '--device-id', '0', '--thrift-port', str(ports[0]),
                '--log-console', str(output), '--', '--grpc-server-addr', '127.0.0.1:' + str(ports[1])]
        ready = False
        with (directory / 'switch.log').open('w+') as log:
            process = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT,
                                       env=dict(os.environ, LC_ALL='C', LD_BIND_NOW='1'))
            try:
                deadline = time.monotonic() + 15
                while process.poll() is None and time.monotonic() < deadline:
                    try:
                        with socket.create_connection(('127.0.0.1', ports[1]), timeout=0.2):
                            ready = True
                        break
                    except OSError:
                        time.sleep(0.1)
                if not ready:
                    log.flush(); log.seek(0)
                    raise ValueError('Installed BMv2/PI gRPC service did not start: ' + log.read()[-4000:])
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill(); process.wait()
            log.flush(); log.seek(0)
            diagnostics = log.read()[-4000:]
        return {'compiler': config['compiler'], 'compiler_args': config.get('compiler_args', []),
                'compiler_output_mode': output_mode,
                'switch': config['switch'],
                'program_sha256': hashlib.sha256(P4_PROGRAM.encode()).hexdigest(),
                'compiled_json_sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
                'grpc_listener_ready': ready, 'log': diagnostics,
                'scope': 'Installed compiler plus local BMv2/PI gRPC listener; no packets or remote services.'}


@contextlib.contextmanager
def redis_fixture(config):
    """Run only the supplied test helper, inside this isolated container."""
    if not config:
        yield None
        return
    executable = Path(config['executable'])
    if hashlib.sha256(executable.read_bytes()).hexdigest() != config['sha256']:
        raise ValueError('Redis test helper differs from its recorded bytes')
    version = command([str(executable), '--version'])
    if version['returncode'] != 0 or 'Redis server v=' not in version['stdout']:
        raise ValueError('Redis test helper cannot execute: ' + json.dumps(version))
    # The fresh anonymous volume contains the shipped configuration. It is
    # writable only by this container and removed by docker run --rm.
    database = Path(config['database_config'])
    data = json.loads(database.read_text())
    socket_path = data['INSTANCES']['redis']['unix_socket_path']
    if Path(socket_path).resolve() != Path('/run/redis/redis.sock'):
        raise ValueError('Unexpected Redis socket path in installed database config')
    with tempfile.TemporaryDirectory(prefix='sonic-redis-smoke-') as name:
        directory = Path(name)
        argv = [str(executable), '--port', '0', '--save', '', '--appendonly', 'no',
                '--unixsocket', socket_path, '--unixsocketperm', '700',
                '--dir', str(directory), '--databases', '32']
        with (directory / 'redis.log').open('w+') as log:
            process = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT)
            try:
                ready = False
                deadline = time.monotonic() + 10
                while process.poll() is None and time.monotonic() < deadline:
                    try:
                        with socket.socket(socket.AF_UNIX) as client:
                            client.settimeout(0.2)
                            client.connect(socket_path)
                            client.sendall(b'*1\r\n$4\r\nPING\r\n')
                            ready = client.recv(64) == b'+PONG\r\n'
                        if ready:
                            break
                    except OSError:
                        time.sleep(0.05)
                if not ready:
                    log.flush(); log.seek(0)
                    raise ValueError('Isolated Redis did not become ready: ' + log.read()[-4000:])
                yield {'version': version['stdout'].strip(), 'sha256': config['sha256'],
                       'database_config_sha256': hashlib.sha256(database.read_bytes()).hexdigest(),
                       'unix_socket': socket_path, 'tcp_disabled': True,
                       'scope': 'Explicit test helper, not an installed image component.'}
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill(); process.wait()


def probe(inputs, variant):
    with redis_fixture(inputs.get('redis_fixture')) as fixture:
        result = probe_installed(inputs, variant)
        result['redis_fixture'] = fixture
        return result


def probe_installed(inputs, variant):
    expected = {'amd64': 'x86_64', 'arm64': 'aarch64'}[inputs['architecture']]
    if platform.machine() != expected:
        raise ValueError('Native machine does not match image architecture')
    result = {'schema': 1, 'variant': variant, 'machine': platform.machine(),
              'loader_checks': [], 'library_loads': [], 'commands': [], 'python_imports': [],
              'debug_pairs': [], 'failures': []}
    for record in inputs['elfs']:
        path = absolute_path(record['path'])
        run = command(['ldd', path])
        run['path'] = path
        run['passed'] = loader_result(run, record.get('needed', []))
        result['loader_checks'].append(run)
        if not run['passed']:
            result['failures'].append('loader: ' + path)
    policy = inputs['policy']
    for group in policy.get('library_groups', []):
        paths = list(group.get('paths', []))
        if group.get('glob'):
            found = sorted(glob.glob(group['glob']))
            if not found:
                result['failures'].append('empty library group: ' + group['name'])
            paths += found
        libraries = [*group.get('preload', []), *paths]
        code = ('import ctypes,json,os,sys; loaded=[]; '
                '[loaded.append((path,ctypes.CDLL(path,mode=os.RTLD_NOW|os.RTLD_GLOBAL))) for path in sys.argv[1:]]; '
                'print(json.dumps([path for path,library in loaded]))')
        run = command([sys.executable, '-c', code, *libraries])
        run.update(name=group['name'], passed=run['returncode'] == 0)
        result['library_loads'].append(run)
        if not run['passed']:
            result['failures'].append('library group: ' + group['name'])
    for spec in policy.get('commands', []):
        run = command(spec['argv'], timeout=spec.get('timeout', 30))
        run['passed'] = run['returncode'] in spec.get('exit_codes', [0]) and bool(
            re.search(spec.get('output_pattern', r'.'), run['stdout'] + run['stderr']))
        result['commands'].append(run)
        if not run['passed']:
            result['failures'].append('command: ' + repr(spec['argv']))
    for module in policy.get('python_imports', []):
        run = command([sys.executable, '-c', 'import importlib,sys; importlib.import_module(sys.argv[1]); print("imported",sys.argv[1])', module])
        run['passed'] = run['returncode'] == 0
        result['python_imports'].append(run)
        if not run['passed']:
            result['failures'].append('Python import: ' + module)
    try:
        result['services'] = service_checks(policy)
    except (OSError, ValueError, configparser.Error) as error:
        result['failures'].append('services: ' + str(error))
    if policy.get('p4_service'):
        try:
            result['p4_service'] = p4_service_check(policy['p4_service'])
        except (OSError, ValueError) as error:
            result['failures'].append('P4 service: ' + str(error))
    if variant == 'debug':
        for record in inputs['debug_pairs']:
            try:
                result['debug_pairs'].append(debug_pair(record))
            except (OSError, ValueError) as error:
                result['failures'].append('GDB: ' + str(error))
    result['passed'] = not result['failures']
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--variant', choices=['runtime', 'debug'], required=True)
    args = parser.parse_args()
    result = probe(json.load(sys.stdin), args.variant)
    print(json.dumps(result, sort_keys=True))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
