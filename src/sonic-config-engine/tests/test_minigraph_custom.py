from mock import MagicMock

import minigraph_ext


class MinigraphCustom:
    @staticmethod
    def parse_xml(parse_xml, *args, **kwargs):
        results = parse_xml(*args, **kwargs)
        results["CUSTOM"] = {"enabled": "true"}
        return results


def test_default_parser_adapter_is_noop(monkeypatch):
    expected = {"DEVICE_METADATA": {"localhost": {}}}
    parse_xml = MagicMock(return_value=expected)
    monkeypatch.setattr(minigraph_ext, "minigraph_custom", None)
    monkeypatch.setattr(minigraph_ext.minigraph, "parse_xml", parse_xml)

    assert minigraph_ext.parse_xml("minigraph.xml") is expected
    parse_xml.assert_called_once_with("minigraph.xml")


def test_optional_parser_adapter(monkeypatch):
    parse_xml = MagicMock(
        return_value={"DEVICE_METADATA": {"localhost": {}}}
    )
    monkeypatch.setattr(minigraph_ext, "minigraph_custom", MinigraphCustom)
    monkeypatch.setattr(minigraph_ext.minigraph, "parse_xml", parse_xml)

    results = minigraph_ext.parse_xml(
        "minigraph.xml",
        port_config_file="port_config.ini",
    )

    assert results["CUSTOM"] == {"enabled": "true"}
    parse_xml.assert_called_once_with(
        "minigraph.xml",
        port_config_file="port_config.ini",
    )
