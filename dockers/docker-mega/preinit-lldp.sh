#!/usr/bin/env bash
#Generate supervisord.conf based on device metadata
mkdir -p /etc/supervisor/conf.d/
# (docker-mega preinit: 'exec supervisord' stripped here by gen_docker_init.py -- mega's own docker-mega-init.sh execs the ONE shared supervisord instead, after every selected feature's preinit has run)
