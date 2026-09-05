/*
Copyright 2019 Broadcom. All rights reserved.
The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
*/

/*
 * plugin implements getpwnam_r for NSS for RADIUS cached MPL(Management
 *     Privilege Attribute).
 */

#include "nss_radius_common.h"

/*
 * NSS entry point for getpwnam().
 */
enum nss_status _nss_radius_getpwnam_r( const char * nam, struct passwd * pwd,
    char * buf, size_t buflen, int * errnop) {

    enum nss_status status = NSS_STATUS_NOTFOUND;
    int mpl = 1;
    int ncfd = -1;
    RADIUS_NSS_CONF_B radius_nss_conf, * conf = &(radius_nss_conf);
    RADIUS_NSS_MPL * rnm;
    char file_buf[RADIUS_MAX_NSS_CONF_SZ];
    char buffer[BUFLEN];
    struct passwd pw, *res = NULL;
    char * prog = "nss";

      /* Ignore filename completion.
       */
    if (!nam || !strcmp(nam, "*") || !pwd || !buf || (buflen == 0))
        return NSS_STATUS_NOTFOUND;

    parse_nss_config(conf, prog, file_buf, sizeof(file_buf), errnop, &ncfd);

    if (radius_lookup_cache(prog, nam, &mpl) == 0) {

        /* The MPL exists for this user in the cache.
         */

        if (conf->debug)
            syslog( LOG_DEBUG, "%s: nam: %s", prog, nam);

        rnm = &((conf->rnm)[mpl-1]);
        if (conf->many_to_one) {
            radius_getpwnam_r(prog, rnm->gecos, &pw, buffer, sizeof(buffer),
                &res);
        } else if (conf->allow_anonymous) {
            radius_create_user(conf, nam, mpl, RADIUS_CONFIRMED);
            radius_getpwnam_r(prog, nam, &pw, buffer, sizeof(buffer), &res);
        }

    } else if (conf->allow_anonymous && is_sshd_lookup(conf, nam)) {

        /* Could be an sshd doing a getpwnam() before pam_authenticate().
         */

        rnm = &((conf->rnm)[mpl-1]);
        if (conf->many_to_one) {
            if (radius_getpwnam_r(prog, rnm->gecos, &pw, buffer, sizeof(buffer),
                    &res) != 0) {
                radius_create_user(conf, rnm->gecos, mpl, RADIUS_CONFIRMED);
                radius_getpwnam_r(prog, rnm->gecos, &pw, buffer, sizeof(buffer),
                    &res);
            }
        } else {
            radius_create_user(conf, nam, mpl, RADIUS_UNCONFIRMED);
            radius_getpwnam_r(prog, nam, &pw, buffer, sizeof(buffer), &res);
        }
    }

    /* Fill the pwd
     */

    if (res) {
        status = NSS_STATUS_SUCCESS;
        radius_copy_pw(conf, res, nam, pwd, buf, buflen, errnop);
    }

    unparse_nss_config(conf, errnop, &ncfd);

    return status;
}

