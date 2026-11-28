# VS container validation workers

Use the [Bazel OCI workflow](../../../.github/workflows/bazel-oci.yml) to build
and validate the selected VS containers. From the revision to test, run

```sh
gh workflow run bazel-oci.yml --ref "$revision" -f skip_vs=false
```

Add `-f enable_debug_layer=true` to exercise the debug companions. The VS build
publishes runtime archives and any enabled debug archives. Each selected family
gets its own `bazel-x64` validation job that downloads that family's artifact and
runs its installed probes. The job log names the container, and its result
records the installer release, revision and archive digests.

See the [CI guide](README.md#shared-helpers) for the artifact handoff.
