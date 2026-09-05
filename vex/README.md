# SONiC VEX statements

This directory holds **VEX** (Vulnerability Exploitability eXchange)
statements that suppress or contextualise specific CVE findings against
the SONiC SBOM. They are consumed by `scripts/sbom_vuln_scan.py` (via
the `--vex <dir>` flag) and merged into the vulnerability report.

## Layout

```
vex/
├── README.md                   (this file)
├── <triage-name>.json          (curated, hand-authored statements)
├── <triage-name>.json
├── ...
└── auto/                       (auto-extracted from patch metadata)
    └── <source-tree>/<patch-name>.json
```

VEX files use **OpenVEX in JSON form** (`.json`). YAML files are also
walked by the scanner but grype rejects YAML — JSON is the format the
toolchain actually consumes, so curated files should be JSON too.

The top-level `*.json` files are **curated** — they require human
analysis and a written justification. The `auto/` subdirectory is
**machine-generated** by `scripts/sbom_extract_vex_from_patches.py`,
which scans the source tree for patches that mention CVEs in their
filename or header. Regenerate at any time; auto-VEX files are
overwritten idempotently.

## Why VEX

The SBOM records every component that ships, including the **upstream
ancestor versions** for SONiC's locally-patched packages (FRR, openssh,
kernel, etc.). Grype matches CVEs against ancestor versions, which
means:

> Even though we backported the fix as a SONiC patch, grype flags the
> CVE because the *upstream* version in `pedigree.ancestors` is still
> the vulnerable one.

A VEX statement says: "yes, we know about CVE-X. It applies to the
upstream version listed in our pedigree, but our build is `not_affected`
because we backported the fix as `<this patch>`."

Without VEX, every patched component generates noise in the vuln report.

## Schema (OpenVEX)

We use [OpenVEX v0.2.0](https://openvex.dev/) because it's natively
supported by grype and is the simplest of the three common VEX
formats (the others are CycloneDX VEX and CSAF VEX).

Minimal example:

```json
{
  "@context": "https://openvex.dev/ns/v0.2.0",
  "@id": "https://github.com/sonic-net/sonic-buildimage/vex/2024-01-15-frr-cve-12345",
  "author": "brad@nexthop.ai",
  "timestamp": "2024-01-15T00:00:00Z",
  "version": 1,
  "statements": [
    {
      "vulnerability": {"name": "CVE-2024-12345"},
      "products": [{"@id": "pkg:deb/sonic/frr@10.5.4-sonic-0"}],
      "status": "not_affected",
      "justification": "vulnerable_code_not_in_execute_path",
      "impact_statement": "Fixed by SONiC patch src/sonic-frr/patch/0105-bgpd-Show-all-advertised-paths.patch applied at build time. Verified by manually walking the call graph from the daemon entry points.",
      "references": ["https://github.com/FRRouting/frr/commit/abc123"]
    }
  ]
}
```

### Status values

`status:` must be one of:

| Status | Meaning |
|---|---|
| `not_affected` | The CVE does **not** apply to this build. Always pair with a `justification`. |
| `affected` | The CVE applies and a fix is pending. Include an `action_statement` describing the mitigation. |
| `fixed` | The CVE was applicable but is now resolved (a fix has shipped in this version). |
| `under_investigation` | Status not yet determined. (auto-VEX uses this for loose CVE mentions.) |

### Justifications

When `status: not_affected`, OpenVEX requires a `justification:`:

| Justification | Meaning |
|---|---|
| `component_not_present` | The vulnerable component isn't actually in the build. |
| `vulnerable_code_not_present` | The component is here but the vulnerable code path was removed/replaced. |
| `vulnerable_code_not_in_execute_path` | The vulnerable code is present but never reachable. |
| `vulnerable_code_cannot_be_controlled_by_adversary` | Reachable but not exploitable. |
| `inline_mitigations_already_exist` | Reachable and exploitable upstream but mitigated by SONiC's wrapper / config. |

`vulnerable_code_not_in_execute_path` is the typical choice when a
SONiC patch backports the upstream fix.

### Products

The `products:` array identifies which SBOM components the statement
applies to. The simplest form is an `@id` with a PURL:

```yaml
products:
  - '@id': pkg:deb/sonic/frr@10.5.4-sonic-0
```

A PURL **prefix** also works (no version specified) — useful for
"applies to whatever version we ship" statements:

```yaml
products:
  - '@id': pkg:deb/sonic/frr
```

For statements covering the whole product family (e.g., affecting
every version of openssh-server SONiC has ever shipped), use:

```yaml
products:
  - '@id': pkg:generic/openssh
```

## Triage workflow

When `sbom_vuln_scan.py` reports a new high/critical finding:

1. Read the upstream advisory linked in the report (`dataSource` URL).
2. Check whether SONiC's pedigree includes a patch that fixes it. The
   easiest way is to grep `src/<component>/patch/` and
   `src/<component>/patches/` for the CVE id.
3. If a patch exists:
   - Confirm the patch actually addresses the CVE (read the diff, not
     just the filename).
   - Add a curated VEX file under `vex/` with `status: not_affected,
     justification: vulnerable_code_not_in_execute_path` and a brief
     `impact_statement` linking the patch.
4. If no patch exists but the code path is unreachable in SONiC:
   - Document the unreachability in `impact_statement` (e.g., "SONiC
     does not enable feature X; the vulnerable function is never
     compiled in" or "this CVE applies to the Windows DLL loader, not
     used on Linux").
   - Use `vulnerable_code_not_in_execute_path` or
     `vulnerable_code_cannot_be_controlled_by_adversary` as appropriate.
5. If the CVE is genuinely applicable:
   - Decide on mitigation (patch backport, version bump, config
     change, accept risk).
   - Add an `affected` or `under_investigation` VEX entry with a
     `action_statement`.

## Re-running auto-VEX extraction

```
python3 scripts/sbom_extract_vex_from_patches.py
```

Re-run after pulling new patches. The script overwrites files in
`auto/` idempotently. Curated files under `vex/` itself are never
touched.

## Consumption

The vuln scanner picks up everything under the directory:

```bash
python3 scripts/sbom_vuln_scan.py \
    --vex vex/ \
    --output target/sonic-broadcom.bin.vuln.json \
    target/sonic-broadcom.bin.cdx.json
```

grype walks the directory tree, parses both YAML and JSON VEX files,
and applies suppressions.

To verify a VEX statement actually took effect, generate two reports
(with and without `--vex`) and diff them:

```bash
python3 scripts/sbom_vuln_scan.py -o /tmp/no-vex.json sbom.cdx.json
python3 scripts/sbom_vuln_scan.py --vex vex/ -o /tmp/with-vex.json sbom.cdx.json
python3 scripts/sbom_vuln_diff.py /tmp/no-vex.json /tmp/with-vex.json
```

The diff's `Removed:` section is the set of findings the VEX
suppressed.
