ONIE_RECOVERY_IMAGE = onie-recovery-x86_64-kvm_x86_64-r0.iso
$(ONIE_RECOVERY_IMAGE)_URL = "https://packages.trafficmanager.net/public/onie/onie-recovery-x86_64-kvm_x86_64-r0.iso"

ONIE_RECOVERY_KVM_4ASIC_IMAGE = onie-recovery-x86_64-kvm_x86_64_4_asic-r0.iso
$(ONIE_RECOVERY_KVM_4ASIC_IMAGE)_URL = "https://packages.trafficmanager.net/public/onie/onie-recovery-x86_64-kvm_x86_64_4_asic-r0.iso"

ONIE_RECOVERY_KVM_6ASIC_IMAGE = onie-recovery-x86_64-kvm_x86_64_6_asic-r0.iso
$(ONIE_RECOVERY_KVM_6ASIC_IMAGE)_URL = "https://packages.trafficmanager.net/public/onie/onie-recovery-x86_64-kvm_x86_64_6_asic-r0.iso"

SONIC_ONLINE_FILES += $(ONIE_RECOVERY_IMAGE) $(ONIE_RECOVERY_KVM_4ASIC_IMAGE) $(ONIE_RECOVERY_KVM_6ASIC_IMAGE)
