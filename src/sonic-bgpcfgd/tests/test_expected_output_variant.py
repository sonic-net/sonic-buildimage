from .util import resolve_expected_output


def test_resolve_expected_output_uses_public_path_by_default(monkeypatch, tmp_path):
    public_path = tmp_path / 'expected.conf'
    public_path.write_text('public')
    monkeypatch.delenv('SONIC_CFGGEN_OUTPUT_VARIANT', raising=False)

    assert resolve_expected_output(str(public_path)) == str(public_path)


def test_resolve_expected_output_uses_existing_variant(monkeypatch, tmp_path):
    public_path = tmp_path / 'expected.conf'
    internal_path = tmp_path / 'expected_internal.conf'
    public_path.write_text('public')
    internal_path.write_text('internal')
    monkeypatch.setenv('SONIC_CFGGEN_OUTPUT_VARIANT', 'internal')

    assert resolve_expected_output(str(public_path)) == str(internal_path)


def test_resolve_expected_output_falls_back_to_public(monkeypatch, tmp_path):
    public_path = tmp_path / 'expected.conf'
    public_path.write_text('public')
    monkeypatch.setenv('SONIC_CFGGEN_OUTPUT_VARIANT', 'internal')

    assert resolve_expected_output(str(public_path)) == str(public_path)
