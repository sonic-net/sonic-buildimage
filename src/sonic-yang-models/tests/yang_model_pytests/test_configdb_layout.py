"""ConfigDB/Redis serializability checks over every sonic-* YANG model.

CONFIG_DB stores one flat Redis hash per row, keyed `<TABLE>|<key>|<key>...`.
A schema that assumes nesting, or two schemas that flatten onto the same hash,
cannot round-trip through it. libyang validates none of that: it checks the
data tree against the schema, and the sonic-yang tests round-trip JSON, so a
model that can never be written to a live device passes everything. DNS_OPTIONS
shipped in exactly that state.

Each test below encodes one property the ConfigDB xlate engine
(`sonic_yang_ext.py`) actually relies on, and reports every violation it finds
rather than stopping at the first, so a developer fixing N models sees all N in
one run.

Allowlists carry shapes that exist in sonic-net/sonic-buildimage only. Those are
not ours to remodel unilaterally, so they are grandfathered in to let the checks
gate new violations. Shapes we added are never allowlisted: they get fixed in
their own change and this file stays as-is.
"""
import os
import tempfile
from collections import defaultdict
from contextlib import contextmanager
from glob import glob

import libyang as ly
import pytest


# Lists whose entire row is an inner list of key/value pairs — the QoS maps and
# CABLE_LENGTH. The xlate engine can only serialize this shape because
# `sonic_yang_ext.Type_1_list_maps_model` names them explicitly; a new one added
# without touching that Python list silently fails to serialize. Mirrored here
# so the models side notices first (sonic-yang-mgmt is not a build dependency of
# this package — `rules/sonic-yang-mgmt-py3.mk` makes sonic_yang_models a
# dependency of sonic_yang_mgmt — so importing the engine here would invert the
# build order).
#
# This set must match Type_1_list_maps_model exactly. Only one drift direction
# is caught here: a name in the engine but not in this set just fails this test.
# The reverse — a name here but not in the engine — leaves this test green while
# the table fails to serialize on a device. So always add to sonic_yang_ext.py
# FIRST, then here.
#
# That reverse direction is checkable, just not from this package.
# `src/sonic-yang-mgmt/tests/libyang-python-tests/test_sonic_yang.py` can import
# Type_1_list_maps_model from its own package and already loads the installed
# models, and it already pins a hardcoded engine-derived set the same way
# (`test_get_create_only_fields_matches_expected_set`). Tracked as a follow-up;
# until then this comment is the accurate statement of what is and is not
# guarded, so do not read the paragraph above as "uncheckable".
_TYPE_1_MAP_LISTS = frozenset({
    'DSCP_TO_TC_MAP_LIST',
    'DOT1P_TO_TC_MAP_LIST',
    'TC_TO_PRIORITY_GROUP_MAP_LIST',
    'TC_TO_QUEUE_MAP_LIST',
    'MAP_PFC_PRIORITY_TO_QUEUE_LIST',
    'PFC_PRIORITY_TO_PRIORITY_GROUP_MAP_LIST',
    'DSCP_TO_FC_MAP_LIST',
    'EXP_TO_FC_MAP_LIST',
    'CABLE_LENGTH_LIST',
    'MPLS_TC_TO_TC_MAP_LIST',
    'TC_TO_DSCP_MAP_LIST',
    'TC_TO_DOT1P_MAP_LIST',
})

# A table container carrying both a `list` row layer and a singleton `container`
# row layer. The container's name is a valid one-segment ConfigDB key, so the
# lists are offered the singleton's row first.
#
# What saves the one entry here is key count, not leaf sets: `TELEMETRY_CLIENT_LIST`
# declares `key "prefix name"`, so `_extractKey` raises on
# `len(keyList) != len(value)` when handed the one-segment `TELEMETRY_CLIENT|Global`
# and declines before a single leaf is consulted. Non-overlapping leaf sets are
# what would matter for a list taking exactly *one* key — that row does match on
# count, every field is then resolved against that list's `leafDict`, and only a
# field failing to resolve keeps the container row from being eaten. So do not
# read this exemption as "safe if the leaf sets differ": a single-keyed list
# beside a singleton container is genuinely ambiguous.
_UPSTREAM_MIXED_ROW_LAYERS = frozenset({
    'sonic-telemetry_client:TELEMETRY_CLIENT',
})


def _is_event_schema(snode):
    """A container that carries an extension from sonic-events-common
    (ALARM_SEVERITY_*, EVENT_SEVERITY_*, …) is an event/notification payload
    schema, not a ConfigDB table — none of these rules apply to it."""
    for ext in snode.extensions():
        if ext.module().name() == "sonic-events-common":
            return True
    return False


def _config_tables(yang_model, include_event_schemas=False):
    """Yield (module, table_container) for every ConfigDB table in the tree.

    Mirrors `sonic_yang_ext._createDBTableToModuleMap`: a module maps to
    ConfigDB when its first top-level container is named after the module, and
    each child container of that one is a table. Helper-only modules (typedefs,
    groupings) have no top-level container and define no tables.

    Event/notification payload containers are filtered out by default, because
    the row-shape rules are properties of a ConfigDB row and an event payload is
    not one. `include_event_schemas=True` reproduces the engine's own walk, which
    applies no such filter — needed by the checks whose subject is
    `confDbYangMap` itself rather than a row.
    """
    for path in sorted(glob(os.path.join(yang_model.model_dir, "sonic-*.yang"))):
        module_name = os.path.splitext(os.path.basename(path))[0]
        module = yang_model.ctx.get_module(module_name)
        if module is None:
            continue
        top = next(iter(module.children(types=(ly.SNode.CONTAINER,))), None)
        if top is None or top.name() != module.name():
            continue
        for table in top.children(types=(ly.SNode.CONTAINER,)):
            if not include_event_schemas and _is_event_schema(table):
                continue
            yield module, table


def _lists(snode):
    return list(snode.children(types=(ly.SNode.LIST,)))


def _containers(snode):
    return list(snode.children(types=(ly.SNode.CONTAINER,)))


def _leaf_names(snode):
    """Leaf and leaf-list names directly under snode. `children` descends
    through choice/case transparently, matching `_createLeafDict`."""
    return {c.name() for c in snode.children(types=(ly.SNode.LEAF, ly.SNode.LEAFLIST))}


def _key_names(list_snode):
    return [k.name() for k in list_snode.keys()]


def _report(violations, explanation):
    assert not violations, explanation + ":\n  " + "\n  ".join(violations)


def test_every_table_has_row_layer(yang_model):
    """A table whose direct children are only leaf/leaf-list has no row key for
    ConfigDB to serialize against. DNS_OPTIONS shipped in this state and was
    unusable on a live system."""
    violations = []
    for module, table in _config_tables(yang_model):
        if not _lists(table) and not _containers(table):
            violations.append(f"{module.name()}:{table.name()}")

    _report(violations,
            "ConfigDB table containers without a row layer (list or inner "
            "container) cannot be serialized to Redis")


def test_table_has_one_kind_of_row_layer(yang_model):
    """A table must not carry both a keyed `list` and a singleton `container`.

    `_xlateContainer` offers every row to the lists first, and the singleton's
    name is a valid one-segment key, so a list that takes exactly one key
    matches `TABLE|Global` on count and resolves every field against its own
    `leafDict` — the container row is then claimed by the list unless some field
    fails to resolve. A list with two or more keys declines on count in
    `_extractKey` before any leaf is consulted, which is the only reason the
    allowlisted entry is safe. Key count is the discriminator; leaf sets are a
    fallback that is not something to design against.
    """
    violations = []
    seen_mixed = set()
    for module, table in _config_tables(yang_model):
        ident = f"{module.name()}:{table.name()}"
        tlists, tconts = _lists(table), _containers(table)
        if tlists and tconts:
            seen_mixed.add(ident)
            # The exemption only holds while every list declines the container's
            # one-segment row on key count. A single-keyed list added to an
            # allowlisted table later would silently swallow that row, so the
            # exemption stops applying rather than covering the new shape.
            single_keyed = [x.name() for x in tlists if len(_key_names(x)) == 1]
            if ident in _UPSTREAM_MIXED_ROW_LAYERS and not single_keyed:
                continue
            detail = (f"lists={[x.name() for x in tlists]} "
                      f"containers={[x.name() for x in tconts]}")
            if single_keyed and ident in _UPSTREAM_MIXED_ROW_LAYERS:
                detail += (f" — exemption no longer applies: {single_keyed} "
                           "take one key and would claim the container's row")
            violations.append(f"{ident}: {detail}")

    # An exemption that stops being needed should fail rather than sit there
    # justifying nothing — the same argument that gave _TYPE_1_MAP_LISTS its
    # stale-entry direction. If upstream remodels TELEMETRY_CLIENT the entry
    # must go, otherwise the next table wanting one is added under a name nobody
    # re-checked.
    violations += [f"stale exemption: {name} no longer carries both row layers"
                   for name in sorted(_UPSTREAM_MIXED_ROW_LAYERS - seen_mixed)]

    _report(violations,
            "ConfigDB table containers with both a list row layer and a "
            "container row layer are ambiguous to deserialize, and every entry "
            "in _UPSTREAM_MIXED_ROW_LAYERS must still name one that does")


def test_sibling_lists_have_distinct_key_counts(yang_model):
    """Sibling lists in one table are told apart by key count and nothing else.

    `_xlateContainer` walks a container's lists in declaration order and
    `_extractKey` accepts or rejects a row purely on
    `len(pkey.split("|")) == len(list.keys())`; the first list that matches
    consumes the row. Two siblings with the same key count are distinguishable
    only by the accident of a field failing to resolve against the first one's
    leaves — `_xlateList` then raises, leaves the row in the working copy and
    lets the next sibling try. That fallback is not something to design against.

    The upstream guidelines say the same thing normatively (SONiC
    `doc/mgmt/SONiC_YANG_Model_Guidelines.md`, rule 18): when one ConfigDB table
    is split across several YANG lists, "utilize composite keys that have a
    different number of key elements to distinguish lists... different key names
    do not count as unambiguous model". Key count is the discriminator.
    """
    violations = []
    for module, table in _config_tables(yang_model):
        by_count = defaultdict(list)
        for tlist in _lists(table):
            by_count[len(_key_names(tlist))].append(tlist)
        for count, siblings in sorted(by_count.items()):
            if len(siblings) < 2:
                continue
            # A row goes to the first list that accepts it, so a list is
            # unreachable when any list declared ahead of it resolves every
            # field the later one does — not only when the first one does.
            #
            # What has to resolve is the row's *fields*. Key leaves travel in
            # the ConfigDB key, not in the hash, and `_xlateList` only ever
            # looks up hash field names in `leafDict`, so counting a candidate's
            # own key leaves on the left-hand side makes it look wider than the
            # row it actually holds. Two same-key-count siblings with different
            # key names and an otherwise identical field set are a real shadow;
            # comparing full leaf sets would under-report them as merely
            # fragile. This is exactly the case rule 18 singles out with
            # "different key names do not count as unambiguous model".
            leaves = [_leaf_names(s) for s in siblings]
            fields = [leaves[i] - set(_key_names(s))
                      for i, s in enumerate(siblings)]
            shadowed = [s.name() for i, s in enumerate(siblings)
                        if any(fields[i] <= leaves[j] for j in range(i))]
            if shadowed:
                detail = (f" — {shadowed} unreachable: every field also resolves "
                          "against a list declared earlier")
            else:
                detail = (" — separated today only by a field failing to resolve "
                          "against an earlier list; give them distinct key counts")
            violations.append(
                f"{module.name()}:{table.name()}: {count} key(s) -> "
                f"{[s.name() for s in siblings]}{detail}")

    _report(violations,
            "Sibling lists in one ConfigDB table share a key count, so a row "
            "cannot be attributed to one of them")


def test_nested_list_does_not_sit_beside_leaves(yang_model):
    """CONFIG_DB cannot nest, so a `list` inside a list row is flattened onto
    that row's hash and the inner list's key becomes a hash FIELD NAME. That is
    only sound when the inner list is the whole row: otherwise one token slot
    carries both schema leaf names and operator-chosen names, the row's field
    set stops being closed, and the inner key cannot be constrained, leafref'd
    or `must`-checked."""
    violations = []
    for module, table in _config_tables(yang_model):
        for tlist in _lists(table):
            non_key_leaf_fields = _leaf_names(tlist) - set(_key_names(tlist))
            if not non_key_leaf_fields:
                continue
            for inner in _lists(tlist):
                ident = (f"{module.name()}:{table.name()}:"
                         f"{tlist.name()}:{inner.name()}")
                violations.append(
                    f"{ident}: inner list key would collide with the field "
                    f"namespace of {sorted(non_key_leaf_fields)}")

    _report(violations,
            "A list nested inside a list row that also declares leaves cannot "
            "be represented in a flat ConfigDB hash. Model the relationship as "
            "its own table with a composite key")


def test_nested_list_maps_are_registered(yang_model):
    """A list whose entire row is an inner list only serializes because
    `sonic_yang_ext.Type_1_list_maps_model` names it. A new table with this
    shape that is not registered there will not round-trip at all.

    Registration is necessary but not sufficient, so the shape is checked too.
    `_xlateType1MapList` takes the *first* inner list only
    (`next(iter(list_snode.children(types=(ly.SNode.LIST,))))`), takes
    `inner_keys[0]` as the key, and sets `inner_listVal` by keeping whichever
    non-key name it sees last while looping the inner leaf dict.
    `_revXlateType1MapList` repeats all three assumptions. A registered list with
    two inner lists, a composite inner key, or two value leaves therefore
    serializes wrongly and silently — the failure mode this check exists to
    prevent one step earlier.
    """
    violations = []
    map_shaped = set()
    for module, table in _config_tables(yang_model):
        for tlist in _lists(table):
            inner_lists = _lists(tlist)
            if not inner_lists:
                continue
            # Recorded before the beside-leaves hand-off, so a registered list
            # that grows a leaf is reported by that check alone rather than also
            # showing up here as a stale entry it is not — the name still names
            # a map-shaped list in the tree, that list just has a second problem,
            # and "remove it from _TYPE_1_MAP_LISTS" would be the wrong remedy.
            map_shaped.add(tlist.name())
            if _leaf_names(tlist) - set(_key_names(tlist)):
                continue  # covered by the beside-leaves check
            if tlist.name() not in _TYPE_1_MAP_LISTS:
                violations.append(f"unregistered: {module.name()}:"
                                  f"{table.name()}:{tlist.name()}")
                continue

            ident = f"{module.name()}:{table.name()}:{tlist.name()}"
            if len(inner_lists) > 1:
                violations.append(
                    f"{ident}: {len(inner_lists)} inner lists "
                    f"{[x.name() for x in inner_lists]}; only the first is read")
            inner = inner_lists[0]
            inner_keys = _key_names(inner)
            if len(inner_keys) != 1:
                violations.append(
                    f"{ident}:{inner.name()}: inner key is {inner_keys}; "
                    "only inner_keys[0] is read")
            inner_values = _leaf_names(inner) - set(inner_keys)
            if len(inner_values) != 1:
                violations.append(
                    f"{ident}:{inner.name()}: {sorted(inner_values)} non-key "
                    "leaves; the last one seen silently wins")

    # The other direction is the one that fails silently on a device, and the
    # engine's list cannot be imported here to compare against. What can be
    # checked is that every name carried here still names a map-shaped list in
    # the tree, so a typo or a name left behind by a removed table is caught
    # rather than sitting in the set justifying nothing.
    violations += [f"stale entry: {name}"
                   for name in sorted(_TYPE_1_MAP_LISTS - map_shaped)]

    # Both directions report together: someone adding a map table and leaving a
    # stale name behind should see both in one run, not one per run.
    _report(violations,
            "Map-shaped tables (inner list as the whole row) must be named in "
            "Type_1_list_maps_model in sonic_yang_ext.py, must match the shape "
            "_xlateType1MapList assumes, and every name in _TYPE_1_MAP_LISTS "
            "must still match a map-shaped list")


def test_single_list_table_is_named_after_its_table(yang_model):
    """A table with exactly one list is xlated only when that list is named
    `<table>_LIST`.

    `_xlateContainer` splits on list count. With one list it xlates it only
    under that name; otherwise the branch is skipped, nothing consumes the rows,
    and it raises `All Keys are not parsed in <table>`. With two or more lists
    the name is not consulted at all, so the rule applies to single-list tables
    only.

    Mostly guarded today by the real-models round trip in
    `src/sonic-yang-mgmt/tests/libyang-python-tests/test_sonic_yang.py`, which
    drives `sample_config_db.json` through `loadData` and back — but that covers
    only the tables with a row in the sample. The ones without a row are all
    single-list tables, so this naming rule is the only thing standing behind
    them, and a table with no sample row is exactly how DNS_OPTIONS reached a
    release.
    """
    violations = []
    for module, table in _config_tables(yang_model):
        tlists = _lists(table)
        if len(tlists) != 1:
            continue
        expected = f"{table.name()}_LIST"
        if tlists[0].name() != expected:
            violations.append(f"{module.name()}:{table.name()}: "
                              f"{tlists[0].name()}, expected {expected}")

    _report(violations,
            "A ConfigDB table with a single list is only xlated when that list "
            "is named <table>_LIST; otherwise its rows are never consumed")


def test_every_table_list_name_carries_LIST(yang_model):
    """Every list under a table must have `_LIST` in its name, or it is never
    written back.

    `_revXlateList` gates on `"_LIST" in list_snode.name()`, with no reference
    to the table name and no count-dependent branch. A differently-named list
    can therefore be read (when a table has two or more lists, where
    `_xlateContainer` does not consult names) and then silently never
    serialized back out.
    """
    violations = []
    for module, table in _config_tables(yang_model):
        for tlist in _lists(table):
            if "_LIST" not in tlist.name():
                violations.append(
                    f"{module.name()}:{table.name()}:{tlist.name()}")

    _report(violations,
            "Lists under a ConfigDB table must carry _LIST in their name or "
            "_revXlateList never writes them back")


def test_no_container_below_the_row_layer(yang_model):
    """A row is one flat hash, and a container nested below the row layer has
    nowhere to go. Both halves are dropped rather than diagnosed, by two
    different paths.

    Under a *list* row, `_xlateContainerInList` does not recurse into a level
    ConfigDB cannot represent — it returns. It returns once when the container
    name is absent from the row, and again when `bool(configC[ccName])` is true
    (the comment beside that guard reads "Empty container - return" while the
    condition is the opposite). Back in `_xlateList`, `yangContainer` is then
    empty, `keyDict[vKey]` is never assigned, and the row is still deleted from
    the working copy, so the container's fields are lost without a diagnostic.

    Under a *container* row it is a different path entirely:
    `_xlateContainerInContainer`, not `_xlateContainerInList`. Neither shape
    survives the flattening, which is why both are reported here.
    """
    violations = []
    for module, table in _config_tables(yang_model):
        for row in _lists(table) + _containers(table):
            for sub in _containers(row):
                violations.append(
                    f"{module.name()}:{table.name()}:{row.name()}:{sub.name()}")

    _report(violations,
            "Containers nested below the ConfigDB row layer have no "
            "representation in a flat Redis hash")


def test_no_list_below_a_container_row(yang_model):
    """A `list` under a singleton container row is a second row layer beneath a
    row that already consumed the only key slot."""
    violations = []
    for module, table in _config_tables(yang_model):
        for row in _containers(table):
            for sub in _lists(row):
                violations.append(
                    f"{module.name()}:{table.name()}:{row.name()}:{sub.name()}")

    _report(violations,
            "Lists nested below a singleton container row have no key slot "
            "left in the ConfigDB key")


def test_every_list_has_keys(yang_model):
    """`_extractKey` derives the ConfigDB key from the list's `key` statement.
    A keyless list has no row identity to serialize against."""
    violations = []
    for module, table in _config_tables(yang_model):
        for row in _lists(table):
            if not _key_names(row):
                violations.append(f"{module.name()}:{table.name()}:{row.name()}")
            for inner in _lists(row):
                if not _key_names(inner):
                    violations.append(
                        f"{module.name()}:{table.name()}:{row.name()}:{inner.name()}")

    _report(violations, "ConfigDB rows modelled as keyless lists have no key to "
                        "serialize against")


def test_table_names_are_unique_across_modules(yang_model):
    """`confDbYangMap` is keyed by table name alone. Two modules declaring the
    same table means the second silently overwrites the first, and every row of
    that table is then validated against the wrong schema.

    This is the one check whose subject is `confDbYangMap` itself rather than a
    ConfigDB row, so it walks what the engine walks. `_createDBTableToModuleMap`
    registers every child container of every module top container with no event
    filter at all, which puts the `sonic-events-*` payload containers in the map
    where they can clobber a real table name. Filtering them out here — right
    for the nine row-shape checks, since an event payload is not a row — would
    let exactly that collision through.
    """
    owners = defaultdict(list)
    for module, table in _config_tables(yang_model, include_event_schemas=True):
        owners[table.name()].append(module.name())

    violations = [f"{name}: {sorted(mods)}"
                  for name, mods in sorted(owners.items()) if len(mods) > 1]

    _report(violations,
            "ConfigDB table names must be unique across modules; confDbYangMap "
            "keeps only the last one loaded")


def test_top_container_matches_module_name(yang_model):
    """`_createDBTableToModuleMap` raises when a module's first top-level
    container is not named after the module, which surfaces at config-load time
    on a device rather than here."""
    violations = []
    for path in sorted(glob(os.path.join(yang_model.model_dir, "sonic-*.yang"))):
        module_name = os.path.splitext(os.path.basename(path))[0]
        module = yang_model.ctx.get_module(module_name)
        if module is None:
            continue
        top = next(iter(module.children(types=(ly.SNode.CONTAINER,))), None)
        if top is None:
            continue  # helper-only module: typedefs, groupings, extensions
        if top.name() != module.name():
            violations.append(f"{module.name()}: top-level container {top.name()}")

    _report(violations,
            "A module's first top-level container must be named after the "
            "module or sonic_yang cannot map its tables")


# =============================================================================
# Negative self-tests
#
# Every check above is green against the current tree, and the ones that are not
# go green as their blocking PRs land — at which point no check in this file has
# ever been seen to produce a violation. An inverted condition, a `_lists` where
# `_containers` was meant, or a subset test comparing the wrong sets would all
# pass silently and forever.
#
# So each check is also run against a synthetic model built to violate it, and
# asserted to report it. Same shape as the negative cases every other module in
# this directory pairs with its positive ones. The module docstring's promise
# that a check "reports every violation it finds rather than stopping at the
# first" is pinned the same way, by a model carrying two violations of one kind.
# =============================================================================


class _SyntheticModel:
    """Stands in for the `yang_model` fixture: the checks only use these two."""

    def __init__(self, ctx, model_dir):
        self.ctx = ctx
        self.model_dir = model_dir


def _module_src(name, body, extra_top=""):
    """A minimal ConfigDB-shaped module: one top-level container named after the
    module, whose children are table containers."""
    return f"""module {name} {{
  yang-version 1.1;
  namespace "http://nexthop.ai/yang/{name}";
  prefix {name.replace('-', '_')};
  container {name} {{
{body}
  }}
{extra_top}
}}
"""


@contextmanager
def _synthetic(*sources):
    """Parse YANG source strings into their own context and yield a stand-in
    model. `sources` is a sequence of (module_name, source) pairs; names must
    start with `sonic-` so `_config_tables`'s glob picks them up."""
    with tempfile.TemporaryDirectory() as model_dir:
        for name, src in sources:
            with open(os.path.join(model_dir, f"{name}.yang"), "w") as f:
                f.write(src)
        ctx = ly.Context(model_dir)
        try:
            for name, _ in sources:
                with open(os.path.join(model_dir, f"{name}.yang")) as f:
                    if not ctx.parse_module_file(f, "yang"):
                        raise RuntimeError(f"failed to parse synthetic {name}")
            yield _SyntheticModel(ctx, model_dir)
        finally:
            ctx.destroy()


def _expect_violation(check, model, *expected_fragments):
    with pytest.raises(AssertionError) as exc_info:
        check(model)
    message = str(exc_info.value)
    for fragment in expected_fragments:
        assert fragment in message, (
            f"{check.__name__} fired but did not mention {fragment!r}:\n{message}")


def test_neg_every_table_has_row_layer():
    with _synthetic(("sonic-neg-rowlayer", _module_src("sonic-neg-rowlayer", """
    container NO_ROWS {
      leaf only_a_leaf { type string; }
    }
"""))) as model:
        _expect_violation(test_every_table_has_row_layer, model,
                          "sonic-neg-rowlayer:NO_ROWS")


def test_neg_table_has_one_kind_of_row_layer():
    with _synthetic(("sonic-neg-mixed", _module_src("sonic-neg-mixed", """
    container MIXED {
      list MIXED_LIST {
        key "name";
        leaf name { type string; }
        leaf other { type string; }
      }
      container global {
        leaf setting { type string; }
      }
    }
"""))) as model:
        _expect_violation(test_table_has_one_kind_of_row_layer, model,
                          "sonic-neg-mixed:MIXED")


def test_neg_stale_upstream_mixed_row_layer_exemption():
    # Nothing in a synthetic tree carries both row layers, so every name in the
    # allowlist is stale — which is the direction that would otherwise let an
    # exemption outlive the shape it was granted for.
    with _synthetic(("sonic-neg-nomixed", _module_src("sonic-neg-nomixed", """
    container PLAIN {
      list PLAIN_LIST {
        key "name";
        leaf name { type string; }
      }
    }
"""))) as model:
        _expect_violation(test_table_has_one_kind_of_row_layer, model,
                          "stale exemption",
                          "sonic-telemetry_client:TELEMETRY_CLIENT")


def test_neg_sibling_lists_have_distinct_key_counts():
    with _synthetic(("sonic-neg-siblings", _module_src("sonic-neg-siblings", """
    container TWINS {
      list TWINS_LIST {
        key "name";
        leaf name { type string; }
        leaf shared { type string; }
      }
      list TWINS_OTHER_LIST {
        key "other_name";
        leaf other_name { type string; }
        leaf shared { type string; }
      }
    }
"""))) as model:
        # The second list's only non-key field also resolves against the first,
        # so it is unreachable — and it is reported as shadowed rather than as
        # merely fragile only because key leaves are subtracted first. With full
        # leaf sets compared, `other_name` would make it look wider than the row
        # it holds and this would under-report.
        _expect_violation(test_sibling_lists_have_distinct_key_counts, model,
                          "sonic-neg-siblings:TWINS",
                          "TWINS_OTHER_LIST",
                          "unreachable")


def test_neg_nested_list_does_not_sit_beside_leaves():
    with _synthetic(("sonic-neg-nested", _module_src("sonic-neg-nested", """
    container NESTED {
      list NESTED_LIST {
        key "name";
        leaf name { type string; }
        leaf beside { type string; }
        list INNER {
          key "inner_name";
          leaf inner_name { type string; }
          leaf value { type string; }
        }
      }
    }
"""))) as model:
        _expect_violation(test_nested_list_does_not_sit_beside_leaves, model,
                          "sonic-neg-nested:NESTED:NESTED_LIST:INNER")


def test_neg_nested_list_maps_are_registered():
    with _synthetic(("sonic-neg-map", _module_src("sonic-neg-map", """
    container UNREG_MAP {
      list UNREG_MAP_LIST {
        key "name";
        leaf name { type string; }
        list INNER {
          key "inner_name";
          leaf inner_name { type string; }
          leaf value { type string; }
        }
      }
    }
"""))) as model:
        _expect_violation(test_nested_list_maps_are_registered, model,
                          "unregistered: sonic-neg-map:UNREG_MAP:UNREG_MAP_LIST")


def test_neg_registered_map_list_shape():
    # A registered name whose shape breaks the three assumptions
    # _xlateType1MapList makes: two inner lists, a composite inner key, and two
    # non-key leaves. Registration alone would have let all three through.
    with _synthetic(("sonic-neg-mapshape", _module_src("sonic-neg-mapshape", """
    container CABLE_LENGTH {
      list CABLE_LENGTH_LIST {
        key "name";
        leaf name { type string; }
        list INNER {
          key "port class";
          leaf port { type string; }
          leaf class { type string; }
          leaf value { type string; }
          leaf extra { type string; }
        }
        list SECOND_INNER {
          key "port";
          leaf port { type string; }
          leaf value { type string; }
        }
      }
    }
"""))) as model:
        _expect_violation(test_nested_list_maps_are_registered, model,
                          "2 inner lists",
                          "inner key is ['port', 'class']",
                          "non-key")


def test_neg_stale_type_1_map_list_entry():
    # No map-shaped list at all, so every registered name is stale.
    with _synthetic(("sonic-neg-nomap", _module_src("sonic-neg-nomap", """
    container PLAIN {
      list PLAIN_LIST {
        key "name";
        leaf name { type string; }
      }
    }
"""))) as model:
        _expect_violation(test_nested_list_maps_are_registered, model,
                          "stale entry: CABLE_LENGTH_LIST")


def test_neg_single_list_table_is_named_after_its_table():
    with _synthetic(("sonic-neg-listname", _module_src("sonic-neg-listname", """
    container MISNAMED {
      list SOMETHING_ELSE_LIST {
        key "name";
        leaf name { type string; }
      }
    }
"""))) as model:
        _expect_violation(test_single_list_table_is_named_after_its_table, model,
                          "sonic-neg-listname:MISNAMED",
                          "expected MISNAMED_LIST")


def test_neg_every_table_list_name_carries_LIST():
    # Two lists, so _xlateContainer does not consult the name on the read side
    # and the single-list check does not apply -- but _revXlateList still gates
    # on "_LIST" and would never write ENTRIES back.
    with _synthetic(("sonic-neg-nolist", _module_src("sonic-neg-nolist", """
    container NOSUFFIX {
      list NOSUFFIX_LIST {
        key "name";
        leaf name { type string; }
      }
      list ENTRIES {
        key "a b";
        leaf a { type string; }
        leaf b { type string; }
      }
    }
"""))) as model:
        _expect_violation(test_every_table_list_name_carries_LIST, model,
                          "sonic-neg-nolist:NOSUFFIX:ENTRIES")


def test_neg_no_container_below_the_row_layer():
    with _synthetic(("sonic-neg-subcont", _module_src("sonic-neg-subcont", """
    container SUBCONT {
      list SUBCONT_LIST {
        key "name";
        leaf name { type string; }
        container nested { leaf value { type string; } }
      }
    }
"""))) as model:
        _expect_violation(test_no_container_below_the_row_layer, model,
                          "sonic-neg-subcont:SUBCONT:SUBCONT_LIST:nested")


def test_neg_no_list_below_a_container_row():
    with _synthetic(("sonic-neg-sublist", _module_src("sonic-neg-sublist", """
    container SUBLIST {
      container global {
        leaf setting { type string; }
        list INNER_LIST {
          key "name";
          leaf name { type string; }
        }
      }
    }
"""))) as model:
        _expect_violation(test_no_list_below_a_container_row, model,
                          "sonic-neg-sublist:SUBLIST:global:INNER_LIST")


def test_neg_every_list_has_keys():
    # A config list must declare a key, so the keyless shape is only
    # expressible as `config false` -- which is enough to prove the check sees
    # a list with no key statement.
    with _synthetic(("sonic-neg-nokey", _module_src("sonic-neg-nokey", """
    container NOKEY {
      list NOKEY_LIST {
        config false;
        leaf name { type string; }
      }
    }
"""))) as model:
        _expect_violation(test_every_list_has_keys, model,
                          "sonic-neg-nokey:NOKEY:NOKEY_LIST")


def test_neg_table_names_are_unique_across_modules():
    body = """
    container DUPLICATE {
      list DUPLICATE_LIST {
        key "name";
        leaf name { type string; }
      }
    }
"""
    with _synthetic(("sonic-neg-dup-a", _module_src("sonic-neg-dup-a", body)),
                    ("sonic-neg-dup-b", _module_src("sonic-neg-dup-b", body))) as model:
        _expect_violation(test_table_names_are_unique_across_modules, model,
                          "DUPLICATE",
                          "sonic-neg-dup-a",
                          "sonic-neg-dup-b")


def test_neg_table_names_unique_check_sees_event_schemas():
    # The engine registers event payload containers into the same map, so a
    # collision between one of those and a real table is a real collision. The
    # nine row-shape checks filter them out; this one must not.
    events_common = """module sonic-events-common {
  yang-version 1.1;
  namespace "http://nexthop.ai/yang/sonic-events-common";
  prefix evtcmn;
  extension EVENT_SEVERITY_3 { description "marks an event payload container"; }
}
"""
    table_body = """
    container COLLIDES {
      list COLLIDES_LIST {
        key "name";
        leaf name { type string; }
      }
    }
"""
    event_body = """
    container COLLIDES {
      evtcmn:EVENT_SEVERITY_3;
      leaf detail { type string; }
    }
"""
    with _synthetic(
            ("sonic-events-common", events_common),
            ("sonic-neg-evt-table", _module_src("sonic-neg-evt-table", table_body)),
            ("sonic-neg-evt-payload",
             _module_src("sonic-neg-evt-payload", event_body).replace(
                 "prefix sonic_neg_evt_payload;",
                 "prefix sonic_neg_evt_payload;\n  import sonic-events-common "
                 "{ prefix evtcmn; }"))) as model:
        _expect_violation(test_table_names_are_unique_across_modules, model,
                          "COLLIDES",
                          "sonic-neg-evt-payload")


def test_neg_top_container_matches_module_name():
    src = """module sonic-neg-topname {
  yang-version 1.1;
  namespace "http://nexthop.ai/yang/sonic-neg-topname";
  prefix sonic_neg_topname;
  container something-else {
    container TBL {
      list TBL_LIST {
        key "name";
        leaf name { type string; }
      }
    }
  }
}
"""
    with _synthetic(("sonic-neg-topname", src)) as model:
        _expect_violation(test_top_container_matches_module_name, model,
                          "sonic-neg-topname: top-level container something-else")


def test_neg_report_accumulates_every_violation():
    # The module docstring promises each check reports every violation rather
    # than stopping at the first. Two tables of the same kind, both named.
    with _synthetic(("sonic-neg-accum", _module_src("sonic-neg-accum", """
    container FIRST_BAD {
      leaf only_a_leaf { type string; }
    }
    container SECOND_BAD {
      leaf also_only_a_leaf { type string; }
    }
"""))) as model:
        _expect_violation(test_every_table_has_row_layer, model,
                          "sonic-neg-accum:FIRST_BAD",
                          "sonic-neg-accum:SECOND_BAD")


def test_neg_allowlisted_mixed_row_layer_with_single_keyed_list():
    # The exemption is granted because TELEMETRY_CLIENT_LIST takes two keys and
    # so declines the one-segment `TELEMETRY_CLIENT|Global` row on count. Add a
    # single-keyed list to that same container and the row is claimed by the
    # list instead, silently -- the shape the exemption was never granted for.
    src = """module sonic-telemetry_client {
  yang-version 1.1;
  namespace "http://nexthop.ai/yang/sonic-telemetry_client";
  prefix sonic_telemetry_client;
  container sonic-telemetry_client {
    container TELEMETRY_CLIENT {
      list TELEMETRY_CLIENT_LIST {
        key "prefix name";
        leaf prefix { type string; }
        leaf name { type string; }
      }
      list TELEMETRY_CLIENT_ONE_KEY_LIST {
        key "name";
        leaf name { type string; }
      }
      container Global {
        leaf retry_interval { type string; }
      }
    }
  }
}
"""
    with _synthetic(("sonic-telemetry_client", src)) as model:
        _expect_violation(test_table_has_one_kind_of_row_layer, model,
                          "sonic-telemetry_client:TELEMETRY_CLIENT",
                          "exemption no longer applies",
                          "TELEMETRY_CLIENT_ONE_KEY_LIST")
