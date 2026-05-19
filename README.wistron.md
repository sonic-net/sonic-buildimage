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
git checkout master-mrvl-prestera

# Execute make init once after cloning the repo,
# or after fetching remote repo with submodule updates
make init

# Execute `apply_patches.sh` for platform specific modifications and issue fixing.
# Usage & options:
# Default: Apply standard patches
./apply_patches.sh
# Enable ZTP workaround patches
./apply_patches.sh -z
# Revert ZTP workaround patches
./apply_patches.sh ZTP=no
# Show help options
./apply_patches.sh -h

# Execute make configure once to configure ASIC
make configure PLATFORM=marvell-prestera PLATFORM_ARCH=arm64

# Build SONiC image with 3 jobs in parallel.
# Note: You can set this higher, but 3 is a good number for most cases
#       and is well-tested.
make NOBUSTER=1 NOBULLSEYE=1 SONIC_BUILD_JOBS=3 target/sonic-marvell-prestera-arm64.bin
```
