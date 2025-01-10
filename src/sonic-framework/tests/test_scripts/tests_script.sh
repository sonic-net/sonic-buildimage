#!/bin/bash

# Creating and activating virtualenv to execute tests.
#
# Define the name of the virtual environment folder
VENV_DIR="venv"

# Check if Python is installed
if ! command -v python3 &> /dev/null
then
    echo "Installing Python3."
    sudo apt-get install -y python3 python3-venv python3-pip
fi

# Check if the virtual environment directory exists
if [ ! -d "$VENV_DIR" ]; then
  # Create the virtual environment
  python3 -m venv "$VENV_DIR"
  echo "Virtual environment created."
else
  echo "Virtual environment already exists."
fi

# Activate the virtual environment
source "$VENV_DIR/bin/activate"
echo "Virtual environment activated."

# All the following steps need to be executed inside virtual Environment.
# From /sonic-buildimage path.
# 
# Getting the absolute path for script and sonic-buildimage.
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd $SCRIPT_DIR/../../../..
BUILD_DIR="$(pwd)"

# Creating a temp directory and
# Downloading required libhiredis packages to the temp directory.
TMP_DIR=$(mktemp -d)
cd "$TMP_DIR" || exit 1

if curl -O https://ftp.debian.org/debian/pool/main/h/hiredis/libhiredis0.14_0.14.1-3_amd64.deb
then echo "Download Request for libhiredis0.14_0.14.1-3_amd64.deb was successful"
else echo "CURL Request for libhiredis0.14_0.14.1-3_amd64.deb failed !"
exit 1
fi

if curl -O https://ftp.debian.org/debian/pool/main/h/hiredis/libhiredis-dev_0.14.1-3_amd64.deb
then echo "Download Request for libhiredis-dev_0.14.1-3_amd64.deb was successful"
else echo "CURL Request for libhiredis-dev_0.14.1-3_amd64.deb failed !"
exit 1
fi

# Installing all the required dependencies/pkgs
# Present in target/debs/bookworm and /tmp/new path.
cd $BUILD_DIR/target/debs/bookworm

sudo dpkg -r libhiredis-dev
sudo apt remove libhiredis0.14
sudo dpkg -r libhiredis1.1.0

if sudo dpkg -i $TMP_DIR/libhiredis0.14_0.14.1-3_amd64.deb
then echo "Successfully installed libhiredis0.14_0.14.1-3_amd64.deb"
else echo "libhiredis0.14_0.14.1-3_amd64.deb installtion failed !"
exit 1
fi

if sudo dpkg -i $TMP_DIR/libhiredis-dev_0.14.1-3_amd64.deb
then echo "Successfully installed libhiredis-dev_0.14.1-3_amd64.deb"
else echo "libhiredis-dev_0.14.1-3_amd64.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-3-200_*.deb
then echo "Successfully installed libnl-3-200_*.deb"
else echo "libnl-3-200_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-3-dev_*.deb
then echo "Successfully installed libnl-3-dev_*.deb"
else echo "libnl-3-dev_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-genl-3-200_*.deb
then echo "Successfully installed libnl-genl-3-200_*.deb"
else echo "libnl-genl-3-200_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-genl-3-dev_*.deb
then echo "Successfully installed libnl-genl-3-dev_*.deb"
else echo "libnl-genl-3-dev_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-route-3-200_*.deb
then echo "Successfully installed libnl-route-3-200_*.deb"
else echo "libnl-route-3-200_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-route-3-dev_*.deb
then echo "Successfully installed libnl-route-3-dev_*.deb"
else echo "libnl-route-3-dev_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-nf-3-200_*.deb
then echo "Successfully installed libnl-nf-3-200_*.deb"
else echo "libnl-nf-3-200_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-nf-3-dev_*.deb
then echo "Successfully installed libnl-nf-3-dev_*.deb"
else echo "libnl-nf-3-dev_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-cli-3-200_*.deb
then echo "Successfully installed libnl-cli-3-200_*.deb"
else echo "libnl-cli-3-200_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libnl-cli-3-dev_*.deb
then echo "Successfully installed libnl-cli-3-dev_*.deb"
else echo "libnl-cli-3-dev_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libyang_*.deb
then echo "Successfully installed libyang_*.deb"
else echo "libyang_*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libyang-*.deb
then echo "Successfully installed libyang-*.deb"
else echo "libyang-*.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libswsscommon_1.0.0_amd64.deb
then echo "Successfully installed libswsscommon_1.0.0_amd64.deb"
else echo "libswsscommon_1.0.0_amd64.deb installtion failed !"
exit 1
fi

if sudo dpkg -i libswsscommon-dev_1.0.0_amd64.deb
then echo "Successfully installed libswsscommon-dev_1.0.0_amd64.deb"
else echo "libswsscommon-dev_1.0.0_amd64.deb installtion failed !"
exit 1
fi

# Removing the temp directory.
[[ -d $TMP_DIR ]] && rm -rf "$TMP_DIR"

# Installing pip and add base-tooling-requirement.text
sudo apt-get update
pip install --upgrade pip

touch base-tooling-requirements.txt
sudo echo "Pympler ==0.8 --hash=sha256:f74cd2982c5cd92ded55561191945616f2bb904a0ae5cdacdb566c6696bdb922" >>base-tooling-requirements.txt

pip install --require-hashes -r base-tooling-requirements.txt

# Installing required redis-tools inside virtualenv
sudo apt install redis-server redis-tools
sudo sed -i 's/notify-keyspace-events ""/notify-keyspace-events AKE/' /etc/redis/redis.conf
sudo sed -ri 's/^# unixsocket/unixsocket/' /etc/redis/redis.conf
sudo sed -ri 's/^unixsocketperm .../unixsocketperm 777/' /etc/redis/redis.conf
sudo sed -ri 's/redis-server.sock/redis.sock/' /etc/redis/redis.conf

# Restarting redis-server :
sudo service redis-server restart &
if [[ $? -ne 0 ]]; then
  echo "Redis service restart failed!"
  exit 1
fi

# Installing all required  dependencies to execute tests
# Installing gtest :
sudo apt-get install -y libgtest-dev
if [[ $? -ne 0 ]]; then
  echo "libgtest-dev installation failed!"
  exit 1
fi

# Installing gmock :
sudo apt-get install -y libgmock-dev
if [[ $? -ne 0 ]]; then
  echo "libgmock-dev installation failed!"
  exit 1
fi

# Installing libjansson-dev
sudo apt-get install -y libjansson-dev
if [[ $? -ne 0 ]]; then
  echo "libjansson-dev installation failed!"
  exit 1
fi

# Installing protobuf-compiler
sudo apt install -y protobuf-compiler
if [[ $? -ne 0 ]]; then
  echo "protobuf-compiler installation failed!"
  exit 1
fi

# Installing libdbus-c++-bin
sudo apt install -y libdbus-c++-bin
if [[ $? -ne 0 ]]; then
  echo "libdbus-c++-bin installation failed!"
  exit 1
fi

# Installing libdbus-c++-dev
sudo apt install -y libdbus-c++-dev
if [[ $? -ne 0 ]]; then
  echo "libdbus-c++-dev installation failed!"
  exit 1
fi

# Creating Directory: /var/run/redis/sonic-db
# Copying database_config.json file to /var/run/redis/sonic-db path.
sudo mkdir -p /var/run/redis/sonic-db
sudo cp $SCRIPT_DIR/database_config.json /var/run/redis/sonic-db/

#Building the sonic-fraework/tests.
cd $BUILD_DIR/src/sonic-framework/
./autogen.sh
./configure
make
cd tests
make

# Executing tests.
if [[ -f tests ]]; then  
        ./tests  
    else  
        echo "Warning: $test not found, skipping..."  
fi  

# Executing tests_asan
if [[ -f tests_asan ]]; then  
        ./tests_asan  
    else  
        echo "Warning: $test_asan not found, skipping..."  
fi 

# Executing tests_tsan
if [[ -f tests_asan ]]; then  
        ./tests_tsan  
    else  
        echo "Warning: $test_tsan not found, skipping..."  
fi 


# Executing tests_usan
if [[ -f tests_asan ]]; then  
        ./tests_usan  
    else  
        echo "Warning: $test_usan not found, skipping..."  
fi 
