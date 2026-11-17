from pathlib import Path
import hashlib,json,subprocess,sys,tempfile,types,unittest
from unittest.mock import patch
W=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(W))
from tools.bazel.ci import mgmt_framework_image as driver
class Fence(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.root=Path(self.tmp.name)
  subprocess.run(['git','init','-q',str(self.root)],check=True)
  (self.root/'source.txt').write_text('original\n')
  subprocess.run(['git','add','source.txt'],cwd=self.root,check=True)
  subprocess.run(['git','-c','user.name=Fixture','-c','user.email=fixture@example.invalid','commit','-qm','fixture'],cwd=self.root,check=True)
 def check(self):
  with patch.object(driver,'ROOT',self.root):driver.clean_source(self.root/'artifacts/current')
 def test_prior_evidence_permitted(self):
  p=self.root/'artifacts/prior/receipt.json';p.parent.mkdir(parents=True);p.write_text('{}\n');self.check()
 def test_untracked_source_rejected(self):
  (self.root/'undeclared.py').write_text('pass\n')
  with self.assertRaises(ValueError):self.check()
 def test_tracked_source_rejected(self):
  (self.root/'source.txt').write_text('changed\n')
  with self.assertRaises(subprocess.CalledProcessError):self.check()
 def test_bare_artifacts_file_rejected(self):
  (self.root/'artifacts').write_text('changed\n')
  with self.assertRaises(ValueError):self.check()
class Checks(unittest.TestCase):
 def test_exact_group_order(self):
  base=['root:x:0:','redis:x:101:'];meta=[{'mode':420}]*3
  driver.compare_group_rows([base,base,base+['_ssh:x:104:']],meta)
  for rows in [[base,list(reversed(base)),base+['_ssh:x:104:']],[base,base,['_ssh:x:104:']+base]]:
   with self.assertRaises(ValueError):driver.compare_group_rows(rows,meta)
 def test_edit_failure_always_restores_source_and_outputs(self):self.exercise('edit')
 def test_invalidation_failure_always_restores_source_and_outputs(self):self.exercise('invalidation')
 def test_restoration_failure_is_recorded_without_source_mutation(self):self.exercise('restore')
 def exercise(self,failure):
  with tempfile.TemporaryDirectory() as tmp:
   root=Path(tmp);a=root/'artifacts/framework';a.mkdir(parents=True)
   source=root/'dockers/docker-sonic-mgmt-framework/mgmt_vars.j2';source.parent.mkdir(parents=True);source.write_bytes(b'original\n');source.chmod(0o640)
   labels=[driver.OWNER+n+s for n in driver.IMAGES for s in ('','.gz')]+[driver.PARENT,driver.TARGETS['source-runtime.tar'],driver.TARGETS['all-debug.tar']]
   files={label:root/('output'+str(i)) for i,label in enumerate(labels)}
   for p in files.values():p.write_bytes(b'original payload')
   original={p:p.read_bytes() for p in files.values()}
   receipt={'status':'built','revision':'f'*40,'artifacts':{n+'.gz':{'sha256':driver.check.sha(files[driver.OWNER+n+'.gz'])} for n in driver.IMAGES}}
   (a/'build-receipt.json').write_text(json.dumps(receipt))
   stages=[]
   def run(command,**kwargs):
    stages.append(len(stages)+1)
    if len(stages)==3:
     self.assertNotEqual(source.read_bytes(),b'original\n')
     if failure=='edit':raise subprocess.CalledProcessError(1,command)
     if failure=='restore':
      for label in labels[:4]:files[label].write_bytes(b'changed image')
    if len(stages)==4:
     self.assertEqual(source.read_bytes(),b'original\n');self.assertEqual(source.stat().st_mode&0o777,0o640)
     if failure=='restore':raise subprocess.CalledProcessError(2,command)
     for p,data in original.items():p.write_bytes(data)
    return subprocess.CompletedProcess(command,0)
   with patch.object(driver,'ROOT',root),patch.object(driver,'clean_source'),patch.object(driver,'identity',return_value={'revision':'f'*40}),patch.object(driver.subprocess,'check_output',side_effect=lambda cmd,**kw:str(files[cmd[-2]].relative_to(root))+'\n'),patch.object(driver.subprocess,'run',side_effect=run):
    with self.assertRaises((ValueError,subprocess.CalledProcessError)):driver.incremental(types.SimpleNamespace(bazel='bazel'),a)
   self.assertEqual(len(stages),4);self.assertEqual(source.read_bytes(),b'original\n')
   result=json.loads((a/'incremental.json').read_text());self.assertEqual(result['status'],'failed')
   if failure=='restore':self.assertIn('restoration_error',result)
   else:self.assertEqual(result['restore']['sha256'],result['baseline']['sha256'])

if __name__=='__main__':unittest.main()
