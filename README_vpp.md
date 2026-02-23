Build guide for vpp platform

```shell
sudo apt-get install -y make automake autoconf
sudo apt install -y python3-pip
sudo pip3 install j2cli
```

Install Docker

1. git clone --recurse-submodules https://github.com/sonic-net/sonic-buildimage.git //or your fork
2. make init
3. make configure PLATFORM=vpp
4. make SONIC_BUILD_JOBS=8 target/sonic-vpp.img.gz (SONIC_BUILD_JOBS == number of cores)

for testing the qemu VM image:
git clone https://github.com/srl-labs/vrnetlab.git

in vrnetlab/sonic/docker/launch.py, in class SONiC_vm:
change second value in self.qemu_args.extend(["-smp", "2"]) to "cpus=4" 
full line change: self.qemu_args.extend(["-smp", "cpus=4"])

gunzip target/sonic-vpp.img.gz
mv target/sonic-vpp.img /path/to/vrnetlab/sonic
cd /path/to/vrnetlab/sonic
rename the image:
mv sonic-vpp.img sonic-vs-{version}.qcow2
make

now in docker images, you can see your sonic-vpp image (vrnetlab/sonic_sonic-vs:{version})

