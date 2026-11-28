"""Host rootfs account databases and maintainer-script ownership/mode effects.

Emulates, from declarative tables, what the Make build (build_debian.sh,
package maintainer scripts in dpkg configure order, sonic_debian_extension.j2)
did to:
  * /etc/{passwd,group,shadow,gshadow,subuid,subgid} (+ the "-" backups that
    shadow-utils/adduser/systemd-sysusers leave behind) and /etc/.pwd.lock;
  * home directories (/home/<USERNAME>, /root dot files from base-files);
  * ownership/mode changes of package paths (dpkg-statoverride, chown/chmod in
    postinsts) and /var/lib/dpkg/statoverride{,-old};
  * small postinst-created files/dirs owned by system accounts.

uid/gid values are not hard-coded: they are allocated with the same algorithms
as adduser (first free id 100..999), groupadd --system (highest free id
<= 999), useradd (max used id + 1 in 1000..60000) and systemd-sysusers
(highest free id <= 999, uid == gid preferred), replayed in Make order and
skipping packages that are not installed on the target architecture.

Passwords use the declared APT mkpasswd execution tool (libxcrypt yescrypt).
The salt is derived deterministically from the user name (Make uses a random salt).
"""
import hashlib
import io
import json
import shlex
from pathlib import Path
import re
import subprocess
import sys
import tarfile

MTIME = 0

# ---------------------------------------------------------------------------
# Declarative tables (Make order).  First field: package whose maintainer
# script performs the operation, or a Make script (always applied).
SCRIPTS = ('build_debian.sh', 'sonic_debian_extension.j2')

ACCOUNT_OPS = [
    # debootstrap: base-passwd copies passwd/group.master, passwd postinst
    # runs `shadowconfig on` (pwconv/grpconv).
    ('base-passwd', 'base', {}),
    # udev postinst
    ('udev', 'sysusers_group', {'name': 'input'}),
    ('udev', 'sysusers_group', {'name': 'sgx'}),
    ('udev', 'sysusers_group', {'name': 'clock'}),
    ('udev', 'sysusers_group', {'name': 'kvm'}),
    ('udev', 'sysusers_group', {'name': 'render'}),
    # docker-ce postinst: groupadd --system docker
    ('docker-ce', 'groupadd_system', {'name': 'docker'}),
    # build_debian.sh:351-370
    ('build_debian.sh', 'useradd', {'name': '$USERNAME', 'groups': ['sudo', 'docker'],
                                    'gecos': '$DEFAULT_USERINFO', 'shell': '/bin/bash', 'home': True}),
    ('build_debian.sh', 'chpasswd', {'name': '$USERNAME', 'password': '$PASSWORD'}),
    ('build_debian.sh', 'groupadd', {'name': 'redis'}),
    ('build_debian.sh', 'usermod_aG', {'name': '$USERNAME', 'group': 'redis'}),
    # package postinsts, dpkg configure order
    ('cron-daemon-common', 'sysusers_group', {'name': 'crontab'}),
    ('uuid-runtime', 'addgroup', {'name': 'uuidd'}),
    ('uuid-runtime', 'adduser', {'name': 'uuidd', 'group': 'uuidd', 'home': '/run/uuidd'}),
    ('dbus-system-bus-common', 'sysusers_user', {'name': 'messagebus', 'gecos': 'System Message Bus', 'home': '/nonexistent'}),
    ('tcpdump', 'addgroup', {'name': 'tcpdump'}),
    ('tcpdump', 'adduser', {'name': 'tcpdump', 'group': 'tcpdump', 'home': '/nonexistent'}),
    ('openssh-client', 'addgroup', {'name': '_ssh'}),
    ('i2c-tools', 'addgroup', {'name': 'i2c'}),
    ('openssh-server', 'sysusers_config', {'path': 'usr/lib/sysusers.d/openssh-server.conf'}),
    # systemd postinst: systemd-sysusers basic.conf systemd-journal.conf systemd-network.conf
    ('systemd', 'sysusers_group', {'name': 'systemd-journal'}),
    ('systemd', 'sysusers_user', {'name': 'systemd-network', 'gecos': 'systemd Network Management', 'home': '/'}),
    ('chrony', 'adduser', {'name': '_chrony', 'group': True, 'gecos': 'Chrony daemon', 'home': '/var/lib/chrony'}),
    ('nslcd', 'adduser', {'name': 'nslcd', 'group': True, 'gecos': 'nslcd name service LDAP connection daemon',
                          'home': '/run/nslcd'}),
    # build_debian.sh:937-944 (CHANGE_DEFAULT_PASSWORD=y only)
    ('build_debian.sh', 'expire_login_users', {}),
]

# dpkg-statoverride --update --add, in configure order; then the j2 sed.
STATOVERRIDES = [
    ('cron', 'root', 'crontab', 0o2755, '/usr/bin/crontab'),
    ('dbus', 'root', 'messagebus', 0o4754, '/usr/lib/dbus-1.0/dbus-daemon-launch-helper'),
    ('chrony', '_chrony', '_chrony', 0o750, '/var/lib/chrony'),
    ('chrony', '_chrony', '_chrony', 0o750, '/var/log/chrony'),
    ('chrony', 'root', '_chrony', 0o640, '/etc/chrony/chrony.keys'),
]
STATOVERRIDE_DROP = '_chrony'  # sonic_debian_extension.j2:261,350 sed -i '/_chrony/d'

# Package files re-emitted with the owner/mode a maintainer script gave them:
# (package, path, user, group, mode, source package, source path).
REEMIT = [
    ('cron', 'usr/bin/crontab', 'root', 'crontab', 0o2755, 'cron', None),            # statoverride
    ('dbus', 'usr/lib/dbus-1.0/dbus-daemon-launch-helper', 'root', 'messagebus', 0o4754, 'dbus', None),
    ('openssh-client', 'usr/bin/ssh-agent', 'root', '_ssh', 0o2755, 'openssh-client', None),  # chgrp/chmod
    ('ifupdown2', 'usr/share/ifupdown2/__main__.py', 'root', 'root', 0o755, 'ifupdown2', None),  # chmod +x
    # ucf --three-way /usr/share/chrony/chrony.keys /etc/chrony/chrony.keys + statoverride
    ('chrony', 'etc/chrony/chrony.keys', 'root', '_chrony', 0o640, 'chrony', 'usr/share/chrony/chrony.keys'),
    # base-files postinst install_from_default (cp -p); /etc/motd is not emitted:
    # sonic_debian_extension.j2 replaces it with the SONiC banner (extension layer).
    ('base-files', 'root/.profile', 'root', 'root', None, 'base-files', 'usr/share/base-files/dot.profile'),
    ('base-files', 'root/.bashrc', 'root', 'root', None, 'base-files', 'usr/share/base-files/dot.bashrc'),
    ('base-files', 'etc/profile', 'root', 'root', None, 'base-files', 'usr/share/base-files/profile'),
    ('base-files', 'usr/share/info/dir', 'root', 'root', 0o644, 'base-files', 'usr/share/base-files/info.dir'),
]

# Directories created or chown/chmod-ed by maintainer scripts: (package, path, user, group, mode).
DIRS = [
    ('base-files', 'mnt', 'root', 'root', 0o755),
    ('base-files', 'srv', 'root', 'root', 0o755),
    ('base-files', 'opt', 'root', 'root', 0o755),
    ('base-files', 'etc/opt', 'root', 'root', 0o755),
    ('base-files', 'var/opt', 'root', 'root', 0o755),
    ('base-files', 'media', 'root', 'root', 0o755),
    ('base-files', 'var/mail', 'root', 'mail', 0o2775),
    ('base-files', 'usr/local/games', 'root', 'root', 0o755),
    ('base-files', 'usr/local/sbin', 'root', 'root', 0o755),
    ('base-files', 'usr/local/src', 'root', 'root', 0o755),
    ('fontconfig-config', 'usr/local/share/fonts', 'root', 'staff', 0o2775),
    ('apt', 'var/cache/apt/archives/partial', '_apt', 'root', 0o700),
    ('cron-daemon-common', 'var/spool/cron/crontabs', 'root', 'crontab', 0o1730),
    ('uuid-runtime', 'var/lib/libuuid', 'uuidd', 'uuidd', 0o2775),
    ('chrony', 'var/lib/chrony', '_chrony', '_chrony', 0o750),
    ('rsyslog', 'var/spool/rsyslog', 'root', 'root', 0o700),
    # systemd postinst mkdir + tmpfiles journal-nocow/systemd.conf (z ... 2755 root systemd-journal)
    ('systemd', 'var/log/journal', 'root', 'systemd-journal', 0o2755),
]

# Empty files created by maintainer scripts: (package, path, user, group, mode).
EMPTY_FILES = [
    ('base-files', 'var/log/wtmp', 'root', 'utmp', 0o664),
    ('base-files', 'var/log/btmp', 'root', 'utmp', 0o660),
    ('base-files', 'var/log/lastlog', 'root', 'utmp', 0o664),
    ('login', 'var/log/faillog', 'root', 'root', 0o644),
    ('passwd', 'etc/.pwd.lock', 'root', 'root', 0o600),  # lckpwdf()
]

SYMLINKS = [
    ('base-files', 'var/spool/mail', '../mail'),
    ('base-files', 'usr/local/man', 'share/man'),
]

# nslcd postinst create_config + debconf answers of the Make build
# (nslcd.config guesses from the build host: no domain, local server).
NSLCD_DEBCONF = {'uri': 'ldap://127.0.0.1/', 'base': 'dc=,dc='}

DB_FILES = ['passwd', 'shadow', 'group', 'gshadow', 'subuid', 'subgid']
DB_MODE = {'passwd': (0o644, 'root'), 'group': (0o644, 'root'), 'shadow': (0o600, 'shadow'),
           'gshadow': (0o640, 'shadow'), 'subuid': (0o644, 'root'), 'subgid': (0o644, 'root')}


# ---------------------------------------------------------------------------
def read_tar(path):
    files = {}
    with tarfile.open(path, 'r:*') as archive:
        for member in archive:
            name = member.name
            while name.startswith('./'):
                name = name[2:]
            name = name.strip('/')
            data = archive.extractfile(member).read() if member.isfile() else None
            files[name] = (member, data)
    return files


class Hasher:
    def __init__(self, executable):
        self.executable = executable

    @staticmethod
    def setting(user):
        """libxcrypt crypt_gensalt('$y$', 0) format (cost 5 = j9T, 16 salt bytes)."""
        itoa64 = './0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
        raw = hashlib.sha256(b'sonic-rootfs-accounts:' + user.encode()).digest()[:16]
        out = ''
        for i in range(0, len(raw), 3):
            chunk = raw[i:i + 3]
            value = int.from_bytes(chunk, 'little')
            for _ in range(0, len(chunk) * 8, 6):
                out += itoa64[value & 0x3f]
                value >>= 6
        return '$y$j9T$' + out

    def __call__(self, user, password):
        setting = self.setting(user)
        done = subprocess.run(
            [self.executable, '--method=yescrypt', '--salt=' + setting, '--stdin'], input=password + '\n',
            env={'LC_ALL': 'C', 'TZ': 'UTC', 'PATH': '/nonexistent'},
            capture_output=True, text=True,
        )
        result = done.stdout.rstrip("\n")
        if done.returncode or not result.startswith(setting + '$') or len(result) != len(setting) + 44:
            raise ValueError('yescrypt failed (rc=%d): %s' % (done.returncode, done.stderr[-400:]))
        return result


class Accounts:
    def __init__(self, day):
        self.day = str(day)
        self.passwd, self.shadow, self.group, self.gshadow = [], [], [], []
        self.subuid, self.subgid = [], []
        self.text = {f: None for f in DB_FILES}
        self.backup = {f: None for f in DB_FILES}

    # serialisation ---------------------------------------------------------
    def render(self, f):
        if f == 'passwd':
            return ''.join(':'.join(map(str, e)) + '\n' for e in self.passwd)
        if f == 'shadow':
            return ''.join(':'.join(e) + '\n' for e in self.shadow)
        if f == 'group':
            return ''.join('%s:%s:%d:%s\n' % (n, p, g, ','.join(m)) for n, p, g, m in self.group)
        if f == 'gshadow':
            return ''.join('%s:%s:%s:%s\n' % (n, p, a, ','.join(m)) for n, p, a, m in self.gshadow)
        return ''.join('%s:%d:%d\n' % e for e in getattr(self, f))

    def write(self, *files):
        """One tool invocation rewriting files: each keeps a '-' backup of its previous content."""
        for f in files:
            self.backup[f] = self.text[f]
            self.text[f] = self.render(f)

    # lookups ---------------------------------------------------------------
    def uids(self):
        return {e[2] for e in self.passwd}

    def gids(self):
        return {e[2] for e in self.group}

    def uid(self, name):
        return next(e[2] for e in self.passwd if e[0] == name)

    def gid(self, name):
        return next(e[2] for e in self.group if e[0] == name)

    def _group(self, name):
        return next(e for e in self.group if e[0] == name)

    def _gshadow(self, name):
        return next(e for e in self.gshadow if e[0] == name)

    # operations ------------------------------------------------------------
    def base(self, passwd_master, group_master):
        for line in passwd_master.splitlines():
            n, p, u, g, gecos, home, shell = line.split(':')
            self.passwd.append([n, 'x', int(u), int(g), gecos, home, shell])
            self.shadow.append([n, p, self.day, '0', '99999', '7', '', '', ''])  # pwconv, login.defs aging
        for line in group_master.splitlines():
            n, p, g, members = line.split(':')
            m = [x for x in members.split(',') if x]
            self.group.append([n, 'x', int(g), m])
            self.gshadow.append([n, p, '', list(m)])  # grpconv
        self.text['subuid'] = self.text['subgid'] = ''  # login postinst ensures empty files
        self.write('passwd', 'shadow', 'group', 'gshadow')

    def _new_group(self, name, gid, password):
        self.group.append([name, 'x', gid, []])
        self.gshadow.append([name, password, '', []])

    def addgroup(self, name):  # adduser --system / addgroup --system: first free 100..999
        if any(entry[0] == name for entry in self.group):
            return
        gid = next(i for i in range(100, 1000) if i not in self.gids())
        self._new_group(name, gid, '!')
        self.write('group', 'gshadow')

    def groupadd_system(self, name):  # SYS_GID_MIN 101 .. SYS_GID_MAX 999, highest free
        if any(entry[0] == name for entry in self.group):
            return
        gid = next(i for i in range(999, 100, -1) if i not in self.gids())
        self._new_group(name, gid, '!')
        self.write('group', 'gshadow')

    def groupadd(self, name):  # GID_MIN 1000 .. GID_MAX 60000: max used + 1
        if any(entry[0] == name for entry in self.group):
            return
        used = [g for g in self.gids() if 1000 <= g <= 60000]
        self._new_group(name, max(used) + 1 if used else 1000, '!')
        self.write('group', 'gshadow')

    def adduser(self, name, group=None, gecos='', home='/nonexistent', shell='/usr/sbin/nologin'):
        """adduser --system: [--group] -> groupadd; useradd; chfn when --gecos."""
        if group is True:
            self.addgroup(name)
            group = name
        uid = next(i for i in range(100, 1000) if i not in self.uids())
        gid = self.gid(group) if group else 65534
        self.passwd.append([name, 'x', uid, gid, '', home, shell])
        self.shadow.append([name, '!', self.day, '', '', '', '', '', ''])
        self.write('passwd', 'shadow')
        if gecos:
            self.passwd[-1][4] = gecos + ',,,'
            self.write('passwd')

    def useradd(self, name, groups, gecos, shell, home):
        used = [u for u in self.uids() if 1000 <= u <= 60000]
        uid = max(used) + 1 if used else 1000
        gid = uid if uid not in self.gids() else max(g for g in self.gids() if 1000 <= g <= 60000) + 1
        self._new_group(name, gid, '!')
        for g in groups:
            self._group(g)[3].append(name)
            self._gshadow(g)[3].append(name)
        self.passwd.append([name, 'x', uid, gid, gecos, '/home/' + name, shell])
        self.shadow.append([name, '!', self.day, '0', '99999', '7', '', '', ''])
        for f in ('subuid', 'subgid'):
            ranges = getattr(self, f)
            start = max([s + c for _, s, c in ranges], default=100000)
            ranges.append((name, start, 65536))
        self.write('passwd', 'shadow', 'group', 'gshadow', 'subuid', 'subgid')
        return home

    def chpasswd(self, name, hashed):
        entry = next(e for e in self.shadow if e[0] == name)
        entry[1], entry[2] = hashed, self.day
        self.write('shadow')

    def usermod_aG(self, name, group):
        self._group(group)[3].append(name)
        self._gshadow(group)[3].append(name)
        self.write('group', 'gshadow')

    def sysusers_group(self, name):
        if any(entry[0] == name for entry in self.group):
            return
        gid = next(i for i in range(999, 0, -1) if i not in self.gids())
        self._new_group(name, gid, '!*')
        self.write('group', 'gshadow')

    def sysusers_user(self, name, gecos, home, shell='/usr/sbin/nologin', group=None):
        if any(entry[0] == name for entry in self.passwd):
            return
        i = next(i for i in range(999, 0, -1) if i not in self.uids() and i not in self.gids())
        self.passwd.append([name, 'x', i, self.gid(group) if group else i, gecos, home, shell])
        self.shadow.append([name, '!*', self.day, '', '', '', '', '', ''])
        if group is None:
            self._new_group(name, i, '!*')
            self.write('passwd', 'shadow', 'group', 'gshadow')
        else:
            self.write('passwd', 'shadow')

    def expire_login_users(self):  # passwd -e for /home users with bash/sh
        names = [e[0] for e in self.passwd if '/home' in e[5] and e[6] in ('/bin/bash', '/bin/sh')]
        for n in names:
            next(e for e in self.shadow if e[0] == n)[2] = '0'
            self.write('shadow')


def dpkg_hash_order(path):
    """dpkg fsys hash bin order (FNV-1a of the path without leading '/', % 262139)."""
    h = 2166136261
    for byte in path.lstrip('/').encode():
        h = ((h ^ byte) * 16777619) & 0xffffffff
    return h % 262139


def statoverride_text(entries):
    return ''.join('%s %s %o %s\n' % e for e in sorted(entries, key=lambda e: dpkg_hash_order(e[3])))


def nslcd_conf(postinst):
    lines = postinst.splitlines()
    start = next(i for i, l in enumerate(lines) if re.match(r'\s*cat > "\$CONFFILE" << EOM\s*$', l))
    end = next(i for i in range(start + 1, len(lines)) if lines[i] == 'EOM')
    out = []
    for line in lines[start + 1:end]:
        line = line.replace('$CONFFILE', '/etc/nslcd.conf')  # unquoted heredoc: shell expansion
        key = line.split(' ', 1)[0]
        if key in NSLCD_DEBCONF and not any(o.startswith(key + ' ') for o in out):
            line = '%s %s' % (key, NSLCD_DEBCONF[key])
        out.append(line)
    return ('\n'.join(out) + '\n').encode()


def main(spec_path, out_path, report_path):
    spec = json.loads(Path(spec_path).read_text())
    make = spec['configuration']
    installed = set(spec['installed'])
    applies = lambda pkg: pkg in SCRIPTS or pkg in installed
    data = {}

    def pkgfile(pkg, path):
        if pkg not in data:
            data[pkg] = read_tar(spec['data'][pkg])
        member, content = data[pkg][path]
        return member, content

    entries = {}  # path -> TarInfo, bytes
    report = {'skipped_ops': [], 'entries': {}}
    report['package_controls'] = {}
    for package, path in sorted(spec['controls'].items()):
        control = read_tar(path)['control'][1].decode()
        fields = {}
        for line in control.splitlines():
            if line and not line[0].isspace() and ':' in line:
                key, value = line.split(':', 1)
                fields[key] = value.strip()
        if fields.get('Package') != package.split(':')[0]:
            raise ValueError('account control identity differs: ' + package)
        report['package_controls'][package] = {'version': fields['Version'], 'sha256': hashlib.sha256(control.encode()).hexdigest()}

    def add(path, kind, uid, gid, mode, content=b'', link='', source=''):
        info = tarfile.TarInfo(path)
        info.type = {'f': tarfile.REGTYPE, 'd': tarfile.DIRTYPE, 'l': tarfile.SYMTYPE}[kind]
        info.uid, info.gid, info.mode, info.mtime = uid, gid, mode, MTIME
        info.uname = info.gname = ''
        info.linkname = link
        info.size = len(content) if kind == 'f' else 0
        if path in entries:
            raise ValueError('duplicate entry ' + path)
        entries[path] = (info, content if kind == 'f' else None)
        report['entries'][path] = source

    hasher = Hasher(spec['mkpasswd'])
    db = Accounts(spec['lastchg_day'])
    homes = []
    for pkg, op, args in ACCOUNT_OPS:
        if not applies(pkg):
            report['skipped_ops'].append([pkg, op, args.get('name', '')])
            continue
        args = {k: (make[v[1:]] if isinstance(v, str) and v.startswith('$') else v) for k, v in args.items()}
        if op == 'base':
            db.base(pkgfile('base-passwd', 'usr/share/base-passwd/passwd.master')[1].decode(),
                    pkgfile('base-passwd', 'usr/share/base-passwd/group.master')[1].decode())
        elif op == 'sysusers_config':
            content = pkgfile(pkg, args['path'])[1]
            report.setdefault('sysusers_configs', {})[pkg] = {'path': args['path'], 'sha256': hashlib.sha256(content).hexdigest()}
            for raw in content.decode().splitlines():
                fields = shlex.split(raw, comments=True)
                if not fields:
                    continue
                # The selected Trixie/FIPS openssh-server declares one u row
                # with an allocated UID and an existing explicit primary group.
                if len(fields) != 5 or fields[0] != 'u' or not fields[2].startswith('-:'):
                    raise ValueError('unhandled declared sysusers row for ' + pkg + ': ' + raw)
                db.sysusers_user(fields[1], fields[3], fields[4], group=fields[2][2:])
        elif op == 'chpasswd':
            db.chpasswd(args['name'], hasher(args['name'], args['password']) if args['password'] else '')
        elif op == 'expire_login_users':
            if make['CHANGE_DEFAULT_PASSWORD'] == 'y':
                db.expire_login_users()
        elif op == 'useradd':
            if db.useradd(**args):
                homes.append(args['name'])
        else:
            getattr(db, op)(**args)
    uid = db.uid
    gid = db.gid

    for f in DB_FILES:
        mode, group = DB_MODE[f]
        add('etc/' + f, 'f', 0, gid(group), mode, db.text[f].encode(), source='accounts')
        if db.backup[f] is not None:
            add('etc/' + f + '-', 'f', 0, gid(group), mode, db.backup[f].encode(), source='accounts backup')

    # useradd -m: home from /etc/skel (bash), then build_debian.sh copies bash.bashrc.
    for name in homes:
        u, g = uid(name), gid(name)
        add('home/' + name, 'd', u, g, 0o755, source='useradd -m')
        for dot in ('.bash_logout', '.bashrc', '.profile'):
            source = {'.bash_logout': 'skel_bash_logout', '.bashrc': 'bashrc', '.profile': 'skel_profile'}[dot]
            content = Path(spec[source]).read_bytes()
            add('home/%s/%s' % (name, dot), 'f', u, g, 0o644, content, source='skel')

    for pkg, path, user, group, mode, src_pkg, src_path in REEMIT:
        if applies(pkg):
            member, content = pkgfile(src_pkg, src_path or path)
            if src_path is None and (member.uid, member.gid, member.mode) == (uid(user), gid(group), mode):
                continue  # The source producer already installed these exact metadata.
            add(path, 'f', uid(user), gid(group), member.mode & 0o7777 if mode is None else mode, content,
                source='%s postinst' % pkg)
    for pkg, path, user, group, mode in DIRS:
        if applies(pkg):
            add(path, 'd', uid(user), gid(group), mode, source='%s postinst' % pkg)
    for pkg, path, user, group, mode in EMPTY_FILES:
        if applies(pkg):
            add(path, 'f', uid(user), gid(group), mode, b'', source='%s postinst' % pkg)
    for pkg, path, target in SYMLINKS:
        if applies(pkg):
            add(path, 'l', 0, 0, 0o777, link=target, source='%s postinst' % pkg)
    if applies('nslcd'):
        postinst = read_tar(spec['controls']['nslcd'])['postinst'][1].decode()
        add('etc/nslcd.conf', 'f', 0, gid('nslcd'), 0o640, nslcd_conf(postinst), source='nslcd postinst')

    overrides, previous = [], None
    for pkg, user, group, mode, path in STATOVERRIDES:
        if applies(pkg):
            previous = list(overrides)
            overrides.append((user, group, mode, path))
    final = [e for e in overrides if STATOVERRIDE_DROP not in (e[0], e[1])]
    add('var/lib/dpkg/statoverride', 'f', 0, 0, 0o644, statoverride_text(final).encode(), source='dpkg-statoverride')
    if previous is not None:
        add('var/lib/dpkg/statoverride-old', 'f', 0, 0, 0o644, statoverride_text(previous).encode(),
            source='dpkg-statoverride backup')

    with tarfile.open(out_path, 'w', format=tarfile.GNU_FORMAT) as archive:
        for path in sorted(entries):
            info, content = entries[path]
            archive.addfile(info, io.BytesIO(content) if content is not None else None)
    report['users'] = [[e[0], e[2], e[3]] for e in db.passwd]
    report['groups'] = [[e[0], e[2]] for e in db.group]
    Path(report_path).write_text(json.dumps(report, indent=1, sort_keys=True) + '\n')


if __name__ == '__main__':
    main(*sys.argv[1:])
