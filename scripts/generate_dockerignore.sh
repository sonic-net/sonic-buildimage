#!/bin/bash

set -e

docker_dir="$1"
docker_prefix="$2"

cd "$docker_dir" || { echo "No such directory: $docker_dir"; exit 1; }

debs_var="${docker_prefix}_debs"
dbg_debs_var="${docker_prefix}_dbg_debs"
pydebs_var="${docker_prefix}_pydebs"
whls_var="${docker_prefix}_whls"
files_var="${docker_prefix}_files"

debs="${!debs_var}"
pydebs="${!pydebs_var}"
dbg_debs="${!dbg_debs_var}"
whls="${!whls_var}"
files="${!files_var}"

dockerignore=".dockerignore"
echo "Dockerfile.j2" > "$dockerignore"
echo "debs/*" >> "$dockerignore"
echo "python-debs/*" >> "$dockerignore"
echo "python-wheels/*" >> "$dockerignore"
echo "files/*" >> "$dockerignore"

if [[ -n "$debs" ]]; then
  for deb in $debs; do
    echo "!debs/$deb" >> "$dockerignore"
  done
fi

if [[ -n "$dbg_debs" ]]; then
  for deb in $dbg_debs; do
    echo "!debs/$deb" >> "$dockerignore"
  done
fi

if [[ -n "$pydebs" ]]; then
  for deb in $dbg_debs; do
    echo "!python-debs/$deb" >> "$dockerignore"
  done
fi

if [[ -n "$whls" ]]; then
  for whl in $whls; do
    echo "!python-wheels/$whl" >> "$dockerignore"
  done
fi

if [[ -n "$files" ]]; then
  for f in $files; do
    echo "!files/$f" >> "$dockerignore"
  done
fi
