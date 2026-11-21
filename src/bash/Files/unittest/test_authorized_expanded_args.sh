#!/bin/bash
set -euo pipefail

if ! grep -q '^#define BASH_SHELL_EXECVE_PLUGIN 1$' ../config.h; then
    exit 0
fi

test_dir=$(mktemp -d)
trap 'rm -rf "$test_dir"' EXIT
cc -shared -fPIC -o "$test_dir/auth_argv_plugin.so" auth_argv_plugin.c
cc -shared -fPIC -o "$test_dir/config_redirect.so" config_redirect.c -ldl
printf 'plugin=%s\n' "$test_dir/auth_argv_plugin.so" > "$test_dir/bash_plugins.conf"

export BASH_PLUGIN_TEST_CONFIG="$test_dir/bash_plugins.conf"
export LD_PRELOAD="$test_dir/config_redirect.so"

../bash --noprofile --norc -c 'value=allowed; /bin/true "$value"'
if ../bash --noprofile --norc -c 'value=blocked; /bin/true "$value"'; then
    echo 'plugin authorized a variable-expanded blocked argument' >&2
    exit 1
fi
if ../bash --noprofile --norc -c 'value="allowed blocked"; /bin/true $value'; then
    echo 'plugin authorized a blocked argument created by word splitting' >&2
    exit 1
fi
