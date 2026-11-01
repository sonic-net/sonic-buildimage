"""Regression coverage for .debs whose recipe fragment was never written.

A derived or extra .deb's fragment is written only by its own rule in
slave.mk, which depends on nothing but the main .deb. The fixture below is
what build_sbom.py saw in a master/broadcom build where make skipped such a
rule: syft's record of platform-modules-ds1000, without the `sonic`
namespace its recipe fragment would have given it.
"""

import contextlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

import build_sbom  # noqa: E402  (needs the path set above)


DS1000_DEB = "platform-modules-ds1000_1.0_amd64.deb"
DX010_DEB = "platform-modules-dx010_1.0_amd64.deb"

# The scanner's record of platform-modules-ds1000, verbatim from the
# sonic-broadcom.bin SBOM of master build 1257418, where its fragment was
# missing.
DS1000_SCANNER_RECORD = {
    "bom-ref": "pkg:deb/platform-modules-ds1000@1.0?arch=amd64&package-id=4df28f3a74915fe4&upstream=sonic-cel-platform-modules",
    "cpe": "cpe:2.3:a:platform-modules-ds1000:platform-modules-ds1000:1.0:*:*:*:*:*:*:*",
    "name": "platform-modules-ds1000",
    "properties": [
        {"name": "syft:package:foundBy", "value": "deb-archive-cataloger"},
        {"name": "syft:package:type", "value": "deb"},
        {"name": "syft:package:metadataType", "value": "dpkg-archive-entry"},
        {"name": "syft:cpe23", "value": "cpe:2.3:a:platform-modules-ds1000:platform_modules_ds1000:1.0:*:*:*:*:*:*:*"},
        {"name": "syft:cpe23", "value": "cpe:2.3:a:platform_modules_ds1000:platform-modules-ds1000:1.0:*:*:*:*:*:*:*"},
        {"name": "syft:cpe23", "value": "cpe:2.3:a:platform_modules_ds1000:platform_modules_ds1000:1.0:*:*:*:*:*:*:*"},
        {"name": "syft:cpe23", "value": "cpe:2.3:a:platform-modules:platform-modules-ds1000:1.0:*:*:*:*:*:*:*"},
        {"name": "syft:cpe23", "value": "cpe:2.3:a:platform-modules:platform_modules_ds1000:1.0:*:*:*:*:*:*:*"},
        {"name": "syft:cpe23", "value": "cpe:2.3:a:platform_modules:platform-modules-ds1000:1.0:*:*:*:*:*:*:*"},
        {"name": "syft:cpe23", "value": "cpe:2.3:a:platform_modules:platform_modules_ds1000:1.0:*:*:*:*:*:*:*"},
        {"name": "syft:cpe23", "value": "cpe:2.3:a:platform:platform-modules-ds1000:1.0:*:*:*:*:*:*:*"},
        {"name": "syft:cpe23", "value": "cpe:2.3:a:platform:platform_modules_ds1000:1.0:*:*:*:*:*:*:*"},
        {"name": "syft:location:0:path", "value": "/platform/common/platform-modules-ds1000_1.0_amd64.deb"},
        {"name": "syft:metadata:installedSize", "value": "1290"},
        {"name": "syft:metadata:source", "value": "sonic-cel-platform-modules"},
        {"name": "sonic:fragment_kind", "value": "scanner"},
        {"name": "sonic:scanner", "value": "syft"},
        {"name": "sonic:scope", "value": "host-image"},
    ],
    "purl": "pkg:deb/platform-modules-ds1000@1.0?arch=amd64&upstream=sonic-cel-platform-modules",
    "type": "library",
    "version": "1.0",
}

# The identity platform-modules-ds1000 has in master build 1256459, where
# its fragment was written.
DS1000_RECIPE_PURL = (
    "pkg:deb/sonic/platform-modules-ds1000@1.0"
    "?arch=amd64&upstream=sonic-cel-platform-modules"
)


DS1000_FALLBACK_PURL = "pkg:deb/sonic/platform-modules-ds1000@1.0?arch=amd64"
SLAVE_MK = (ROOT / "slave.mk").read_text(encoding="utf-8")
FUNCTIONS = (ROOT / "rules" / "functions").read_text(encoding="utf-8")


def write_fragment(debs: Path, deb: str) -> Path:
    """A .deb and, beside it, a fragment in the shape sbom_fragment.py writes."""
    name, version, arch = deb[:-len(".deb")].split("_")
    purl = f"pkg:deb/sonic/{name}@{version}?arch={arch}"
    debs.mkdir(parents=True, exist_ok=True)
    (debs / deb).write_bytes(b"")
    fragment = debs / f"{deb}.cdx.json"
    fragment.write_text(json.dumps({
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "metadata": {"properties": [
            {"name": "sonic:fragment_kind", "value": "recipe-emit"},
        ]},
        "components": [{
            "bom-ref": purl,
            "type": "library",
            "name": name,
            "version": version,
            "purl": purl,
            "properties": [
                {"name": "sonic:fragment_kind", "value": "recipe-emit"},
                {"name": "sonic:artifact_filename", "value": deb},
            ],
        }],
    }), encoding="utf-8")
    return fragment


def make_older(path: Path, than: Path) -> None:
    stamp = os.stat(than).st_mtime - 10
    os.utime(path, (stamp, stamp))


def property_values(component: dict, name: str) -> list:
    return [p["value"] for p in component.get("properties", [])
            if p.get("name") == name]


def make_variable(name: str) -> str:
    """The one-line definition of a slave.mk variable, verbatim."""
    lines = [line for line in SLAVE_MK.splitlines()
             if line.startswith(f"{name} = ")]
    if len(lines) != 1:
        raise AssertionError(f"{name} is not defined once in slave.mk")
    return lines[0]


def make_rule(target_list: str) -> str:
    """A slave.mk rule over $(SONIC_<kind>), from its target line to the
    SONIC_TARGET_LIST line after it, verbatim."""
    head = f"$(addprefix $(DEBS_PATH)/, $({target_list})) : $(DEBS_PATH)/% :"
    if SLAVE_MK.count(head) != 1:
        raise AssertionError(f"the {target_list} rule is not in slave.mk once")
    start = SLAVE_MK.index(head)
    end = SLAVE_MK.index("SONIC_TARGET_LIST", start)
    return SLAVE_MK[start:end]


def make_define(text: str, name: str) -> str:
    match = re.search(rf"^define {name}\n.*?^endef\n", text, re.M | re.S)
    if not match:
        raise AssertionError(f"{name} is not defined in rules/functions")
    return match.group(0)


class FallbackComponentTest(unittest.TestCase):
    def test_missing_fragment_is_named_in_the_sonic_namespace(self):
        component = build_sbom.fallback_component(DS1000_DEB)
        self.assertEqual(component["purl"], DS1000_FALLBACK_PURL)
        self.assertEqual(component["bom-ref"], component["purl"])
        self.assertEqual(property_values(component, "sonic:artifact_filename"),
                         [DS1000_DEB])
        self.assertEqual(property_values(component, "sonic:fragment_kind"),
                         ["recipe-fallback"])

    def test_fallback_version_is_escaped_like_a_fragment(self):
        component = build_sbom.fallback_component("libfoo_1.2-3+fips_amd64.deb")
        self.assertEqual(component["purl"],
                         "pkg:deb/sonic/libfoo@1.2-3%2Bfips?arch=amd64")

    def test_unparseable_filename_has_no_fallback(self):
        self.assertIsNone(build_sbom.fallback_component("not-a-deb.deb"))

    def test_fallback_outranks_the_scanner_record(self):
        merged = build_sbom.merge_components(
            [build_sbom.fallback_component(DS1000_DEB)],
            [json.loads(json.dumps(DS1000_SCANNER_RECORD))],
        )
        self.assertEqual(len(merged), 1)
        component = merged[0]
        self.assertEqual(component["purl"], DS1000_RECIPE_PURL)
        self.assertEqual(component["bom-ref"], DS1000_RECIPE_PURL)
        self.assertEqual(component["cpe"], DS1000_SCANNER_RECORD["cpe"])
        self.assertEqual(property_values(component, "sonic:scope"),
                         ["host-image"])


class CheckFragmentsTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.target = Path(self.directory.name) / "target"
        self.debs = self.target / "debs" / "trixie"
        write_fragment(self.debs, DX010_DEB)

    def tearDown(self):
        self.directory.cleanup()

    def check(self, strict, debs, debs_path=None):
        fragments = build_sbom.FragmentIndex(str(self.target))
        path = str(self.debs) if debs_path is None else debs_path
        stderr = io.StringIO()
        with mock.patch.dict(os.environ, {"SBOM_STRICT": strict}), \
                contextlib.redirect_stderr(stderr):
            try:
                result = build_sbom.check_fragments(debs, fragments, path)
            except build_sbom.SbomInputMissing as e:
                result = e
        return result, stderr.getvalue()

    def test_strict_missing_fragment_is_an_error_naming_the_package(self):
        result, stderr = self.check("y", [DX010_DEB, DS1000_DEB])
        self.assertIsInstance(result, build_sbom.SbomInputMissing)
        self.assertIn("ERROR", stderr)
        self.assertIn(DS1000_DEB, stderr)
        self.assertNotIn(f"{DX010_DEB}:", stderr)
        self.assertIn("-clean", stderr)

    def test_lenient_missing_fragment_warns_and_falls_back(self):
        result, stderr = self.check("n", [DX010_DEB, DS1000_DEB])
        self.assertEqual([c["purl"] for c in result], [DS1000_FALLBACK_PURL])
        self.assertIn("WARNING", stderr)
        self.assertIn(DS1000_DEB, stderr)

    def test_every_fragment_present_passes_strict(self):
        result, stderr = self.check("y", [DX010_DEB])
        self.assertEqual(result, [])
        self.assertEqual(stderr, "")

    def test_non_deb_installer_entries_are_not_checked(self):
        result, _ = self.check("y", [DX010_DEB, "sonic_platform-1.0-py3-none-any.whl"])
        self.assertEqual(result, [])

    def test_fragment_older_than_its_deb_is_stale(self):
        make_older(self.debs / f"{DX010_DEB}.cdx.json", self.debs / DX010_DEB)
        result, stderr = self.check("y", [DX010_DEB])
        self.assertIsInstance(result, build_sbom.SbomInputMissing)
        self.assertIn("older than the .deb", stderr)

    def test_fragment_in_another_distro_does_not_count(self):
        write_fragment(self.target / "debs" / "bookworm", DS1000_DEB)
        result, stderr = self.check("y", [DS1000_DEB])
        self.assertIsInstance(result, build_sbom.SbomInputMissing)
        self.assertIn(DS1000_DEB, stderr)

    def test_without_a_debs_path_any_fragment_of_that_name_counts(self):
        write_fragment(self.target / "debs" / "bookworm", DS1000_DEB)
        result, _ = self.check("y", [DS1000_DEB], debs_path="")
        self.assertEqual(result, [])


class MainTest(unittest.TestCase):
    """build_sbom.py's main(), with no scanner and no external tools."""

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        work = Path(self.directory.name)
        self.target = work / "target"
        self.debs = self.target / "debs" / "trixie"
        (work / "fsroot-test").mkdir()
        write_fragment(self.debs, DX010_DEB)
        (self.debs / DS1000_DEB).write_bytes(b"")
        self.env = {
            "ENABLE_SBOM": "y",
            "TARGET_PATH": str(self.target),
            "TARGET_MACHINE": "test",
            "CONFIGURED_ARCH": "amd64",
            "SBOM_SCAN_TOOL": "none",
            "SBOM_FORMAT": "cyclonedx",
            "SBOM_INCLUDE_LICENSES": "n",
            "SBOM_INSTALLER_DOCKERS": "",
            "SBOM_INSTALLER_DEBS": f"{DX010_DEB} {DS1000_DEB}",
            "SBOM_INSTALLER_DEBS_PATH": str(self.debs),
        }

    def tearDown(self):
        self.directory.cleanup()

    def run_main(self, strict):
        stderr = io.StringIO()
        env = dict(self.env, SBOM_STRICT=strict)
        with mock.patch.dict(os.environ, env), \
                mock.patch.object(sys, "argv", ["build_sbom.py"]), \
                mock.patch.object(build_sbom, "validate_document"), \
                mock.patch.object(build_sbom, "emit_provenance"), \
                contextlib.redirect_stderr(stderr):
            rc = build_sbom.main()
        out = self.target / "sonic-test.bin.cdx.json"
        doc = json.loads(out.read_text(encoding="utf-8")) if out.exists() else None
        return rc, doc, stderr.getvalue()

    def test_strict_main_fails_naming_the_package_and_writes_nothing(self):
        rc, doc, stderr = self.run_main("y")
        self.assertEqual(rc, 1)
        self.assertIsNone(doc)
        self.assertIn(DS1000_DEB, stderr)

    def test_lenient_main_names_the_package_in_the_sonic_namespace(self):
        rc, doc, stderr = self.run_main("n")
        self.assertEqual(rc, 0)
        purls = {c.get("purl") for c in doc["components"]}
        self.assertIn(DS1000_FALLBACK_PURL, purls)
        self.assertIn("pkg:deb/sonic/platform-modules-dx010@1.0?arch=amd64", purls)
        self.assertIn(DS1000_DEB, stderr)

    def test_strict_main_passes_when_every_fragment_is_present(self):
        write_fragment(self.debs, DS1000_DEB)
        rc, doc, _ = self.run_main("y")
        self.assertEqual(rc, 0)
        self.assertIn(DS1000_FALLBACK_PURL, {c.get("purl") for c in doc["components"]})
        kinds = {property_values(c, "sonic:fragment_kind")[0]
                 for c in doc["components"] if c.get("name", "").startswith("platform-modules-")}
        self.assertEqual(kinds, {"recipe-emit"})


class FragmentScriptTest(unittest.TestCase):
    """sbom_fragment.py's exit status, which a strict recipe stops on."""

    def run_script(self, artifact, recipe_type="EXTRA_DEB"):
        env = dict(os.environ, ENABLE_SBOM="y", ARTIFACT=str(artifact),
                   RECIPE_TYPE=recipe_type)
        return subprocess.run(
            [sys.executable, str(ROOT / "scripts" / "sbom_fragment.py")],
            env=env, capture_output=True, text=True,
        ).returncode

    def run_main(self, env):
        import sbom_fragment
        with mock.patch.dict(os.environ, env, clear=True), \
                contextlib.redirect_stderr(io.StringIO()):
            return sbom_fragment.main()

    def test_written_fragment_exits_zero(self):
        with tempfile.TemporaryDirectory() as directory:
            deb = Path(directory) / DS1000_DEB
            deb.write_bytes(b"")
            self.assertEqual(self.run_script(deb), 0)
            self.assertTrue(Path(f"{deb}.cdx.json").exists())

    def test_unwritable_fragment_exits_nonzero_and_leaves_none(self):
        with tempfile.TemporaryDirectory() as directory:
            deb = Path(directory) / DS1000_DEB
            deb.write_bytes(b"")
            Path(f"{deb}.cdx.json.tmp").mkdir()
            self.assertEqual(self.run_script(deb), 1)
            self.assertFalse(Path(f"{deb}.cdx.json").exists())

    def test_failed_write_removes_its_temporary_file(self):
        import sbom_fragment
        with tempfile.TemporaryDirectory() as directory:
            deb = Path(directory) / DS1000_DEB
            deb.write_bytes(b"")
            env = {"ENABLE_SBOM": "y", "ARTIFACT": str(deb), "RECIPE_TYPE": "EXTRA_DEB"}
            with mock.patch.object(sbom_fragment.json, "dump",
                                   side_effect=OSError("disk full")):
                self.assertEqual(self.run_main(env), 1)
            self.assertEqual(sorted(p.name for p in Path(directory).iterdir()),
                             [DS1000_DEB])

    def test_fragment_construction_failure_exits_nonzero(self):
        import sbom_fragment
        with tempfile.TemporaryDirectory() as directory:
            deb = Path(directory) / DS1000_DEB
            deb.write_bytes(b"")
            env = {"ENABLE_SBOM": "y", "ARTIFACT": str(deb), "RECIPE_TYPE": "EXTRA_DEB"}
            with mock.patch.object(sbom_fragment, "build_fragment",
                                   side_effect=RuntimeError("boom")):
                self.assertEqual(self.run_main(env), 1)

    def test_missing_artifact_or_recipe_type_exits_nonzero(self):
        with tempfile.TemporaryDirectory() as directory:
            deb = Path(directory) / DS1000_DEB
            deb.write_bytes(b"")
            self.assertEqual(self.run_main({"ENABLE_SBOM": "y", "ARTIFACT": str(deb)}), 1)
            self.assertEqual(self.run_main({"ENABLE_SBOM": "y", "RECIPE_TYPE": "EXTRA_DEB"}), 1)

    def test_missing_artifact_is_not_a_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(self.run_script(Path(directory) / DS1000_DEB), 0)

    def test_copied_deb_is_named_like_a_downloaded_one(self):
        with tempfile.TemporaryDirectory() as directory:
            deb = Path(directory) / "arista-fwutil_202511.9_amd64.deb"
            deb.write_bytes(b"")
            self.assertEqual(self.run_script(deb, "COPY_DEB"), 0)
            copied = json.loads(Path(f"{deb}.cdx.json").read_text(encoding="utf-8"))
            self.assertEqual(self.run_script(deb, "ONLINE_DEB"), 0)
            online = json.loads(Path(f"{deb}.cdx.json").read_text(encoding="utf-8"))
        self.assertEqual(copied["components"][0]["purl"],
                         "pkg:deb/sonic/arista-fwutil@202511.9?arch=amd64")
        self.assertEqual(copied["components"][0]["purl"],
                         online["components"][0]["purl"])


def active_recipe(rule: str) -> list:
    """A rule's recipe lines that run, each with the shell `if` depth it
    runs at. Comment lines are dropped; a line inside an `if` block runs
    only on that branch."""
    lines, depth = [], 0
    for line in rule.splitlines()[1:]:
        if not line.startswith("\t"):
            continue
        text = line.strip()
        if not text or text.startswith("#"):
            continue
        if re.match(r"(fi|fi\s*;.*)$", text):
            depth -= 1
            continue
        lines.append((text, depth))
        if re.match(r"if\b.*\bthen\s*$", text):
            depth += 1
    return lines


class RecipeTextTest(unittest.TestCase):
    """The slave.mk rules make cannot run here, held to their text."""

    MAIN_RULES = (("SONIC_MAKE_DEBS", "MAKE_DEB"),
                  ("SONIC_DPKG_DEBS", "DPKG_DEB"),
                  ("SONIC_ONLINE_DEBS", "ONLINE_DEB"),
                  ("SONIC_COPY_DEBS", "COPY_DEB"))

    def test_each_main_deb_recipe_dates_its_companions_on_every_path(self):
        for target_list, recipe_type in self.MAIN_RULES:
            with self.subTest(target_list):
                recipe = active_recipe(make_rule(target_list))
                self.assertIn(("$(call main_deb_newer_than_companions,$*)", 0), recipe,
                              "companions are not dated outside the cache branch")
                emits = [depth for text, depth in recipe if text.startswith(
                    f"$(call sbom_emit_fragment,$(DEBS_PATH)/$*,{recipe_type},")]
                self.assertEqual(emits, [0], "the main deb's fragment is not emitted on every path")

    def test_recipe_parser_sees_the_cache_branch(self):
        recipe = active_recipe(make_rule("SONIC_MAKE_DEBS"))
        self.assertIn(("$(call SAVE_CACHE,$*,$@)", 1), recipe)

    def test_installer_requires_and_reports_its_image_debs(self):
        installer = SLAVE_MK[SLAVE_MK.index("$(addprefix $(TARGET_PATH)/, $(SONIC_INSTALLERS)) : $(TARGET_PATH)/% :"):]
        prerequisites = installer[:installer.index("\t$(HEADER)")]
        self.assertIn("$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(SONIC_INSTALLER_IMAGE_DEBS))",
                      prerequisites)
        reported = re.search(r'SBOM_INSTALLER_DEBS="([^"]*)"', installer).group(1).split()
        for listed in ("$($*_INSTALLS)", "$($*_LAZY_INSTALLS)",
                       "$($*_LAZY_BUILD_INSTALLS)", "$(SONIC_INSTALLER_IMAGE_DEBS)"):
            self.assertIn(listed, reported)
        self.assertIn('SBOM_INSTALLER_DEBS_PATH="$(IMAGE_DISTRO_DEBS_PATH)"', installer)

    def test_image_debs_include_the_kernel_and_fips_installers(self):
        start = SLAVE_MK.index("SONIC_INSTALLER_IMAGE_DEBS = ")
        image_debs = SLAVE_MK[start:SLAVE_MK.index("\n\n", start)]
        for deb in ("$(LINUX_KERNEL)", "$(OPENSSH_SERVER)", "$(BASH)",
                    "$(SONIC_INSTALLER_EXTRA_DEBS)", "$(FIPS_BASEIMAGE_INSTALLERS)"):
            self.assertIn(deb, image_debs)


# Stands in for scripts/sbom_fragment.py: fails once for the artifact named
# by FAIL_ONCE while FAIL_ONCE_MARKER exists, and otherwise runs the real
# script, copied beside it.
FAIL_ONCE_SCRIPT = """\
import os, runpy, sys
marker = os.environ.get("FAIL_ONCE_MARKER", "")
if (marker and os.path.exists(marker)
        and os.path.basename(os.environ.get("ARTIFACT", "")) == os.environ.get("FAIL_ONCE")):
    os.remove(marker)
    sys.exit(1)
runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            "real_sbom_fragment.py"), run_name="__main__")
"""


@unittest.skipUnless(shutil.which("make"), "GNU make is required")
class CompanionRuleTest(unittest.TestCase):
    """The derived and extra rules, verbatim from slave.mk, run through make.

    The main recipes here stand in for slave.mk's, which make cannot run
    outside a build. Each writes its main .deb and then, in the same tick,
    its companions, except one derived deb it does not produce, and calls
    the slave.mk macro that dates them.
    RecipeTextTest holds the real main recipes to the same call. The goals
    visit one companion of each main before its recipe runs and the others
    after, which is the order that left a companion's rule skipped. One main
    has only extras, as in the incident, and the other only derived debs.
    """

    EXTRAS_MAIN = DX010_DEB
    WANTED_EXTRAS = ("platform-modules-haliburton_1.0_amd64.deb", DS1000_DEB)
    UNWANTED_EXTRA = "platform-modules-ds3000_1.0_amd64.deb"
    DERIVED_MAIN = "libfoo_1.0_amd64.deb"
    WANTED_DERIVED = ("libfoo-dev_1.0_amd64.deb", "python3-foo_1.0_amd64.deb")
    UNWANTED_DERIVED = "libfoo-dbgsym_1.0_amd64.deb"

    MAKEFILE = """\
.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e
.SECONDEXPANSION:
DEBS_PATH = debs
ENABLE_SBOM = y
SBOM_STRICT = y
.DEFAULT_GOAL := all
HEADER = :
FOOTER = :
{definitions}
SONIC_MAKE_DEBS = {extras_main} {derived_main}
$(eval $(call add_extra_package,{extras_main},{extra1}))
$(eval $(call add_extra_package,{extras_main},{extra2}))
$(eval $(call add_extra_package,{extras_main},{extra3}))
$(eval $(call add_derived_package,{derived_main},{derived1}))
$(eval $(call add_derived_package,{derived_main},{derived2}))
$(eval $(call add_derived_package,{derived_main},{derived3}))

all: $(DEBS_PATH)/{extra1} $(DEBS_PATH)/{derived1} $(DEBS_PATH)/{extra2} $(DEBS_PATH)/{derived2}

.platform:
\ttouch -d @1000000000 $@

$(addprefix $(DEBS_PATH)/, $(SONIC_MAKE_DEBS)) : $(DEBS_PATH)/% : .platform
\tmkdir -p $(DEBS_PATH)
\ttouch $(DEBS_PATH)/$* $(addprefix $(DEBS_PATH)/,$(filter-out {derived3},$($*_EXTRA_DEBS) $($*_DERIVED_DEBS)))
\ttouch $@.recipe-ran
\t$(call main_deb_newer_than_companions,$*)
\tfor f in $(DEBS_PATH)/$* $(addprefix $(DEBS_PATH)/,$($*_EXTRA_DEBS) $($*_DERIVED_DEBS)); do [ ! -e $$f ] || stat -c '%n %.9Y' $$f; done > $@.dated
\t$(call sbom_emit_fragment,$(DEBS_PATH)/$*,MAKE_DEB,,,,,)

{derived_rule}
{extra_rule}
"""

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.work = Path(self.directory.name)
        self.debs = self.work / "debs"
        scripts = self.work / "scripts"
        scripts.mkdir()
        for script in ("sbom_fragment.py", "sbom_cve_refs.py", "sbom_purl.py"):
            shutil.copy(ROOT / "scripts" / script, scripts / script)
        definitions = "\n".join((
            make_define(FUNCTIONS, "add_derived_package"),
            make_define(FUNCTIONS, "add_extra_package"),
            make_variable("sbom_emit_fragment"),
            make_variable("main_deb_newer_than_companions"),
            make_variable("sbom_fragment_missing"),
            ".PHONY: sbom-fragment-missing",
            "sbom-fragment-missing: ;",
        ))
        (self.work / "Makefile").write_text(self.MAKEFILE.format(
            definitions=definitions,
            extras_main=self.EXTRAS_MAIN, derived_main=self.DERIVED_MAIN,
            extra1=self.WANTED_EXTRAS[0], extra2=self.WANTED_EXTRAS[1],
            extra3=self.UNWANTED_EXTRA,
            derived1=self.WANTED_DERIVED[0], derived2=self.WANTED_DERIVED[1],
            derived3=self.UNWANTED_DERIVED,
            derived_rule=make_rule("SONIC_DERIVED_DEBS"),
            extra_rule=make_rule("SONIC_EXTRA_DEBS"),
        ), encoding="utf-8")

    def tearDown(self):
        self.directory.cleanup()

    def make(self, jobs=1, **env):
        clean = {k: v for k, v in os.environ.items()
                 if k not in ("MAKEFLAGS", "MFLAGS", "MAKELEVEL")}
        return subprocess.run(["make", f"-j{jobs}"], cwd=self.work,
                              env=dict(clean, **env), capture_output=True, text=True)

    def assert_fragments(self):
        fragments = {}
        for deb in (self.EXTRAS_MAIN, self.DERIVED_MAIN,
                    *self.WANTED_EXTRAS, *self.WANTED_DERIVED):
            fragment = self.debs / f"{deb}.cdx.json"
            self.assertTrue(fragment.exists(), f"no fragment for {deb}")
            self.assertGreaterEqual(os.stat(fragment).st_mtime_ns,
                                    os.stat(self.debs / deb).st_mtime_ns,
                                    f"the fragment for {deb} is older than it")
            fragments[deb] = json.loads(fragment.read_text(encoding="utf-8"))
        for deb in (self.UNWANTED_EXTRA, self.UNWANTED_DERIVED):
            self.assertFalse((self.debs / f"{deb}.cdx.json").exists(),
                             f"{deb} is not wanted and has a fragment")
        self.assertFalse((self.debs / self.UNWANTED_DERIVED).exists(),
                         "dating the companions created a deb the recipe did not write")
        for main, companions, recipe_type in (
                (self.EXTRAS_MAIN, self.WANTED_EXTRAS, "EXTRA_DEB"),
                (self.DERIVED_MAIN, self.WANTED_DERIVED, "DERIVED_DEB")):
            for deb in companions:
                component = fragments[deb]["components"][0]
                self.assertEqual(property_values(component, "sonic:recipe_type"),
                                 [recipe_type], deb)
                self.assertEqual(property_values(component, "sonic:parent_artifact"),
                                 [main], deb)
        self.assertEqual(fragments[DS1000_DEB]["components"][0]["purl"],
                         DS1000_FALLBACK_PURL)

    def test_wanted_companions_get_their_fragments_and_unwanted_ones_none(self):
        for jobs in (1, 4):
            with self.subTest(jobs=jobs):
                if self.debs.exists():
                    shutil.rmtree(self.debs)
                run = self.make(jobs)
                self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
                self.assert_fragments()

    def test_companions_are_dated_a_second_before_their_main_deb(self):
        # A second, so the order holds on a filesystem whose timestamps
        # have one-second resolution, and whatever tick the writes land in.
        run = self.make()
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        for main in (self.EXTRAS_MAIN, self.DERIVED_MAIN):
            dated = dict(line.rsplit(" ", 1) for line in
                         (self.debs / f"{main}.dated").read_text(encoding="utf-8").splitlines())
            main_time = float(dated.pop(f"debs/{main}"))
            self.assertTrue(dated, "no companion was dated, so this checked nothing")
            for companion, stamp in dated.items():
                self.assertLessEqual(float(stamp), main_time - 1, companion)

    def test_retry_after_a_failed_fragment_writes_it(self):
        for failing in (DS1000_DEB, self.WANTED_DERIVED[1], self.DERIVED_MAIN):
            with self.subTest(failing=failing):
                if self.debs.exists():
                    shutil.rmtree(self.debs)
                scripts = self.work / "scripts"
                shutil.copy(ROOT / "scripts" / "sbom_fragment.py",
                            scripts / "real_sbom_fragment.py")
                (scripts / "sbom_fragment.py").write_text(FAIL_ONCE_SCRIPT, encoding="utf-8")
                marker = self.work / "fail-once"
                marker.write_text("", encoding="utf-8")
                env = {"FAIL_ONCE": failing, "FAIL_ONCE_MARKER": str(marker)}
                first = self.make(**env)
                self.assertNotEqual(first.returncode, 0, "the injected failure did not fail make")
                self.assertFalse(marker.exists())
                retry = self.make(**env)
                self.assertEqual(retry.returncode, 0, retry.stdout + retry.stderr)
                self.assert_fragments()

    def test_companion_without_its_fragment_is_rebuilt_in_an_old_workspace(self):
        run = self.make()
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        lost = self.debs / f"{DS1000_DEB}.cdx.json"
        lost.unlink()
        for ran in self.debs.glob("*.recipe-ran"):
            ran.unlink()
        run = self.make()
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        self.assertEqual(list(self.debs.glob("*.recipe-ran")), [],
                         "a main recipe ran, so this checked nothing")
        self.assert_fragments()


if __name__ == "__main__":
    unittest.main()
