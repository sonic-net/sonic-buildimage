
#include "i2c-mei_type.h"



/* ========== PciLib.h ========= */ 

/**
  Macro that converts PCI Bus, PCI Device, PCI Function and PCI Register to an
  address that can be passed to the PCI Library functions.

  @param  Bus       PCI Bus number. Range 0..255.
  @param  Device    PCI Device number. Range 0..31.
  @param  Function  PCI Function number. Range 0..7.
  @param  Register  PCI Register number. Range 0..255 for PCI. Range 0..4095
                    for PCI Express.

  @return The encoded PCI address.

**/
#define PCI_LIB_ADDRESS(Bus,Device,Function,Register)   \
  (((Register) & 0xfff) | (((Function) & 0x07) << 12) | (((Device) & 0x1f) << 15) | (((Bus) & 0xff) << 20))



/* ========== Qubbing ========= */ 

UINT8 PciRead8(UINT64 addr);

UINT8 PciWrite8(UINT64 addr, UINT8 data);

UINT16 PciRead16(UINT64 addr);

UINT16 PciWrite16(UINT64 addr, UINT8 data);

UINT32 PciRead32(UINT64 addr);

UINT32 PciWrite32(UINT64 addr, UINT8 data);


void I2C_Set(UINT8 smbus, UINT8 daddr, INT32 reg, UINT8 *data, UINT8 dlen);

void I2C_Read(UINT8 smbus, UINT8 daddr, INT32 reg, UINT8 dlen);

void VersionRead(void);
void I2C_Probe(void);