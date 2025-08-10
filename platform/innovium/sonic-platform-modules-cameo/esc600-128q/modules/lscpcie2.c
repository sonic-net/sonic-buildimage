/*
 *  COPYRIGHT (c) 2007 by Lattice Semiconductor Corporation
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */
/** @file lscpcie2.c
 * 
 * Generic PCI/PCI-Express Device Driver for Lattice Eval Boards.
 *
 * NOTE: code has been targeted for RedHat WorkStation 4.0 update 4
 *  kernel 2.6.9-42.ELsmp #1 SMP Wed Jul 12 23:27:17 EDT 2006 i686 i686 i386 GNU/Linux
 *
 *
 * A Linux kernel device driver, for Lattice PCIe Eval boards on the PCIe bus,
 * that maps the
 * device's PCI address windows (its BAR0-n) into shared memory that is
 * accessible by a user-space driver that implements the real control of
 * the device.
 * 
 * The intent is to map each active BAR to a corresponding minor device
 * so that the user space driver can open that minor device and mmap it
 * to get access to the registers.
 *
 * The BAR register definitions are Demo/application specific.  The driver
 * does not make any assumptions about the number of BARs that exist or
 * their size or use.  These are policies of the demo.  The driver just
 * makes them available to user space.
 *
 * The driver places no policies on the use of the device.  It simply allows
 * direct access to the PCI memory space occupied by the device.  Any number
 * of processes can open the device.  It is up to the higher level application
 * space driver to coordinate multiple accesses. The policy is basically the
 * same as a flat memory space embedded system.
 *
 * The ioctl system call can be used to control interrupts or other global
 * settings of the device.
 *
 * BUILDING:
 * 
 * Compile as regular user (no need to be root)
 * The product is a kernel module: lscpcie2.ko
 *
 *
 * INSTALLING:
 *
 * Need to be root to install a module.
 * 
 * Use the shell scripts insdrvr and rmdrvr to install and remove
 * the driver. 
 * The scripts may perform udev operations to make the devices known to the /dev
 * file system.
 *
 * Manual:
 * install with system call: /sbin/insmod lscpice.ko
 * remove with system call: /sbin/rmmod lscpcie2.ko
 * check status of module: cat /proc/modules
 * cat /proc/devices
 *
 * The printk() messages can be seen by running the command dmesg.
 *
 * The Major device number is dynamically assigned.  This info can
 * be found in /proc/devices.
 *
 *
 *
 * The minor number refers to the specific device.
 * Previous incarnations used the minor number to encode the board and BAR to
 * access.  This has been abandoned, and the minor now referes to the specific
 * device controlled by this driver (i.e. the eval board).  Once open() the
 * user has access to all BARs and board resources through the same file
 * descriptor.  The user space code knows how many BARs are active via ioctl
 * calls to return the PCI resource info.
 *
 *
 * Diagnostic information can be seen with: cat /proc/driver/lscpcie2
 *
 * The standard read/write system operations are not implemented because the
 * user has direct access to the device registers via a standard pointer.
 *
 * This driver implements the 2.6 kernel method of sysfs and probing to register
 * the driver, discover devices and make them available to user space programs.
 * A major player is creating a specific Class lscpcie2 which 
 *
 * register it with the PCI subsystem to probe for the eval board(s)
 * register it as a character device (cdev) so it can get a major number and minor numbers
 * create a special sysfs Class and add each discovered device under the class
 * udev processes the /sys/class/ tree to populate 
 *
 *
 * BASED ON:
 * Original lscpcie Linux driver which did things the 2.4 kernel way
 *
 * My previous work on vxp524 driver
 *
 * And:
 *    ------------------------------------------------------------------------
 *    Host shared memory driver (Mark McLeland 3Com/Cal Poly Project)
 *    ------------------------------------------------------------------------
 *    linux/drivers/char/mem.c (RedHat 2.4.7 kernel)
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Added devfs support. 
 *    Jan-11-1998, C. Scott Ananian <cananian@alumni.princeton.edu>
 *  Shared /dev/zero mmaping support, Feb 2000, Kanoj Sarcar <kanoj@sgi.com>
 *    -----------------------------------------------------------------------
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
//#include <linux/devfs_fs_kernel.h>
#include <linux/stat.h>
#include <linux/init.h>

#include <linux/tty.h>
#include <linux/selection.h>
#include <linux/kmod.h>

/* For devices that use/implement shared memory (mmap) */
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <linux/pci.h>
#include <linux/mman.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/pgalloc.h>

#include "Ioctl.h"

#ifndef CONFIG_PCI
	#error No PCI Bus Support in kernel!
#endif

#define USE_PROC  /* For debugging */

// Some Useful defines
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef OK
#define OK 0
#endif
#ifndef ERROR
#define ERROR -1
#endif

/* Change these defines to increase number of boards supported by the driver */

#define NUM_BARS MAX_PCI_BARS  /* defined in Ioctl.h */
#define NUM_BOARDS 4          // 4 PCIe boards per system is a lot of PCIe slots and eval boards to have on hand
#define MAX_BOARDS (NUM_BOARDS) 
#define MINORS_PER_BOARD 1     // 1 minor number per discrete device
#define MAX_MINORS (MAX_BOARDS * MINORS_PER_BOARD)  


#define DMA_BUFFER_SIZE (64 * 1024)


// Note: used as indexes into lists of strings
#define SC_BOARD    1
#define ECP2M_BOARD 2
#define ECP3_BOARD  3
#define BASIC_DEMO  1
#define SFIF_DEMO   2

#ifndef DMA_32BIT_MASK
#define DMA_32BIT_MASK  0x00000000ffffffffULL
#endif

#ifndef VM_RESERVED
# define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

MODULE_AUTHOR("Lattice Semiconductor");
MODULE_DESCRIPTION("Lattice PCIe Eval Board Device Driver");

/* License this so no annoying messages when loading module */
MODULE_LICENSE("Dual BSD/GPL");

MODULE_ALIAS("lscpcie2");





/*-------------------------------------------------*/
/*-------------------------------------------------*/
/*-------------------------------------------------*/
/*           DATA TYPES
 */
/*-------------------------------------------------*/
/*-------------------------------------------------*/
/*-------------------------------------------------*/

/**
 * This is the private data for each board's BAR that is mapped in.
 * NOTE: each structure MUST have minor as the first entry because it
 * it tested by a void * to see what BAR it is - See mmap()
 */
typedef struct PCI_Dev_BAR
{
	int      	 bar;
	void         *pci_addr; /**< the physical bus address of the BAR (assigned by PCI system), used in mmap */
	void         *kvm_addr; /**< the virtual address of a BAR mapped into kernel space, used in ioremap */
	int          memType;
	int          dataSize;
	unsigned long len;
	unsigned long pci_start;   /* info gathered from pci_resource_*() */
	unsigned long pci_end;
	unsigned long pci_flags;
} pci_dev_bar_t;


typedef struct PCIE_Board
{
	u32     ID;      /**< PCI device ID of the board (0x5303, 0xe235, etc) */
	u32     demoID;   /**< PCI subsys device ID of the board (0x3030, 0x3010, etc) */
	u32 	demoType;  /**< Basic or SFIF demo ID */
	u32	boardType; /**< SC or ECP2M device ID */
	u32     instanceNum;   /**< tracks number of identical board,demo devices in system */
	u32	majorNum;      /**< copy of driver's Major number for use in places where only device exists */
	u32	minorNum;      /**< specific Minor number asigned to this board */
	u32     numBars;           /**< number of valid BARs this board has, used to limit access into Dev_BARs[] */
	int     IRQ;               /**< -1 if no interrupt support, otherwise the interrupt line/vector */

	//---------------- BAR Assignments -------------
	u32	mmapBAR;          /**< which BAR is used for mmap into user space.  Can change with IOCTL call */
	u32	ctrlBAR;          /**< which BAR is used for control access, i.e. lighting LEDs, masking Intrs */
	void	*ctrlBARaddr;	  /**< above BAR memory ioremap'ed into driver space to access */


	//---------------- DMA Buffer -------------
	bool	hasDMA;        /**< true = allocated a buffer for DMA transfers by SFIF */
	size_t	dmaBufSize;    /**< size in bytes of the allocated kernel buffer */	
	dma_addr_t dmaPCIBusAddr;  /**< PCI bus address to access the DMA buffer - program into board */
	void	*dmaCPUAddr;       /**< CPU (software) address to access the DMA buffer - use in driver */


	struct  pci_dev *pPciDev;  /**< pointer to the PCI core representation of the board */

	pci_dev_bar_t Dev_BARs[NUM_BARS];  /**< database of valid, mapped BARs belonging to this board */

	struct cdev	charDev; /**< the character device implemented by this driver */


} pcie_board_t;



/**
 * The main container of all the data structures and elements that comprise the
 * lscpcie2 device driver.  Main elements are the array of detected eval boards,
 * the sysfs class, the assigned driver number.
 */
typedef struct LSCPCIe2
{

	dev_t 	drvrDevNum;      /**> starting [MAJOR][MINOR] device number for this driver */
	u32 	numBoards;       /**> total number of boards controlled by driver */

	u8	numSC_Basic;     /**> count of number of SC Basic boards found */
	u8	numSC_SFIF;      /**> count of number of SC SFIF boards found */
	u8	numECP2M_Basic;      /**> count of number of ECP2M Basic boards found */
	u8	numECP2M_SFIF;      /**> count of number of ECP2M SFIF boards found */
	u8	numECP3_Basic;      /**> count of number of ECP3 Basic boards found */
	u8	numECP3_SFIF;      /**> count of number of ECP3 SFIF boards found */
	

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	struct class *sysClass;   /**> the top entry point of lscpcie2 in /sys/class */
#else
	struct class_simple *sysClass;   /**> the top entry point of lscpcie2 in /sys/class */
#endif


	pcie_board_t Board[NUM_BOARDS];  /**> Database of LSC PCIe Eval Boards Installed */

} lscpcie2_t;


/*-------------------------------------------------*/
/*-------------------------------------------------*/
/*-------------------------------------------------*/
/*
 *            DRIVER GLOBAL VARIABLES
 */
/*-------------------------------------------------*/
/*-------------------------------------------------*/
/*-------------------------------------------------*/


/**
 * The driver's global database of all boards and run-time information.
 */
static lscpcie2_t lscpcie2;



static int DrvrDebug = 0;


static const char Version[] = "lscpcie2 v2.1.7 - ECP3 support";  /**< version string for display */


static const char *BoardName[4] = {"??", "SC", "ECP2M", "ECP3"};
static const char *DemoName[3] = {"??", "Basic", "SFIF"};



/**
 * List of boards we will attempt to find and associate with the driver.
 */
static struct pci_device_id lscpcie2_pci_id_tbl[] = 
{
	{ 0x1204, 0x5303, 0x1204, 0x3030, },   // SC SFIF
	{ 0x1204, 0xe250, 0x1204, 0x3030, },   // ECP2M SFIF
	{ 0x1204, 0xec30, 0x1204, 0x3030, },   // ECP3 SFIF
	{ 0x1204, 0xe250, 0x1204, 0x3010, },   // ECP2M50 Basic on Sol. Brd
	{ 0x1204, 0x5303, 0x1204, 0x3010, },   // SC Basic
	{ 0x1204, 0xec30, 0x1204, 0x3010, },   // ECP3 Basic
#if 0
	{ 0x1204, 0xe235, 0x1204, 0xe235, },   // ECP2M Basic - old ID; enable if have old demo bitstream
	{ 0x1204, 0xe235, 0x1204, 0x5303, },   // ECP2M Basic - old ID; enable if have old demo bitstream
	{ 0x1204, 0x5303, 0x1204, 0x5303, },   // SC Basic - old ID;  enable if have old demo bitstream
#endif
	{ }			/* Terminating entry */
};

MODULE_DEVICE_TABLE(pci, lscpcie2_pci_id_tbl);


/*========================================================================*/
/*========================================================================*/
/*========================================================================*/
/*
 *            PROC DEBUG STUFF
 */
/*========================================================================*/
/*========================================================================*/
/*========================================================================*/
#ifdef USE_PROC /* don't waste space if unused */

/**
 * Procedure to format and display data into the /proc filesystem when 
 * a user cats the /proc/driver/lscpie2 file.
 * Displays the driver major/minor #'s, BARs that are allocated, interrupt
 * resources.  General infomation about the board that was initialized.
 */
int lscpcie2_read_procmem(char *buf, char **start, off_t offset,
						 int count, int *eof, void *data)
{
	int i, n;
	int len = 0;
	// int limit = count - 80; /* Don't print more than this */
	pci_dev_bar_t *p;  

	*start = buf + offset;

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: /proc entry created\n");

	/* Put any messages in here that will be displayed by cat /proc/driver/.. */
	len += sprintf(buf+len, "\nLSC PCIE Device Driver Info\n");
	len += sprintf(buf+len, "\nNumBoards: %d  Major#: %d\n", lscpcie2.numBoards, MAJOR(lscpcie2.drvrDevNum));

	for (n = 0; n < NUM_BOARDS; n++)
	{

		if (lscpcie2.Board[n].ID != 0)
		{

			len += sprintf(buf+len, "Board:%d = %x  Demo=%x IRQ=%d\n", lscpcie2.Board[n].instanceNum, 
									lscpcie2.Board[n].ID,
									lscpcie2.Board[n].demoID,
									lscpcie2.Board[n].IRQ);

			for (i = 0; i < NUM_BARS; i++)
			{
				p = &lscpcie2.Board[n].Dev_BARs[i];
				len += sprintf(buf+len, "BAR[%d]  pci_addr=%p  kvm_addr=%p\n"
							   "          type=%d  dataSize=%d  len=%ld\n"
							   "          start=%lx  end=%lx  flags=%lx\n",
							   i, 
							   p->pci_addr,
							   p->kvm_addr,
							   p->memType,
							   p->dataSize,
							   p->len,
							   p->pci_start,
							   p->pci_end,
							   p->pci_flags);
			}
		}
	}

	if (len < offset + count)
		*eof = 1;	/* Mark that this is a complete buffer (the End of File) */

	/* Not sure about all this, but it works */
	len = len - offset;
	if (len > count)
		len = count;
	if (len < 0)
		len = 0;


	return(len);
}


#endif /* USE_PROC */



/**
 * Initialize the board's resources.
 * This is called when probe() has found a matching PCI device (via the PCI subsystem
 * probing for boards on behalf of the driver).  The board resources are mapped in
 * and its setup to be accessed.
 */
static pcie_board_t* initBoard(struct pci_dev *PCI_Dev_Cfg, void * devID)
{
	int i;
	unsigned char irq;
	pcie_board_t *pBrd;
	pci_dev_bar_t *pBAR;
	pci_dev_bar_t *p;
	u16 SubSystem;
	u16 VendorID;
	u16 DeviceID;

	/****************************************************/
	/* Device info passed in from the PCI controller via probe() */
	/****************************************************/

// TODO
// Add writing an 'E' to the LEDs to show an error if initialization fails
// Problem is we don't have BARs setup till end of function :-(


	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: init EvalBoard\n");

	/* Next available board structure in data base */
	pBrd = &lscpcie2.Board[lscpcie2.numBoards];

	if (pci_read_config_word(PCI_Dev_Cfg, PCI_VENDOR_ID, &VendorID))
	{
		printk(KERN_ERR "lscpcie2: init EvalBoard cfg access failed!\n");
		return(NULL);
	}
	if (VendorID != 0x1204)
	{
		printk(KERN_ERR "lscpcie2: init EvalBoard not Lattice ID!\n");
		return(NULL);
	}

	if (pci_read_config_word(PCI_Dev_Cfg, PCI_DEVICE_ID, &DeviceID))
	{
		printk(KERN_ERR "lscpcie2: init EvalBoard cfg access failed!\n");
		return(NULL);
	}

	if (pci_read_config_word(PCI_Dev_Cfg, PCI_SUBSYSTEM_ID, &SubSystem))
	{
		printk(KERN_ERR "lscpcie2: init EvalBoard cfg access failed!\n");
		return(NULL);
	}

	pBrd->ID = DeviceID;
	pBrd->demoID = SubSystem;
	pBrd->pPciDev = PCI_Dev_Cfg;
	pBrd->majorNum = MAJOR(lscpcie2.drvrDevNum);
	pBrd->minorNum = MINOR(lscpcie2.drvrDevNum) + lscpcie2.numBoards;


	// Figure out if board is SC or ECP2M or ECP3, if demo is Basic or SFIF
	if ((DeviceID == 0x5303) && (SubSystem == 0x3030))
	{
		++lscpcie2.numSC_SFIF;
		pBrd->instanceNum  = lscpcie2.numSC_SFIF;
		pBrd->boardType = SC_BOARD;
		pBrd->demoType = SFIF_DEMO;
		pBrd->ctrlBAR = 0;
	}
	else if ((DeviceID == 0xe250) && (SubSystem == 0x3030))
	{
		++lscpcie2.numECP2M_SFIF;
		pBrd->instanceNum  = lscpcie2.numECP2M_SFIF;
		pBrd->boardType = ECP2M_BOARD;
		pBrd->demoType = SFIF_DEMO;
		pBrd->ctrlBAR = 0;
	}
	else if ((DeviceID == 0x5303) && (SubSystem == 0x3010))
	{
		++lscpcie2.numSC_Basic;
		pBrd->instanceNum  = lscpcie2.numSC_Basic;
		pBrd->boardType = SC_BOARD;
		pBrd->demoType = BASIC_DEMO;
		pBrd->ctrlBAR = 0;
	}
	else if ((DeviceID == 0xe250) && (SubSystem == 0x3010))
	{
		++lscpcie2.numECP2M_Basic;
		pBrd->instanceNum  = lscpcie2.numECP2M_Basic;
		pBrd->boardType = ECP2M_BOARD;
		pBrd->demoType = BASIC_DEMO;
		pBrd->ctrlBAR = 0;
	}
	else if ((DeviceID == 0xec30) && (SubSystem == 0x3010))
	{
		++lscpcie2.numECP3_Basic;
		pBrd->instanceNum  = lscpcie2.numECP3_Basic;
		pBrd->boardType = ECP3_BOARD;
		pBrd->demoType = BASIC_DEMO;
		pBrd->ctrlBAR = 0;
	}
	else if ((DeviceID == 0xec30) && (SubSystem == 0x3030))
	{
		++lscpcie2.numECP3_SFIF;
		pBrd->instanceNum  = lscpcie2.numECP3_SFIF;
		pBrd->boardType = ECP3_BOARD;
		pBrd->demoType = SFIF_DEMO;
		pBrd->ctrlBAR = 0;
	}



#if 0  // OLD DEMO ID's for old bitstreams and boards
	else if ((DeviceID == 0x5303) && (SubSystem == 0x5303))
	{
		++lscpcie2.numSC_Basic;
		pBrd->instanceNum  = lscpcie2.numSC_Basic;
		pBrd->boardType = SC_BOARD;
		pBrd->demoType = BASIC_DEMO;
		pBrd->ctrlBAR = 1;
	}
	else if ((DeviceID == 0xe235) && (SubSystem == 0x5303))
	{
		++lscpcie2.numECP2M_Basic;
		pBrd->instanceNum  = lscpcie2.numECP2M_Basic;
		pBrd->boardType = ECP2M_BOARD;
		pBrd->demoType = BASIC_DEMO;
		pBrd->ctrlBAR = 1;
	}
	else if ((DeviceID == 0xe235) && (SubSystem == 0xe235))
	{
		++lscpcie2.numECP2M_Basic;
		pBrd->instanceNum  = lscpcie2.numECP2M_Basic;
		pBrd->boardType = ECP2M_BOARD;
		pBrd->demoType = BASIC_DEMO;
		pBrd->ctrlBAR = 1;
	}
#endif
	else
	{
		printk(KERN_ERR "lscpcie2: init ERROR! unknown board: %x %x\n", DeviceID, SubSystem);
		pBrd->instanceNum  = 0;
		pBrd->boardType = 0;
		pBrd->demoType = 0;
		return(NULL);
	}

	// For now, all demos use only one BAR and that BAR is for control plane and is also what will
	// be mmap'ed into user space for the driver interface to access.
	pBrd->mmapBAR = pBrd->ctrlBAR;


	//=============== Interrupt handling stuff ========================
	if (pci_read_config_byte(PCI_Dev_Cfg, PCI_INTERRUPT_LINE, &irq))
		pBrd->IRQ = -1;  // no interrupt
	else
		pBrd->IRQ = irq;

	if (DrvrDebug)
	{
		printk(KERN_INFO "lscpcie2: init brdID: %x  demoID: %x\n", DeviceID, SubSystem);
		printk(KERN_INFO "lscpcie2: init Board[] =%d\n", lscpcie2.numBoards);
		printk(KERN_INFO "lscpcie2: init IRQ=%d\n", irq);
	}


	//================ DMA Common Buffer (Consistent) Allocation ====================
	// First see if platform supports 32 bit DMA address cycles (like what won't!)
	if (pci_set_dma_mask(PCI_Dev_Cfg, DMA_32BIT_MASK))
	{
		printk(KERN_WARNING "lscpcie2: init DMA not supported!\n");
		pBrd->hasDMA = FALSE;
	}
	else
	{	
		pBrd->hasDMA = TRUE;
		pBrd->dmaBufSize = DMA_BUFFER_SIZE;
		pBrd->dmaCPUAddr = pci_alloc_consistent(PCI_Dev_Cfg, pBrd->dmaBufSize, &pBrd->dmaPCIBusAddr);
		if (pBrd->dmaCPUAddr == NULL)
		{
			printk(KERN_WARNING "lscpcie2: init DMA alloc failed! No DMA buffer.\n");
			pBrd->hasDMA = FALSE;
		}
	}


	/* Get info on all the PCI BAR registers */
	pBrd->numBars = 0;  // initialize
	for (i = 0; i < NUM_BARS; i++)
	{
		p = &(pBrd->Dev_BARs[i]);
		p->pci_start = pci_resource_start(PCI_Dev_Cfg, i);
		p->pci_end   = pci_resource_end(PCI_Dev_Cfg, i);
		p->len       = pci_resource_len(PCI_Dev_Cfg, i);
		p->pci_flags = pci_resource_flags(PCI_Dev_Cfg, i);

		if ((p->pci_start > 0) && (p->pci_end > 0))
		{
			++(pBrd->numBars);
			p->bar = i;
			p->pci_addr = (void *)p->pci_start;
			p->memType = p->pci_flags;   /* IORESOURCE Definitions: (see ioport.h)
						      * 0x0100 = IO
						      * 0x0200 = memory
						      * 0x0400 = IRQ
						      * 0x0800 = DMA
						      * 0x1000 = PREFETCHable
						      * 0x2000 = READONLY
						      * 0x4000 = cacheable
						      * 0x8000 = rangelength ???
						      */
			/*============================================================*
			*                                                             *
			* Windows DDK definitions CM_PARTIAL_RESOURCE_DESCRIPTOR.Type *
			*                                                             *
			* #define CmResourceTypeNull                0                 *
			* #define CmResourceTypePort                1                 *
			* #define CmResourceTypeInterrupt           2                 *
			* #define CmResourceTypeMemory              3                 *
			* #define CmResourceTypeDma                 4                 *
			* #define CmResourceTypeDeviceSpecific      5                 *
			* #define CmResourceTypeBusNumber           6                 *
			* #define CmResourceTypeMaximum             7                 *
			* #define CmResourceTypeNonArbitrated     128                 *
			* #define CmResourceTypeConfigData        128                 *
			* #define CmResourceTypeDevicePrivate     129                 *
			* #define CmResourceTypePcCardConfig      130                 *
			* #define CmResourceTypeMfCardConfig      131                 *
			*============================================================*/
			if (DrvrDebug)
			{
				printk(KERN_INFO "lscpcie2: init BAR=%d\n", i);
				printk(KERN_INFO "lscpcie2: init start=%lx\n", p->pci_start);
				printk(KERN_INFO "lscpcie2: init end=%lx\n", p->pci_end);
				printk(KERN_INFO "lscpcie2: init len=0x%lx\n", p->len);
				printk(KERN_INFO "lscpcie2: init flags=0x%lx\n", p->pci_flags);
			}
		}
	}


	// Map the BAR into kernel space so the driver can access registers.
	// The driver can not directly read/write the PCI physical bus address returned
	// by pci_resource_start().  In our current implementation the driver really
	// doesn't access the device registers, so this is not used.  It could be used
	// if the driver took a more active role in managing the devices on the board.

	// Map the default BAR into the driver's address space for access to LED registers,
	// masking off interrupts, and any other direct hardware controlled by the driver.
	// Note that the BAR may be different per demo.  Basic uses BAR1, SFIF & SGDMA use BAR0
	pBAR = &(pBrd->Dev_BARs[pBrd->ctrlBAR]);
	if (pBAR->pci_start)
	{
		pBrd->ctrlBARaddr = ioremap(pBAR->pci_start,   // PCI bus start address
					    pBAR->len);    // BAR size
		pBAR->kvm_addr = pBrd->ctrlBARaddr;  // for historic reasons

		if (pBrd->ctrlBARaddr)
		{
			writew(0x80f3, pBrd->ctrlBARaddr + 8);   // display an 'E' for error (erased if all goes well)
		}
		else 
		{
			printk(KERN_ERR "lscpcie2: init ERROR with ioremap\n");
			return(NULL);
		}

	}
	else
	{
		printk(KERN_ERR "lscpcie2: init ERROR ctrlBAR %d not avail!\n", pBrd->ctrlBAR);
		return(NULL);
	}

	++lscpcie2.numBoards;

	return(pBrd);  // pointer to board found and initialized

}


/*========================================================================*/
/*========================================================================*/
/*========================================================================*/
/*
 *            DRIVER FILE OPERATIONS (OPEN, CLOSE, MMAP, IOCTL)
 */
/*========================================================================*/
/*========================================================================*/
/*========================================================================*/


/**
 * Open
 * Any number of devices can open this address space.  The main reason for
 * this method is so the user has a file descriptor to pass to mmap() to
 * get the device memory mapped into their address space.
 *
 * The minor number is the index into the Board[] list.
 * It specifies exactly what board and is correlated to the device node filename.
 * Only valid board devices that have been enumerated by probe() and initialized
 * are in the list, are in /sys/class/lscpcie2/ and should appear in /dev/lscpcie/
 *
 * Note that the PCI device has already been enabled in probe() and init so it
 * doesn't need to be done again.
 */
int lscpcie2_open(struct inode *inode, struct file *filp)
{
	u32 brdNum;
	pcie_board_t *pBrd; 


	/* Extract the board number from the minor number */
 	brdNum = iminor(inode);

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: open(): board#=%d\n", brdNum);

	/* Validate (paranoid) */
	if (brdNum >= lscpcie2.numBoards)
		return(-ENODEV);

	// This is what the user wants to access
	pBrd = &lscpcie2.Board[brdNum];


	if (pBrd->ID == 0)
		return(-ENODEV);  // Board[] entry not configured correctly

// TODO
	// Maybe increment a reference count, don't let more than one user open a board???

	/* This allows ioctl quick access to the boards global resources */
	filp->private_data = pBrd;

	// Need to possibly connect up interrupts
	// pci_enable_device(pBrd->pPciDev);  // we may want to do this to "power-up" a closed board?

	// Write an 'O' to the LEDs to signal its openned
	if (pBrd->ctrlBARaddr)
		writew(0x00ff, pBrd->ctrlBARaddr + 8);   // display an 'O' 

	return(0);
}


/**
 * Close.
 * The complement to open().
 */
int lscpcie2_release(struct inode *inode, struct file *filp)
{
	struct PCIE_Board *pBrd = filp->private_data;

	u32 mnr = iminor(inode);

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: close() - closing board=%d\n", mnr);

	// Write a 'C' to the LEDs to signal its closed
	if (pBrd->ctrlBARaddr)
		writew(0x00f3, pBrd->ctrlBARaddr + 8);   // display a 'C' 

// TODO
	// Maybe decrement a reference count

	// pci_disable_device(pBrd->pPciDev);  // we may want to do this to "power-down" the board?

	return(0);
}



/**
 * ioctl.
 * Allow simple access to generic PCI control type things like enabling
 * device interrupts and such.
 * IOCTL works on a board object as a whole, not a BAR.
 */
long  lscpcie2_ioctl( struct file *filp, unsigned int cmd, unsigned long arg)
{
	int i;
	int status = OK;
	
	pcie_board_t *pBrd = NULL; 
	PCIResourceInfo_t *pInfo;
	ExtraResourceInfo_t *pExtra;

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: ioctl(cmd=%d arg=%lx size=%d)\n"
						, _IOC_NR(cmd), arg, _IOC_SIZE(cmd));

	if (_IOC_TYPE(cmd) != LSCPCIE_MAGIC)
		return(-EINVAL);
	if (_IOC_NR(cmd) > IOCTL_LSCPCIE_MAX_NR)
		return(-EINVAL);

	pBrd = filp->private_data;
	switch (cmd)
	{

		case IOCTL_LSCPCIE_GET_VERSION_INFO:
			// first make sure the pointer passed in arg is still valid user page
			if (!access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd)))
			{
				status = -EFAULT;
				break;  // abort
			}

			pInfo = kmalloc(sizeof(MAX_DRIVER_VERSION_LEN ), GFP_KERNEL);
			if (pInfo == NULL)
			{
				status = -EFAULT;
				break;  // abort
			}


			strncpy((void *)arg, Version, MAX_DRIVER_VERSION_LEN - 1);
			kfree(pInfo);  // release kernel temp buffer

			break;

		case IOCTL_LSCPCIE_SET_BAR:
			// The argument passed in is the direct BAR number (0-5) to use for mmap
			pBrd->mmapBAR = arg;
			break;


		case IOCTL_LSCPCIE_GET_RESOURCES:
			// first make sure the pointer passed in arg is still valid user page
			if (!access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd)))
			{
				status = -EFAULT;
				break;  // abort
			}

			pInfo = kmalloc(sizeof(PCIResourceInfo_t), GFP_KERNEL);
			if (pInfo == NULL)
			{
				status = -EFAULT;
				break;  // abort
			}

			if (pBrd->IRQ > 0)
			    pInfo->hasInterrupt = TRUE;
			else
			    pInfo->hasInterrupt = FALSE;
			pInfo->intrVector = pBrd->IRQ;
			pInfo->numBARs = pBrd->numBars;
			for (i = 0; i < MAX_PCI_BARS; i++)
			{
			    pInfo->BAR[i].nBAR = pBrd->Dev_BARs[i].bar;
			    pInfo->BAR[i].physStartAddr = (ULONG)pBrd->Dev_BARs[i].pci_addr;
			    pInfo->BAR[i].size = pBrd->Dev_BARs[i].len;
			    pInfo->BAR[i].memMapped = (pBrd->Dev_BARs[i].kvm_addr) ? 1 : 0;
			    pInfo->BAR[i].flags = (USHORT)(pBrd->Dev_BARs[i].pci_flags);
			    pInfo->BAR[i].type = (UCHAR)((pBrd->Dev_BARs[i].memType)>>8);  // get the bits that show IO or mem
			}
			for (i = 0; i < 0x100; ++i)
			    pci_read_config_byte(pBrd->pPciDev, i, &(pInfo->PCICfgReg[i]));

			if (copy_to_user((void *)arg, (void *)pInfo, sizeof(PCIResourceInfo_t)) != 0)
				status = -EFAULT; // Not all bytes were copied so this is an error
			kfree(pInfo);  // release kernel temp buffer

			break;


		case IOCTL_LSCPCIE2_GET_EXTRA_INFO:
			// first make sure the pointer passed in arg is still valid user page
			if (!access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd)))
			{
				status = -EFAULT;
				break;  // abort
			}

			pExtra = kmalloc(sizeof(ExtraResourceInfo_t), GFP_KERNEL);
			if (pExtra == NULL)
			{
				status = -EFAULT;
				break;  // abort
			}

			pExtra->devID = pBrd->minorNum;     // board number of specific device

			pExtra->busNum = pBrd->pPciDev->bus->number;  // PCI bus number board located on
			pExtra->deviceNum = PCI_SLOT(pBrd->pPciDev->devfn);     // PCI device number assigned to board
			pExtra->functionNum = PCI_FUNC(pBrd->pPciDev->devfn);   // our function number
			pExtra->UINumber = pBrd->minorNum;      // slot number (not implemented) 

			// Device DMA Common buffer memory info
			pExtra->hasDmaBuf = pBrd->hasDMA;        // true if DMA buffer has been allocated by driver 
			pExtra->DmaBufSize = pBrd->dmaBufSize;   // size in bytes of said buffer 
			pExtra->DmaAddr64 = 0;      // driver only asks for 32 bit, SGDMA only supports 32 bit 
			pExtra->DmaPhyAddrHi = 0;    // not used, only 32 bit
			pExtra->DmaPhyAddrLo = pBrd->dmaPCIBusAddr;    // DMA bus address to be programmed into device 

			strncpy(pExtra->DriverName, Version, MAX_DRIVER_NAME_LEN-1);   // version and name

			if (copy_to_user((void *)arg, (void *)pExtra, sizeof(ExtraResourceInfo_t)) != 0)
				status = -EFAULT; // Not all bytes were copied so this is an error
			kfree(pExtra);  // release kernel temp buffer

			break;

		default:
			status = -EINVAL;   // invalid IOCTL argument
	}

	return(status);
}


/**
 * mmap.
 * This is the most important driver method.  This maps the device's PCI
 * address space (based on the select mmap BAR number) into the user's
 * address space, allowing direct memory access with standard pointers.
 */
int lscpcie2_mmap(struct file *filp,
				 struct vm_area_struct *vma)
{
	int num;
	int sysErr;
	pcie_board_t *pBrd = filp->private_data;
	pci_dev_bar_t *pBAR;
	unsigned long phys_start;	 /* starting address to map */
	unsigned long mapSize;			/* requested size to map */
	unsigned long offset;		 /* how far into window to start map */

	// Map the BAR of the board, specified by mmapBAR (normally the default one that the
	// demo supports - normally only one valid BAR in our demos)
	pBAR = &(pBrd->Dev_BARs[pBrd->mmapBAR]);

	mapSize = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;

	num = pBAR->bar;  // this is a check to make sure we really initialized the BAR and structures

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: mmap Board=%d  BAR=%d\n", pBrd->minorNum, num);

	if (num == -1)
    {
    	if (DrvrDebug)
            printk(KERN_INFO "BAR not activated, no memory\n");
        
        
		return(-ENOMEM);   /* BAR not activated, no memory */
    }
    
    printk(KERN_INFO "\nasked for memory size %x BAR LEN. %x   VMA_START(%x) end %x\n",mapSize,pBAR->len,vma->vm_start,vma->vm_end);
#if 0
	if (mapSize > pBAR->len)
    {
        	if (DrvrDebug)
            printk(KERN_INFO "asked for too much memory.\n");
            
		return(-EINVAL);  /* asked for too much memory. */
    }
#endif    
	/* Calculate the starting address, based on the offset passed by user */
	phys_start = (unsigned long)(pBAR->pci_addr) + offset;

	if (DrvrDebug)
	{
		printk(KERN_INFO "lscpcie2: remap_page_range(0x%lx, 0x%x, %d, ...)\n",
		   vma->vm_start, (uint32_t)phys_start, (uint32_t)mapSize);
	}

	/* Make sure the memory is treated as uncached, non-swap device memory */
	vma->vm_flags = vma->vm_flags | VM_LOCKED | VM_IO | VM_RESERVED;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10))
	/* Do the page mapping the new 2.6.10+ way */
	sysErr = remap_pfn_range(vma,
				  (unsigned long)vma->vm_start,
				  (phys_start>>PAGE_SHIFT),
				  mapSize,
				  vma->vm_page_prot);

#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,8))
	/* Do the page mapping the intermediate way */
	sysErr = remap_page_range(vma,
				  (unsigned long)vma->vm_start,
				  phys_start,
				  mapSize,
				  vma->vm_page_prot);
#else
	#error Unsupported kernel version!!!!
#endif


	if (sysErr < 0)
	{
		printk(KERN_ERR "lscpcie2: remap_page_range() failed!\n");
		return(-EAGAIN);
	}

	return(0);
}




/**
 * read.
 * Read from system CommonBuffer DMA memory into users buffer.
 * User passes length (in bytes) like reading from a file.
 */
ssize_t lscpcie2_read(struct file *filp,
		 	char __user *userBuf,
			size_t len,
			loff_t *offp)
{
	pcie_board_t *pBrd = filp->private_data;

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: read len=%d\n", (u32)len);

	if (!pBrd->hasDMA)
		return(-EINVAL);   // invalid, no DMA buffer allocated

	if (len > pBrd->dmaBufSize)
		len = pBrd->dmaBufSize;  // trim it down

	if (copy_to_user(userBuf, pBrd->dmaCPUAddr, len) != 0)
		return(-EFAULT);
	
	return(len);
}


/**
 * write.
 * Write from users buffer into system CommonBuffer DMA memory.
 * User passes length (in bytes) like writing to a file.
 */
ssize_t lscpcie2_write(struct file *filp,
		 	const char __user *userBuf,
			size_t len,
			loff_t *offp)
{
	pcie_board_t *pBrd = filp->private_data;

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: write len=%d\n", (u32)len);

	if (!pBrd->hasDMA)
		return(-EINVAL);   // invalid, no DMA buffer allocated

	if (len > pBrd->dmaBufSize)
		len = pBrd->dmaBufSize;  // trim it down

	if (copy_from_user(pBrd->dmaCPUAddr, userBuf, len) != 0)
		return(-EFAULT);
	
	return(len);
}





/*==================================================================*/
/*==================================================================*/
/*==================================================================*/
/*
 *              M O D U L E   F U N C T I O N S
 */
/*==================================================================*/
/*==================================================================*/
/*==================================================================*/

/**
 * The file operations table for the device.
 * read/write/seek, etc. are not implemented because device access
 * is memory mapped based.
 */
static struct file_operations drvr_fops =
{
	owner:   THIS_MODULE,
	open:    lscpcie2_open,
	release: lscpcie2_release,
	unlocked_ioctl:   lscpcie2_ioctl,
	mmap:    lscpcie2_mmap,
	read:	 lscpcie2_read,
	write:	 lscpcie2_write,
};


/*------------------------------------------------------------------*/



/**
 * Called by the PCI subsystem when it has probed the PCI buses and has
 * found a device that matches the criteria registered in the pci table.
 * For each board found, the type and demo are determined in the initBoard
 * routine.  All resources are allocated.  A new device is added to the
 * /sys/class/lscpcie2/ tree with the name created by the:
 * <board><demo><instance> information.
 */
static int lscpcie2_probe(struct pci_dev *pdev,
				 const struct pci_device_id *ent)
{
	static char devNameStr[12] = "lscpcie2__";
	pcie_board_t *brd;
	int err;

	devNameStr[9] = '0' + lscpcie2.numBoards;

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: pci probe for: %s  pdev=%p  ent=%p\n", 
					devNameStr, pdev, ent);

	/*
	 * Enable the bus-master bit values.
	 * Some PCI BIOSes fail to set the master-enable bit.
	 * Some demos support being an initiator, so need bus master ability.
	 */
	err = pci_request_regions(pdev, devNameStr);
	if (err)
		return err;

	pci_set_master(pdev);

	err = pci_enable_device(pdev);
	if (err)
		return err;

	/*
	 * Call to perform board specific initialization and figure out
	 * which BARs are active, interrupt vectors, register ISR, what board
	 * it is (SC or ECP2M or ECP3), what demo (Basic or SFIF) and what instance
	 * number (is it the 2nd time we've seen a SC Basic?)
	 * Returns pointer to the Board structure after all info filled in.
	 */
	brd = initBoard(pdev, (void *)ent);

	if (brd == NULL)
	{
		printk(KERN_ERR "lscpcie2: Error initializing Eval Board\n");
		// Clean up any resources we acquired along the way
		pci_release_regions(pdev);
		
		return(-1);
	}
		


	// Initialize the CharDev entry for this new found eval board device
	brd->charDev.owner = THIS_MODULE;
	kobject_set_name(&(brd->charDev.kobj), "lscpcie2");

	cdev_init(&(brd->charDev), &drvr_fops);

//?????
// Does cdev_add initialize reference count in the kobj?
//?????

	/* Create the minor numbers here and register the device as a character device.
	 * A number of minor devices can be associated with this particular board.
	 * The hope/idea is that we give the starting minor number and the number of them
	 * and all those devices will be associated to this one particular device.
	 */
	if (cdev_add(&(brd->charDev), MKDEV(brd->majorNum,brd->minorNum), MINORS_PER_BOARD))
	{
		printk(KERN_ERR "lscpcie2: Error adding char device\n");
		kobject_put(&(brd->charDev.kobj));
		return(-1);	
	}


	/* This creates a new entry in the /sys/class/lscpcie2/ tree that represents this
	 * new device in user space.  An entry in /dev will be created based on the name
	 * given in the last argument.  udev is responsible for mapping sysfs Classes to
	 * device nodes, and is done outside this kernel driver.
	 *
	 * The name is constructed from the board type, demo type and board instance.
	 * Examples are "sc_basic_0", "sc_basic_1", "ecp2m_sfif_0"
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	device_create(lscpcie2.sysClass,
				NULL,
				MKDEV(brd->majorNum,brd->minorNum),
				 &(pdev->dev),   // this is of type struct device, the PCI device?
				"%s_%s_%d", BoardName[brd->boardType], DemoName[brd->demoType], brd->instanceNum);
#else
	class_simple_device_add(lscpcie2.sysClass,
				MKDEV(brd->majorNum,brd->minorNum),
				NULL,   // this is of type struct device, but who?????
				"%s_%s_%d", BoardName[brd->boardType], DemoName[brd->demoType], brd->instanceNum);
#endif



	/* Store a pointer to the Board structure with this PCI device instance for easy access
	 * to board info later on.
	 */
	pci_set_drvdata(pdev, brd);

	// Write an 'I' to the LEDs at end of initialization
	if (brd->ctrlBARaddr)
		writew(0x2233, brd->ctrlBARaddr + 8);   // display an 'I' 

	return 0;
}



/**
 * Undo all resource allocations that happened in probe() during device discovery
 * and initialization.  Major steps are:
 * 1.) release PCI resources
 * 2.) release minor numbers
 * 3.) delete the character device associated with the Major/Minor
 * 4.) remove the entry from the sys/class/lscpcie2/ tree
 */ 
static void lscpcie2_remove(struct pci_dev *pdev)
{
	pcie_board_t *brd = pci_get_drvdata(pdev);

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: pci remove for device: pdev=%p board=%p\n", pdev, brd);


	// Write an 'R' to the LEDs when device is removed
	if (brd->ctrlBARaddr)
		writew(0x98c7, brd->ctrlBARaddr + 8);   // display an 'R' 

	// Release DMA Buffer
	if (brd->hasDMA)
	{
		pci_free_consistent(pdev, brd->dmaBufSize, brd->dmaCPUAddr, brd->dmaPCIBusAddr);
	}


	// Shut off interrupt sources - not implemented in Basic or SFIF

	// Free our internal access to the control BAR address space
	if (brd->ctrlBARaddr)
		iounmap(brd->ctrlBARaddr);

	// No more access after this call
	pci_release_regions(pdev);

	// Unbind the minor numbers of this device
	// using the MAJOR_NUM + board_num + Minor Range of this board
	cdev_del(&(brd->charDev));

	unregister_chrdev_region(MKDEV(brd->majorNum, brd->minorNum), MINORS_PER_BOARD);


	// Remove the device entry in the /sys/class/lscpcie2/ tree

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	device_destroy(lscpcie2.sysClass, MKDEV(brd->majorNum, brd->minorNum));
#else
	class_simple_device_remove(MKDEV(brd->majorNum, brd->minorNum));
#endif

}


/*-------------------------------------------------------------------------*/
/*             DRIVER INSTALL/REMOVE POINTS                                */
/*-------------------------------------------------------------------------*/

/*
 *	Variables that can be overriden from module command line
 */
static int	debug = 0;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "lscpcie2 enable debugging (0-1)");

/**
 * Main structure required for registering a driver with the PCI core.
 * name must be unique across all registered PCI drivers, and shows up in
 * /sys/bus/pci/drivers/
 * id_table points to the table of Vendor,Device,SubSystem matches
 * probe is the function to call when enumerating PCI buses to match driver to device
 * remove is the function called when PCI is shutting down and devices/drivers are 
 * being removed.
 */
static struct pci_driver lscpcie2_driver = {
	.name = "lscpcie2",
	.id_table = lscpcie2_pci_id_tbl,
	.probe = lscpcie2_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
   .remove = __devexit_p(lscpcie2_remove),
#else
   .remove = lscpcie2_remove,
#endif    

/*
	.save_state - Save a device's state before its suspended
	.suspend - put device into low power state
	.resume - wake device from low power state
	.enable_wake - enable device to generate wake events from low power state
*/
};


/*-------------------------------------------------------------------------*/

/**
 * Initialize the driver.
 * called by init_module() when module dynamically loaded by insmod
 */
static int __init lscpcie2_init(void)
{
	int result;
	int i, n;
	int err;
	//pci_dev_bar_t *p;
	//pcie_board_t *pB;

	printk(KERN_INFO "lscpcie2: _init()   debug=%d\n", debug);
	DrvrDebug = debug;

	/* Initialize the driver database to nothing found, no BARs, no devices */
	memset(&lscpcie2, 0, sizeof(lscpcie2));
	for (n = 0; n < NUM_BOARDS; n++)
		for (i = 0; i < NUM_BARS; i++)
			lscpcie2.Board[n].Dev_BARs[i].bar = -1;

	/*
	 * Register device driver as a character device and get a dynamic Major number
         * and reserve enough minor numbers for the maximum amount of boards * BARs
         * we'd expect to find in a system.
	 */
	result = alloc_chrdev_region(&lscpcie2.drvrDevNum,   // return allocated Device Num here
				      0, 	    // first minor number
				      MAX_MINORS,   
                                      "lscpcie2");

	if (result < 0)
	{
		printk(KERN_WARNING "lscpcie2: can't get major/minor numbers!\n");
		return(result);
	}


	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: Major=%d  num boards=%d\n", MAJOR(lscpcie2.drvrDevNum), lscpcie2.numBoards );

	
	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: cdev_init()\n");



	/* Create the new sysfs Class entry that will hold the tree of detected Lattice PCIe Eval
	 * board devices.
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	lscpcie2.sysClass = class_create(THIS_MODULE, "lscpcie2");
#else
	lscpcie2.sysClass = class_simple_create(THIS_MODULE, "lscpcie2");
#endif
	if (IS_ERR(lscpcie2.sysClass))
	{
		printk(KERN_ERR "lscpcie2: Error creating simple class interface\n");
		return(-1);	
	}



	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: registering driver with PCI\n");


	/* Register our PCI components and functions with the Kernel PCI core.
	 * Returns negative number for error, and 0 if success.  It does not always
	 * return the number of devices found and bound to the driver because of hot
	 * plug - they could be bound later.
	 */
	err = pci_register_driver(&lscpcie2_driver);

	if (DrvrDebug)
		printk(KERN_INFO "lscpcie2: pci_register_driver()=%d\n", err);

	if (err < 0)
		return(err);


#ifdef USE_PROC /* only when available */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	proc_create("driver/lscpcie2", 0, 0, &drvr_fops);
#else
	create_proc_read_entry("driver/lscpcie2", 0, 0, lscpcie2_read_procmem, NULL);
#endif    
#endif


	return(0); /* succeed */

}


/**
 * Driver clean-up.
 * Called when module is unloaded by kernel or rmmod
 */
static void __exit lscpcie2_exit(void)
{
	int i;

	printk(KERN_INFO "lscpcie2: _exit()\n");


	pci_unregister_driver(&lscpcie2_driver);

	for (i = 0; i < NUM_BOARDS; i++)
	{
		if (lscpcie2.Board[i].ID != 0)
		{
			/* Do the cleanup for each active board */
			printk(KERN_INFO "lscpcie2: Cleaning up board: %d\n", i);

			// Disable and release IRQ if still active
		}
	}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
	class_destroy(lscpcie2.sysClass);
#else
	class_simple_destroy(lscpcie2.sysClass);
#endif

	// Free every minor number and major number we reserved in init
	unregister_chrdev_region(lscpcie2.drvrDevNum, MAX_MINORS);


#ifdef USE_PROC
	remove_proc_entry("driver/lscpcie2", NULL);
#endif

	return;
}


/*
 * Kernel Dynamic Loadable Module Interface APIs
 */

module_init(lscpcie2_init);
module_exit(lscpcie2_exit);



