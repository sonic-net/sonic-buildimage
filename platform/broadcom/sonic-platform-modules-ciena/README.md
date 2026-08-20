# Ciena Platform Kernel Modules for SONiC

This repository contains kernel modules for Ciena network platforms running SONiC.

## Directory Structure

```
.
âââ common/                  # Shared kernel modules
â   âââ linux/              # Generic headers (symlink to .)
âââ 8140/                    # CN8140 (Rudra40) platform
â   âââ include/linux/      # Platform-specific headers
â   âââ modules/            # Platform-specific modules
âââ Makefile                # Top-level build system
âââ README.md               # This file
```

#### Build for CN8140 (Rudra40)

```bash
# Navigate to the module directory
cd /localdisk/nhilderm/sonic_dockerbuild/sonic-buildimage/platform/broadcom/sonic-platform-modules-ciena

# Build only CN8140 modules
KERNEL_SRC=/lib/modules/$(uname -r)/build make PLATFORMS="8140"

# Or build all platforms
KERNEL_SRC=/lib/modules/$(uname -r)/build make

# Clean build artifacts
make clean
```

#### Build Variables

- `KERNEL_SRC` - Path to kernel headers (default: `/lib/modules/$(uname -r)/build`)
- `PLATFORMS` - Space-separated list of platforms to build (default: `8140`)

## Deployment to Target System

### Step 1: Copy Modules to Target

On the **build host**:

```bash
# Set target IP address
TARGET_IP=10.184.33.158

# Copy common modules
scp common/*.ko admin@${TARGET_IP}:/tmp/

# Copy CN8140 platform modules
scp 8140/modules/*.ko admin@${TARGET_IP}:/tmp/
```

### Step 2: Install Modules on Target

On the **target system** (as root):

```bash
# Switch to root
sudo su -

# Create module directory
mkdir -p /lib/modules/$(uname -r)/ciena/

# Copy all modules to the kernel module directory
cp /tmp/*.ko /lib/modules/$(uname -r)/ciena/

# Update module dependencies
depmod -a

# Verify modules are registered
ls -lh /lib/modules/$(uname -r)/ciena/
```

### Step 3: Load Modules

```
echo "Loading Ciena CN8140 kernel modules..."

# Load modules in dependency order
modprobe uio
modprobe sirilx_uio
modprobe i2c-ciena
modprobe i2c-dev
modprobe led-sysfs
```

## Unloading Modules

To unload all modules (reverse order):

```bash
# Unload in reverse dependency order
modprobe -r led-sysfs
modprobe -r i2c-dev
modprobe -r i2c-ciena
modprobe -r sirilx_uio
modprobe -r uio
```
