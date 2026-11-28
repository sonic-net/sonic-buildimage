# Existing package test inputs

These three small, architecture-tagged test DEBs are retained existing inputs.
No test or build rule creates a Debian package. They originated from the
previous package-import tests and are now owned by the direct Syncd consumer.

Each package contains `usr/share/example/value`, a symlink and hardlink to it,
and a `postinst` control script that exits 99. Reading the script is allowed;
executing it would make a test fail. The gzip/XZ samples use `Architecture: all`;
the foreign sample uses `arm64` to test rejection by the AMD64 image contract.
File ownership is 123:456, the regular-file mode is 0640, and the samples retain
a PAX extended attribute for preservation checks.

The tests verify original package SHA-256 values against checked-in expectations.
Changing a sample requires deliberate review; there is no automatic generator
in the Bazel graph.
