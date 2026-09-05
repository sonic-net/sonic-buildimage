#######################################################################
#
# Copyright 2019 Broadcom. All rights reserved.
# The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
#
#######################################################################
#
# Restrict rbash
#

if [ "$0" == "rbash" ] ; then

    # Prevent from executing arbitrary commands. Only allow CWD commands
    # Add shell scripts to the user's home directory to allow them to run
    # commands.

    PATH=
    export PATH
fi
