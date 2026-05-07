#######################################
# This files contains the state machine implementation of the Ostara
# boot state machine in GRUB.
#
# The environment is stored in the EFI partition at the root level.
# It contains the variables and their possible values:
#   netboot_env = [0, 1]
#   install_env = [0, 1]
#   bootcount_env = [0, 1, 2]
#   warmboot_env = [0, 1]
#   active = [0,1]
#   onie_run = [0, 1]
# The state is loaded by the call to load_env and the variables are stored
# in the GRUB environment. This can be verified with the set command.
# onie_run is internal to the boot architecture of two GRUB binaries. This env
# is set by ONIE GRUB and is unset, SONiC GRUB will chain boot to ONIE to check
# warmboot and install flags. Once, the ONIE GRUB is run, the SONiC GRUB is run
# and netboot and bootcount are checked.
#
# The environment is stored into variabled append with '_env' to prevent
# accidental writing of the state variables to the environment with a save_env
# call.
# save_state must be called to convert the state variable to the environment
# variables.
#######################################

#######################################
# Load the GRUB environment variables
#
# Globals:
#   netboot_env: Flag loaded from file.
#   bootcount_env: Flag loaded from file.
#   warmboot_env: Flag loaded from file.
#   active_env: Flag loaded from file.
#   onie_run: Flag load from file.
# Arguments:
#   None
#######################################
function load_state {
  echo "Loading state"
  if [ ! -s "(hd0,gpt1)/gpins_env" ]; then
    echo "State File not detected"
    return
  fi
  env_file="(hd0,gpt1)/gpins_env"
  load_env --file "${env_file}"
  set netboot="${netboot_env}"
  set bootcount="${bootcount_env}"
  set warmboot="${warmboot_env}"
  set active="${active_env}"
  set kargs=""
}

#######################################
# Compute the state
#
# Determine which partition to boot from.
# default stores the menu entry to boot.
# A is the most recently installed image
# and B is the oldest image.
# Also, set any kernel arguments.
#
# Globals:
#   kargs: [netboot,]
#   default: Default GRUB menu entry. 0=A, 1=B, "ONIE"=ONIE
#   netboot: 1 if netboot was requested.
#   bootcount: Boot order, 2=A, 1=B, 0=ONIE
#   warmboot: 1 if warmboot was requested.
#   active: 0 if the primary image is A and 1 if the primary image is B.
# Arguments:
#   None
#######################################
function compute_state {
  echo "Computing state"
  if [ -z "${onie_run}" ]; then
    echo "Cisco ONIE detected"
    # Boot primary SONiC image
    set default=0
    return
  fi
  if [ -z "${active}" ]; then
    echo "active not set"
    set active=0
  fi
  if [ "${onie_run}" -eq 0 ]; then
    set default="ONIE"
    return
  fi
  # Warmboot is checked in ONIE, this should already be set to 0.
  set warmboot=0
  if [ "${netboot}" -eq 1 ]; then
    set onie_run=0
    set netboot=0
    set kargs="netboot=1"
    set default="${active}"
    return
  fi
  if [ "${bootcount}" -eq 2 ]; then
    set bootcount=1
    set default="${active}"
  elif [ "${bootcount}" -eq 1 ]; then
    set bootcount=0
    if [ "${active}" -eq 1 ]; then
      set default=0
    else
      set default=1
    fi
  elif [ "${bootcount}" -eq 0 ]; then
    set default="ONIE"
  fi

  set onie_run=0
}

#######################################
# Save the state
#
# Save the state the the environment file.
#
# Globals:
#   onie_run: Flag (internal) to persist to file.
#   netboot: State var to persist to file.
#   bootcount: State var to persist to file.
#   warmboot: State var to persist to file.
# Arguments:
#   None
#######################################
function save_state {
  # Don't save the state if we don't detect Google ONIE
  if [ ! -s "(hd0,gpt1)/gpins_env" ]; then
    echo "State File not detected"
    return
  fi
  echo "Saving state"
  env_file="(hd0,gpt1)/gpins_env"
  set netboot_env="${netboot}"
  set bootcount_env="${bootcount}"
  set warmboot_env="${warmboot}"
  save_env --file "${env_file}" onie_run
  save_env --file "${env_file}" bootcount_env
  save_env --file "${env_file}" netboot_env
  save_env --file "${env_file}" warmboot_env
}
