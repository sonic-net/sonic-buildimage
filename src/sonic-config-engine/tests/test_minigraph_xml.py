import os
from unittest import TestCase

from lxml import etree as ET
from mock import patch

import minigraph


class RecordingResolver(ET.Resolver):
    def __init__(self):
        self.requests = []

    def resolve(self, url, public_id, context):
        self.requests.append(url)
        return self.resolve_string("F070-ENTITY-CONTENT", context)


class TestMinigraphXmlParser(TestCase):
    def get_parser(self, read_xml=minigraph.parse_xml, *args):
        # Capture parsing before platform-specific configuration processing.
        with patch.object(ET, "parse", side_effect=StopIteration) as parse, \
                patch.object(os.path, "isfile", return_value=True):
            with self.assertRaises(StopIteration):
                read_xml("minigraph.xml", *args)
        return parse.call_args[1]["parser"]

    def test_explicit_parser_options(self):
        readers = [
            (minigraph.parse_xml, ()),
            (minigraph.parse_hostname, ()),
            (minigraph.parse_asic_sub_role, ("asic0",)),
            (minigraph.parse_asic_switch_type, ("asic0", "test-host")),
        ]
        for read_xml, args in readers:
            with patch.object(
                ET, "XMLParser", wraps=ET.XMLParser
            ) as constructor:
                self.get_parser(read_xml, *args)
            constructor.assert_called_once_with(
                resolve_entities=False, no_network=True
            )

    def test_internal_entity_is_not_expanded(self):
        root = ET.fromstring(
            b'<!DOCTYPE root [<!ENTITY value "F070-ENTITY-CONTENT">]>'
            b'<root>&value;</root>',
            self.get_parser(),
        )
        self.assertIsNone(root.text)
        self.assertEqual(len(list(root.iter(ET.Entity))), 1)

    def assert_external_entity_is_not_resolved(self, url):
        parser = self.get_parser()
        resolver = RecordingResolver()
        parser.resolvers.add(resolver)
        xml = (
            '<!DOCTYPE root [<!ENTITY value SYSTEM "{}">]>'
            '<root>&value;</root>'
        ).format(url)
        root = ET.fromstring(xml.encode("utf-8"), parser)
        self.assertIsNone(root.text)
        self.assertEqual(len(list(root.iter(ET.Entity))), 1)
        self.assertEqual(resolver.requests, [])

    def test_local_file_entity_is_not_resolved(self):
        self.assert_external_entity_is_not_resolved(
            "file:///f070-test-entity"
        )

    def test_network_entity_is_not_resolved(self):
        self.assert_external_entity_is_not_resolved(
            "http://example.invalid/f070-test-entity"
        )

    def test_predefined_entities_remain_supported(self):
        root = ET.fromstring(
            b'<root>A &amp; B &lt; C &gt; D</root>', self.get_parser()
        )
        self.assertEqual(root.text, "A & B < C > D")

    def test_existing_minigraph_tree_is_unchanged(self):
        filename = os.path.join(
            os.path.dirname(__file__), "simple-sample-graph.xml"
        )
        expected = ET.parse(filename)
        actual = ET.parse(filename, parser=self.get_parser())
        self.assertEqual(ET.tostring(actual), ET.tostring(expected))

    def test_malformed_xml_still_fails(self):
        with self.assertRaises(ET.XMLSyntaxError):
            ET.fromstring(b"<root>", self.get_parser())
