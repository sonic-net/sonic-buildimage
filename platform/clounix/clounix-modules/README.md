# NB Kernel module driver
## Build steps
Build paramters:
      1. KVERSION, it supports 5.10.0-6-amd64 and 4.19.0-12-2-amd64, the default value is 5.10.0-6-amd64, 
'''
    ./build_env
    make KVERSION=4.19.0-12-2-amd64
'''
## release for customer
      1. generate version header file.
'''
    sh gen_rel_ver_header.sh
'''
      2. update the latest changes and version header from github.
