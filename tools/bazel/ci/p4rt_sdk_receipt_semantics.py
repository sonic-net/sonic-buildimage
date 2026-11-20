"""Verify typed SDK receipts; selected JSON only, no payload parsing or execution."""
import re
from pathlib import PurePosixPath


def verify(read, *, source, owner, preparation_targets, common_revision):
    view = read('sdk-preparation.json')
    owner_source = read('owner-source.json')
    public = read('public-receipt.json')
    assert set(view) == {'schema', 'status', 'revision', 'owner_revision', 'architecture',
                         'artifacts', 'action_audit', 'sdk_build', 'parent_common', 'original_sha256'}
    assert type(view['schema']) is int and view['schema'] == 1 and view['status'] == 'prepared'
    assert view['revision'] == source and view['owner_revision'] == owner_source['commit'] == owner
    assert view['architecture'] == owner_source['architecture'] == 'amd64'
    assert owner_source['bazel'] == '6.4.0'
    def digest(value):
        assert isinstance(value, str) and re.fullmatch('[0-9a-f]{64}', value)
        return value
    def natural(value):
        assert type(value) is int and value >= 0
        return value
    artifacts = view['artifacts']
    assert set(artifacts) == set(preparation_targets) and len(artifacts) == 10
    for name, item in artifacts.items():
        assert set(item) == {'target', 'bytes', 'sha256'}
        assert item['target'] == preparation_targets[name] and natural(item['bytes']) > 0
        digest(item['sha256'])
    audit = view['action_audit']
    assert set(audit) == {'schema', 'targets', 'deb_outputs', 'packaging_wrappers', 'action_count', 'output_count'}
    assert type(audit['schema']) is int and audit['schema'] == 1 and audit['deb_outputs'] == audit['packaging_wrappers'] == []
    assert len(audit['targets']) == len(set(audit['targets'])) == 10
    assert set(audit['targets']) == set(preparation_targets.values())
    natural(audit['action_count']); natural(audit['output_count'])
    originals = view['original_sha256']
    assert set(originals) == {'owner-preparation.json', 'owner-preparation/action-audit.json', 'owner-sdk/sdk-build.json'}
    for value in originals.values(): digest(value)
    inventory = owner_source['sdk_inventory']
    assert inventory['sdk-build.json']['kind'] == 'file'
    assert inventory['sdk-build.json']['sha256'] == originals['owner-sdk/sdk-build.json']
    parent = view['parent_common']
    assert set(parent) == {'sha256', 'common_runtime_tar_sha256', 'parent_source_tar_sha256'}
    assert all(public['owner_handoff']['parent_common'][key] == value for key, value in parent.items())
    assert parent['parent_source_tar_sha256'] == artifacts['config-engine.source.tar']['sha256']
    assert parent['common_runtime_tar_sha256'] == artifacts['common-runtime.tar']['sha256']
    assert inventory['lib/libswsscommon.so.0']['kind'] == 'file'
    assert digest(parent['sha256']) == inventory['lib/libswsscommon.so.0']['sha256']
    sdk = view['sdk_build']
    assert set(sdk) == {'kind', 'architecture', 'producer_bazel_version', 'producer_target', 'archive_sha256',
                        'common_source_commit', 'common_source_has_tracked_changes', 'common_module_sha256',
                        'image_source_revision', 'yang_modules', 'libnl', 'libyang'}
    assert sdk['kind'] == 'sonic-swss-common-bazel-sdk' and sdk['architecture'] == 'amd64'
    assert sdk['producer_bazel_version'] == '8.5.1'
    assert sdk['producer_target'] == preparation_targets['common-sdk.tar']
    assert sdk['archive_sha256'] == artifacts['common-sdk.tar']['sha256']
    assert sdk['common_module_sha256'] == artifacts['common.MODULE.bazel']['sha256']
    assert re.fullmatch('[0-9a-f]{40}', common_revision) and sdk['common_source_commit'] == common_revision
    assert sdk['image_source_revision'] == source and sdk['yang_modules'] is True
    assert sdk['common_source_has_tracked_changes'] is False
    prefix = 'usr/lib/x86_64-linux-gnu/'
    library_files = {}
    for component, archives, stems, suffix, real_suffix in (
        ('libnl', ('libnl-nf-sdk.tar', 'libnl-genl-sdk.tar'),
         ('libnl-genl-3', 'libnl-nf-3', 'libnl-route-3', 'libnl-3'), '.so.200', '.26.0'),
        ('libyang', ('libyang-runtime.tar',), ('libyang',), '.so.3', '.9.1'),
    ):
        record = sdk[component]
        assert set(record) == {'producer_targets', 'parent_source_tar_sha256', 'files'}
        assert record['producer_targets'] == {preparation_targets[name]: artifacts[name]['sha256'] for name in archives}
        assert record['parent_source_tar_sha256'] == parent['parent_source_tar_sha256']
        required = {prefix + stem + suffix for stem in stems} | {prefix + stem + suffix + real_suffix for stem in stems}
        aliases = {prefix + stem + '.so' for stem in stems} if component == 'libnl' else set()
        # These exact declared aliases are the repaired link inputs, not optional metadata.
        required |= aliases
        files = record['files']; assert required <= files.keys()
        if component == 'libnl':
            assert files['usr/include/libnl3/netlink/netlink.h']['kind'] == 'file'
        for name, item in files.items():
            path = PurePosixPath(name)
            assert not path.is_absolute() and '..' not in path.parts and str(path) == name
            assert name in required or component == 'libnl' and name.startswith('usr/include/libnl3/')
            sdk_name = 'lib/' + name.removeprefix(prefix) if name.startswith(prefix) else name.removeprefix('usr/')
            installed = inventory[sdk_name]
            assert item['kind'] == installed['kind']
            if name in aliases or name in {prefix + stem + suffix for stem in stems}:
                assert set(item) == {'kind', 'linkname'} and item['kind'] == 'symlink'
                stem = name.removeprefix(prefix).removesuffix('.so') if name in aliases else name.removeprefix(prefix).removesuffix(suffix)
                expected = stem + suffix if name in aliases else stem + suffix + real_suffix
                assert item['linkname'] == installed['target'] == expected
            else:
                expected_keys = {'kind', 'sha256'} | ({'elf_machine'} if name in required else set())
                assert set(item) == expected_keys and item['kind'] == 'file'
                assert digest(item['sha256']) == installed['sha256']
                if name in required: assert item['elf_machine'] == 62
        library_files[component] = len(files)
