"""Select ONIE platform names from original and shared prepared ASIC markers."""
import json
from pathlib import Path
import sys

request, output = sys.argv[1:]
spec = json.loads(Path(request).read_text()); machine = spec['machine']
selected = {platform for platform, marker in spec['markers'] if machine == 'generic' or machine in Path(marker).read_text().splitlines()}
if machine == 'alpinevs': selected.add('x86_64-kvm_x86_64-r0')
if not selected: raise ValueError('prepared device data has no platforms for ' + machine)
Path(output).write_text(''.join(name + '\n' for name in sorted(selected)))
