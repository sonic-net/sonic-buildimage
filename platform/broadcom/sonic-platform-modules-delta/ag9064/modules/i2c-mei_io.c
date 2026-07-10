
#include "i2c-mei_rw.h"


/* ========== IoLibGcc.c ========= */ 

/**
  Reads an 8-bit I/O port.

  Reads the 8-bit I/O port specified by Port. The 8-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
//__inline__
UINT8
IoRead8 (
  IN      UINTN                     Port
  )
{
  UINT8   Data;

  __asm__ __volatile__ ("inb %w1,%b0" : "=a" (Data) : "d" ((UINT16)Port));
  return Data;
}

/**
  Writes an 8-bit I/O port.

  Writes the 8-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 8-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
//__inline__
UINT8
IoWrite8 (
  IN      UINTN                     Port,
  IN      UINT8                     Value
  )
{
  __asm__ __volatile__ ("outb %b0,%w1" : : "a" (Value), "d" ((UINT16)Port));
  return Value;;
}

/**
  Reads a 16-bit I/O port.

  Reads the 16-bit I/O port specified by Port. The 16-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().
  If Port is not aligned on a 16-bit boundary, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
//__inline__
UINT16
IoRead16 (
  IN      UINTN                     Port
  )
{
  UINT16   Data;

  if((Port & 1) != 0)
    printk("Failed\n");
  __asm__ __volatile__ ("inw %w1,%w0" : "=a" (Data) : "d" ((UINT16)Port));
  return Data;
}

/**
  Writes a 16-bit I/O port.

  Writes the 16-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 16-bit I/O port operations are not supported, then ASSERT().
  If Port is not aligned on a 16-bit boundary, then ASSERT().
  
  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
//__inline__
UINT16
IoWrite16 (
  IN      UINTN                     Port,
  IN      UINT16                    Value
  )
{
  if((Port & 1) != 0)
    printk("Failed\n");
  __asm__ __volatile__ ("outw %w0,%w1" : : "a" (Value), "d" ((UINT16)Port));
  return Value;;
}

/**
  Reads a 32-bit I/O port.

  Reads the 32-bit I/O port specified by Port. The 32-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().
  If Port is not aligned on a 32-bit boundary, then ASSERT().
  
  @param  Port  The I/O port to read.

  @return The value read.

**/
//__inline__
UINT32
IoRead32 (
  IN      UINTN                     Port
  )
{
  UINT32   Data;

  if((Port & 3) != 0)
    printk("Failed\n");
  __asm__ __volatile__ ("inl %w1,%0" : "=a" (Data) : "d" ((UINT16)Port));
  return Data;
}

/**
  Writes a 32-bit I/O port.

  Writes the 32-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 32-bit I/O port operations are not supported, then ASSERT().
  If Port is not aligned on a 32-bit boundary, then ASSERT().
  
  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
//__inline__
UINT32
IoWrite32 (
  IN      UINTN                     Port,
  IN      UINT32                    Value
  )
{
  if((Port & 3) != 0)
    printk("Failed\n");
  __asm__ __volatile__ ("outl %0,%w1" : : "a" (Value), "d" ((UINT16)Port));
  return Value;
}



/* ========== GccInline.c ========= */ 

/**
  Enables CPU interrupts.

  Enables CPU interrupts.

**/
VOID
EnableInterrupts (
  VOID
  )
{
  __asm__ __volatile__ ("sti"::: "memory");
}

/**
  Disables CPU interrupts.

  Disables CPU interrupts.

**/
VOID
DisableInterrupts (
  VOID
  )
{  
  __asm__ __volatile__ ("cli"::: "memory");
}

/**
  Reads the current value of the EFLAGS register.

  Reads and returns the current value of the EFLAGS register. This function is
  only available on IA-32 and X64. This returns a 32-bit value on IA-32 and a
  64-bit value on X64.

  @return EFLAGS on IA-32 or RFLAGS on X64.

**/
UINTN
AsmReadEflags (
  VOID
  )
{
  UINTN Eflags;
  
  __asm__ __volatile__ (
    "pushfq         \n\t"
    "pop     %0         "
    : "=r" (Eflags)       // %0
    );
    
  return Eflags;
}



/* ========== X86GetInterruptState.c ========= */ 

/**
  Retrieves the current CPU interrupt state.

  Returns TRUE is interrupts are currently enabled. Otherwise
  returns FALSE.

  @retval TRUE  CPU interrupts are enabled.
  @retval FALSE CPU interrupts are disabled.

**/
BOOLEAN
GetInterruptState (
  VOID
  )
{
  IA32_EFLAGS32                     EFlags;

  EFlags.UintN = AsmReadEflags ();
  return (BOOLEAN)(1 == EFlags.Bits.IF);
}



/* ========== Cpu.c ========= */ 

/**
  Disables CPU interrupts and returns the interrupt state prior to the disable
  operation.

  @retval TRUE  CPU interrupts were enabled on entry to this call.
  @retval FALSE CPU interrupts were disabled on entry to this call.

**/
BOOLEAN
SaveAndDisableInterrupts (
  VOID
  )
{
  BOOLEAN                           InterruptState;

  InterruptState = GetInterruptState ();
  DisableInterrupts ();
  return InterruptState;
}

/**
  Set the current CPU interrupt state.

  Sets the current CPU interrupt state to the state specified by
  InterruptState. If InterruptState is TRUE, then interrupts are enabled. If
  InterruptState is FALSE, then interrupts are disabled. InterruptState is
  returned.

  @param  InterruptState  TRUE if interrupts should be enabled. FALSE if
                          interrupts should be disabled.

  @return InterruptState

**/
BOOLEAN
SetInterruptState (
  IN      BOOLEAN                   InterruptState
  )
{
  if (InterruptState) {
    EnableInterrupts ();
  } else {
    DisableInterrupts ();
  }
  return InterruptState;
}



/* ========== pciLib.c ========= */ 

//
// Declare I/O Ports used to perform PCI Confguration Cycles
//
#define PCI_CONFIGURATION_ADDRESS_PORT  0xCF8
#define PCI_CONFIGURATION_DATA_PORT     0xCFC

/**
  Convert a PCI Library address to PCI CF8 formatted address.

  Declare macro to convert PCI Library address to PCI CF8 formatted address.
  Bit fields of PCI Library and CF8 formatted address is as follows:
  PCI Library formatted address    CF8 Formatted Address
 =============================    ======================
    Bits 00..11  Register           Bits 00..07  Register
    Bits 12..14  Function           Bits 08..10  Function
    Bits 15..19  Device             Bits 11..15  Device
    Bits 20..27  Bus                Bits 16..23  Bus
    Bits 28..31  Reserved(MBZ)      Bits 24..30  Reserved(MBZ)
                                    Bits 31..31  Must be 1

  @param  A The address to convert.

  @retval The coverted address.

**/
#define PCI_TO_CF8_ADDRESS(A) \
  ((UINT32) ((((A) >> 4) & 0x00ffff00) | ((A) & 0xfc) | 0x80000000))

/**
  Reads an 8-bit PCI configuration register.

  Reads and returns the 8-bit PCI configuration register specified by Address.
  This function must guarantee that all PCI read and write operations are
  serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address The address that encodes the PCI Bus, Device, Function and
                  Register.

  @return The read value from the PCI configuration register.

**/
UINT8
PciCf8Read8 (
  IN      UINTN                     Address
  )
{
  BOOLEAN  InterruptState;
  UINT32   AddressPort;
  UINT8    Result;
  
  InterruptState = SaveAndDisableInterrupts ();
  AddressPort = IoRead32 (PCI_CONFIGURATION_ADDRESS_PORT);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  Result = IoRead8 (PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3));
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, AddressPort);
  SetInterruptState (InterruptState);
  return Result;
}

/**
  Writes an 8-bit PCI configuration register.

  Writes the 8-bit PCI configuration register specified by Address with the
  value specified by Value. Value is returned. This function must guarantee
  that all PCI read and write operations are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address The address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  Value   The value to write.

  @return The value written to the PCI configuration register.

**/
UINT8
PciCf8Write8 (
  IN      UINTN                     Address,
  IN      UINT8                     Value
  )
{
  BOOLEAN  InterruptState;
  UINT32   AddressPort;
  UINT8    Result;
  
  InterruptState = SaveAndDisableInterrupts ();
  AddressPort = IoRead32 (PCI_CONFIGURATION_ADDRESS_PORT);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  Result = IoWrite8 (
             PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 3),
             Value
             );
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, AddressPort);
  SetInterruptState (InterruptState);
  return Result;
}

/**
  Reads a 16-bit PCI configuration register.

  Reads and returns the 16-bit PCI configuration register specified by Address.
  This function must guarantee that all PCI read and write operations are
  serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address The address that encodes the PCI Bus, Device, Function and
                  Register.

  @return The read value from the PCI configuration register.

**/
UINT16
PciCf8Read16 (
  IN      UINTN                     Address
  )
{
  BOOLEAN  InterruptState;
  UINT32   AddressPort;
  UINT16   Result;
  
  InterruptState = SaveAndDisableInterrupts ();
  AddressPort = IoRead32 (PCI_CONFIGURATION_ADDRESS_PORT);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  Result = IoRead16 (PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2));
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, AddressPort);
  SetInterruptState (InterruptState);
  return Result;
}

/**
  Writes a 16-bit PCI configuration register.

  Writes the 16-bit PCI configuration register specified by Address with the
  value specified by Value. Value is returned. This function must guarantee
  that all PCI read and write operations are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 16-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address The address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  Value   The value to write.

  @return The value written to the PCI configuration register.

**/
UINT16
PciCf8Write16 (
  IN      UINTN                     Address,
  IN      UINT16                    Value
  )
{
  BOOLEAN  InterruptState;
  UINT32   AddressPort;
  UINT16   Result;
  
  InterruptState = SaveAndDisableInterrupts ();
  AddressPort = IoRead32 (PCI_CONFIGURATION_ADDRESS_PORT);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  Result = IoWrite16 (
             PCI_CONFIGURATION_DATA_PORT + (UINT16)(Address & 2),
             Value
             );
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, AddressPort);
  SetInterruptState (InterruptState);
  return Result;
}

/**
  Reads a 32-bit PCI configuration register.

  Reads and returns the 32-bit PCI configuration register specified by Address.
  This function must guarantee that all PCI read and write operations are
  serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address The address that encodes the PCI Bus, Device, Function and
                  Register.

  @return The read value from the PCI configuration register.

**/
UINT32
PciCf8Read32 (
  IN      UINTN                     Address
  )
{
  BOOLEAN  InterruptState;
  UINT32   AddressPort;
  UINT32   Result;
  
  InterruptState = SaveAndDisableInterrupts ();
  AddressPort = IoRead32 (PCI_CONFIGURATION_ADDRESS_PORT);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  Result = IoRead32 (PCI_CONFIGURATION_DATA_PORT);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, AddressPort);
  SetInterruptState (InterruptState);
  return Result;
}

/**
  Writes a 32-bit PCI configuration register.

  Writes the 32-bit PCI configuration register specified by Address with the
  value specified by Value. Value is returned. This function must guarantee
  that all PCI read and write operations are serialized.

  If Address > 0x0FFFFFFF, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().
  If the register specified by Address >= 0x100, then ASSERT().

  @param  Address The address that encodes the PCI Bus, Device, Function and
                  Register.
  @param  Value   The value to write.

  @return The value written to the PCI configuration register.

**/
UINT32
PciCf8Write32 (
  IN      UINTN                     Address,
  IN      UINT32                    Value
  )
{
  BOOLEAN  InterruptState;
  UINT32   AddressPort;
  UINT32   Result;
  
  InterruptState = SaveAndDisableInterrupts ();
  AddressPort = IoRead32 (PCI_CONFIGURATION_ADDRESS_PORT);
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, PCI_TO_CF8_ADDRESS (Address));
  Result = IoWrite32 (
             PCI_CONFIGURATION_DATA_PORT,
             Value
             );
  IoWrite32 (PCI_CONFIGURATION_ADDRESS_PORT, AddressPort);
  SetInterruptState (InterruptState);
  return Result;
}



/* ========== Other ========= */ 

// UINT8 PciRead8(UINT64 addr)
// {
// 	printf("[%s] addr: %8X.\n", __func__, addr);
// 	return 0x01;
// }

// UINT8 PciWrite8(UINT64 addr, UINT8 data)
// {
// 	printf("[%s] addr: %8X	data: %2X.\n", __func__, addr, data);
// 	return 0x02;	
// }


// UINT16 PciRead16(UINT64 addr)
// {
// 	printf("[%s] addr: %8X.\n", __func__, addr);
// 	return 0x03;
// }

// UINT16 PciWrite16(UINT64 addr, UINT8 data)
// {
// 	printf("[%s] addr: %8X	data: %2X.\n", __func__, addr, data);
// 	return 0x04;
// }


// UINT32 PciRead32(UINT64 addr)
// {
// 	printf("[%s] addr: %8X.\n", __func__, addr);
// 	return 0x05;
// }

// UINT32 PciWrite32(UINT64 addr, UINT8 data)
// {
// 	printf("[%s] addr: %8X	data: %2X.\n", __func__, addr, data);
// 	return 0x06;
// }

UINT8 PciRead8(UINT64 addr)
{
	return PciCf8Read8 (addr);
}

UINT8 PciWrite8(UINT64 addr, UINT8 data)
{
	return PciCf8Write8 (addr, data);	
}


UINT16 PciRead16(UINT64 addr)
{
	return PciCf8Read16 (addr);
}

UINT16 PciWrite16(UINT64 addr, UINT8 data)
{
	return PciCf8Write16 (addr, data);
}


UINT32 PciRead32(UINT64 addr)
{
	return PciCf8Read32 (addr);
}

UINT32 PciWrite32(UINT64 addr, UINT8 data)
{
	return PciCf8Write32 (addr, data);
}
