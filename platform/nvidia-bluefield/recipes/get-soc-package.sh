#!/bin/bash

base_url=${1}
package_name=${2}

full_url=$(lynx --listonly --nonumbers -dump ${base_url} | grep ${package_name})
wget ${full_url}
rpm2cpio ${package_name}*.src.rpm | cpio -idmv
tar xf *.tar.gz
