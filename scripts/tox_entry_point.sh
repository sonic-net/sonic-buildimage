#!/bin/sh

# Copyright © 2023 Orange
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cd $(dirname $0)
cd ..
export DEBIAN_FRONTEND=noninteractive
sudo -E apt-get -qq -y update 2>&1 >/dev/null || \
        apt-get -qq -y update 2>&1 >/dev/null
sudo -E apt-get -qq -y install git gzip python3-pip tox >&1 >/dev/null || \
        apt-get -qq -y install git gzip python3-pip tox >&1 >/dev/null
# Run tests inside a python tox virtual environment
# the PBR_VERSION variable is normally not needed and is here to avoid troubles in some CI
# that raise an exception for versioning w/o sdsit tarball or access to an upstream git repo
export PBR_VERSION=5.10.0
tox
