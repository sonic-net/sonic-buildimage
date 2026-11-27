---
applyTo: "platform/**"
---

# Instructions for `platform/`

`platform/<name>/` holds the build rules for one ASIC or target (`broadcom`, `mellanox`, `marvell-prestera`, `marvell-teralynx`, `vs`, `vpp`, `aspeed`, ...). A few directories are shared (`template`, `components`, `pddf`, `checkout`, `generic`). Per-device configuration belongs in `device/`, not here.

## Build rules

- `rules.mk` is the entry point of a platform and `include`s every other `.mk` of the directory through `$(PLATFORM_PATH)`. A new `.mk` file is not built until it is included there.
- Each `.mk` that defines a cached target has a `.dep` file with the same name (`docker-pde.mk` and `docker-pde.dep`). Copy a neighbouring pair instead of writing one from scratch. The `.dep` lists the `.mk` and `.dep` themselves and the sources (`git ls-files`) in `DEP_FILES`, and sets `_CACHE_MODE`, `_DEP_FLAGS` and `_DEP_FILES` for the target.
- A variable that changes a target's output must be part of that target's `_DEP_FLAGS` (or `SONIC_COMMON_FLAGS_LIST`). CI audits pull requests for build-cache inputs that are not tracked.
- Reuse `platform/template/` for `docker-syncd-*` and `docker-gbsyncd-*` rules instead of duplicating them.
- Naming: lowercase, hyphenated file names (`platform-modules-<vendor>.mk`, `docker-syncd-<platform>.mk`) and UPPER_CASE Make variables. Use tabs in recipes.
- Several directories are git submodules (for example `platform/vpp`, `platform/alpinevs`, `platform/broadcom/saibcm-modules`, `platform/*/sonic-platform-modules-arista`). Change them in their own repository and only update the submodule pointer here.

## Files and binaries

- Do not commit binaries. Fetch SDKs, firmware and packages at build time from a pinned version or URL in the `.mk` file (see `platform/mellanox/component-versions.mk`).
- A pull request that adds a binary file fails the `binary-artifacts-check` workflow unless the path is listed in `.github/binary-artifacts-allowlist.txt`. Add an entry with a comment explaining why the file has to be committed.
- Shell scripts use `#!/bin/bash` and 4-space indentation. Python follows PEP 8.

## CI and pull requests

- Pull requests only build the platform jobs mapped to the changed paths by `scripts/selective_build.py`. When adding a platform directory or a build job, update `PLATFORM_PATHS` (and `ALL_PLATFORMS`) in that script in the same pull request.
- Changes to shared files (`Makefile`, `Makefile.work`, `slave.mk`, `rules/`, pipeline files) build every platform.
- Commit format: `[platform/<name>]: Short description`. Every commit needs a `Signed-off-by:` line (`git commit -s`).
- Keep a pull request to one platform and one logical change.
