try:
    from importlib.util import find_spec
except ImportError:
    from pkgutil import find_loader

    def find_spec(module_name):
        return find_loader(module_name)

import minigraph


if find_spec("minigraph_custom") is not None:
    import minigraph_custom
else:
    minigraph_custom = None


def parse_xml(*args, **kwargs):
    """Parse a minigraph through an optional deployment-specific adapter."""
    if minigraph_custom and hasattr(minigraph_custom, "parse_xml"):
        return minigraph_custom.parse_xml(minigraph.parse_xml, *args, **kwargs)
    return minigraph.parse_xml(*args, **kwargs)
