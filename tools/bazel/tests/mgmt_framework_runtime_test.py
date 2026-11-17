import copy
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import time
import types
import unittest
from unittest import mock

W=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(W))

def load(name,path):
 spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module

# Dependency-free guards exercise real fixture helpers; installed Redis,
# Supervisor and REST execution are deliberately left to image acceptance.
transport=types.ModuleType('supervisor.xmlrpc');transport.SupervisorTransport=object
with mock.patch.dict(sys.modules, {'redis':types.ModuleType('redis'),'supervisor':types.ModuleType('supervisor'),'supervisor.xmlrpc':transport}):
 probe=load('framework_probe',W/'dockers/docker-sonic-mgmt-framework/bazel/runtime_probe.py')
loaded=load('framework_loaded',W/'tools/bazel/ci/mgmt_framework_loaded.py')

class Endpoint:
 def __init__(self,pid,ping=True): self.pid,self.responds=pid,ping
 def ping(self):return self.responds
 def info(self,section):assert section=='server';return {'process_id':self.pid}

class StartupTest(unittest.TestCase):
 def records(self):
  return {'start':{'statename':'EXITED','exitstatus':0,'pid':0},
          'dependent-startup':{'statename':'RUNNING','exitstatus':0,'pid':11},
          'rsyslogd':{'statename':'RUNNING','exitstatus':0,'pid':12},
          'rest-server':{'statename':'RUNNING','exitstatus':0,'pid':13},
          'supervisor-proc-exit-listener':{'statename':'RUNNING','exitstatus':0,'pid':14}}
 def test_original_listener_can_complete_successfully(self):
  r=self.records();self.assertTrue(probe.services_ready(r,set(r)))
  r['dependent-startup'].update(statename='EXITED',pid=0);self.assertTrue(probe.services_ready(r,set(r)))
 def test_failed_oneshots_or_missing_or_dead_persistent_service_rejected(self):
  original=self.records()
  for name,changes in [('start',{'exitstatus':1}),('dependent-startup',{'statename':'EXITED','exitstatus':3}),('rsyslogd',{'statename':'EXITED'}),('rest-server',{'pid':0})]:
   with self.subTest(name=name):
    r=copy.deepcopy(original);r[name].update(changes);self.assertFalse(probe.services_ready(r,set(original)))
  r=copy.deepcopy(original);r.pop('supervisor-proc-exit-listener');self.assertFalse(probe.services_ready(r,set(original)))
 def test_both_redis_endpoints_must_name_actual_child(self):
  self.assertEqual(probe.private_redis_identity(Endpoint(321),Endpoint(321),321),{'unix':321,'tcp':321})
  for unix,tcp in [(Endpoint(99),Endpoint(321)),(Endpoint(321),Endpoint(99)),(Endpoint(321,False),Endpoint(321))]:
   with self.assertRaises(AssertionError):probe.private_redis_identity(unix,tcp,321)
 def test_real_process_executable_argv_and_ancestry(self):
  p=subprocess.Popen(['/usr/bin/sleep','60'])
  try:
   deadline=time.monotonic()+5
   while True:
    observed=probe.process_record(p.pid)
    if observed['argv'] != ['']:
     break
    self.assertIsNone(p.poll(), 'fixture process exited before inspection')
    self.assertLess(time.monotonic(),deadline, 'fixture process arguments did not become available')
    time.sleep(0.01)
   self.assertEqual(observed,{'pid':p.pid,'parent_pid':os.getpid(),'executable':'/usr/bin/sleep','argv':['/usr/bin/sleep','60']})
  finally:p.terminate();p.wait()
 def test_container_preserves_declared_user_and_private_service_paths(self):
  command=loaded.probe_command('image:unique','runtime',Path('/probe'),Path('/report'),Path('/redis'))
  self.assertFalse(any(x.startswith('--user') for x in command))
  for value in ['--network=none','--cap-drop=ALL','--read-only','--security-opt=no-new-privileges']:self.assertIn(value,command)
  mounts=[command[i+1] for i,x in enumerate(command) if x=='--tmpfs']
  for name in ['/run:','/var/log:','/var/spool:','/var/sonic:']:self.assertTrue(any(x.startswith(name) for x in mounts))
  self.assertIn('image:unique',command)
 def fixture(self,root,duplicate=False,symlink=False):
  archive=root/'config-engine-source.tar'
  with tarfile.open(archive,'w') as t:
   for _ in range(2 if duplicate else 1):
    i=tarfile.TarInfo('./run/redis/sonic-db/database_config.json')
    if symlink:i.type=tarfile.SYMTYPE;i.linkname='/unexpected';t.addfile(i)
    else:i.size=3;t.addfile(i,io.BytesIO(b'{}\n'))
  return {'artifacts':{archive.name:{'sha256':loaded.check.sha(archive)}}}
 def test_exact_owner_database_payload_copied(self):
  with tempfile.TemporaryDirectory() as t:
   root=Path(t);fixture=root/'fixture';fixture.mkdir();report=self.fixture(root)
   loaded.copy_database_fixture(root,report,fixture);self.assertEqual((fixture/'database_config.json').read_bytes(),b'{}\n')
 def test_duplicate_or_link_database_member_rejected(self):
  for duplicate,symlink in [(True,False),(False,True)]:
   with tempfile.TemporaryDirectory() as t:
    root=Path(t);fixture=root/'fixture';fixture.mkdir();report=self.fixture(root,duplicate,symlink)
    with self.assertRaises(ValueError):loaded.copy_database_fixture(root,report,fixture)
 def test_source_tar_tamper_rejected(self):
  with tempfile.TemporaryDirectory() as t:
   root=Path(t);fixture=root/'fixture';fixture.mkdir();report=self.fixture(root)
   (root/'config-engine-source.tar').write_bytes(b'changed')
   with self.assertRaises(ValueError):loaded.copy_database_fixture(root,report,fixture)



from tools.bazel.ci import mgmt_framework_image as image_driver

class ImageContractTest(unittest.TestCase):
 def test_group_union_preserves_all_rows_and_metadata(self):
  base=['root:x:0:', 'redis:x:101:']; debug=base+['_ssh:x:104:']; metadata=[{'kind':'file','mode':0o644,'uid':0,'gid':0}]*3
  self.assertEqual(image_driver.compare_group_rows([base,base,debug],metadata)['declared_addition'],'_ssh:x:104:')
  for rows,meta in [([base,base,debug+['other:x:88:']],metadata),([base,base,['root:x:0:','_ssh:x:104:']],metadata),([base,base,debug+['root:x:9:']],metadata),([base,base,debug],[*metadata[:2],dict(metadata[2],gid=1)])]:
   with self.subTest(rows=rows,meta=meta), self.assertRaises(ValueError):image_driver.compare_group_rows(rows,meta)
 def test_exact_inherited_environment(self):
  parent={'Env':['PATH=/usr/bin','LANG=C'],'User':'','WorkingDir':'/'}
  current={**parent,'Env':parent['Env']+['DEBIAN_FRONTEND=noninteractive']}
  image_driver.verify_environment(parent,current)
  for bad in [dict(current,Env=current['Env']+['EXTRA=1']),dict(current,Env=current['Env']+['LANG=C']),dict(current,Env=parent['Env']),dict(current,Env=['PATH=/bin','LANG=C','DEBIAN_FRONTEND=noninteractive']),dict(current,User='100')]:
   with self.subTest(bad=bad), self.assertRaises(ValueError):image_driver.verify_environment(parent,bad)

if __name__ == '__main__':
    unittest.main()
