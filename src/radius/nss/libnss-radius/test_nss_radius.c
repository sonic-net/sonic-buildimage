/*
Copyright 2019 Broadcom. All rights reserved.
The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
*/

/*
 * Test code for _nss_radius_getpwnam_r(): NSS entry point for getpwnam
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
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

enum nss_status _nss_radius_getpwnam_r( const char * nam, struct passwd * pwd,
    char * buf, size_t buflen, int * errnop);

int main(int ac, char * av[]) {


    enum nss_status status;
    struct passwd pw;
    char buf[256];
    int errno;

    char * users[] = { "admin", "user", "netops", "operator", "unknown", 0 };
    char ** u;

    printf("buf: %p, len: %lx\n", buf, sizeof(buf));

    for ( u = users ; *u ; u++) {
        status = _nss_radius_getpwnam_r( *u, &pw, buf, sizeof(buf),
                     &errno);

        printf("\n%s: status:%d\n", *u, status);

        if (status != NSS_STATUS_SUCCESS)
            continue;

        printf("\tnam: %s, passwd: %s, uid: %u, gid: %u\n",
            pw.pw_name, pw.pw_passwd, pw.pw_uid, pw.pw_gid);
        printf("\tnam: %p, passwd: %p, uid: %u, gid: %u\n",
            pw.pw_name, pw.pw_passwd, pw.pw_uid, pw.pw_gid);

        printf("\n");

        printf("\tgecos: %s, dir: %s, shell: %s\n",
            pw.pw_gecos, pw.pw_dir, pw.pw_shell);
        printf("\tgecos: %p, dir: %p, shell: %p\n",
            pw.pw_gecos, pw.pw_dir, pw.pw_shell);
    }

    return status;
}

