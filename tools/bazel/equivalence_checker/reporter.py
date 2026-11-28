"""Turns the diagnostics a run collected into the report a human reads."""

import collections
import json
from pathlib import Path

import rules_engine
from context import Context


def print_report(ctx: Context, classified: rules_engine.Classified) -> None:
    """Print how many of each diagnostic the run collected."""
    remaining = rules_engine.unaccepted(classified)
    accepted = {
        rule_id: found for rule_id, found in classified.items() if rule_id is not None
    }

    print(f"\n{len(ctx.index)} artifacts, {len(ctx.sink.diagnostics)} diagnostics.")

    if accepted:
        print(f"\n{sum(len(found) for found in accepted.values())} accepted:")
        for rule_id, found in sorted(
            accepted.items(), key=lambda pair: len(pair[1]), reverse=True
        ):
            print(f"    {len(found):6d}  {rule_id}")

    by_code = collections.Counter(str(diagnostic.code) for diagnostic in remaining)
    print(f"\n{len(remaining)} not accepted:")
    for code, count in by_code.most_common():
        print(f"    {count:6d}  {code}")

    if remaining:
        print(f"\nFAIL: {len(remaining)} diagnostics no rule accepts.")
    else:
        print("\nPASS: every diagnostic is accounted for by a rule.")


def write_report(ctx: Context, classified: rules_engine.Classified, path: Path) -> None:
    """Write every diagnostic the run collected to `path`, as JSON."""
    path.parent.mkdir(parents=True, exist_ok=True)
    _ = path.write_text(
        json.dumps(
            {
                "artifacts": [
                    {
                        "identifier": str(artifact.identifier),
                        "type": str(artifact.type),
                        "bazel": str(artifact.bazelVersion),
                        "make": str(artifact.makeVersion),
                    }
                    for artifact in ctx.index.artifacts.values()
                ],
                "diagnostics": {
                    "accepted": [
                        {
                            "artifact": str(diagnostic.artifact),
                            "code": str(diagnostic.code),
                            "msg": diagnostic.msg,
                            "accepted_by": rule_id,
                        }
                        for rule_id, found in classified.items()
                        if rule_id is not None
                        for diagnostic in found
                    ],
                    "not_accepted": [
                        {
                            "artifact": str(diagnostic.artifact),
                            "code": str(diagnostic.code),
                            "msg": diagnostic.msg,
                        }
                        for diagnostic in rules_engine.unaccepted(classified)
                    ],
                },
            },
            indent=2,
        )
    )
