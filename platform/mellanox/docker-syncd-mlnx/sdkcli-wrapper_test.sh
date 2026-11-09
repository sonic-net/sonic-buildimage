#!/bin/bash
#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Tests sdkcli-wrapper.sh against a stub in place of the real SDK CLI, so it
# needs neither the SDK nor a container. Run it by hand:
#
#   platform/mellanox/docker-syncd-mlnx/sdkcli-wrapper_test.sh

# No "set -e": a failing check must not stop the run, so that every check runs
# and all failures are counted and reported. The wrapper's own nonzero exit
# status is also an expected result here.
set -u

unset STUB_RC

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

STUB="$WORK/sdkcli-stub"
CALLS="$WORK/calls"
WRAPPER="$WORK/sdkcli"

# Create a fake stub sdkcli script that records its arguments to the file with
# path $CALLS and exits with status $STUB_RC if set, otherwise 0.
cat > "$STUB" <<'EOF'
#!/bin/bash
printf 'called\n' >> "$CALLS"
for arg in "$@"; do
    printf 'arg:%s\n' "$arg" >> "$CALLS"
done
exit "${STUB_RC:-0}"
EOF
chmod +x "$STUB"

# Point a copy of the wrapper at a fake stub rather than the actual SDK CLI.
sed "s|^SDKCLI=.*|SDKCLI=$STUB|" "$HERE/sdkcli-wrapper.sh" > "$WRAPPER"
chmod +x "$WRAPPER"
if ! grep -qxF "SDKCLI=$STUB" "$WRAPPER"; then
    echo "FAIL: could not point the wrapper at the stub;" \
         "has the SDKCLI= line changed?" >&2
    exit 1
fi

failures=0

# Run the wrapper with the given arguments, and capture the output.
# Sets $rc to the wrapper's exit status, $out to its stderr, and combines
# the lines the stub writes in the $CALLS log file into a single line
# it stores in $calls.
# Usage:
#   run_wrapper ARGS...
run_wrapper() {
    # Empty the stub's log, so it holds only this run's calls.
    : > "$CALLS"

    # Run the wrapper, telling the stub where to log its calls.
    # Do not put any commands between the following two lines, or rc may capture
    # the wrong command's status.
    out=$(CALLS="$CALLS" "$WRAPPER" "$@" 2>&1 >/dev/null)
    rc=$?

    # Join the stub's log onto one line, so it compares as a single string.
    calls=$(tr '\n' ' ' < "$CALLS")
}

# Describe how the run differed from what was expected, or print nothing.
# Usage:
#   mismatch EXPECTED_RC EXPECTED_CALLS
mismatch() {
    if [ "$rc" -ne "$1" ] || [ "$calls" != "$2" ]; then
        echo "exit $rc (want $1), stub saw '$calls' (want '$2')"
    fi
}

# Report the result of a check.
# Usage:
#   report KIND NAME PROBLEM
# If PROBLEM is empty, prints "PASS: $KIND: $NAME", otherwise
# prints "FAIL: $KIND: $NAME: $PROBLEM".
report() {
    local kind=$1 name=$2 problem=$3
    if [ -z "$problem" ]; then
        echo "PASS: $kind: $name"
    else
        echo "FAIL: $kind: $name: $problem"
        failures=$((failures + 1))
    fi
}

# Print the sdkcli command line from the given arguments.
# Used as a string for test case names.
# Usage:
#   command_line ARGS...
command_line() {
    printf 'sdkcli'
    # With no arguments, printf would still apply the format once, printing ''.
    if [ $# -gt 0 ]; then
        printf ' %q' "$@"
    fi
}

# Check that the wrapper accepts the given arguments.
# The wrapper is expected to exec the stub without writing anything to stderr,
# and the stub to exit with status $STUB_RC if set, otherwise 0.
# Usage:
#   check_accepted EXPECTED_CALLS ARGS...
check_accepted() {
    local want_calls=$1 want_rc=${STUB_RC:-0} name problem
    shift
    name=$(command_line "$@")
    if [ -n "${STUB_RC-}" ]; then
        name="$name (CLI exits $STUB_RC)"
    fi
    run_wrapper "$@"
    problem=$(mismatch "$want_rc" "$want_calls")
    if [ -z "$problem" ] && [ -n "$out" ]; then
        problem="unexpected stderr: $out"
    fi
    report accept "$name" "$problem"
}

# Check that the wrapper refuses the given arguments.
# It is expected to exit with status 2, with a message on stderr containing
# "does not accept arguments".
# Usage:
#   check_refused ARGS...
check_refused() {
    local name problem
    name=$(command_line "$@")
    run_wrapper "$@"
    problem=$(mismatch 2 "")
    if [ -z "$problem" ] &&
       ! grep -q "does not accept arguments" <<< "$out"; then
        problem="no explanation on stderr"
    fi
    report refuse "$name" "$problem"
}

# The checks below run the wrapper against the stub, so none of them would
# notice it running the wrong binary. The Dockerfile links the SDK CLI's venv
# at /opt/sdkcli/venv, and the wrapper has to run the CLI from there.
want_line="SDKCLI=/opt/sdkcli/venv/bin/sdkcli"
problem=
if ! grep -qxF "$want_line" "$HERE/sdkcli-wrapper.sh"; then
    problem="sdkcli-wrapper.sh does not contain the line '$want_line'"
fi
report wrapper "runs the SDK CLI from /opt/sdkcli/venv" "$problem"

# "sdkcli start" always runs the daemon with -d.
check_accepted "called arg:start arg:-d "  start
check_accepted "called arg:start arg:-d "  start -d
check_accepted "called arg:start arg:-d "  start --daemon

# Anything else after "start" is refused without running the CLI.
check_refused start --debug
check_refused start -d extra
check_refused start extra
check_refused start -d -d

# The real CLI takes "start [log_file] [options]"; a log path would override the
# syslog logging the wrapper forces.
check_refused start /var/log/nvued.log
check_refused start -d /var/log/nvued.log
check_refused start /var/log/nvued.log -d

# Everything else is passed through unchanged. "start" is only special as the
# first argument, and an argument with a space in it stays one argument.
check_accepted "called "
check_accepted "called arg:--help "          --help
check_accepted "called arg:show arg:port "   show port
check_accepted "called arg:show arg:start "  show start
check_accepted "called arg:set arg:a b "     set "a b"

# The CLI's exit status is the wrapper's, for both paths that run it.
STUB_RC=3 check_accepted "called arg:show arg:port "  show port
STUB_RC=4 check_accepted "called arg:start arg:-d "   start

if [ "$failures" -ne 0 ]; then
    echo "$failures check(s) failed"
    exit 1
fi
echo "All checks passed"
