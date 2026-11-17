"""Only explicit non-comparable target exclusions are accepted by default."""

from diagnostics import Codes
from rules_engine import AcceptanceRule, DiagnosticMatcher, Rules

RULES = Rules(
    AcceptanceRule(
        id="excluded-by-tag",
        matcher=DiagnosticMatcher(codes=Codes.COLLECTION_EXCLUDED_BY_TAG),
        reason="The target explicitly declares that it has no Make counterpart.",
    ),
)
