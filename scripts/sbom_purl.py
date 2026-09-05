#!/usr/bin/env python3
"""
sbom_purl.py — write package URLs the way everything else reads them.

Shared by sbom_fragment.py, which names what a recipe built, and
build_sbom.py, which names what the build observed installed. Both used
to assemble the string by interpolation, so one Debian version arrived
as `3.5.6-1~deb13u2+fips` from the recipe and `3.5.6-1~deb13u2%2Bfips`
from syft — two spellings of one package, and byte comparison calls
those two packages. One writer means the two cannot disagree again.

Encoding is not cosmetic here. The specification gives `@`, `?`, `#`,
`/` and `:` meaning inside a package URL, so a name or version carrying
one has to escape it or the reader splits the string in the wrong
place. The identifiers this build emits carry them routinely: a Debian
version uses `+` for a local rebuild and a leading `N:` for an epoch,
and a Go module version can contain `/`.

Qualifiers are sorted by key, which the specification requires and
which also means two producers listing the same qualifiers in a
different order still write the same string.
"""

import urllib.parse
from typing import Optional


def encode(text) -> str:
    """Percent-encode one part of a package URL.

    Nothing is left safe: `/` separates namespace segments and `:`
    separates the scheme, so a name or version containing either must
    encode it. The unreserved set (`-`, `.`, `_`, `~`) is preserved by
    urllib and is what Debian and Go versions are mostly made of, so
    ordinary identifiers come through unchanged.
    """
    return urllib.parse.quote(str(text or ""), safe="")


def build(
    type_: str,
    name: str,
    version: Optional[str] = None,
    namespace: Optional[str] = None,
    qualifiers: Optional[dict] = None,
) -> str:
    """Assemble a package URL from its parts, encoding each one.

    ``namespace`` may itself contain `/` — a Go module path is a
    namespace of several segments — so it is split and each segment
    encoded separately, which keeps the separators that belong and
    escapes the ones that do not.
    """
    out = f"pkg:{type_.lower()}/"
    if namespace:
        segments = [s for s in str(namespace).split("/") if s]
        if segments:
            out += "/".join(encode(s) for s in segments) + "/"
    out += encode(name)
    if version:
        out += "@" + encode(version)
    if qualifiers:
        pairs = [
            f"{k.lower()}={encode(v)}"
            for k, v in sorted(qualifiers.items())
            if v not in (None, "")
        ]
        if pairs:
            out += "?" + "&".join(pairs)
    return out


def with_version(purl: str, version: str) -> str:
    """Return ``purl`` stating ``version`` instead of the one it carries.

    Used when two records for one package merge and the loser held the
    version that was actually installed. The version sits between the
    last `@` of the path and the `?` that begins the qualifiers, so both
    tails are cut before the split and put back afterwards — a qualifier
    value may itself contain an `@`, which is why this cannot be done on
    the whole string.
    """
    head, sep, tail = purl.partition("?")
    if not sep:
        head, hsep, htail = purl.partition("#")
        tail = htail
        sep = hsep
    base, at, _ = head.rpartition("@")
    if not at:
        base = head
    return base + "@" + encode(version) + sep + tail


def qualifiers_of(purl: str) -> dict:
    """Read a package URL's qualifiers back into a dict.

    The subpath is cut first: `#` ends the qualifier string, and a
    value may legitimately contain one once encoded.
    """
    if not purl or "?" not in purl:
        return {}
    tail = purl.split("?", 1)[1].split("#", 1)[0]
    out = {}
    for pair in tail.split("&"):
        if not pair:
            continue
        key, _, value = pair.partition("=")
        if key:
            out[key.lower()] = urllib.parse.unquote(value)
    return out


def with_qualifier(purl: str, key: str, value: str) -> str:
    """Return ``purl`` carrying ``key=value``, leaving an existing one alone.

    Used when a component that knows what a package is merges with one
    that knew where it was installed: the qualifier the loser carried
    is the only record of it, and re-sorting the merged set keeps the
    result a well-formed identifier rather than an appended string.
    """
    if not purl or not value:
        return purl
    quals = qualifiers_of(purl)
    if key.lower() in quals:
        return purl
    quals[key.lower()] = value
    head = purl.split("?", 1)[0]
    subpath = ""
    if "#" in purl:
        subpath = "#" + purl.split("#", 1)[1]
        head = head.split("#", 1)[0]
    pairs = [f"{k}={encode(v)}" for k, v in sorted(quals.items())]
    return head + "?" + "&".join(pairs) + subpath
