/*
 * Simple Elf loader for pciesvc library code
 *
 * Authors: Rob Gardner, Joseph Dobosenski
 *
 */

#include "kpcimgr_api.h"
#include <linux/elf.h>
#include <linux/reboot.h>
#include <linux/stacktrace.h>
#include <linux/of.h>
#include "version.h"

MODULE_LICENSE("GPL");
MODULE_VERSION("OLdev-1.0");
MODULE_INFO(build, PCIESVC_VERSION);
MODULE_INFO(intree, "Y"); /* no out-of-tree module taints kernel */

int kpcimgr_module_register(struct module *mod,
			    struct kpcimgr_entry_points_t *ep, int relocate);

/* loaded elf data, code, symtab, etc */
static void *elfdata, *bin_image;
static int elf_valid_bytes, bin_valid_bytes, start_offset;
static char *strtbl;
static Elf64_Sym *symtbl, *symtbl_end;

/*
 * Retrieve kstate_t from kpcimgr via device tree
 */
kstate_t *get_kpci_state(void)
{
	static kstate_t *kstate = NULL;
	struct device_node *dn;
	u32 kstate_idx;
	int err;

	if (kstate != NULL)
		return kstate;

	dn = of_find_compatible_node(NULL, NULL, "pensando,kpcimgr");
	if (dn == NULL) {
		pr_err("pciesvc_loader: no compatible device found in dtree\n");
		return NULL;
	}

	err = of_property_read_u32(dn, "kstate-index", &kstate_idx);
	if (err) {
		pr_err("pciesvc_loader: please upgrade to new kstate-index dtree\n");
		return NULL;
	}

	kstate = (kstate_t *) of_iomap(dn, kstate_idx);
	if (kstate == NULL) {
		pr_err("pciesvc_loader: failed to map kstate\n");
		return NULL;
	}

	return kstate;
}

/*
 * Allocate executable memory
 */
void *vmalloc_exec(unsigned long size)
{
	pgprot_t prot = PAGE_KERNEL_EXEC;

	return __vmalloc(size, GFP_KERNEL, prot);
}

/*
 * Simple Elf Loader
 */
int load_elf(void)
{
	Elf64_Ehdr *h = elfdata;
	Elf64_Shdr *sh;
	size_t length;
	char *sh_strtbl;
	int i;

	strtbl = NULL;
	symtbl = NULL;

	if (h == NULL)
		return -ENOENT;

	if (strncmp(h->e_ident, ELFMAG, SELFMAG))
		return -EINVAL;

	if (h->e_machine != EM_AARCH64)
		return -ENOTTY;

	/* determine memory needed for executable image */
	sh = elfdata + h->e_shoff;
	for (i=0; i < h->e_shnum; i++, sh++)
		if (sh->sh_flags & SHF_ALLOC)
			length = sh->sh_addr + sh->sh_size;

	/* round up to page align */
	bin_valid_bytes = length;
	length = (length + 0xfff) & ~0xfffL;

	/* allocate executable memory */
	if (bin_image)
		vfree(bin_image);
	bin_image = vmalloc_exec(length);
	memset(bin_image, 0, length);

	/* iterate over sections */
	sh = elfdata + h->e_shoff;
	sh_strtbl = elfdata + sh[h->e_shstrndx].sh_offset;
	for (i=0; i < h->e_shnum; i++, sh++) {

		if (sh->sh_flags & SHF_ALLOC) {
			if (sh->sh_type == SHT_NOBITS)
				memset(bin_image + sh->sh_addr, 0, sh->sh_size);
			else
				memcpy(bin_image + sh->sh_addr, elfdata + sh->sh_offset, sh->sh_size);
			if (sh->sh_flags & SHF_EXECINSTR)
				start_offset = sh->sh_addr;
		} else {
			if (sh->sh_type == SHT_SYMTAB &&
			    strcmp(sh_strtbl + sh->sh_name, ".symtab") == 0) {
				symtbl = elfdata + sh->sh_offset;
				symtbl_end = elfdata + sh->sh_offset + sh->sh_size;
			}
			if (sh->sh_type == SHT_STRTAB &&
			    strcmp(sh_strtbl + sh->sh_name, ".strtab") == 0)
				strtbl = elfdata + sh->sh_offset;
		}
	}
	flush_icache_range((unsigned long)bin_image, length);
	return 0;
}

static ssize_t run_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "%lx\n", (long)bin_image);
}

static ssize_t run_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf,
			   size_t count)
{
	void *(*start_fn)(void *), (*version_fn)(char **);
	struct kpcimgr_entry_points_t *ep;
	struct module mod;
	char *version;
	int ret;

	if (elfdata == NULL)
		return -ENODEV;

	ret = load_elf();
	if (ret)
		return ret;

	start_fn = bin_image + start_offset;
	ep = start_fn(NULL);

	pr_info("pciesvc_loader: expected mgr ver=%d, lib version=%d.%d\n",
	       ep->expected_mgr_version,
	       ep->lib_version_major, ep->lib_version_minor);

	version_fn = ep->entry_point[K_ENTRY_GET_VERSION];
	(void) version_fn(&version);
	pr_info("pciesvc_loader: lib returned version string '%s'\n", version);

	mod.core_layout.base = bin_image;
	mod.core_layout.size = bin_valid_bytes;
	mod.init_layout.base = 0;
	mod.init_layout.size = 0;
	strcpy_s(mod.name, sizeof(mod.name), "pciesvc.lib");

	ret = kpcimgr_module_register(&mod, ep, 1);
	if (ret) {
		pr_err("kpcimgr_module_register returned %d\n", ret);
		pr_err("bin_image was at 0x%lx\n", (long)bin_image);
		vfree(bin_image);
		bin_image = NULL;
	}

	return count;
}


static ssize_t valid_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	Elf64_Ehdr *h = elfdata;
	int valid;

	valid = (h && h->e_machine == EM_AARCH64 &&
		 strncmp(h->e_ident, ELFMAG, SELFMAG) == 0);

	return sprintf(buf, "%d\n", valid);
}

static ssize_t valid_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf,
			   size_t count)
{
	ssize_t rc;
	long val;

	rc = kstrtol(buf, 0, &val);
	if (rc)
		return rc;
	elf_valid_bytes = val;
	pr_info("%s: valid set to %d\n", __func__, elf_valid_bytes);

	return count;
}

static ssize_t code_read(struct file *file, struct kobject *kobj,
			   struct bin_attribute *attr, char *out,
			   loff_t off, size_t count)
{
	if (elfdata == NULL)
		return 0;

	if (off > elf_valid_bytes)
		return 0;
	if (off + count > elf_valid_bytes)
		count = elf_valid_bytes - off;

	memcpy(out, elfdata + off, count);
	return count;
}

static ssize_t code_write(struct file *filp, struct kobject *kobj,
			     struct bin_attribute *bin_attr, char *buf,
			     loff_t off, size_t count)
{
	static int code_size = 64*1024;

	if (elfdata == NULL) {
		elfdata = vmalloc(code_size);
		if (elfdata == NULL) {
			pr_err("module_alloc\n");
			return -ENOMEM;
		}
	}

	if (off + count > code_size) {
		void *new = vmalloc(2*code_size);
		if (new == NULL) {
			pr_err("module_alloc\n");
			return -ENOMEM;
		}
		memcpy(new, elfdata, code_size);
		vfree(elfdata);
		elfdata = new;
		code_size = 2*code_size;
	}

	memcpy(elfdata + off, buf, count);
	elf_valid_bytes = off + count;
	return count;
}

struct kobject *pciesvc_kobj;
static DEVICE_ATTR_RW(valid);
static DEVICE_ATTR_RW(run);
static struct attribute *dev_attrs[] = {
	&dev_attr_valid.attr,
	&dev_attr_run.attr,
	NULL,
};

#define CODE_BLOCK_SIZE 256*1024
static BIN_ATTR_RW(code, CODE_BLOCK_SIZE);
static struct bin_attribute *dev_bin_attrs[] = {
	&bin_attr_code,
	NULL,
};

const struct attribute_group pciesvc_attr_group = {
	.attrs = dev_attrs,
	.bin_attrs = dev_bin_attrs,
};


static void pciesvc_sysfs_init(void)
{
	pciesvc_kobj = kobject_create_and_add("pciesvc", kernel_kobj);
	if (pciesvc_kobj == NULL) {
		pr_err("failed to create kernel obj\n");
		return;
	}

	if (sysfs_create_group(pciesvc_kobj, &pciesvc_attr_group)) {
		pr_err("sysfs_create_group failed\n");
		return;
	}
}

/*
 * This is a routine that is called from an optional hook
 * in kernel/kallsyms.c/kallsyms_lookup()
 */
char *pciesvc_address_lookup(unsigned long addr,
			     unsigned long *symbolsize,
			     unsigned long *offset,
			     char **modname, char *namebuf)
{
	kstate_t *ks = get_kpci_state();
	unsigned long base, code_sz;
        unsigned long candidate = 0;
        Elf64_Sym *sym;
        int i, index;

	base = (unsigned long) ks->code_base;
	code_sz = ks->code_size;

	/* range check */
	if (addr < base || addr >= base + code_sz)
		return NULL;

	addr = addr - base;

        if (!symtbl || !strtbl)
                return NULL;

        /* Look up symbol in our table */
	/* Could sort this, but we typically would get here via panic */
        for (i = 0, sym = symtbl; sym < symtbl_end; i++, sym++){
                if (ELF64_ST_TYPE(sym->st_info) == STT_FUNC) {
                        if (sym->st_value > candidate && sym->st_value <= addr){
                                candidate = sym->st_value;
				index = i;
                        }
                }
        }

	if (candidate) {
		sym = &symtbl[index];
		sprintf(namebuf, "%s", strtbl + sym->st_name);
		if (symbolsize)
			*symbolsize = sym->st_size;
		if (offset)
			*offset = addr - candidate;
		*modname = "pciesvc.lib";
		return namebuf;
	}
	else
		return NULL;
}

/*
 * Panic hook. On systems with kdump enabled, this won't get
 * called unless you put "crash_kexec_post_notifiers" on the
 * boot command line.
 */
static int pciesvc_panic(struct notifier_block *nb,
			 unsigned long code, void *unused)
{
	unsigned long entries[48], offset, size;
	struct stack_trace trace;
	char *modname, buffer[256];
	const char *name;
	int i, len;

	trace.nr_entries = 0;
	trace.max_entries = ARRAY_SIZE(entries);
	trace.entries = entries;
	trace.skip = 0;

	save_stack_trace(&trace);
	pr_emerg("pciesvc stack trace:\n");
	for (i=0; i<trace.nr_entries; i++) {
		name = pciesvc_address_lookup(entries[i], &size, &offset,
					      &modname, buffer);
		if (name) {
			len = strlen(buffer);
			len += sprintf(buffer + len, "+%#lx/%#lx", offset, size);
			if (modname)
				len += sprintf(buffer + len, " [%s]", modname);
		}
		else
			sprint_symbol(buffer, entries[i]);
		pr_emerg("%s\n", buffer);
	}

	return NOTIFY_DONE;
}

struct notifier_block panic_notifier;
#ifdef KERNEL_HAS_GENERIC_LOOKUP
void register_generic_address_lookup(void *fn);
#endif

static int __init pciesvc_loader_probe(void)
{
	kstate_t *ks;

	ks = get_kpci_state();
	if (ks == NULL)
		return -ENXIO;

	pciesvc_sysfs_init();

	/* register panic notifier */
	panic_notifier.notifier_call = pciesvc_panic;
        atomic_notifier_chain_register(&panic_notifier_list,
                                       &panic_notifier);

#ifdef KERNEL_HAS_GENERIC_LOOKUP
	register_generic_address_lookup(pciesvc_address_lookup);
#endif
	return 0;
}

static void __exit pciesvc_loader_cleanup(void)
{
	kstate_t *ks;

	/* unregister panic notifier */
       atomic_notifier_chain_unregister(&panic_notifier_list,
                                         &panic_notifier);

#ifdef KERNEL_HAS_GENERIC_LOOKUP
	register_generic_address_lookup(NULL);
#endif
	pr_info("Cleaning up module.\n");
	sysfs_remove_group(pciesvc_kobj, &pciesvc_attr_group);
	kobject_del(pciesvc_kobj);

	ks = get_kpci_state();
	vfree(ks);
	vfree(elfdata);
	vfree(bin_image);
}

module_init(pciesvc_loader_probe);
module_exit(pciesvc_loader_cleanup);
MODULE_LICENSE("GPL");
