/*
Copyright 2019 Broadcom. All rights reserved.
The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
*/

/*
 * cache_radius to be invoked after a successful pam_radius_auth.so
 * module invocation as:
 *
 *   auth  [success=1 default=ignore] pam_exec.so /usr/sbin/cache_radius
 *
 * cache_radius -u to be invoked to clear aged unconfirmed users.
 *
 *   auth  [success=1 default=ignore] pam_exec.so /usr/sbin/cache_radius -u
 *
 */

#include "nss_radius_common.h"

static int radius_update_cache_cleanup(int status, mode_t mask, FILE * fp) {

    /* Umask restore.
     */
    umask(mask);

    if (fp)
        fclose(fp);

    return status;
}

static int radius_update_cache( char * prog, const char * nam, int mpl) {

    mode_t  mask;
    char filename[PATH_MAX];
    int written;
    struct stat sb;
    FILE * fp = NULL;

    /* Umask save, change.
     */
    mask = umask(022);

    if (((written = snprintf( filename, sizeof(filename), "%s/%s",
        RADIUS_ATTRIBUTE_CACHE_DIR, nam)) + sizeof(RADIUS_ATTR_MPL) + 1)
            >= sizeof(filename)) {
        syslog( LOG_ERR, "%s: \"%s\": too long.", prog, filename);
        return(radius_update_cache_cleanup(STATUS_EINVAL, mask, fp));
    }

    /* Create the directory.
     */
    if ((!((stat( RADIUS_CACHE_DIR, &sb) == 0) && S_ISDIR(sb.st_mode))
            && (mkdir( RADIUS_CACHE_DIR, 0755) == -1))
    || (!((stat( RADIUS_ATTRIBUTE_CACHE_DIR, &sb) == 0) && S_ISDIR(sb.st_mode))
        && (mkdir( RADIUS_ATTRIBUTE_CACHE_DIR, 0755) == -1))
    || (!((stat( filename, &sb) == 0) && S_ISDIR(sb.st_mode))
        && (mkdir( filename, 0755) == -1))) {
        syslog( LOG_ERR, "%s: \"%s\": mkdir() fails. Or ! dir errno %d",
            prog, filename, errno);
        return(radius_update_cache_cleanup(STATUS_ENOENT, mask, fp));
    }

    /* Create the file.
     */
    strcat( filename, "/");
    strcat( filename, RADIUS_ATTR_MPL);

    if (   ((fp = fopen( filename, "w")) == NULL)
        || (fprintf(fp, "%d\n", mpl) < 0)) {
        syslog( LOG_ERR,
          "%s: \"%s\": fopen(): %p or fprintf() fails. errno %d", prog,
          filename, (void *) fp, errno);
        return(radius_update_cache_cleanup(STATUS_EPERM, mask, fp));
    }

    syslog(LOG_INFO, "%s: MPL %d updated for user %s", prog, mpl, nam);

    return(radius_update_cache_cleanup(0, mask, fp));
}


static int main_cleanup(int status, RADIUS_NSS_CONF_B * conf, int * pncfd) {
    int my_errno = 0;
    if (conf)
        unparse_nss_config(conf, &my_errno, pncfd);
    return status;
}

int main(int ac, char * av[]) {

    int mpl = 1, cached_mpl = 1;
    int status = 0;
    int my_errno = 0;
    char * user = NULL, * privilege;
    RADIUS_NSS_CONF_B radius_nss_conf, * conf = & radius_nss_conf;
    RADIUS_NSS_MPL * rnm;
    char file_buf[RADIUS_MAX_NSS_CONF_SZ];
    int ncfd = -1;
    int no_clear_unconfirmed = 0;
    int clear_unconfirmed_limit = 0;
    int refresh_user = 0;
    char buf[BUFLEN];
    struct passwd pw, *result = NULL;

    /* Set the logging parameters.
     */

    openlog(av[0], LOG_CONS | LOG_PID, LOG_AUTHPRIV);

    /* Parse Command-Line options.
     */

    if ((ac == 2) && (strncmp(av[1], "-n", 3) == 0)) {

        no_clear_unconfirmed = 1;

    } else if (ac > 1) {

        syslog(LOG_WARNING,
            "%s: Ignoring unknown option:\"%s\"\n", av[0], av[1]);
    }

    /* Read environment PAM_USER.
     */

    if (   ((user = getenv("PAM_USER")) == NULL)
        || (user[0] == 0)) {
        syslog(LOG_WARNING,
            "%s: Missing or bad PAM_USER in environment:\"%s\"\n",
            av[0], user ? user : "(null)");
        exit(main_cleanup(STATUS_ENOENT, NULL, NULL));
    }

    /* Read environment Privilege.
     */

    if ((privilege = getenv("Privilege")) != NULL) {
        mpl = atoi(privilege);
        if (!((RADIUS_MIN_MPL <= mpl) && (mpl <= RADIUS_MAX_MPL))) {
            syslog(LOG_WARNING, "%s: Invalid Management-Privilege-Level %d\n",
                av[0], mpl);
            mpl = 1;
        }
    } else {
        syslog(LOG_INFO, "%s: Missing or bad Privilege in environment:\"%s\"\n",
            av[0], privilege ? privilege : "");
    }

    parse_nss_config(conf, av[0], file_buf, sizeof(file_buf), &my_errno, &ncfd);

    if (     (radius_lookup_cache( conf->prog, user, &cached_mpl) != 0)
          || (cached_mpl != mpl)
          || (radius_getpwnam_r(conf->prog, user, &pw, buf, sizeof(buf),&result)
                 != 0)) {

        radius_update_cache( conf->prog, user, mpl);
        refresh_user = 1;

    }

    if (conf->many_to_one) {

        rnm = &((conf->rnm)[mpl-1]);
        if (radius_getpwnam_r(conf->prog, rnm->gecos, &pw, buf, sizeof(buf),
                &result) != 0) {
            radius_create_user(conf, rnm->gecos, mpl, RADIUS_CONFIRMED);
        }

        if (mpl != cached_mpl) {

            syslog(LOG_WARNING, "%s: Management-Privilege-Level init/changed:"
                " %d --> %d\n", conf->prog, cached_mpl, mpl);
            fprintf(stdout, "Management-Privilege-Level inited or changed.\n");
            fprintf(stdout, "Please login again.\n");
            exit(main_cleanup(STATUS_EPERM, conf, &ncfd));

        }

    } else if (!(conf->many_to_one) && refresh_user) {

        radius_update_user( conf, user, mpl);

    }

    if (!no_clear_unconfirmed && (conf->many_to_one == 0)) {
        clear_unconfirmed_limit = conf->unconfirmed_clear_limit;
        while(radius_clear_unconfirmed_users(conf) == 0) {
            if (clear_unconfirmed_limit-- <= 0) {
                syslog(LOG_INFO, "%s: Clear unconfirmed limit %d reached:",
                    conf->prog, conf->unconfirmed_clear_limit);
                break;
            }
        }
    }

    exit(main_cleanup(0, conf, &ncfd));
}
