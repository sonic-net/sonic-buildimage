#ifndef __PASSWD_UTILS_H__
#define __PASSWD_UTILS_H__

#include <pwd.h>    // struct passwd

extern struct passwd * fgetpwnam(const char * user);

#endif // __PASSWD_UTILS_H__
