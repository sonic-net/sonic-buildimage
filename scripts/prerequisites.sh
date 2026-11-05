#!/usr/bin/env bash
# Strict mode: exit on error, undefined variable, or pipeline failure
set -euo pipefail

###############################################
# Error Handling
###############################################
trap 'echo "[ERROR] Script failed at line $LINENO. Exiting."; exit 1' ERR

# Wrapper to provide better log messages for each step
run_step() {
    local msg="$1"
    shift
    echo "==> $msg"
    "$@"
    echo "==> Completed: $msg"
}

###############################################
# Configurable Variables
# Can be overridden via environment variables:
#   SONIC_REPO=<url> SONIC_DIR=<path> BRANCH=<name> bash prerequisites.sh
###############################################
SONIC_REPO="${SONIC_REPO:-https://github.com/sonic-net/sonic-buildimage.git}"
SONIC_DIR="${SONIC_DIR:-$HOME/sonic-buildimage}"
BRANCH="${BRANCH:-master}"

###############################################
# Steps Begin
###############################################

run_step "Updating apt package index" \
    sudo apt update

run_step "Installing prerequisites (python3-pip, git)" \
    sudo apt install -y python3-pip git

run_step "Installing jinjanator (j2)" \
    bash -c 'pip3 install --user jinjanator || sudo apt-get install -y j2cli'

echo "==> Testing j2 availability..."
if ! command -v j2 >/dev/null 2>&1; then
    echo "[ERROR] j2 is not runnable."
    echo "If installed via pip, ensure ~/.local/bin is in your PATH."
    echo "Please logout and login, then run the script again."
    exit 1
else
    echo "==> j2 is available: $(command -v j2)"
fi

###############################################
# Docker Install Check
###############################################
if ! command -v docker >/dev/null 2>&1; then
    run_step "Installing Docker dependencies" \
        sudo apt install -y ca-certificates curl gnupg lsb-release

    run_step "Adding Docker GPG key" \
        bash -c "curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg"

    run_step "Adding Docker apt repository" \
        bash -c "echo \
        \"deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] \
        https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable\" \
        | sudo tee /etc/apt/sources.list.d/docker.list >/dev/null"

    run_step "Updating apt after adding Docker repo" \
        sudo apt update

    run_step "Installing Docker Engine" \
        sudo apt install -y docker-ce docker-ce-cli containerd.io
else
    echo "==> Docker already installed, skipping."
fi

run_step "Adding user '${USER}' to docker group" \
    sudo gpasswd -a "${USER}" docker

echo "==> Testing Docker availability without sudo..."
if ! docker ps >/dev/null 2>&1; then
    echo "[WARNING] Docker cannot be run without sudo yet."
    echo "Please log out and log back in for the docker group membership to take effect."
    echo "After logging back in, you can continue with the build process."
else
    echo "==> Docker is available without sudo."
fi

###############################################
# Clone SONiC Repo
###############################################
if [ -d "$SONIC_DIR" ]; then
    echo "==> Directory $SONIC_DIR already exists, skipping clone."
else
    run_step "Cloning SONiC repository" \
        git clone --recurse-submodules "$SONIC_REPO" "$SONIC_DIR"
fi

cd "$SONIC_DIR"

run_step "Checking out branch ${BRANCH}" \
    git checkout "${BRANCH}"

run_step "Fetching latest code" \
    git fetch

###############################################
# Done
###############################################
echo "===================================================="
echo "Done!"
echo "===================================================="

