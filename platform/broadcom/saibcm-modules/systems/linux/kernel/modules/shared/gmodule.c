/*
 * $Id: gmodule.c,v 1.20 Broadcom SDK $
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 * 
 * Generic Linux Module Framework
 *
 * Hooks up your driver to the kernel 
 */

#include <lkm.h>
#include <gmodule.h>
#include <linux/init.h>
/* Module Vector Table */
static gmodule_t* _gmodule = NULL;




static int _dbg_enable = 0;

static int
gvprintk(const char* fmt, va_list args)
    __attribute__ ((format (printf, 1, 0)));

static int
gvprintk(const char* fmt, va_list args)
{
    static char _buf[256];

    _buf[0] = 0;
    sprintf(_buf, "%s (%d): ", _gmodule->name, current->pid);
    vsprintf(_buf+strlen(_buf), fmt, args);
    printk("%s",_buf);

    return 0;
}

int
gprintk(const char* fmt, ...)
{
    int rv;

    va_list args;
    va_start(args, fmt);
    rv = gvprintk(fmt, args);
    va_end(args);
    return rv;
}

int 
gdbg(const char* fmt, ...)
{
    int rv = 0;

    va_list args;
    va_start(args, fmt);
    if(_dbg_enable) {
	rv = gvprintk(fmt, args);
    }
    va_end(args);
    return rv;
}


/*
 * Proc FS Utilities
 */
#if PROC_INTERFACE_KERN_VER_3_10
int 
pprintf(struct seq_file *m, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    seq_vprintf(m, fmt, args);
    va_end(args);
    return 0;
}

static int _gmodule_proc_show(struct seq_file *m, void *v){
    _gmodule->pprint(m);
    return 0;
}

static int
_gmodule_proc_open(struct inode * inode, struct file * file) {
    if(_gmodule->open) {
        _gmodule->open();
    }

    return single_open(file, _gmodule_proc_show, NULL);
}

static ssize_t
_gmodule_proc_write(struct file *file, const char *buffer,
                   size_t count, loff_t *loff)
{
    /* Workaround to toggle debugging */
    if(count > 2) {
	if(buffer[0] == 'd') {
	    _dbg_enable = buffer[1] - '0';
	    GDBG("Debugging Enabled");
	}
    }
    return count;
}

static int _gmodule_proc_release(struct inode * inode, struct file * file) {
    if(_gmodule->close) {
        _gmodule->close();
    }

    return single_release(inode, file);
}

struct proc_ops _gmodule_proc_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        _gmodule_proc_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_write =       _gmodule_proc_write,
    .proc_release =     _gmodule_proc_release,
};
#else
int
gmodule_vpprintf(char** page_ptr, const char* fmt, va_list args)
{
    *page_ptr += vsprintf(*page_ptr, fmt, args);
    return 0;
}

int
gmodule_pprintf(char** page_ptr, const char* fmt, ...)
{
    int rv;

    va_list args;
    va_start(args, fmt);
    rv = gmodule_vpprintf(page_ptr, fmt, args); 
    va_end(args);
    return rv;
}

static char* _proc_buf = NULL;

int 
pprintf(struct seq_file *m, const char* fmt, ...)
{  
    int rv;

    va_list args;
    va_start(args, fmt);
    rv = gmodule_vpprintf(&_proc_buf, fmt, args); 
    va_end(args);
    return rv;
}

#define PSTART(b) _proc_buf = b
#define PPRINT proc_print
#define PEND(b) (_proc_buf-b)

static int
_gmodule_pprint(char* buf)
{
    PSTART(buf);
    _gmodule->pprint(NULL);
    return PEND(buf);
}

static int 
_gmodule_read_proc(char *page, char **start, off_t off,
		   int count, int *eof, void *data)
{
    *eof = 1;
    return _gmodule_pprint(page);
}

static int 
_gmodule_write_proc(struct file *file, const char *buffer,
		    unsigned long count, void *data)
{
    /* Workaround to toggle debugging */
    if(count > 2) {
	if(buffer[0] == 'd') {
	    _dbg_enable = buffer[1] - '0';
	    GDBG("Debugging Enabled");
	}
    }
    return count;
}
#endif

static int
_gmodule_create_proc(void)
{
    struct proc_dir_entry* ent;
#if PROC_INTERFACE_KERN_VER_3_10
    if((ent = proc_create(_gmodule->name,
                          S_IRUGO | S_IWUGO,
                          NULL,
                          &_gmodule_proc_fops)) != NULL) {
        return 0;
    }
#else
    if((ent = create_proc_entry(_gmodule->name, S_IRUGO | S_IWUGO, NULL)) != NULL) {
        ent->read_proc = _gmodule_read_proc;
        ent->write_proc = _gmodule_write_proc;
        return 0;
    }
#endif
    return -1;
}

static void 
_gmodule_remove_proc(void)
{
    remove_proc_entry(_gmodule->name, NULL);
}

static int
_gmodule_open(struct inode *inode, struct file *filp)
{
    if(_gmodule->open) {
	_gmodule->open();
    }
    return 0;
}

static int 
_gmodule_release(struct inode *inode, struct file *filp)
{
    if(_gmodule->close) {
	_gmodule->close();
    }
    return 0;
}

static long
_gmodule_unlocked_ioctl(struct file *filp,
                        unsigned int cmd, unsigned long arg)
{
    if(_gmodule->ioctl) {
	return _gmodule->ioctl(cmd, arg);
    } else {
	return -1;
    }
}

static long
_gmodule_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    if(_gmodule->ioctl) {
	return _gmodule->ioctl(cmd, arg);
    } else {
	return -1;
    }
}


static int
_gmodule_mmap(struct file *filp, struct vm_area_struct *vma)
{
    if (_gmodule->mmap) {
        return _gmodule->mmap(filp, vma);
    }
#ifdef BCM_PLX9656_LOCAL_BUS
	vma->vm_flags |= VM_RESERVED | VM_IO;
	pgprot_val(vma->vm_page_prot) |= _PAGE_NO_CACHE | _PAGE_GUARDED;

	if (io_remap_pfn_range(	vma,
				vma->vm_start,
				vma->vm_pgoff,
				vma->vm_end - vma->vm_start,
				vma->vm_page_prot)) {
                return (-EAGAIN);
	}
	return (0);
#else/* BCM_PLX9656_LOCAL_BUS */
    return -EPERM;
#endif/* BCM_PLX9656_LOCAL_BUS */
}

/* FILE OPERATIONS */

struct file_operations _gmodule_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = _gmodule_unlocked_ioctl,
    .open =       _gmodule_open,
    .release =    _gmodule_release,
    .mmap =       _gmodule_mmap,
    .compat_ioctl = _gmodule_compat_ioctl,
};


void __exit
cleanup_module(void)
{
    if(!_gmodule) return;

    /* Remove ourselves from user-mode access */
    unregister_chrdev(_gmodule->major, _gmodule->name);

    /* Remove any proc entries */
    if(_gmodule->pprint) {
	_gmodule_remove_proc();
    }

    /* Module-specific cleanup */
    if(_gmodule->cleanup) {
	_gmodule->cleanup();
    }
}

int __init
init_module(void)
{  
    int rc;

    /* Get our definition */
    _gmodule = gmodule_get();
    if(!_gmodule) return -ENODEV;


    /* Register ourselves */
    rc = register_chrdev(_gmodule->major, 
			 _gmodule->name, 
			 &_gmodule_fops);
    if (rc < 0) {
	printk(KERN_WARNING "%s: can't get major %d",
	       _gmodule->name, _gmodule->major);
	return rc;
    }

    if(_gmodule->major == 0) {
	_gmodule->major = rc;
    }

    /* Specific module Initialization */
    if(_gmodule->init) {
	int rc;
	if((rc = _gmodule->init()) < 0) {
            unregister_chrdev(_gmodule->major, _gmodule->name);
	    return rc;
	}
    }

    /* Add a /proc entry, if valid */
    if(_gmodule->pprint) {    
	_gmodule_create_proc();
    }

    return 0; /* succeed */
}
