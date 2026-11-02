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
#
# In the pmon image, as root: each group from a fresh setup (R=''), then
# Python's component.py through driver.py.  Out: /g/out/<group>.{rec,calls,stdout}
# PLATFORM_COMMON_DIR and MLNX_PLATFORM_API_DIR name the Python trees; see
# README.md.
#
# Stops at the first group that does not run to the end, so a regeneration
# never leaves a partial .rec or .calls behind looking like a finished one.
set -euo pipefail
: "${PLATFORM_COMMON_DIR:?}" "${MLNX_PLATFORM_API_DIR:?}"
mkdir -p /g/out
for g in generic sn2201 sn4280 simx bmc; do
  PLAT=$(cat "/g/$g.plat")
  R='' BIN=/tmp/c/bin PLAT="$PLAT" sh /g/setup.sh || { echo "setup failed $g" >&2; exit 1; }
  (cd /tmp && env -u LANG -u LC_ALL -u LC_CTYPE PLATFORM="$PLAT" PATH="/tmp/c/bin:$PATH" PYTHONUTF8=0 PYTHONCOERCECLOCALE=0 PYTHONDONTWRITEBYTECODE=1 \
    python3 -u /g/driver.py "/g/$g.json" "/g/out/$g.rec") > "/g/out/$g.stdout" 2> "/g/out/$g.stderr" \
    || { echo "driver.py failed for $g; see /g/out/$g.stderr" >&2; exit 1; }
  cp /tmp/c/calls "/g/out/$g.calls"
  echo "$g $(wc -l < "/g/out/$g.rec") steps"
done
chown -R "$(stat -c %u:%g /g/setup.sh)" /g/out
