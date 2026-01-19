# Build SONiC Switch Images

## Description

Following are the instructions on how to build an ONIE compatible
network operating system (NOS) installer image for Wistron network switches.

## Usage

To build SONiC installer image, run the following commands:

```shell
# Enter the source directory
cd sonic-buildimage

# (Optional) Checkout a specific branch. By default, it uses master branch.
git checkout ES-1227-54TS-P2

# Execute make init once after cloning the repo,
# or after fetching remote repo with submodule updates
make init

# Execute patch for platform specific modifications and issue fixing.
for file in 0001-sonic-kernel-modification-for-wistron.patch \
    0001-fix-sonic-swss-build-error.patch; do \
    patch -p1 < $file
done

# Execute make configure once to configure ASIC
make configure PLATFORM=marvell-prestera PLATFORM_ARCH=arm64

# Build SONiC image with 4 jobs in parallel.
# Note: You can set this higher, but 4 is a good number for most cases
#       and is well-tested.
make NOBUSTER=1 NOBULLSEYE=1 SONIC_BUILD_JOBS=4 target/sonic-marvell-prestera-arm64.bin
```
