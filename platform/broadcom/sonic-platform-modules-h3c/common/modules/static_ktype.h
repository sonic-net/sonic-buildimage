/*
* Copyright (c) 2019  <sonic@h3c.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef __STATIC_KTYPE_H
#define __STATIC_KTYPE_H

/* default kobject attribute operations */
static ssize_t static_kobj_attr_show(struct kobject *kobj, struct attribute *attr,
                                     char *buf)
{
    struct kobj_attribute *kattr;
    ssize_t ret = -EIO;

    kattr = container_of(attr, struct kobj_attribute, attr);
    if (kattr->show)
        ret = kattr->show(kobj, kattr, buf);
    return ret;
}

static ssize_t static_kobj_attr_store(struct kobject *kobj, struct attribute *attr,
                                      const char *buf, size_t count)
{
    struct kobj_attribute *kattr;
    ssize_t ret = -EIO;

    kattr = container_of(attr, struct kobj_attribute, attr);
    if (kattr->store)
        ret = kattr->store(kobj, kattr, buf, count);
    return ret;
}

const struct sysfs_ops kobj_sysfs_ops =
{
    .show   = static_kobj_attr_show,
    .store  = static_kobj_attr_store,
};

static void static_kobj_release(struct kobject *kobj)
{
    pr_debug("no need to free static kobject: (%p): %s\n", kobj, __func__);
    //kfree(kobj);
}

//静态kobject 不是通过kmalloc动态申请的。销毁时不需要free
static struct kobj_type static_kobj_ktype =
{
    .release    = static_kobj_release,
    .sysfs_ops  = &kobj_sysfs_ops,
};

#endif
