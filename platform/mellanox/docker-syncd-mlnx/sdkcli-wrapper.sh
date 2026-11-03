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

# Runs the SDK CLI. Every command is passed through unchanged except
# "sdkcli start", which always starts the daemon in daemon mode so that it
# logs to syslog, and accepts no other arguments.

set -euo pipefail

SDKCLI=/opt/sdkcli/venv/bin/sdkcli

# The SDK CLI treats "start" as a command only when it is the first argument.
if [ "${1-}" = "start" ]; then
    shift
    case "$#:${1-}" in
        0:|1:-d|1:--daemon)
            exec "$SDKCLI" start -d
            ;;
    esac
    cat >&2 <<EOF
Error: 'sdkcli start' does not accept arguments (got: $*).
On this system the SDK CLI daemon is always started in daemon mode,
and it logs to syslog. Run 'sdkcli start' with no arguments.
'-d' or '--daemon' is also accepted, but it is not needed.
EOF
    exit 2
fi

exec "$SDKCLI" "$@"
