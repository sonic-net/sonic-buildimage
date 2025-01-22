#!/usr/bin/env bash
#
# Same method from platform/barefoot/docker-syncd-bfn/start.sh
#
. /opt/bfn/install/bin/dma_setup.sh
# . /opt/bfn/install/bin/bf_kdrv_mod_load /opt/bfn/install


supervisorctl start rsyslogd
supervisorctl start saiserver
