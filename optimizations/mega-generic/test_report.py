"""Tests for mega_gen.report — anchor-match-or-abort editing, dependency
graph (JSON + Mermaid), dry-run validation, and CHANGES.md manifest.
"""

from __future__ import annotations

import json

import pytest

from mega_gen.report import (
    AnchorNotFoundError,
    ChangesManifest,
    DependencyEdge,
    DependencyGraph,
    FileChange,
    ValidationResult,
    _marker_begin,
    _marker_end,
    anchor_insert_after,
    anchor_insert_before,
    anchor_replace,
    build_changes_manifest,
    build_dependency_graph,
    format_dry_run_report,
)


# ===================================================================
# 1.  anchor_replace
# ===================================================================


class TestAnchorReplace:
    """The core idempotent-edit primitive."""

    def test_first_run_replaces_anchor(self):
        text = "line1\nORIGINAL_ANCHOR\nline3\n"
        new, already = anchor_replace(
            text, "ORIGINAL_ANCHOR", "REPLACED", label="test-label"
        )
        assert not already
        assert "REPLACED" in new
        assert "ORIGINAL_ANCHOR" not in new
        assert _marker_begin("test-label") in new
        assert _marker_end("test-label") in new

    def test_idempotent_rerun(self):
        text = "line1\nORIGINAL_ANCHOR\nline3\n"
        first, _ = anchor_replace(
            text, "ORIGINAL_ANCHOR", "v1", label="lbl"
        )
        second, already = anchor_replace(
            first, "ORIGINAL_ANCHOR", "v2", label="lbl"
        )
        assert already
        assert "v2" in second
        assert "v1" not in second
        # Markers still present exactly once
        assert second.count(_marker_begin("lbl")) == 1
        assert second.count(_marker_end("lbl")) == 1

    def test_triple_rerun_stable(self):
        text = "before\nANCHOR\nafter\n"
        r1, _ = anchor_replace(text, "ANCHOR", "R", label="x")
        r2, _ = anchor_replace(r1, "ANCHOR", "R", label="x")
        r3, _ = anchor_replace(r2, "ANCHOR", "R", label="x")
        assert r2 == r3  # truly idempotent

    def test_anchor_not_found_raises(self):
        text = "nothing here\n"
        with pytest.raises(AnchorNotFoundError, match="not found"):
            anchor_replace(text, "MISSING", "X", label="lbl")

    def test_preserves_surrounding_text(self):
        text = "header\nANCHOR\nfooter\n"
        new, _ = anchor_replace(text, "ANCHOR", "body", label="t")
        assert new.startswith("header\n")
        assert new.endswith("footer\n")


# ===================================================================
# 2.  anchor_insert_before
# ===================================================================


class TestAnchorInsertBefore:
    def test_first_run_inserts_before_anchor(self):
        text = "line1\nTARGET\nline3\n"
        new, already = anchor_insert_before(
            text, "TARGET", "INSERTED", label="ib"
        )
        assert not already
        assert new.index("INSERTED") < new.index("TARGET")
        assert _marker_begin("ib") in new

    def test_idempotent_rerun(self):
        text = "line1\nTARGET\nline3\n"
        first, _ = anchor_insert_before(
            text, "TARGET", "v1", label="ib"
        )
        second, already = anchor_insert_before(
            first, "TARGET", "v2", label="ib"
        )
        assert already
        assert "v2" in second
        assert "v1" not in second
        assert second.count("TARGET") == 1

    def test_anchor_not_found_raises(self):
        with pytest.raises(AnchorNotFoundError):
            anchor_insert_before("abc", "MISSING", "X", label="lbl")


# ===================================================================
# 3.  anchor_insert_after
# ===================================================================


class TestAnchorInsertAfter:
    def test_first_run_inserts_after_anchor(self):
        text = "line1\nTARGET\nline3\n"
        new, already = anchor_insert_after(
            text, "TARGET", "INSERTED", label="ia"
        )
        assert not already
        assert new.index("INSERTED") > new.index("TARGET")

    def test_idempotent_rerun(self):
        text = "line1\nTARGET\nline3\n"
        first, _ = anchor_insert_after(
            text, "TARGET", "v1", label="ia"
        )
        second, already = anchor_insert_after(
            first, "TARGET", "v2", label="ia"
        )
        assert already
        assert "v2" in second
        assert "v1" not in second

    def test_anchor_not_found_raises(self):
        with pytest.raises(AnchorNotFoundError):
            anchor_insert_after("abc", "MISSING", "X", label="lbl")


# ===================================================================
# 4.  DependencyGraph
# ===================================================================


class TestDependencyGraph:
    def _sample_graph(self) -> DependencyGraph:
        g = DependencyGraph(
            nodes=["database", "swss", "bgp"],
            edges=[
                DependencyEdge("database", "swss", "runtime_order", "band 10→30"),
                DependencyEdge("swss", "bgp", "runtime_order", "band 30→50"),
                DependencyEdge("mega", "database", "build_strategy", "rsync"),
                DependencyEdge("swss", "syncd", "external_dep", "lockstep"),
            ],
            node_attrs={
                "database": {"strategy": "rsync (built image)", "proven": "y"},
                "swss": {"strategy": "rsync (built image)", "proven": "y"},
                "bgp": {"strategy": "rsync (built image)", "proven": "y"},
            },
        )
        return g

    def test_to_json_roundtrips(self):
        g = self._sample_graph()
        j = g.to_json()
        data = json.loads(j)
        assert len(data["nodes"]) == 3
        assert len(data["edges"]) == 4
        edge_kinds = {e["kind"] for e in data["edges"]}
        assert "runtime_order" in edge_kinds
        assert "external_dep" in edge_kinds

    def test_to_mermaid_has_structure(self):
        g = self._sample_graph()
        m = g.to_mermaid()
        assert m.startswith("graph LR")
        assert "database" in m
        assert "swss" in m
        assert "bgp" in m
        assert "-->" in m  # runtime_order
        assert "--x" in m  # external_dep

    def test_empty_graph(self):
        g = DependencyGraph()
        assert g.to_json() == json.dumps({"nodes": [], "edges": []}, indent=2)
        assert "graph LR" in g.to_mermaid()


# ===================================================================
# 5.  build_dependency_graph (with mock FeatureSpecs)
# ===================================================================


def _make_spec(
    feature: str,
    *,
    stem: str = "",
    has_common: bool = False,
    uses_svd_directly: bool = False,
    has_init_script: bool = False,
    has_init_template: bool = False,
    proven: bool = True,
    core: bool = False,
):
    """Build a minimal FeatureSpec-like object for graph tests."""
    from unittest.mock import MagicMock

    spec = MagicMock()
    spec.feature = feature
    spec.stem = stem or f"docker-{feature}"
    spec.dockerfile_common = MagicMock() if has_common else None
    spec.uses_supervisord_directly = uses_svd_directly
    spec.init_script = MagicMock() if has_init_script else None
    spec.init_script_template = MagicMock() if has_init_template else None
    spec.proven = proven
    spec.core = core
    return spec


class TestBuildDependencyGraph:
    def test_three_features_has_runtime_order_edges(self):
        specs = [
            _make_spec("database"),
            _make_spec("swss"),
            _make_spec("bgp"),
        ]
        g = build_dependency_graph(specs)
        runtime_edges = [e for e in g.edges if e.kind == "runtime_order"]
        assert len(runtime_edges) == 2
        assert runtime_edges[0].source == "database"
        assert runtime_edges[0].target == "swss"
        assert runtime_edges[1].source == "swss"
        assert runtime_edges[1].target == "bgp"

    def test_swss_adds_syncd_external_dep(self):
        specs = [_make_spec("database"), _make_spec("swss")]
        g = build_dependency_graph(specs)
        ext = [e for e in g.edges if e.kind == "external_dep"]
        assert len(ext) == 1
        assert ext[0].source == "swss"
        assert ext[0].target == "syncd"
        assert "syncd" in g.nodes

    def test_no_swss_no_syncd(self):
        specs = [_make_spec("database"), _make_spec("lldp")]
        g = build_dependency_graph(specs)
        ext = [e for e in g.edges if e.kind == "external_dep"]
        assert len(ext) == 0
        assert "syncd" not in g.nodes

    def test_build_strategy_edges(self):
        specs = [
            _make_spec("database"),
            _make_spec("swss", has_common=True),
        ]
        g = build_dependency_graph(specs)
        strat = [e for e in g.edges if e.kind == "build_strategy"]
        assert len(strat) == 2
        labels = {e.target: e.label for e in strat}
        assert labels["database"] == "rsync"
        assert labels["swss"] == "include"

    def test_node_attrs_populated(self):
        specs = [_make_spec("gnmi", uses_svd_directly=True)]
        g = build_dependency_graph(specs)
        assert g.node_attrs["gnmi"]["init"] == "direct-supervisord"

    def test_single_feature_no_runtime_edges(self):
        specs = [_make_spec("database")]
        g = build_dependency_graph(specs)
        runtime = [e for e in g.edges if e.kind == "runtime_order"]
        assert len(runtime) == 0

    def test_mega_node_added(self):
        specs = [_make_spec("database")]
        g = build_dependency_graph(specs)
        assert "mega" in g.nodes


# ===================================================================
# 6.  build_changes_manifest
# ===================================================================


class TestBuildChangesManifest:
    def test_generated_files_action_created(self):
        manifest = build_changes_manifest(
            file_patches={},
            generated_files={"rules/docker-mega.mk": "# content\n"},
            features=["database", "swss"],
        )
        assert len(manifest.changes) == 1
        assert manifest.changes[0].action == "created"
        assert manifest.changes[0].relpath == "rules/docker-mega.mk"

    def test_patched_files_action_patched(self):
        manifest = build_changes_manifest(
            file_patches={"rules/config": "INCLUDE_LLDP ?= n\n"},
            generated_files={},
            features=["database"],
        )
        assert len(manifest.changes) == 1
        assert manifest.changes[0].action == "patched"

    def test_overlap_not_double_counted(self):
        """A file in both generated_files and file_patches is counted once
        as 'created' (the generated version takes precedence)."""
        manifest = build_changes_manifest(
            file_patches={"rules/docker-mega.mk": "old"},
            generated_files={"rules/docker-mega.mk": "new"},
            features=["database"],
        )
        assert len(manifest.changes) == 1
        assert manifest.changes[0].action == "created"

    def test_to_markdown_has_table(self):
        manifest = build_changes_manifest(
            file_patches={"rules/config": "x"},
            generated_files={"rules/docker-mega.mk": "y\nz\n"},
            features=["database", "swss"],
            container_name="mega",
            warnings_count=3,
        )
        md = manifest.to_markdown()
        assert "CHANGES.md" in md
        assert "| Action |" in md
        assert "mega" in md
        assert "database" in md
        assert "**Warnings:** 3" in md

    def test_line_counts(self):
        manifest = build_changes_manifest(
            file_patches={},
            generated_files={"a.txt": "one\ntwo\nthree\n"},
            features=[],
        )
        assert manifest.changes[0].line_count == 3

    def test_graph_embedded_in_markdown(self):
        g = DependencyGraph(
            nodes=["database"],
            edges=[
                DependencyEdge("mega", "database", "build_strategy", "rsync")
            ],
        )
        manifest = build_changes_manifest(
            file_patches={},
            generated_files={"a.txt": "x"},
            features=["database"],
            graph=g,
        )
        md = manifest.to_markdown()
        assert "```mermaid" in md
        assert "graph LR" in md


# ===================================================================
# 7.  ValidationResult + format_dry_run_report
# ===================================================================


class TestFormatDryRunReport:
    def test_passing_report(self):
        vr = ValidationResult(
            ok=True,
            feature_count=2,
            resolved_features=["database", "swss"],
            stats={"total_features": 2, "proven": 2},
        )
        report = format_dry_run_report(vr)
        assert "PASS" in report
        assert "database" in report
        assert "total_features: 2" in report

    def test_failing_report(self):
        vr = ValidationResult(
            ok=False,
            feature_count=0,
            errors=["hard-excluded features requested: syncd"],
        )
        report = format_dry_run_report(vr)
        assert "FAIL" in report
        assert "ERROR:" in report
        assert "syncd" in report

    def test_warnings_shown(self):
        vr = ValidationResult(
            ok=True,
            feature_count=1,
            resolved_features=["database"],
            warnings=["unproven features: foobar"],
            discovery_warnings=["database: something odd"],
        )
        report = format_dry_run_report(vr)
        assert "WARNING:" in report
        assert "foobar" in report
        assert "something odd" in report

    def test_graph_in_report(self):
        g = DependencyGraph(
            nodes=["database"],
            edges=[],
        )
        vr = ValidationResult(
            ok=True, feature_count=1, resolved_features=["database"], graph=g
        )
        report = format_dry_run_report(vr)
        assert "```mermaid" in report
        assert '"nodes"' in report  # JSON output

    def test_feature_table_rendered(self):
        specs = [
            _make_spec("database", proven=True, core=True),
            _make_spec("gnmi", uses_svd_directly=True, proven=True),
        ]
        # Add the missing attributes format_dry_run_report reads
        for spec in specs:
            spec.supervisord_is_template = True
            spec.supervisord_conf = True
        vr = ValidationResult(
            ok=True,
            feature_count=2,
            resolved_features=["database", "gnmi"],
            specs=specs,
        )
        report = format_dry_run_report(vr)
        assert "Per-feature summary" in report
        assert "database" in report
        assert "gnmi" in report


# ===================================================================
# 8.  _describe_file
# ===================================================================


class TestDescribeFile:
    def test_known_exact_match(self):
        from mega_gen.report import _describe_file

        desc = _describe_file("rules/docker-mega.mk", "mega")
        assert "Build rules" in desc

    def test_base_image_files(self):
        from mega_gen.report import _describe_file

        desc = _describe_file(
            "dockers/docker-mega/base_image_files/vtysh", "mega"
        )
        assert "vtysh" in desc
        assert "retargeted" in desc

    def test_preinit_script(self):
        from mega_gen.report import _describe_file

        desc = _describe_file(
            "dockers/docker-mega/preinit-teamd.sh", "mega"
        )
        assert "teamd" in desc

    def test_service_file(self):
        from mega_gen.report import _describe_file

        desc = _describe_file(
            "files/build_templates/per_namespace/swss.service.j2", "mega"
        )
        assert "fan-in" in desc.lower() or "Systemd" in desc

    def test_fallback(self):
        from mega_gen.report import _describe_file

        desc = _describe_file("some/random/file.txt", "mega")
        assert "mega-gen" in desc


# ===================================================================
# 9.  Edge-case / integration-level anchor tests
# ===================================================================


class TestAnchorEdgeCases:
    def test_multiple_anchors_only_first_replaced(self):
        text = "A\nANCHOR\nB\nANCHOR\nC\n"
        new, _ = anchor_replace(text, "ANCHOR", "X", label="t")
        # Only the first occurrence is replaced
        assert new.count("ANCHOR") == 1
        assert _marker_begin("t") in new

    def test_different_labels_coexist(self):
        text = "A1\nA2\n"
        r1, _ = anchor_replace(text, "A1", "R1", label="label-1")
        r2, _ = anchor_replace(r1, "A2", "R2", label="label-2")
        assert _marker_begin("label-1") in r2
        assert _marker_begin("label-2") in r2
        assert "R1" in r2
        assert "R2" in r2

    def test_insert_before_preserves_anchor(self):
        """anchor_insert_before must leave the anchor text intact so the
        same file can be further edited at the same anchor on the next
        pipeline stage if needed."""
        text = "before\n# ANCHOR LINE\nafter\n"
        new, _ = anchor_insert_before(
            text, "# ANCHOR LINE", "INSERTED", label="t"
        )
        assert "# ANCHOR LINE" in new
        assert new.index("INSERTED") < new.index("# ANCHOR LINE")

    def test_insert_after_preserves_anchor(self):
        text = "before\n# ANCHOR LINE\nafter\n"
        new, _ = anchor_insert_after(
            text, "# ANCHOR LINE", "INSERTED", label="t"
        )
        assert "# ANCHOR LINE" in new
        assert new.index("INSERTED") > new.index("# ANCHOR LINE")

    def test_empty_replacement(self):
        text = "A\nANCHOR\nB\n"
        new, _ = anchor_replace(text, "ANCHOR", "", label="t")
        assert _marker_begin("t") in new
        assert _marker_end("t") in new
        assert "ANCHOR" not in new
