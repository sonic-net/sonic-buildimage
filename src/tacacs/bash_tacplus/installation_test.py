"""Bash plugin installation and original maintainer registration contract."""
from pathlib import Path
import sys,tarfile,unittest
from elf_metadata import Elf
PAYLOAD,CONTROL=map(Path,sys.argv[1:]);del sys.argv[1:]
class Installation(unittest.TestCase):
 def test_original_libtool_installation(self):
  with tarfile.open(PAYLOAD) as a:
   files={m.name.removeprefix('./'):m for m in a};prefix='usr/lib/x86_64-linux-gnu/security/'
   member=files[prefix+'bash_tacplus.so'];self.assertEqual((member.mode,member.uid,member.gid),(0o644,0,0));elf=Elf(a.extractfile(member).read());self.assertEqual(elf.soname,'bash_tacplus.so');self.assertTrue({'libtac.so.2','libtacsupport.so.2'}<=set(elf.needed));self.assertIn('on_shell_execve',elf.exports)
   la=a.extractfile(files[prefix+'bash_tacplus.la']).read();self.assertIn(b"libdir='/usr/lib/x86_64-linux-gnu/security'",la);self.assertIn(b"dependency_libs=' -ltacsupport -ltac'",la);self.assertIn(b'2.5.4 Debian-2.5.4-4',la)
  with tarfile.open(CONTROL) as a:
   files={m.name.removeprefix('./'):m for m in a};self.assertIn(b'Package: bash-tacplus\n',a.extractfile(files['control']).read());post=a.extractfile(files['postinst']).read();self.assertIn(b'/etc/bash_plugins.conf',post);self.assertIn(b'bash_tacplus.so',post)
if __name__=='__main__':unittest.main()
