"""Differences between the Make and Bazel builds that are accepted as expected.

Each rule names a difference the two builds produce on purpose, and says why.
Any diagnostic that is not accepted by a rule here will fail the run.
"""

from diagnostics import Codes
from rules_engine import AcceptanceRule, DiagnosticMatcher, Rules, literal


LIBREBOOTGNOI = "/usr/lib/x86_64-linux-gnu/librebootgnoi.so.0.0.0"

LIBREBOOTGNOI_RULES = [
    AcceptanceRule(
        id="sysmgr-librebootgnoi-not-shipped",
        # The shared library itself is shipped now, and its two symlinks with it.
        # These are the libtool by-products that have no Bazel equivalent.
        matcher=DiagnosticMatcher(
            codes=Codes.EXTRACTION_MAKE_ONLY,
            name=(
                literal("/usr/lib/x86_64-linux-gnu/librebootgnoi.a"),
                literal("/usr/lib/x86_64-linux-gnu/librebootgnoi.la"),
            ),
        ),
        reason="libtool also emits a static archive and its own metadata file; Bazel emits neither.",
    ),
    AcceptanceRule(
        id="sysmgr-exit-handlers",
        # Named one at a time, because these come from the link line rather than
        # from the sources, and a new one would mean the link line changed again.
        matcher=DiagnosticMatcher(
            codes=(Codes.ELFCOMPARE_FUNCTION_ADDED, Codes.ELFCOMPARE_IMPORT_ADDED),
            name=(
                literal(LIBREBOOTGNOI),
                literal("/usr/bin/rebootbackend"),
            ),
            msg=(
                '*"name": "atexit"*',
                '*"name": "at_quick_exit"*',
                '*"name": "__cxa_at_quick_exit@GLIBC_2.10"*',
            ),
        ),
        reason=(
            "The Bazel toolchain links libc_nonshared.a explicitly, which defines these."
        ),
    ),
    AcceptanceRule(
        id="sysmgr-dynamic-needed-order",
        matcher=DiagnosticMatcher(
            codes=Codes.ELFCOMPARE_DEPENDENCY,
            name=(
                literal(LIBREBOOTGNOI),
                literal("/usr/bin/rebootbackend"),
            ),
            msg=(
                literal('{"category": "dependency", "section": ".dynamic", "name": "dynamic.needed", "left": ["libstdc++.so.6", "libc.so.6", "libgcc_s.so.1"], "right": ["libstdc++.so.6", "libgcc_s.so.1", "libc.so.6"]}'),
                literal('{"category": "dependency", "section": ".dynamic", "name": "dynamic.needed", "left": ["libswsscommon.so.0", "libdbus-c++-1.so.0", "libprotobuf.so.32", "librebootgnoi.so.0", "libhiredis.so.1.1.0", "libstdc++.so.6", "libgcc_s.so.1", "libc.so.6"], "right": ["librebootgnoi.so.0", "libprotobuf.so.32", "libhiredis.so.1.1.0", "libswsscommon.so.0", "libdbus-c++-1.so.0", "libstdc++.so.6", "libgcc_s.so.1", "libc.so.6"]}'),
            ),
        ),
        reason="The same libraries on both sides, listed in a different order. We accept the difference as it's not going to cause shadowing.",
    ),
]

PIN_NOT_ENFORCED = """
Make records the version it used in files/build/versions/**/versions-deb-*, but it does not hold itself to it.
src/sonic-build-hooks/scripts/pre_run_buildinfo only installs the apt preferences file
when SONIC_VERSION_CONTROL_COMPONENTS contains `deb`, which is not always.
"""

# Differences that exist only because Make and Bazel resolved the same package to
# different versions. Each names the paths and versions it covers, so that the
# next drift fails instead of being absorbed.
UNENFORCED_DEB_PIN_RULES = [
    AcceptanceRule(
        id="accept-openssl-provider-legacy-drift",
        matcher=DiagnosticMatcher(
            codes=Codes.ELFCOMPARE_INCOMPLETE,
            name=literal("/usr/lib/x86_64-linux-gnu/ossl-modules/legacy.so"),
        ),
        reason="Make ships openssl-provider-legacy 3.5.7-1~deb13u2 where Bazel ships 3.5.6-1~deb13u2.",
        comment=PIN_NOT_ENFORCED,
    ),
    AcceptanceRule(
        id="accept-libexpat-drift",
        # The versioned sonames carry the version, so a different one stops matching.
        matcher=DiagnosticMatcher(
            codes=(Codes.EXTRACTION_BAZEL_ONLY, Codes.FILE_TARGET_MISMATCH),
            name=(
                literal("/usr/lib/x86_64-linux-gnu/libexpat.so.1"),
                literal("/usr/lib/x86_64-linux-gnu/libexpat.so.1.10.2"),
                literal("/usr/lib/x86_64-linux-gnu/libexpatw.so.1"),
                literal("/usr/lib/x86_64-linux-gnu/libexpatw.so.1.10.2"),
            ),
        ),
        reason=(
            "Make ships libexpat1 2.8.3-1~deb13u1 where Bazel ships 2.7.1-2, so the sonames in the debug image point at the older library."
        ),
        comment=PIN_NOT_ENFORCED,
    ),
]

# Bazel unpacks a package's data.tar and flattens the layers.
# Make runs dpkg, which also executes maintainer scripts, so it ends up with superfluous state.
NO_DPKG_RULES = [
    AcceptanceRule(
        id="accept-dpkg-admin-state",
        matcher=DiagnosticMatcher(
            codes=(Codes.FILE_CONTENT_MISMATCH, Codes.EXTRACTION_MAKE_ONLY),
            source="//dockers/*",
            name=(
                literal("/etc/group"),
                literal("/etc/group-"),
                literal("/etc/gshadow"),
                literal("/etc/gshadow-"),
                literal("/var/cache/debconf/config.dat"),
                literal("/var/cache/debconf/config.dat-old"),
                literal("/var/cache/debconf/templates.dat"),
                literal("/var/cache/debconf/templates.dat-old"),
                literal("/var/lib/dpkg/triggers/File"),
                literal("/var/lib/ucf/hashfile"),
                literal("/var/lib/ucf/registry"),
            ),
        ),
        reason="Bazel runs no maintainer scripts, so this state stays as the base image left it.",
        comment="""
        /etc/group and /etc/gshadow: openssh-client.postinst adds the `_ssh` group.
        /var/cache/debconf: templates registered by maintainer scripts. Both images set DEBIAN_FRONTEND=noninteractive, so nothing reads them.
        /var/lib/dpkg/triggers/File: we don't plan on updating packages once they ship, so this flag is superfluous.
        /var/lib/ucf: empty on both sides.
        """,
    ),
    AcceptanceRule(
        id="accept-update-alternatives-not-replayed",
        matcher=DiagnosticMatcher(
            codes=(Codes.FILE_CONTENT_MISMATCH, Codes.EXTRACTION_MAKE_ONLY),
            source="//dockers/*",
            name="/var/lib/dpkg/alternatives/*",
        ),
        reason=(
            "vim_alternatives_pkg writes the /etc/alternatives symlinks directly instead of registring them."
        ),
    ),
    AcceptanceRule(
        id="accept-dpkg-diversions-not-replayed",
        matcher=DiagnosticMatcher(
            codes=(Codes.FILE_CONTENT_MISMATCH, Codes.EXTRACTION_MAKE_ONLY),
            source="//dockers/*",
            name=(
                literal("/var/lib/dpkg/diversions"),
                literal("/var/lib/dpkg/diversions-old"),
                literal("/usr/share/vim/vim91/doc/help.txt.vim-tiny"),
                literal("/usr/share/vim/vim91/doc/tags.vim-tiny"),
            ),
        ),
        reason="Bazel has no dpkg-divert, so it overwrites directly, where Make renames the archives first.",
        comment="help.txt and tags themselves are byte-identical, it's just the backups that are missing.",
    ),
    AcceptanceRule(
        id="accept-systemd-user-units-not-enabled",
        matcher=DiagnosticMatcher(
            codes=Codes.EXTRACTION_MAKE_ONLY,
            source="//dockers/*",
            name=(
                "/etc/systemd/user/*",
                "/var/lib/systemd/deb-systemd-user-helper-enabled/*",
            ),
        ),
        reason=(
            "deb-systemd-helper enables ssh-agent.socket from a maintainer script. "
            "Not needed, since we run supervisord anyway."
        ),
    ),
]

RULES = Rules(
    AcceptanceRule(
        id="excluded-by-tag",
        matcher=DiagnosticMatcher(codes=Codes.COLLECTION_EXCLUDED_BY_TAG),
        reason=(
            "The target carries the exclusion tag, so it was deliberately left out "
            "of the comparison and has nothing to answer for."
        ),
    ),
    AcceptanceRule(
        id="accept-debian-changelogs",
        matcher=DiagnosticMatcher(
            name="*/changelog.gz",
            codes=Codes.EXTRACTION_MAKE_ONLY,
        ),
        reason="We don't ship changelogs in Bazel-built debs.",
    ),
    AcceptanceRule(
        id="accept-var-lib-dpkg-info-make-only",
        matcher=DiagnosticMatcher(
            name="/var/lib/dpkg/info/*",
            source="*dockers/*",
            codes=Codes.EXTRACTION_MAKE_ONLY,
        ),
        reason="Irrelevant entries once we're in the image.",
    ),
    AcceptanceRule(
        id="accept-build-machinery-residue",
        matcher=DiagnosticMatcher(
            codes=(Codes.FILE_CONTENT_MISMATCH, Codes.EXTRACTION_MAKE_ONLY),
            source="//dockers/*",
            # Listed on by one instead of globbed, so that files that can actually cause issues (e.g. rsyslog.conf)
            # will trigger an error when they mismatch.
            name=(
                literal("/cache.tgz"),
                literal("/etc/ld.so.cache"),
                literal("/etc/shadow"),
                literal("/usr/local/lib/python3.13/dist-packages/bitarray-2.8.1.dist-info/RECORD"),
                literal("/usr/local/share/buildinfo/post-versions/purge-versions-deb"),
                literal("/usr/local/share/buildinfo/post-versions/versions-deb-trixie-amd64"),
                literal("/usr/local/share/buildinfo/post-versions/versions-mirror"),
                literal("/usr/local/share/buildinfo/pre-versions/versions-deb-trixie-amd64"),
                literal("/usr/local/share/buildinfo/pre-versions/versions-py3-trixie-amd64"),
                literal("/usr/local/share/buildinfo/sonic-build-hooks_1.0_all.deb"),
                literal("/usr/local/share/buildinfo/versions/versions-deb"),
                literal("/var/cache/ldconfig/aux-cache"),
                literal("/var/lib/apt/extended_states"),
                literal("/var/lib/dpkg/status"),
                literal("/var/lib/dpkg/status-old"),
                literal("/var/log/alternatives.log"),
                literal("/var/log/apt/eipp.log.xz"),
                literal("/var/log/apt/history.log"),
                literal("/var/log/apt/term.log"),
                literal("/var/log/dpkg.log"),
            ),
        ),
        reason=(
            "Bazel assembles layers, so it has no history or logs to record."
        ),
    ),
    AcceptanceRule(
        id="accept-base-image-binaries-without-debug-symbols",
        # Named one at a time, so a new binary losing its debug half is caught.
        #
        # TODO(bazel-ready): ship the dbgsym debs Make already builds.
        # To do that, we'd have to rely on target/dbs, which we don't want to do for now.
        # Or, alternatively, get the debug information from sonic-swss-common.
        matcher=DiagnosticMatcher(
            codes=(Codes.EXTRACTION_NO_DEBUG, Codes.EXTRACTION_MAKE_ONLY),
            name=(
                literal("/usr/bin/eventd"),
                literal("/usr/bin/eventdb"),
                literal("/usr/bin/events_tool"),
                literal("/usr/bin/sonic-db-cli"),
                literal("/usr/bin/swssloglevel"),
                literal("/usr/lib/python3/dist-packages/swsscommon/_swsscommon.so.0.0.0"),
                literal("/usr/lib/x86_64-linux-gnu/libsonicdbcli.so.0.0.0"),
                literal("/usr/lib/x86_64-linux-gnu/libswsscommon.so.0.0.0"),
                literal("/usr/lib/x86_64-linux-gnu/libyang.so.3.9.1"),
                literal("/usr/lib/debug/.dwz/x86_64-linux-gnu/libswsscommon.debug"),
                literal("/usr/lib/debug/.dwz/x86_64-linux-gnu/sonic-eventd.debug"),
            ),
        ),
        reason=(
            "We'd have to depend on target/debs to get debug binaries for these, so we don't."
        ),
    ),
    *UNENFORCED_DEB_PIN_RULES,
    *LIBREBOOTGNOI_RULES,
    *NO_DPKG_RULES,
)
