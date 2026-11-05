#ifndef __MISSING_MEMFD_CREATE_H__
#define __MISSING_MEMFD_CREATE_H__

// The memfd_create syscall has been available since Linux 3.17 (2014-10-05)
// However, the API memfd_create() was only added to glibc 2.27 (2018-02-01)
// As of 2020-01-15, SONiC is running with Linux 4.9 (2016-12-11) and
// glibc 2.24 (2016-08-05), which is why we need the following.
#if !__GLIBC_PREREQ(2,27)

#   ifndef _GNU_SOURCE
#      define _GNU_SOURCE
#   endif
#   include <unistd.h>
#   include <sys/syscall.h>         /* syscall(), SYS_memfd_create */

#   ifdef SYS_memfd_create
        static inline int memfd_create(const char *name, unsigned int flags)
        {
            return syscall(SYS_memfd_create, name, flags);
        }
#   else // SYS_memfd_create
#       include <errno.h>           /* errno, ENOSYS */
        static inline int memfd_create(const char *name, unsigned int flags)
        {
            errno = ENOSYS;
            return -1;
        }
#   endif // SYS_memfd_create

#   ifndef MFD_CLOEXEC
#       define MFD_CLOEXEC     0x0001U
#   endif

#endif // !__GLIBC_PREREQ(2,27)

#endif // __MISSING_MEMFD_CREATE_H__
