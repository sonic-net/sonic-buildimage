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

#include "nss_radius_common.h"

enum nss_status _nss_radius_getpwnam_r( const char * nam, struct passwd * pwd,
    char * buf, size_t buflen, int * errnop);

static int write_account_state(int gid, int docker_member)
{
    FILE *fp;

    fp = fopen(ETC_PASSWD, "w");
    if (fp == NULL)
        return 1;

    fprintf(fp, "remote_user:x:2001:%d:remote_user:/home/remote_user:/bin/bash\n",
        gid);
    fclose(fp);

    fp = fopen(ETC_GROUP, "w");
    if (fp == NULL)
        return 1;
    fprintf(fp, "users:x:100:\n");
    fprintf(fp, "docker:x:999:%s\n", docker_member ? "remote_user" : "");
    fclose(fp);

    return 0;
}

static int write_privileged_account_state(void)
{
    FILE *fp;

    fp = fopen(ETC_PASSWD, "w");
    if (fp == NULL)
        return 1;
    fprintf(fp, "remote_user_su:x:2002:1000:remote_user_su:"
        "/home/remote_user_su:/bin/bash\n");
    fclose(fp);

    fp = fopen(ETC_GROUP, "w");
    if (fp == NULL)
        return 1;
    /* Primary groups need not repeat the user in their member list. */
    fprintf(fp, "admin:x:1000:\n");
    fprintf(fp, "sudo:x:27:remote_user_su\n");
    fprintf(fp, "docker:x:999:remote_user_su\n");
    fclose(fp);

    return 0;
}

static int test_existing_user_group_reconciliation(void)
{
    RADIUS_NSS_CONF_B conf = {0};
    int status;

    conf.prog = "test_nss_radius";
    conf.rnm[0].gid = 100;
    conf.rnm[0].groups = "";
    conf.rnm[0].gecos = "remote_user";
    conf.rnm[0].shell = "/bin/bash";
    conf.many_to_one = 1;

    if (write_account_state(999, 1) != 0)
        return 1;
    radius_test_last_user_mod_gid[0] = '\0';
    radius_test_last_user_mod_groups[0] = '\0';
    radius_test_user_mod_calls = 0;

    status = radius_reconcile_user(&conf, "remote_user", 1);
    if (status != 1 || radius_test_user_mod_calls != 1 ||
        strcmp(radius_test_last_user_mod_gid, "100") != 0 ||
        strcmp(radius_test_last_user_mod_groups, "") != 0)
        return 1;

    if (write_account_state(100, 1) != 0)
        return 1;
    radius_test_user_mod_calls = 0;
    status = radius_reconcile_user(&conf, "remote_user", 1);
    if (status != 1 || radius_test_user_mod_calls != 1)
        return 1;

    if (write_account_state(100, 0) != 0)
        return 1;
    radius_test_user_mod_calls = 0;
    status = radius_reconcile_user(&conf, "remote_user", 1);
    if (status != 0 || radius_test_user_mod_calls != 0)
        return 1;

    conf.rnm[0].gid = 1000;
    conf.rnm[0].groups = "admin,sudo,docker";
    conf.rnm[0].gecos = "remote_user_su";
    if (write_privileged_account_state() != 0)
        return 1;
    radius_test_user_mod_calls = 0;
    status = radius_reconcile_user(&conf, "remote_user_su", 1);
    if (status != 0 || radius_test_user_mod_calls != 0)
        return 1;

    conf.rnm[0].groups = "27,999";
    radius_test_user_mod_calls = 0;
    status = radius_reconcile_user(&conf, "remote_user_su", 1);
    if (status != 0 || radius_test_user_mod_calls != 0)
        return 1;

    conf.many_to_one = 0;
    conf.rnm[0].gid = 100;
    conf.rnm[0].groups = "";
    if (write_account_state(100, 0) != 0)
        return 1;
    radius_test_last_user_mod_gid[0] = '\0';
    if (radius_update_user(&conf, "remote_user", 1) != 0 ||
        strcmp(radius_test_last_user_mod_gid, "") != 0) {
        return 1;
    }

    unlink(ETC_PASSWD);
    unlink(ETC_GROUP);
    unlink(RADIUS_USER_LOCK);
    return 0;
}

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

    if (test_existing_user_group_reconciliation() != 0) {
        fprintf(stderr, "existing user group reconciliation test failed\n");
        return 1;
    }

    return status;
}
