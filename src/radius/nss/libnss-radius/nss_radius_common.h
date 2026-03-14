/*
Copyright 2020 Broadcom. All rights reserved.
The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
*/

/*
 * nss_radius_common.h
 *
 * Common code for NSS for RADIUS cached MPL(Management Privilege Attribute).
 * Used by both NSS module(uses the cache) and PAM module(updates the cache)
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <nss.h>

#define RADIUS_MAX_MPL (15)
#define RADIUS_MIN_MPL (1)

#define RADIUS_NSS_CONF "/etc/radius_nss.conf"
#define RADIUS_MAX_NSS_CONF_SZ 2048

#define RADIUS_ATTRIBUTE_CACHE_DIR "/var/cache/radius/user"
#define RADIUS_CACHE_DIR "/var/cache/radius"
#define RADIUS_ATTR_MPL "Management-Privilege-Level"

#define ETC_PASSWD "/etc/passwd"

#define USERADD "/usr/sbin/useradd"
#define USERMOD "/usr/sbin/usermod"
#define USERDEL "/usr/sbin/userdel"

#define BUFLEN 4096

#define UNCONFIRMED_AGEOUT_DEFAULT        600
#define UNCONFIRMED_CLEAR_LIMIT_DEFAULT   10

#define RADIUS_CONFIRMED        0
#define RADIUS_UNCONFIRMED      1

#if defined(TEST_RADIUS_NSS)

#undef RADIUS_NSS_CONF
#define RADIUS_NSS_CONF "radius_nss.conf"

#undef RADIUS_CACHE_DIR
#define RADIUS_CACHE_DIR "."

#undef RADIUS_ATTRIBUTE_CACHE_DIR
#define RADIUS_ATTRIBUTE_CACHE_DIR "user"


#undef ETC_PASSWD
#define ETC_PASSWD "passwd"

#undef USERADD
#define USERADD "/bin/echo"
#undef USERMOD
#define USERMOD "/bin/echo"
#undef USERDEL
#define USERDEL "/bin/echo"

#define syslog(priority,format,...) fprintf(stderr,format"\n",__VA_ARGS__)

#endif


#define STATUS_EPERM             1
#define STATUS_ENOENT            2
#define STATUS_ESRCH             3
#define STATUS_EIO               5
#define STATUS_E2BIG             7
#define STATUS_EINVAL           22
#define STATUS_EFBIG            27



typedef struct _radius_nss_mpl {
    gid_t       gid;
    char        * groups;    /* Supplementary groups */
    char        * gecos;
    char        * shell;
} RADIUS_NSS_MPL;

typedef struct _radius_nss_conf {
    char * prog;
    int debug;
    int trace;
    int many_to_one;
    int allow_anonymous;
    char * unconfirmed_regexp;
    int unconfirmed_ageout;
    int unconfirmed_clear_limit;
    RADIUS_NSS_MPL rnm[RADIUS_MAX_MPL];
} RADIUS_NSS_CONF_B;

int parse_nss_config( RADIUS_NSS_CONF_B * conf, char * prog,
    char * file_buf, int file_buf_sz, int * errnop, int * plockfd);

int unparse_nss_config( RADIUS_NSS_CONF_B * conf, int * errnop, int * plockfd);

int radius_lookup_cache( char * prog, const char * nam, int * pmpl);

int radius_fill_pw( RADIUS_NSS_CONF_B * conf, int mpl,
    const char * nam, struct passwd * pwd,
    char * buffer, size_t buflen, int * errnop);

int radius_copy_pw( RADIUS_NSS_CONF_B * conf, struct passwd * res,
    const char * nam, struct passwd * pwd,
    char * buffer, size_t buflen, int * errnop);

int is_sshd_lookup(RADIUS_NSS_CONF_B * conf, const char * nam);

int radius_getpwnam_r(char * prog, const char * name,
    struct passwd * pwd, char * buf, int buflen, struct passwd ** result);
int radius_update_user(RADIUS_NSS_CONF_B * conf, const char * user, int mpl);
int radius_create_user(RADIUS_NSS_CONF_B * conf, const char * user, int mpl,
    int unconfirmed);
int radius_clear_unconfirmed_users(RADIUS_NSS_CONF_B * conf);

