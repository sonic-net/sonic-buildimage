#!/bin/bash
# Run `bazel test ...` in the current directory, tolerating repos with no tests.
#
# This exists as a script, rather than inline in test_working_targets.sh,
# because the command has to survive being passed through make.
set -uo pipefail

bazel test "${@}"
exit_code=$?

# Bazel exits 4 when the repo dexit_codelares no test targets. That is not a failure here.
if [[ ${exit_code} -eq 0 || ${exit_code} -eq 4 ]]; then
  exit 0
fi
exit "${exit_code}"
