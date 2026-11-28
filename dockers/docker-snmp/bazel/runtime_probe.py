#!/usr/bin/env python3
"""Exercise installed SNMP interfaces and real AgentX queries without source paths."""
import asyncio
import grp
import importlib
import importlib.metadata
import json
import os
from pathlib import Path
import pwd
import socket
import subprocess
import sys
import tempfile
import time

import redis

sys.path.insert(0, str(Path(__file__).parent))
import runtime_smoke


def require(value, message):
    if not value:
        raise AssertionError(message)


def command(*args, env=None):
    result = subprocess.run(args, env=env, capture_output=True, text=True, timeout=30)
    require(result.returncode == 0, result.stdout + result.stderr)
    return result.stdout + result.stderr


def main():
    require(sys.flags.optimize == 1, 'exercise the production PYTHONOPTIMIZE setting')
    require(importlib.metadata.version('asyncsnmp') == '2.1.0', 'wrong SNMP wheel')
    require(importlib.metadata.version('sonic-platform-common') == '1.0', 'wrong platform wheel')
    expected_versions = json.loads(sys.argv[2])
    versions = {name: importlib.metadata.version(name) for name in expected_versions}
    require(versions == expected_versions, f'installed Python distributions differ from Make: {versions} != {expected_versions}')
    for name in ['sonic_ax_impl', 'ax_interface', 'sonic_platform_base', 'sonic_py_common',
                 'swsscommon.swsscommon', 'psutil', 'python_arptable', 'hiredis', 'yaml', 'smbus']:
        module = importlib.import_module(name)
        require(str(Path(module.__file__).resolve()).startswith(('/usr/lib/', '/usr/local/lib/')), name)
    user = pwd.getpwnam('Debian-snmp')
    require((user.pw_uid, user.pw_gid, user.pw_dir, user.pw_shell) == (101,102,'/var/lib/snmp','/bin/false'), 'snmpd account differs')
    require(grp.getgrnam('Debian-snmp').gr_gid == 102, 'snmpd group differs')
    require('update_frequency' in command('/usr/bin/python3','-m','sonic_ax_impl','--help'), 'SNMP entrypoint is absent')
    for path in ('/usr/sbin/snmpd', '/usr/bin/snmpget', '/usr/bin/snmpgetnext', '/usr/bin/ipmitool'):
        output = command('/lib64/ld-linux-x86-64.so.2','--list',path)
        require('not found' not in output, 'missing runtime library: '+path)
    debug_checks = []
    if sys.argv[1] == 'docker-snmp-dbg':
        # Both executables come from source owners and carry split native symbols.
        evidence = json.loads(Path('/probe/validation.json').read_text())
        expected = {'usr/bin/ssh', 'usr/bin/sonic-db-cli'}
        selected = [item for item in evidence['source_debug_pairs'] if item['path'] in expected]
        require({item['path'] for item in selected} == expected, 'missing source debug pairs')
        debug_checks = [runtime_smoke.debug_pair(item) for item in selected]
        for item in debug_checks:
            item['with_symbols']['source'] = Path(item['with_symbols']['source']).name
    processes, logs = [], []
    with tempfile.TemporaryDirectory(prefix='snmp-image-') as temporary:
        work = Path(temporary)
        env = dict(os.environ, MIBS='', SNMP_PERSISTENT_DIR=str(work/'persist'), PLATFORM='sonic-bazel-build')
        (work/'persist').mkdir()
        data={'DEVICE_METADATA':{'localhost':{'hostname':'bazel-snmp-image','hwsku':'Force10-S6000','switch_type':'switch'}},
              'build_version':'bazel-snmp-ci','debian_version':'13','kernel_version':'test',
              'SNMP_COMMUNITY':{'bazel-test':{'TYPE':'RO'}}}
        render = lambda name: command('sonic-cfggen','-a',json.dumps(data),'-t','/usr/share/sonic/templates/'+name,env=env)
        require('SONiC.bazel-snmp-ci' in render('sysDescription.j2'), 'sysDescription render failed')
        require('rocommunity bazel-test' in render('snmpd.conf.j2'), 'SNMP community render failed')
        ordinary=render('supervisord.conf.j2')
        require('command=/usr/bin/env python3 -m sonic_ax_impl\n' in ordinary, 'normal supervisor command differs')
        data['DEVICE_METADATA']['localhost']['switch_type']='chassis-packet'
        require('--enable_dynamic_frequency' in render('supervisord.conf.j2'), 'chassis supervisor command differs')
        def start(name,args):
            log=(work/(name+'.log')).open('w+');logs.append(log)
            process=subprocess.Popen(args,cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT)
            processes.append(process);return process
        def details():
            return '\n'.join(p.name+':\n'+p.read_text() for p in work.glob('*.log'))
        try:
            with socket.socket() as reserve:
                reserve.bind(('127.0.0.1',0));redis_port=reserve.getsockname()[1]
            redis_socket=work/'redis.sock'
            # The test-only imported closure is mounted outside the image. Its
            # matching loader prevents host/container glibc mixing.
            fixture=Path('/probe/redis')
            start('redis',[str(fixture/'usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2'),
                  '--library-path',str(fixture/'usr/lib/x86_64-linux-gnu'),str(fixture/'usr/bin/redis-server'),
                  '--bind','127.0.0.1','--port',str(redis_port),'--unixsocket',str(redis_socket),
                  '--save','','--appendonly','no'])
            client=redis.Redis(unix_socket_path=str(redis_socket),db=4,socket_timeout=1)
            deadline=time.monotonic()+15
            while True:
                try:
                    if client.ping():break
                except redis.RedisError:
                    require(time.monotonic()<deadline,details());time.sleep(.1)
            client.hset('DEVICE_METADATA|localhost',mapping={'hostname':'bazel-snmp-image'})
            database=json.loads(Path('/run/redis/sonic-db/database_config.json').read_text())
            database['INSTANCES']['redis'].update(port=redis_port,unix_socket_path=str(redis_socket))
            database_file=work/'database_config.json';database_file.write_text(json.dumps(database))
            with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as reserve:
                reserve.bind(('127.0.0.1',0));port=reserve.getsockname()[1]
            agent_socket=work/'agentx.sock';config=work/'snmpd.conf'
            config.write_text('master agentx\nagentxsocket '+str(agent_socket)+'\nrocommunity bazel-test 127.0.0.1\n')
            start('snmpd',['/usr/sbin/snmpd','-f','-Lo','-C','-c',str(config),'-p',str(work/'snmpd.pid'),
                          '-I','-system_mib','udp:127.0.0.1:'+str(port)])
            launcher='''import asyncio,sys
from swsscommon import swsscommon
swsscommon.SonicDBConfig.load_sonic_db_config(sys.argv[1])
from sonic_ax_impl import mibs
mibs.Namespace.db_config_loaded=True
mibs.config(unix_socket_path=sys.argv[2])
from ax_interface import constants,Agent
constants.AGENTX_SOCKET_PATH=sys.argv[3]
constants.SNMPD_CONFIG_FILE=sys.argv[4]
from sonic_ax_impl.mibs.ietf.rfc1213 import SysNameMIB
loop=asyncio.new_event_loop()
asyncio.set_event_loop(loop)
loop.run_until_complete(Agent(SysNameMIB,False,5,loop).run_in_event_loop())
'''
            agent=start('agent',['/usr/bin/python3','-c',launcher,str(database_file),str(redis_socket),str(agent_socket),str(config)])
            results=[]
            for program,oid in [('snmpget','.1.3.6.1.2.1.1.5.0'),('snmpgetnext','.1.3.6.1.2.1.1.5')]:
                deadline=time.monotonic()+30
                while True:
                    result=subprocess.run(['/usr/bin/'+program,'-v2c','-c','bazel-test','-On','-t','1','-r','0','127.0.0.1:'+str(port),oid],env=env,capture_output=True,text=True,timeout=5)
                    if result.returncode==0 and 'bazel-snmp-image' in result.stdout:
                        require('.1.3.6.1.2.1.1.5.0' in result.stdout,result.stdout)
                        results.append({'program':program,'response':result.stdout.strip()});break
                    require(time.monotonic()<deadline and agent.poll() is None,result.stdout+result.stderr+details());time.sleep(.25)
            print(json.dumps({'status':'passed','queries':results,'debug_checks':debug_checks,'python_versions':versions,'templates':['sysDescription','snmpd','supervisord ordinary/chassis'],
                              'scope':'Installed image SysNameMIB via private Redis/Common/AgentX/snmpd; hardware and multi-namespace are outside this fixture'}))
        finally:
            for process in reversed(processes):
                if process.poll() is None:process.terminate()
                try:process.wait(timeout=5)
                except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
            for log in logs:log.close()
            if 'client' in locals():client.close()

if __name__=='__main__':main()
