HOWTO Use Virtual Switch (VM)

1. Install libvirt, kvm, qemu

```
sudo apt-get install libvirt-clients qemu-kvm libvirt-bin
```

2. Create SONiC VM for single ASIC HWSKU

```
$ sudo virsh
Welcome to virsh, the virtualization interactive terminal.

Type:  'help' for help with commands
       'quit' to quit

virsh # 
virsh # create sonic.xml
Domain sonic created from sonic.xml

virsh # 
```

2. Create SONiC VM for multi-ASIC HWSKU

- Based on the number of asics of hwsku, update device/x86_64-kvm_x86_64-r0/asic.conf 
```
NUM_ASIC=<n>
DEV_ID_ASIC_0=0
DEV_ID_ASIC_1=1
DEV_ID_ASIC_2=2
DEV_ID_ASIC_3=3
..
DEV_ID_ASIC_<n-1>=<n-1>
```
For example, a four asic VS asic.conf will be:
```
NUM_ASIC=4
DEV_ID_ASIC_0=0
DEV_ID_ASIC_1=1
DEV_ID_ASIC_2=2
DEV_ID_ASIC_3=3
```
- Create a topology.sh script which will create the internal asic topology for
the specific hwsku.
For example, for msft_multi_asic_vs:
https://github.com/Azure/sonic-buildimage/blob/master/device/virtual/x86_64-kvm_x86_64-r0/msft_multi_asic_vs/topology.sh

- With the updated asic.conf and topology.sh, build sonic-vs.img which can be used to 
bring up multi-asic virtual switch.

- Update platform/vs/sonic_multiasic.xml with higher memory and vcpu as required.
  - For 4-asic vs platform msft_four_asic_vs hwsku, 8GB memory and 10vCPUs.
  - For 7-ASIC vs platform msft_multi_asic_vs hwsku, 8GB and 16vCPUs.
- Update the number of front-panel interfaces in sonic_multliasic.xml
    - For 4-ASIC vs platform, 8 front panel interfaces.
    - For 6-ASIC vs platform, 64 front panel interfaces.

- With multi-asic sonic_vs.img and sonic_multiasic.xml file, bring up multi-asic
vs as:

```
$ sudo virsh
Welcome to virsh, the virtualization interactive terminal.

Type:  'help' for help with commands
       'quit' to quit

virsh #
virsh # create sonic_multiasic.xml 
Domain sonic created from sonic.xml

virsh #
```

- Steps to convert a prebuilt single asic sonic-vs.img:
  - Use the updated sonic_multiasic.xml file and bring up virtual switch.
  - Update /usr/share/sonic/device/x86_64-kvm_x86_64-r0/asic.conf as above.
  - Add topology.sh in /usr/share/sonic/device/x86_64-kvm_x86_64-r0/<HWSKU>
  - stop database service and remove database docker, so that when vs is 
rebooted, database_global.json is created with the right namespaces.
    - systemctl stop database
    - docker rm database
  - sudo reboot
  - Once rebooted, VS should be multi-asic VS.

- Start topology service.
```
sudo systemctl start topology.
```

- Load configuration using minigraph or config_dbs.

3. Access virtual switch:

    1. Connect SONiC VM via console
    ```
    $ telnet 127.0.0.1 7000
    ```
    
    OR

    2. Connect SONiC VM via SSH
        
        1. Connect via console (see 3.1 above)

        2. Request a new DHCP address
        ```
        sudo dhclient -v
        ```
        
        3. Connect via SSH
        ```
        $ ssh -p 3040 admin@127.0.0.1
        ```
