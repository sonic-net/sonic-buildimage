"""Current original PAM RADIUS payload, six-source feature ABI and controls."""
import json
from pathlib import Path
import sys
import tarfile
import tempfile
import subprocess
import unittest
from elf_metadata import Elf

PAYLOAD,CONTROL,METADATA,CONFIG,LOGROTATE,DLOPEN=map(Path,sys.argv[1:]);del sys.argv[1:]
class Installation(unittest.TestCase):
 def test_original_library_installation_and_feature_abi(self):
  with tarfile.open(PAYLOAD) as archive:
   files={m.name.removeprefix('./'):m for m in archive};prefix='lib/x86_64-linux-gnu/'
   for name in ['libradius-1.1.8.so','libeap-1.1.8.so','security/pam_radius_auth.so']:
    member=files[prefix+name];self.assertEqual((member.mode,member.uid,member.gid),(0o644,0,0));elf=Elf(archive.extractfile(member).read());self.assertEqual(elf.machine,62)
    if name.startswith('security/'):
     self.assertIsNone(elf.soname,'original PAM Make uses plain -shared without a SONAME')
     self.assertTrue({'pam_sm_authenticate','pam_sm_setcred','GenerateNTResponse','pam_peap_authenticate','_pam_radius_incr','_pam_radius_trace'}<=elf.exports)
     self.assertTrue({'libradius-1.1.8.so','libeap-1.1.8.so','libssl.so.3'}<=set(elf.needed))
     self.assertIn(b'/etc/pam_radius_auth.conf',archive.extractfile(member).read())
    else:self.assertEqual(elf.soname,name)
   self.assertEqual(files['etc/pam_radius_auth.conf'].mode,0o600)
   self.assertEqual(archive.extractfile(files['etc/pam_radius_auth.conf']).read(),CONFIG.read_bytes())
   self.assertEqual(files['etc/logrotate.d/pam_radius'].mode,0o644)
   self.assertEqual(archive.extractfile(files['etc/logrotate.d/pam_radius']).read(),LOGROTATE.read_bytes())
   self.assertTrue(files['etc/pam_radius_auth.d/statistics'].isdir())
 def test_declared_target_runtime_resolves_module_without_authentication(self):
  with tempfile.TemporaryDirectory() as name,tarfile.open(PAYLOAD) as archive:
   member=next(m for m in archive if m.name.removeprefix('./')=='lib/x86_64-linux-gnu/security/pam_radius_auth.so')
   module=Path(name)/'pam_radius_auth.so';module.write_bytes(archive.extractfile(member).read())
   done=subprocess.run([str(DLOPEN.resolve()),str(module)],capture_output=True,text=True)
   self.assertEqual(done.returncode,0,done.stdout+done.stderr)
 def test_original_empty_shlibdeps_override_and_control_effects(self):
  package=json.loads(METADATA.read_text())['packages']['libpam-radius-auth'];self.assertFalse(package['shlibdeps']);self.assertNotIn('Depends',package['control_fields']);self.assertTrue(package['elf_needed'])
  with tarfile.open(CONTROL) as archive:
   files={m.name.removeprefix('./'):m for m in archive};control=archive.extractfile(files['control']).read().decode()
   self.assertIn('Package: libpam-radius-auth\n',control);self.assertIn('Version: 1.4.1-1\n',control);self.assertNotIn('\nDepends:',control)
   self.assertIn(b'activate-noawait ldconfig',archive.extractfile(files['triggers']).read())
   conffiles=archive.extractfile(files['conffiles']).read();self.assertIn(b'/etc/pam_radius_auth.conf',conffiles);self.assertIn(b'/etc/logrotate.d/pam_radius',conffiles)
if __name__=='__main__':unittest.main()
