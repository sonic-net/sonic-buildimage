"""Every ConfigDB table container must have a row layer - either a `list`
(keyed rows) or an inner `container` (named singleton row). A table whose
direct children are only leaf/leaf-list has no row key for ConfigDB to
serialize against and cannot be persisted to Redis. The DNS_OPTIONS
schema originally had this shape and was unusable in a live system; this
test guards against that class of regression for all sonic-* models."""
import os
from glob import glob

import libyang as ly


def _is_event_schema(snode):
    """A container that carries an extension from sonic-events-common
    (ALARM_SEVERITY_*, EVENT_SEVERITY_*, …) is an event/notification payload
    schema, not a ConfigDB table — it doesn't need a row layer."""
    for ext in snode.extensions():
        if ext.module().name() == "sonic-events-common":
            return True
    return False


def test_every_table_has_row_layer(yang_model):
    yang_files = glob(os.path.join(yang_model.model_dir, "sonic-*.yang"))
    violations = []
    for path in yang_files:
        module_name = os.path.splitext(os.path.basename(path))[0]
        module = yang_model.ctx.get_module(module_name)
        if module is None:
            continue
        top = next(iter(module.children(types=(ly.SNode.CONTAINER,))), None)
        if top is None or top.name() != module.name():
            continue
        for table in top.children(types=(ly.SNode.CONTAINER,)):
            if _is_event_schema(table):
                continue
            has_list = any(table.children(types=(ly.SNode.LIST,)))
            has_inner_container = any(table.children(types=(ly.SNode.CONTAINER,)))
            if not has_list and not has_inner_container:
                violations.append(f"{module.name()}:{table.name()}")

    assert not violations, (
        "ConfigDB table containers without a row layer (list or inner "
        "container) cannot be serialized to Redis:\n  "
        + "\n  ".join(violations)
    )
