"""A golden test for the report a classified run renders.

The JSON report is what CI publishes and what anyone reading a failed build opens, so
its shape is an interface. This pins that shape: fixture artifacts, fixture rules and
fixture diagnostics go in, and the text that comes out has to match the golden below.

To adopt a deliberate change to the report, print the new text and paste it over the
golden it replaces, rather than editing the golden by hand:

    bazel run //tools/bazel/equivalence_checker:reporter_test -- --print
"""

import sys
import tempfile
import unittest
from pathlib import Path

import reporter
import rules_engine
from context import Context
from diagnostics import (
    ArtifactIdentifier,
    ArtifactIndex,
    ArtifactType,
    Codes,
    ComparableArtifact,
    Diagnostic,
    DiagnosticSink,
    Modifier,
)
from rules_engine import AcceptanceRule, DiagnosticMatcher, Rules
from tools import Bazel, Tool, Tools

DEB = ArtifactIdentifier(
    name="@example//:example_deb", source=None, modifiers=frozenset()
)
BINARY = ArtifactIdentifier(
    name="/usr/bin/example", source=DEB, modifiers=frozenset({Modifier.RUNTIME})
)
BINARY_DEBUG = ArtifactIdentifier(
    name="/usr/bin/example", source=DEB, modifiers=frozenset({Modifier.DEBUG})
)
CHANGELOG = ArtifactIdentifier(
    name="/usr/share/doc/example/changelog.gz", source=DEB, modifiers=frozenset()
)

RULES = Rules(
    AcceptanceRule(
        id="changelogs",
        matcher=DiagnosticMatcher(codes=Codes.EXTRACTION_MAKE_ONLY, name="*/changelog.gz"),
        reason="Bazel debs carry no changelog.",
    ),
    AcceptanceRule(
        id="added-imports",
        matcher=DiagnosticMatcher(codes=Codes.ELFCOMPARE_IMPORT_ADDED, name="/usr/bin/example"),
        reason="A static link imports more.",
    ),
)


def artifact(
    identifier: ArtifactIdentifier, kind: ArtifactType, path: str
) -> ComparableArtifact:
    """One pair of paths to compare, named relative to the repo root.

    Relative paths keep the rendered report the same on every machine.
    """
    return ComparableArtifact(
        identifier=identifier,
        bazelVersion=Path("bazel-out/example") / path,
        makeVersion=Path("target/example") / path,
        type=kind,
    )


ARTIFACTS = [
    artifact(DEB, ArtifactType.DEB, "example.deb"),
    artifact(BINARY, ArtifactType.ELF_EXECUTABLE, "usr/bin/example"),
    artifact(BINARY_DEBUG, ArtifactType.ELF_DEBUG_INFO, "usr/bin/example.debug"),
    artifact(CHANGELOG, ArtifactType.FILE, "usr/share/doc/example/changelog.gz"),
]

DIAGNOSTICS = [
    # Accepted by the rules above: one per rule, and one rule twice, so the report
    # has to group as well as label.
    Diagnostic(CHANGELOG, Codes.EXTRACTION_MAKE_ONLY.value, "only Make ships this file"),
    Diagnostic(BINARY, Codes.ELFCOMPARE_IMPORT_ADDED.value, '{"name": "_ZN7example3RunEv"}'),
    Diagnostic(BINARY, Codes.ELFCOMPARE_IMPORT_ADDED.value, '{"name": "_ZN7example4StopEv"}'),
    # Accepted by nothing, so these are what fails the run.
    Diagnostic(BINARY, Codes.ELFCOMPARE_SECURITY.value, '{"name": "security.bind_now"}'),
    Diagnostic(BINARY_DEBUG, Codes.ELFCOMPARE_FUNCTION_REMOVED.value, '{"name": "_ZN7exampleD1Ev"}'),
]


WITH_FINDINGS = r"""{
  "artifacts": [
    {
      "identifier": "@example//:example_deb",
      "type": "DEB",
      "bazel": "bazel-out/example/example.deb",
      "make": "target/example/example.deb"
    },
    {
      "identifier": "/usr/bin/example (runtime) in @example//:example_deb",
      "type": "ELF_EXECUTABLE",
      "bazel": "bazel-out/example/usr/bin/example",
      "make": "target/example/usr/bin/example"
    },
    {
      "identifier": "/usr/bin/example (debug) in @example//:example_deb",
      "type": "ELF_DEBUG_INFO",
      "bazel": "bazel-out/example/usr/bin/example.debug",
      "make": "target/example/usr/bin/example.debug"
    },
    {
      "identifier": "/usr/share/doc/example/changelog.gz in @example//:example_deb",
      "type": "FILE",
      "bazel": "bazel-out/example/usr/share/doc/example/changelog.gz",
      "make": "target/example/usr/share/doc/example/changelog.gz"
    }
  ],
  "diagnostics": {
    "accepted": [
      {
        "artifact": "/usr/share/doc/example/changelog.gz in @example//:example_deb",
        "code": "MAKE_ONLY",
        "msg": "only Make ships this file",
        "accepted_by": "changelogs"
      },
      {
        "artifact": "/usr/bin/example (runtime) in @example//:example_deb",
        "code": "import-added",
        "msg": "{\"name\": \"_ZN7example3RunEv\"}",
        "accepted_by": "added-imports"
      },
      {
        "artifact": "/usr/bin/example (runtime) in @example//:example_deb",
        "code": "import-added",
        "msg": "{\"name\": \"_ZN7example4StopEv\"}",
        "accepted_by": "added-imports"
      }
    ],
    "not_accepted": [
      {
        "artifact": "/usr/bin/example (runtime) in @example//:example_deb",
        "code": "security",
        "msg": "{\"name\": \"security.bind_now\"}"
      },
      {
        "artifact": "/usr/bin/example (debug) in @example//:example_deb",
        "code": "function-removed",
        "msg": "{\"name\": \"_ZN7exampleD1Ev\"}"
      }
    ]
  }
}"""


# (description, the diagnostics a run collected, the report it should render)
GOLDEN_CASES = [
    ("a run with findings", DIAGNOSTICS, WITH_FINDINGS),
]


def render(diagnostics: list[Diagnostic]) -> str:
    """The report a run that collected `diagnostics` writes."""
    index = ArtifactIndex()
    for one in ARTIFACTS:
        index.add(one)
    # Nothing the reporter touches shells out, so the tools only have to exist.
    stub = Tool("stub", ("/bin/true",))
    with tempfile.TemporaryDirectory() as tmp:
        ctx = Context(
            sink=DiagnosticSink(list(diagnostics)),
            index=index,
            bazel=Bazel(),
            tools=Tools(readelf=stub, abidiff=stub, elfcompare=stub, dpkg_deb=stub),
            needs_build=False,
            debian_release="trixie",
            jobs=1,
            workdir=Path(tmp),
        )
        written = Path(tmp) / "report.json"
        reporter.write_report(ctx, rules_engine.classify(diagnostics, RULES), written)
        return written.read_text()


class Golden(unittest.TestCase):
    def setUp(self) -> None:
        # The whole report, not the first 640 characters of it: a truncated diff
        # cannot be read against the golden above it.
        self.maxDiff = None

    def test_the_rendered_report_matches_its_golden(self) -> None:
        for description, diagnostics, golden in GOLDEN_CASES:
            with self.subTest(description):
                self.assertEqual(render(diagnostics), golden)


if __name__ == "__main__":
    if "--print" in sys.argv:
        for description, diagnostics, _ in GOLDEN_CASES:
            print(f"--- {description}\n{render(diagnostics)}\n")
    else:
        unittest.main()
