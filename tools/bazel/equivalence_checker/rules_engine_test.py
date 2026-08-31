"""Tests for the rule layer's matching and classification mechanisms."""

import unittest
from dataclasses import dataclass

import rules_engine
from diagnostics import ArtifactIdentifier, Codes, Diagnostic, Modifier
from rules_engine import ANY_CODE, AcceptanceRule, DiagnosticMatcher, Rules, literal

DEB = "@example//:example_deb"
OTHER_DEB = "@other//:other_deb"
BINARY = "/usr/bin/example"

DEBUG = frozenset({Modifier.DEBUG})
RUNTIME = frozenset({Modifier.RUNTIME})

# Test cases with mangling, taken from rebootbackend.
OURS = '{"name": "_ZN13rebootbackend12ThreadStatus10get_activeEv"}'
OURS_AT_GLOBAL_SCOPE = '{"name": "_ZN15HostServiceDbus13getConnectionEv"}'
THEIRS = '{"name": "_ZN4absl12lts_2025051212log_internal10LogMessage5FlushEv"}'
OUR_SYMBOLS = ("*13rebootbackend*", "*15HostServiceDbus*")

# Messages carrying lists, to test the sanitizing of matchers.
NEEDED = '{"name": "dynamic.needed", "left": ["libc.so.6"], "right": ["libc.so.6"]}'
NEEDED_CHANGED = '{"name": "dynamic.needed", "left": ["libc.so.6"], "right": ["libm.so.6"]}'


def diagnostic(
    code: Codes,
    name: str = "/usr/bin/thing",
    source: str | None = DEB,
    msg: str = "",
    modifiers: frozenset[Modifier] = frozenset(),
) -> Diagnostic:
   """A single mock diagnostic"""
   within = (
        None
        if source is None
        else ArtifactIdentifier(name=source, source=None, modifiers=frozenset())
   )
   return Diagnostic(ArtifactIdentifier(name, within, modifiers), code.value, msg)


TAGGED = AcceptanceRule(
    id="tagged",
    matcher=DiagnosticMatcher(codes=Codes.COLLECTION_EXCLUDED_BY_TAG),
    reason="a code, and nothing else",
)
CHANGELOGS = AcceptanceRule(
    id="changelogs",
    matcher=DiagnosticMatcher(codes=Codes.EXTRACTION_MAKE_ONLY, name="*/changelog.gz"),
    reason="a code and a name glob",
)
ONE_LIBRARY = AcceptanceRule(
    id="one-library",
    matcher=DiagnosticMatcher(
        codes=Codes.EXTRACTION_MAKE_ONLY, name="/usr/lib/*/libexample.*", source=DEB
    ),
    reason="a code, a name glob and a source",
)
ADDED_IMPORTS = AcceptanceRule(
    id="added-imports",
    matcher=DiagnosticMatcher(codes=Codes.ELFCOMPARE_IMPORT_ADDED, name=BINARY, source=DEB),
    reason="one code on one binary in one package",
)
DEBUG_FUNCTIONS = AcceptanceRule(
    id="debug-functions",
    matcher=DiagnosticMatcher(
        codes=Codes.ELFCOMPARE_FUNCTION_ADDED, name=BINARY, source=DEB, modifier=Modifier.DEBUG
    ),
    reason="one half of a pair",
)
ONE_MESSAGE = AcceptanceRule(
    id="one-message",
    matcher=DiagnosticMatcher(codes=Codes.ELFCOMPARE_SECURITY, msg="*bind_now*"),
    reason="one finding out of the several a code reports",
)
EVERYTHING = AcceptanceRule(
    id="everything", matcher=DiagnosticMatcher(codes=ANY_CODE), reason="a catch-all"
)
STATIC_THIRD_PARTY = AcceptanceRule(
    id="static-third-party",
    matcher=DiagnosticMatcher(
        codes=Codes.ELFCOMPARE_FUNCTION_ADDED,
        name=BINARY,
        msg_exclude=OUR_SYMBOLS,
    ),
    reason="a catch-all held back from the symbols this repository compiles",
)

SAMPLE_RULES = Rules(
    TAGGED, CHANGELOGS, ONE_LIBRARY, ADDED_IMPORTS, DEBUG_FUNCTIONS, ONE_MESSAGE
)


def accepting(found: Diagnostic) -> str | None:
    """The id of the rule that accepts `found`, or None if it fails the run."""
    rule = SAMPLE_RULES.accepting(found)
    return rule.id if rule is not None else None


@dataclass
class MatcherTestCase:
    description: str
    matcher: DiagnosticMatcher
    diagnostics: list[Diagnostic]
    expected: list[bool]


MATCHER_CASES = [
    MatcherTestCase(
        "a matcher accepts the code it names, and only that code",
        DiagnosticMatcher(codes=Codes.EXTRACTION_MAKE_ONLY),
        [
            diagnostic(Codes.EXTRACTION_MAKE_ONLY),
            diagnostic(Codes.EXTRACTION_BAZEL_ONLY),
            # Two families could one day share a code name, and a rule must not
            # reach across: EXCLUDED_BY_TAG is a collection code, MAKE_ONLY an
            # extraction one.
            diagnostic(Codes.COLLECTION_EXCLUDED_BY_TAG),
        ],
        [True, False, False],
    ),
    MatcherTestCase(
        "a tuple of codes accepts every code it names, and nothing else",
        DiagnosticMatcher(codes=(Codes.EXTRACTION_MAKE_ONLY, Codes.EXTRACTION_BAZEL_ONLY)),
        [
            diagnostic(Codes.EXTRACTION_MAKE_ONLY),
            diagnostic(Codes.EXTRACTION_BAZEL_ONLY),
            diagnostic(Codes.FILE_CONTENT_MISMATCH),
        ],
        [True, True, False],
    ),
    MatcherTestCase(
        "a tuple of one behaves as the bare code it holds",
        DiagnosticMatcher(codes=(Codes.EXTRACTION_MAKE_ONLY,)),
        [diagnostic(Codes.EXTRACTION_MAKE_ONLY), diagnostic(Codes.EXTRACTION_BAZEL_ONLY)],
        [True, False],
    ),
    MatcherTestCase(
        "an empty tuple names no code, so it accepts nothing",
        DiagnosticMatcher(codes=()),
        [diagnostic(Codes.EXTRACTION_MAKE_ONLY), diagnostic(Codes.COLLECTION_EXCLUDED_BY_TAG)],
        [False, False],
    ),
    MatcherTestCase(
        "a tuple of codes from different families accepts each in its own family",
        DiagnosticMatcher(codes=(Codes.COLLECTION_EXCLUDED_BY_TAG, Codes.ELFCOMPARE_FUNCTION_ADDED)),
        [
            diagnostic(Codes.COLLECTION_EXCLUDED_BY_TAG),
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY),
        ],
        [True, True, False],
    ),
    MatcherTestCase(
        "ANY_CODE matches a code from every family",
        DiagnosticMatcher(codes=ANY_CODE),
        [
            diagnostic(Codes.ELFCOMPARE_IMPORT_ADDED),
            diagnostic(Codes.COLLECTION_EXCLUDED_BY_TAG),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY),
            diagnostic(Codes.FILE_CONTENT_MISMATCH),
        ],
        [True, True, True, True],
    ),
    MatcherTestCase(
        "name, source and msg default to matching everything",
        DiagnosticMatcher(codes=ANY_CODE),
        [diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/anything", source="@x//:y", msg="text")],
        [True],
    ),
    MatcherTestCase(
        "the name pattern is a glob that does not spill onto a neighbour",
        DiagnosticMatcher(codes=ANY_CODE, name="/usr/lib/*/libexample.*"),
        [
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/usr/lib/x86_64-linux-gnu/libexample.so.0"),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/usr/lib/x86_64-linux-gnu/libother.so.0"),
        ],
        [True, False],
    ),
    MatcherTestCase(
        "the source pattern is matched against the source artifact",
        DiagnosticMatcher(codes=ANY_CODE, source=DEB),
        [
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, source=DEB),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, source=OTHER_DEB),
            diagnostic(Codes.COLLECTION_EXCLUDED_BY_TAG, source=None),
        ],
        [True, False, False],
    ),
    MatcherTestCase(
        "a sourceless diagnostic is still matched by the default source pattern",
        DiagnosticMatcher(codes=ANY_CODE),
        [diagnostic(Codes.COLLECTION_EXCLUDED_BY_TAG, source=None)],
        [True],
    ),
    MatcherTestCase(
        "the msg pattern distinguishes two findings of the same code",
        DiagnosticMatcher(codes=ANY_CODE, msg="*bind_now*"),
        [
            diagnostic(Codes.ELFCOMPARE_SECURITY, msg='{"name": "security.bind_now"}'),
            diagnostic(Codes.ELFCOMPARE_SECURITY, msg='{"name": "security.relro"}'),
        ],
        [True, False],
    ),
    MatcherTestCase(
        "a tuple of name patterns takes a file matching any one of them",
        DiagnosticMatcher(codes=ANY_CODE, name=("/usr/bin/*", "/usr/lib/*")),
        [
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/usr/bin/thing"),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/usr/lib/libexample.so.0"),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/etc/passwd"),
        ],
        [True, True, False],
    ),
    MatcherTestCase(
        "a tuple of sources reaches several packages without a rule for each",
        DiagnosticMatcher(codes=ANY_CODE, source=(DEB, OTHER_DEB)),
        [
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, source=DEB),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, source=OTHER_DEB),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, source="@third//:third_deb"),
        ],
        [True, True, False],
    ),
    MatcherTestCase(
        "a tuple of msg patterns takes a finding matching any one of them",
        DiagnosticMatcher(codes=ANY_CODE, msg=("*bind_now*", "*relro*")),
        [
            diagnostic(Codes.ELFCOMPARE_SECURITY, msg='{"name": "security.bind_now"}'),
            diagnostic(Codes.ELFCOMPARE_SECURITY, msg='{"name": "security.relro"}'),
            diagnostic(Codes.ELFCOMPARE_SECURITY, msg='{"name": "security.pie"}'),
        ],
        [True, True, False],
    ),
    MatcherTestCase(
        # A bare string is one pattern, not a sequence of one-character ones.
        "msg_exclude written as a single pattern holds that pattern back",
        DiagnosticMatcher(codes=ANY_CODE, msg_exclude='*13rebootbackend*'),
        [
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, msg=OURS),
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, msg=THEIRS),
        ],
        [False, True],
    ),
    MatcherTestCase(
        "literal() pins one message, brackets and all",
        DiagnosticMatcher(codes=ANY_CODE, msg=literal(NEEDED)),
        [
            diagnostic(Codes.ELFCOMPARE_DEPENDENCY, msg=NEEDED),
            diagnostic(Codes.ELFCOMPARE_DEPENDENCY, msg=NEEDED_CHANGED),
        ],
        [True, False],
    ),
    MatcherTestCase(
        # Why literal() has to exist: ["libc.so.6"] reads as "one of . 6 c i b l o s
        # and a quote", which the message itself does not match.
        "a message carrying a list does not match itself unescaped",
        DiagnosticMatcher(codes=ANY_CODE, msg=NEEDED),
        [diagnostic(Codes.ELFCOMPARE_DEPENDENCY, msg=NEEDED)],
        [False],
    ),
    MatcherTestCase(
        "patterns are case sensitive, so a path is not matched by its lowercasing",
        DiagnosticMatcher(codes=ANY_CODE, name="/usr/bin/Thing"),
        [diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/usr/bin/thing")],
        [False],
    ),
    MatcherTestCase(
        "a modifier reaches one half of a pair and leaves the other alone",
        DiagnosticMatcher(codes=ANY_CODE, modifier=Modifier.DEBUG),
        [
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, modifiers=DEBUG),
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, modifiers=RUNTIME),
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED),
        ],
        [True, False, False],
    ),
    MatcherTestCase(
        "msg_exclude holds back a diagnostic the rest of the matcher would take",
        DiagnosticMatcher(codes=ANY_CODE, msg_exclude=OUR_SYMBOLS),
        [
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, msg=THEIRS),
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, msg=OURS),
        ],
        [True, False],
    ),
    MatcherTestCase(
        "any one of several msg_exclude patterns is enough to hold a diagnostic back",
        DiagnosticMatcher(codes=ANY_CODE, msg_exclude=OUR_SYMBOLS),
        [
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, msg=OURS),
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, msg=OURS_AT_GLOBAL_SCOPE),
        ],
        [False, False],
    ),
    MatcherTestCase(
        "a matcher naming no msg_exclude holds nothing back",
        DiagnosticMatcher(codes=ANY_CODE),
        [diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, msg=OURS)],
        [True],
    ),
    MatcherTestCase(
        "msg_exclude outranks msg, so a narrower pattern cannot reach around it",
        DiagnosticMatcher(codes=ANY_CODE, msg="*get_active*", msg_exclude=OUR_SYMBOLS),
        [diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, msg=OURS)],
        [False],
    ),
    MatcherTestCase(
        "a rule naming no modifier reaches both halves, and neither",
        DiagnosticMatcher(codes=ANY_CODE),
        [
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, modifiers=DEBUG),
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, modifiers=RUNTIME),
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED),
        ],
        [True, True, True],
    ),
]


@dataclass
class AcceptanceTestCase:
    description: str
    diagnostics: list[Diagnostic]
    expected_accepted_by: list[str | None]


ACCEPTANCE_CASES = [
    AcceptanceTestCase(
        "a rule naming only a code accepts it wherever it turns up",
        [
            diagnostic(Codes.COLLECTION_EXCLUDED_BY_TAG, name="//dockers/docker-x:x.gz", source=None),
            diagnostic(Codes.COLLECTION_EXCLUDED_BY_TAG, name=BINARY, source=OTHER_DEB),
        ],
        ["tagged", "tagged"],
    ),
    AcceptanceTestCase(
        "a name glob picks out one file, leaving its neighbours to fail the run",
        [
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/usr/share/doc/example/changelog.gz"),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, name=BINARY),
        ],
        ["changelogs", None],
    ),
    AcceptanceTestCase(
        "a rule narrowed by name and source takes neither on its own",
        [
            diagnostic(
                Codes.EXTRACTION_MAKE_ONLY,
                name="/usr/lib/x86_64-linux-gnu/libexample.so.0",
                source=DEB,
            ),
            diagnostic(
                Codes.EXTRACTION_MAKE_ONLY,
                name="/usr/lib/x86_64-linux-gnu/libexample.so.0",
                source=OTHER_DEB,
            ),
            diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/usr/lib/x86_64-linux-gnu/libother.so.0"),
        ],
        ["one-library", None, None],
    ),
    AcceptanceTestCase(
        "a rule scoped to one package does not accept the same name from another",
        [
            diagnostic(Codes.ELFCOMPARE_IMPORT_ADDED, name=BINARY, source=DEB),
            diagnostic(Codes.ELFCOMPARE_IMPORT_ADDED, name=BINARY, source=OTHER_DEB),
        ],
        ["added-imports", None],
    ),
    AcceptanceTestCase(
        "a rule naming a modifier reaches the debug half only",
        [
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, name=BINARY, source=DEB, modifiers=DEBUG),
            diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, name=BINARY, source=DEB, modifiers=RUNTIME),
        ],
        ["debug-functions", None],
    ),
    AcceptanceTestCase(
        "a msg glob accepts one finding of a code and not the others",
        [
            diagnostic(Codes.ELFCOMPARE_SECURITY, msg='{"name": "security.bind_now"}'),
            diagnostic(Codes.ELFCOMPARE_SECURITY, msg='{"name": "security.relro"}'),
        ],
        ["one-message", None],
    ),
    AcceptanceTestCase(
        "a code no rule names falls through, whichever family it comes from",
        [
            diagnostic(code, name=BINARY, source=DEB, modifiers=RUNTIME)
            for code in (
                Codes.ELFCOMPARE_IMPORT_REMOVED,
                Codes.ELFCOMPARE_FUNCTION_REMOVED,
                Codes.ELFCOMPARE_DIFFERENT_STRIP_LEVELS,
                Codes.EXTRACTION_BAZEL_ONLY,
                Codes.FILE_CONTENT_MISMATCH,
                Codes.COLLECTION_MODULE_UNREACHABLE,
            )
        ],
        [None] * 6,
    ),
]


# Dagnostics for classification testing
EXCLUDED = diagnostic(Codes.COLLECTION_EXCLUDED_BY_TAG, source=None)
CHANGELOG = diagnostic(Codes.EXTRACTION_MAKE_ONLY, name="/usr/share/doc/x/changelog.gz")
MISMATCH_A = diagnostic(Codes.FILE_CONTENT_MISMATCH, name="/a")
MISMATCH_B = diagnostic(Codes.FILE_CONTENT_MISMATCH, name="/b")
THIRD_PARTY_SYMBOL = diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, name=BINARY, msg=THEIRS)
FIRST_PARTY_SYMBOL = diagnostic(Codes.ELFCOMPARE_FUNCTION_ADDED, name=BINARY, msg=OURS)

@dataclass
class ClassifyTestCase:
    description: str
    diagnostics: list[Diagnostic]
    rules: Rules
    expected: dict[str | None, list[Diagnostic]]

CLASSIFY_CASES = [
    ClassifyTestCase(
        "a run with no diagnostics classifies to nothing at all",
        [],
        SAMPLE_RULES,
        {},
    ),
    ClassifyTestCase(
        "classify groups by accepting rule, and files the rest under None",
        [EXCLUDED, MISMATCH_A],
        Rules(TAGGED),
        {"tagged": [EXCLUDED], None: [MISMATCH_A]},
    ),
    ClassifyTestCase(
        "diagnostics keep the order they were recorded in, so a report is stable",
        [MISMATCH_A, MISMATCH_B],
        Rules(),
        {None: [MISMATCH_A, MISMATCH_B]},
    ),
    ClassifyTestCase(
        "an empty rule set accepts nothing",
        [EXCLUDED],
        Rules(),
        {None: [EXCLUDED]},
    ),
    ClassifyTestCase(
        "nothing is left over when a rule covers every diagnostic",
        [EXCLUDED, MISMATCH_A],
        Rules(EVERYTHING),
        {"everything": [EXCLUDED, MISMATCH_A]},
    ),
    ClassifyTestCase(
        "the first matching rule wins, so order decides which reason is reported",
        [CHANGELOG],
        Rules(CHANGELOGS, EVERYTHING),
        {"changelogs": [CHANGELOG]},
    ),
    ClassifyTestCase(
        "an earlier catch-all shadows the narrower rule behind it",
        [CHANGELOG],
        Rules(EVERYTHING, CHANGELOGS),
        {"everything": [CHANGELOG]},
    ),
    ClassifyTestCase(
        "a catch-all with msg_exclude takes the static-link symbols and leaves ours failing",
        [THIRD_PARTY_SYMBOL, FIRST_PARTY_SYMBOL],
        Rules(STATIC_THIRD_PARTY),
        {"static-third-party": [THIRD_PARTY_SYMBOL], None: [FIRST_PARTY_SYMBOL]},
    ),
]


class RuleLayer(unittest.TestCase):

    def test_a_matcher_takes_what_it_names(self) -> None:
        for case in MATCHER_CASES:
            with self.subTest(case.description):
                self.assertEqual(
                    [case.matcher.matches(diagnostic) for diagnostic in case.diagnostics],
                    case.expected,
                )

    def test_a_rule_accepts_what_its_matcher_takes(self) -> None:
        for case in ACCEPTANCE_CASES:
            with self.subTest(case.description):
                self.assertEqual(
                    [accepting(diagnostic) for diagnostic in case.diagnostics], case.expected_accepted_by
                )

    def test_a_run_comes_out_grouped(self) -> None:
        for case in CLASSIFY_CASES:
            with self.subTest(case.description):
                classified = rules_engine.classify(case.diagnostics, case.rules)
                self.assertEqual(classified, case.expected)
                self.assertEqual(
                    rules_engine.unaccepted(classified), case.expected.get(None, [])
                )

    def test_duplicate_rule_ids_are_refused(self) -> None:
        """Two rules under one id would make the report's per-rule counts a lie."""
        with self.assertRaises(ValueError) as refusal:
            _ = Rules(TAGGED, TAGGED)
        self.assertEqual(str(refusal.exception), "duplicate rule ids: tagged")


if __name__ == "__main__":
    unittest.main()
