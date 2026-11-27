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
# Enables sdkcli tab completion and the "?" contextual help key binding.
#
# Sourced from /etc/bash.bashrc, so it runs only in interactive bash. It is
# never executed directly and so intentionally has no shebang.
#

# nvue registers its completion through a function that calls
# _get_comp_words_by_ref and __ltrim_colon_completions, both of which come from
# the bash-completion package. Debian ships the loader for it commented out in
# /etc/bash.bashrc, so load it here before the completion is registered,
# otherwise pressing Tab fails with "command not found". The posix check is the
# one Debian guards its own loader with: bash_completion is not posix-safe.
if ! shopt -oq posix; then
    . /usr/share/bash-completion/bash_completion
fi

# Keep activate from touching PS1. It would otherwise export PS1, and
# deactivate restores the value but leaves it exported. An exported PS1 leaks
# into child processes and defeats the [ -z "${PS1-}" ] interactivity test
# used by this container's own /etc/bash.bashrc and /etc/profile.
VIRTUAL_ENV_DISABLE_PROMPT=1

# Activate the venv, then run the setup script.
. /opt/sdkcli/venv/bin/activate
. /opt/sdkcli/venv/bin/sdkcli_env_setup.sh
deactivate

unset VIRTUAL_ENV_DISABLE_PROMPT
