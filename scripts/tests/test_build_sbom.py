import importlib.util
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest


SCRIPTS = Path(__file__).parents[1]
sys.path.insert(0, str(SCRIPTS))
SPEC = importlib.util.spec_from_file_location(
    "build_sbom", SCRIPTS / "build_sbom.py"
)
build_sbom = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(build_sbom)


REPO = "pkg:github/sonic-net/sonic-platform-daemons@32927500100e"


def artifact(purl, name, filename):
    return {
        "bom-ref": purl, "type": "library", "name": name, "purl": purl,
        "version": purl.rsplit("@", 1)[-1],
        "properties": [
            {"name": "sonic:fragment_kind", "value": "recipe-emit"},
            {"name": "sonic:artifact_filename", "value": filename},
        ],
    }


def source(purl, filename):
    """The repository component sbom_fragment.py emits beside an artifact."""
    return {
        "bom-ref": purl, "type": "library", "purl": purl,
        "name": purl.split("/")[-1].split("@")[0],
        "version": purl.rsplit("@", 1)[-1],
        "externalReferences": [{"type": "vcs", "url": "https://example"}],
        "properties": [
            {"name": "sonic:fragment_kind", "value": "recipe-emit-source"},
            {"name": "sonic:source_deb", "value": filename},
        ],
    }


def write_fragment(path, *components):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump({
            "bomFormat": "CycloneDX",
            "metadata": {"properties": [
                {"name": "sonic:fragment_kind", "value": "recipe-emit"},
            ]},
            "components": list(components),
        }, f)


def parents(deps, ref):
    return sorted(d["ref"] for d in deps if ref in d["dependsOn"])


class SourceRepositoryGraphTest(unittest.TestCase):

    def test_every_artifact_built_from_one_repository_depends_on_it(self):
        xcvrd = "sonic_xcvrd-1.0-py3-none-any.whl"
        psud = "sonic_psud-1.0-py3-none-any.whl"
        comps = build_sbom.merge_components([
            artifact("pkg:pypi/sonic-xcvrd@1.0", "sonic-xcvrd", xcvrd),
            source(REPO, xcvrd),
            artifact("pkg:pypi/sonic-psud@1.0", "sonic-psud", psud),
            source(REPO, psud),
        ])
        self.assertEqual(
            [c["purl"] for c in comps if c["purl"] == REPO], [REPO])
        deps = build_sbom.build_dependency_graph(comps)
        self.assertEqual(parents(deps, REPO),
                         ["pkg:pypi/sonic-psud@1.0", "pkg:pypi/sonic-xcvrd@1.0"])

    def test_the_repository_deb_outranks_a_source_component_found_first(self):
        # The wheel's fragment sits higher in target/ than the .deb's,
        # so os.walk reads it first.
        repo = "pkg:github/sonic-net/sonic-host-services@9f8a0566acb8"
        whl = "sonic_host_services-1.0-py3-none-any.whl"
        deb = "sonic-host-services-rs_1.0.0_amd64.deb"
        with tempfile.TemporaryDirectory() as target:
            write_fragment(
                os.path.join(target, "python-wheels", f"{whl}.cdx.json"),
                artifact("pkg:pypi/sonic-host-services@1.0",
                         "sonic-host-services", whl),
                source(repo, whl),
            )
            write_fragment(
                os.path.join(target, "python-wheels", "debs", f"{deb}.cdx.json"),
                artifact(repo, "sonic-host-services-rs", deb),
            )
            comps = build_sbom.merge_components(
                build_sbom.FragmentIndex(target).all)
        [merged] = [c for c in comps if c["purl"] == repo]
        self.assertEqual(merged["name"], "sonic-host-services-rs")
        self.assertEqual(
            build_sbom._property(merged, "sonic:artifact_filename"), deb)
        deps = build_sbom.build_dependency_graph(comps)
        self.assertEqual(parents(deps, repo),
                         ["pkg:pypi/sonic-host-services@1.0"])

    def test_a_crate_two_debs_ship_hangs_off_both(self):
        crate = "pkg:cargo/tokio@1.47.1"

        def dep(filename):
            return {
                "bom-ref": crate, "type": "library", "name": "tokio",
                "version": "1.47.1", "purl": crate,
                "properties": [
                    {"name": "sonic:fragment_kind", "value": "recipe-emit-rust"},
                    {"name": "sonic:source_deb", "value": filename},
                ],
            }

        comps = build_sbom.merge_components([
            artifact("pkg:github/sonic-net/sonic-swss@652e3bdb6caf",
                     "swss", "swss_1.0.0_amd64.deb"),
            dep("swss_1.0.0_amd64.deb"),
            artifact("pkg:github/sonic-net/sonic-dash-ha@5a2508a40456",
                     "sonic-dash-ha", "sonic-dash-ha_1.0.0_amd64.deb"),
            dep("sonic-dash-ha_1.0.0_amd64.deb"),
        ])
        deps = build_sbom.build_dependency_graph(comps)
        self.assertEqual(parents(deps, crate), [
            "pkg:github/sonic-net/sonic-dash-ha@5a2508a40456",
            "pkg:github/sonic-net/sonic-swss@652e3bdb6caf",
        ])


if __name__ == "__main__":
    unittest.main()
