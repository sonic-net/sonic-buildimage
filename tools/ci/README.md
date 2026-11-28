# Shared buildimage CI tools

[runner/](runner/README.md) provisions and checks the dedicated GitHub Actions
runner for complete VS image builds. Its source location is shared across
container migrations. Installed service paths, repository routing and runner
labels are documented in the runner guide.

`tests/package_cache_test.py` checks the native `Makefile.cache` implementation
across fresh checkouts, source changes and architecture changes. It creates a
small file artifact and needs no package build or privileged operation.

```sh
python3 -B -m unittest discover -s tools/ci/runner -p '*_test.py'
python3 -B -m unittest discover -s tools/ci/tests -p '*_test.py'
```
