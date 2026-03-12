Build guide for vpp platform

```shell
sudo apt update
sudo apt install -y git make automake autoconf build-essential
sudo apt install -y python3-pip
sudo pip3 install j2cli
sudo pip3 install jinjanator
```

Install Docker

```
# After installation, add yourself to the docker group (so you don't need sudo for docker)
sudo gpasswd -a ${USER} docker
```
logout and login to apply changes

```
# Load the overlay kernel module (required by the build)
sudo modprobe overlay
```

1. git clone --recurse-submodules https://github.com/sonic-net/sonic-buildimage.git //or preferably your fork
2. make init
3. edit .gitmodules file for the repo where your changes were made, repo link and branch name
4. git sync submodule
5. cd into the submodule, "git fetch origin", "git checkout {branch_name}"
6. make configure PLATFORM=vpp
7. make SONIC_BUILD_JOBS=8 target/sonic-vpp.img.gz (SONIC_BUILD_JOBS == number of cores)

Build problems:
1. infinite DEADLYSIGNAL loop in dhcp4relay build/test
Patch the dhcp4relay and dhcp6relay Makefiles
```
sed -i 's/-fsanitize=address//g' src/dhcprelay/dhcp4relay/Makefile
sed -i 's/-fsanitize=address//g' src/dhcprelay/dhcp6relay/Makefile
```
2. Can't build kvm image, building in a VM
Enable nested virtualization via Powershell admin:
```
Set-VMProcessor -VMName "your-vm-name" -ExposeVirtualizationExtensions $true
```

for testing the qemu VM image:
git clone https://github.com/srl-labs/vrnetlab.git

in vrnetlab/sonic/docker/launch.py, in class SONiC_vm:
change second value in self.qemu_args.extend(["-smp", "2"]) to "cpus=4" 
full line change: self.qemu_args.extend(["-smp", "cpus=4"])

```
gunzip target/sonic-vpp.img.gz
# assumes vrnetlab is under ~/dev/
mv target/sonic-vpp.img ~/dev/vrnetlab/sonic
cd ~/dev/vrnetlab/sonic
# rename the image:
mv sonic-vpp.img sonic-vs-{version}.qcow2
make
```

now in docker images, you can see your sonic-vpp image (vrnetlab/sonic_sonic-vs:{version})

