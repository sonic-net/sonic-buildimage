"""mega_gen -- generic mega-container merge generator.

For any chosen feature subset, mechanically composes `dockers/docker-mega/*`
and the associated build-template edits out of each feature's original
per-container sources in a sonic-buildimage tree.

See `optimizations/mega-generic/README.md` and the
`generic_mega-container_merge_script` plan for the full design. This package
is under active construction; only the pieces documented in the README as
implemented should be assumed to exist.
"""
