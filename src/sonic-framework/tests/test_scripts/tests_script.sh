#!/bin/bash

# execute all the steps inside virtual Environment.
# From /sonic-buildimage path.
#
# Creating a new folder inside /tmp and
# Downloading libhiredis packages to /tmp/new path.
mkdir /tmp/new
cd /tmp/new
curl -O https://ftp.debian.org/debian/pool/main/h/hiredis/libhiredis0.14_0.14.1-3_amd64.deb
curl -O https://ftp.debian.org/debian/pool/main/h/hiredis/libhiredis-dev_0.14.1-3_amd64.deb

# Install all the dependencies/pkgs
# which are present in target/debs/bookworm path.
cd -
cd ../../../../target/debs/bookworm
sudo dpkg -r libhiredis-dev
sudo apt remove libhiredis0.14
sudo dpkg -r libhiredis1.1.0
sudo dpkg -i /tmp/new/libhiredis0.14_0.14.1-3_amd64.deb
sudo dpkg -i /tmp/new/libhiredis-dev_0.14.1-3_amd64.deb
sudo dpkg -i libnl-3-200_*.deb
sudo dpkg -i libnl-3-dev_*.deb
sudo dpkg -i libnl-genl-3-200_*.deb
sudo dpkg -i libnl-genl-3-dev_*.deb
sudo dpkg -i libnl-route-3-200_*.deb
sudo dpkg -i libnl-route-3-dev_*.deb
sudo dpkg -i libnl-nf-3-200_*.deb
sudo dpkg -i libnl-nf-3-dev_*.deb
sudo dpkg -i libnl-cli-3-200_*.deb
sudo dpkg -i libnl-cli-3-dev_*.deb
sudo dpkg -i libyang_*.deb
sudo dpkg -i libyang-*.deb
sudo dpkg -i libswsscommon_1.0.0_amd64.deb
sudo dpkg -i libswsscommon-dev_1.0.0_amd64.deb

# Install pip and add base-tooling-requirement.text
sudo apt-get update
sudo pip install --upgrade pip

touch base-tooling-requirements.txt
sudo echo "Pympler ==0.8 --hash=sha256:f74cd2982c5cd92ded55561191945616f2bb904a0ae5cdacdb566c6696bdb922" >>base-tooling-requirements.txt

pip install --require-hashes -r base-tooling-requirements.txt

# Install redis-tools inside virtualenv
sudo apt install redis-server redis-tools
sudo sed -i 's/notify-keyspace-events ""/notify-keyspace-events AKE/' /etc/redis/redis.conf
sudo sed -ri 's/^# unixsocket/unixsocket/' /etc/redis/redis.conf
sudo sed -ri 's/^unixsocketperm .../unixsocketperm 777/' /etc/redis/redis.conf
sudo sed -ri 's/redis-server.sock/redis.sock/' /etc/redis/redis.conf

# Start redis-server :
sudo service redis-server restart &

# Install gtest and gmock
sudo apt-get install -y libgtest-dev
sudo apt-get install -y libgmock-dev

# Install dependencies need to execute tests
sudo apt-get install libjansson-dev
sudo apt install protobuf-compiler
sudo apt install libdbus-c++-bin
sudo apt install libdbus-c++-dev

# Create /var/run/redis/sonic-db folder.
# copy the database_config.json file to /var/run/redis/sonic-db path.
sudo mkdir -p /var/run/redis/sonic-db
sudo cp ../../../src/sonic-framework/tests/test_scripts/database_config.json /var/run/redis/sonic-db/

#Build the sonic-fraework/tests.
cd ../../../src/sonic-framework/
./autogen.sh
./configure
make

cd tests
make

#execure the sonic-framework tests.
./tests
./tests_asan
./tests_tsan
./tests_usan
