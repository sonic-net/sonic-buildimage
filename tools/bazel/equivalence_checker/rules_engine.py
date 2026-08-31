"""The machinery for accepting known differences.

What a rule *is*, and how a diagnostic is matched against one. The rules
themselves live in `rules.py`.
"""

import collections
import fnmatch
from collections.abc import Iterator
from dataclasses import dataclass
from typing import TypeAlias

from diagnostics import Codes, Diagnostic, Modifier


@dataclass(frozen=True)
class AnyCode:
    """Matches every code, whichever family it belongs to."""


ANY_CODE = AnyCode()

CodeMatcher: TypeAlias = Codes | tuple[Codes, ...] | AnyCode
StrMatcher: TypeAlias = str | tuple[str, ...]


def literal(text: str) -> str:
    """Sanitize `text` so escape special fnmatch characters."""
    return text.replace("[", "[[]").replace("?", "[?]").replace("*", "[*]")


@dataclass(frozen=True)
class DiagnosticMatcher:
    """Which diagnostics something applies to.

    Every field can take either one value or a tuple.
    And a tuple means "any of these match".

    `codes` names codes from the flat `Codes` enum, ANY_CODE for matching any code.
    `name` and `source` are fnmatch patterns over a ComparableArtifact's name and
    source.
    `msg` is the same, over the diagnostic's own message.
    All of them default to everything ("*").

    `msg_exclude` holds patterns over the message, but reads the other way around:
    a diagnostic matching any of them is left unaccepted, whatever the rest
    of the matcher says.

    `modifier` matches the modifiers tuple, so a rule can reach
    a binary's debug information without also reaching the binary.
    """

    codes: CodeMatcher
    name: StrMatcher = "*"
    source: StrMatcher = "*"
    msg: StrMatcher = "*"
    msg_exclude: StrMatcher = ()
    modifier: Modifier | None = None

    def matches(self, diagnostic: Diagnostic) -> bool:
        artifact = diagnostic.artifact
        # A diagnostic raised before anything is unpacked has no source artifact.
        source = artifact.source.name if artifact.source is not None else ""
        return (
            self._code_matches(diagnostic.code)
            and self._modifier_matches(artifact.modifiers)
            and self._matches_any(artifact.name, self.name)
            and self._matches_any(source, self.source)
            and self._matches_any(diagnostic.msg, self.msg)
            and not self._excluded(diagnostic.msg)
        )

    def _matches_any(self, value: str, patterns: StrMatcher) -> bool:
        if isinstance(patterns, str):
            patterns = (patterns,)
        return any(fnmatch.fnmatchcase(value, pattern) for pattern in patterns)

    def _excluded(self, msg: str) -> bool:
        return self._matches_any(msg, self.msg_exclude)

    def _modifier_matches(self, modifiers: frozenset[Modifier]) -> bool:
        return self.modifier is None or self.modifier in modifiers

    def _code_matches(self, code: Codes) -> bool:
        if isinstance(self.codes, AnyCode):
            return True
        wanted = self.codes if isinstance(self.codes, tuple) else (self.codes,)
        return code in wanted


@dataclass(frozen=True)
class AcceptanceRule:
    """A difference in produced artifacts that we're ready to accept."""

    id: str
    matcher: DiagnosticMatcher
    reason: str
    comment: str = ""

    def matches(self, diagnostic: Diagnostic) -> bool:
        return self.matcher.matches(diagnostic)


class Rules:
    """Every accepted difference.

    Wrapped in a class so the whole collection can be checked as it is built.
    """

    def __init__(self, *rules: AcceptanceRule) -> None:
        duplicates = sorted(
            rule_id
            for rule_id, count in collections.Counter(r.id for r in rules).items()
            if count > 1
        )
        if duplicates:
            raise ValueError(f"duplicate rule ids: {', '.join(duplicates)}")

        self._rules = rules

    def __iter__(self) -> Iterator[AcceptanceRule]:
        return iter(self._rules)

    def accepting(self, diagnostic: Diagnostic) -> AcceptanceRule | None:
        """The first rule that accepts `diagnostic`, or None if none does."""
        return next((rule for rule in self._rules if rule.matches(diagnostic)), None)


# Diagnostics grouped by the id of the rule that accepts them.
# Unaccepted diagnostics are stored under `None`.
Classified: TypeAlias = dict[str | None, list[Diagnostic]]


def classify(diagnostics: list[Diagnostic], rules: Rules) -> Classified:
    """Group every diagnostic under the rule that accepts it, or under None."""
    grouped: Classified = collections.defaultdict(list)
    for diagnostic in diagnostics:
        rule = rules.accepting(diagnostic)
        grouped[rule.id if rule is not None else None].append(diagnostic)
    return dict(grouped)


def unaccepted(classified: Classified) -> list[Diagnostic]:
    """The diagnostics no rule accepts, which will fail a run."""
    return classified.get(None, [])
