"""Deployment-specific minigraph parser hook."""

MTU_1500_HWSKUS = {"M2-W5000-48CON2SFP"}
DEFAULT_MTU = "1500"
COMMUNITY_DEFAULT_MTU = "9100"

def parse_xml(base_parse_xml, *args, **kwargs):
    """Apply M2-W5000-48CON2SFP specific defaults to parsed minigraph data."""
    results = base_parse_xml(*args, **kwargs)

    hwsku = (results.get("DEVICE_METADATA", {})
                    .get("localhost", {})
                    .get("hwsku"))
    if hwsku not in MTU_1500_HWSKUS:
        return results

    for table in ("PORT", "PORTCHANNEL"):
        for entry in results.get(table, {}).values():
            if isinstance(entry, dict) and entry.get("mtu", COMMUNITY_DEFAULT_MTU) == COMMUNITY_DEFAULT_MTU:
                entry["mtu"] = DEFAULT_MTU

    return results
