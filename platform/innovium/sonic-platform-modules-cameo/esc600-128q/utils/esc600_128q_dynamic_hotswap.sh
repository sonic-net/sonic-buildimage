#!/bin/bash
export CAMEOLIB=/lib/credo_sdk/libcameo_mdio.so
export CREDO_100G_PATH=/lib/credo_sdk
export CREDO_400G_PATH=/lib/credo_sdk
export LOGLEVEL=INFO

python /usr/local/bin/esc600_128q_dynamic_hotswap.py

