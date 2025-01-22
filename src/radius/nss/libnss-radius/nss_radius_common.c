/*
Copyright 2019 Broadcom. All rights reserved.
The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
*/

/*
 * Common code for NSS for RADIUS cached MPL(Management Privilege Attribute).
 * Used by both NSS module(uses the cache) and PAM module(updates the cache)
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
#include <sys/file.h>
#include <regex.h>
#include <time.h>
#include <sys/wait.h>

#include "nss_radius_common.h"

static void dump_rnm(int mpl, RADIUS_NSS_MPL * rnm, char * msg) {

    syslog( LOG_DEBUG, "dump_rnm: %s:"
      "mpl %d gid %d groups \"%s\" gecos \"%s\" shell \"%s\"",
      (msg ? msg : ""), mpl, rnm->gid, rnm->groups, rnm->gecos, rnm->shell);
}

static char * parse_line(RADIUS_NSS_CONF_B * conf, char * file_buf,
    char ** pscanpos, int * premain) {

    char * line = NULL;
    int skipline = 0;

    for( ; (*premain); (*pscanpos)++, (*premain)--) {
        if (skipline) {
            if (    ((**pscanpos) == '\n')
                 || ((**pscanpos) == '\r')
                 || ((**pscanpos) == '\f')) {
                 skipline = 0;
            }
            continue;
        } else if (line == NULL) {
            if (isspace(**pscanpos)) {
                continue;
            } else if ((**pscanpos) == '#') {
                skipline = 1;
            } else {
                line = *pscanpos;
            }
            continue;
        } else if (   ((**pscanpos) == '\n')
                   || ((**pscanpos) == '\r')
                   || ((**pscanpos) == '\f')) {
            break;
        }
    }

    if (line) {
        *((*pscanpos)++) = 0;
        if (*premain)
            (*premain)--;
    }

    if (conf->trace)
        syslog(LOG_DEBUG, "parse_line: \"%s\"\n", line ? line : "");

    return line;
}

static char * parse_token(RADIUS_NSS_CONF_B * conf, char ** pbufpos,
    int * premain) {
    char * token = NULL;

    for( token = *pbufpos;
         (*premain) && (**pbufpos) && ((**pbufpos) != ';') ;
         (*pbufpos)++, (*premain)--)
        ; /* Empty Body */

    if (*premain) {
        *((*pbufpos)++) = 0;
        (*premain)--;
    }

    if (conf->trace)
        syslog(LOG_DEBUG, "parse_token: \"%s\"\n", token ? token : "");

    return token;
}

static int parse_user(RADIUS_NSS_CONF_B * conf, char * line) {
    RADIUS_NSS_MPL parse_rnm;
    char * token, * bufpos;
    int mpl = 0;
    int ret = 0, remain;

    bufpos = line;
    remain = strlen(line);
    memset((char *) &parse_rnm, 0, sizeof(parse_rnm));

    while (remain && (token = parse_token(conf, &bufpos, &remain))) {
        if (strncmp(token, "user_priv=", 10) == 0) {
            mpl = atoi(&token[10]);
        } else if (strncmp(token, "group=", 6) == 0) {
            parse_rnm.groups = &(token[6]);
        } else if (strncmp(token, "pw_info=", 8) == 0) {
            parse_rnm.gecos = &(token[8]);
        } else if (strncmp(token, "uid=", 4) == 0) {
            syslog( LOG_WARNING, "%s: Ignoring \"%s\"."
                " Using system assigned User ID", RADIUS_NSS_CONF, token);
        } else if (strncmp(token, "gid=", 4) == 0) {
            parse_rnm.gid = atol(&(token[4]));
        } else if (strncmp(token, "dir=", 4) == 0) {
            syslog( LOG_WARNING, "%s: Ignoring \"%s\"."
                " Using system assigned home directory",
                RADIUS_NSS_CONF, token);
        } else if (strncmp(token, "shell=", 6) == 0) {
            parse_rnm.shell = &(token[6]);
        }
    }

    if (conf->trace)
        dump_rnm(mpl, &parse_rnm, "parse");

    if (   ((RADIUS_MIN_MPL > mpl) || (mpl > RADIUS_MAX_MPL))
        || (parse_rnm.gid == 0)
        || (parse_rnm.groups == NULL)
        || (parse_rnm.gecos == NULL)
        || (parse_rnm.shell == NULL)) {

        syslog( LOG_WARNING, "%s: Bad user_priv line \"%s\"", RADIUS_NSS_CONF,
            line);
        ret = 1;
    } else {
        (conf->rnm)[mpl-1] = parse_rnm;
    }

    return ret;
}

static void init_rnm(RADIUS_NSS_CONF_B * conf) {

    RADIUS_NSS_MPL * rnm = conf->rnm;

      /* Set rnm[0,max_mpl-1].
       */

    memset((char *) rnm, 0, sizeof(conf->rnm));

    rnm[0].gid   = 999;
    rnm[0].groups = "docker";
    rnm[0].gecos = "remote_user";
    rnm[0].shell = "/usr/bin/sonic-launch-shell";
    rnm[RADIUS_MAX_MPL-1].gid   = 1000;
    rnm[RADIUS_MAX_MPL-1].groups = "admin,sudo,docker";
    rnm[RADIUS_MAX_MPL-1].gecos = "remote_user_su";
    rnm[RADIUS_MAX_MPL-1].shell = "/usr/bin/sonic-launch-shell";

}

static int user_add(const char* name, char* gid, char* sec_grp, char* gecos, 
                        char* home, char* shell, const char* unconfirmed_user, int many_to_one) {
    pid_t pid, w;
    int status = 0;
    int wstatus;
    char cmd[64];

    snprintf(cmd, 63, "%s", USERADD);

    pid = fork();

    if(pid > 0) {
        do {
            w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
            if (w == -1)
                return -1;
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
        if WIFEXITED(wstatus)
            return WEXITSTATUS(wstatus);
        else
            return -1;

    // Child

    } else if(pid == 0) {

        if (many_to_one)
          execl(cmd, cmd, "-g", gid, "-G", sec_grp, "-c", gecos, "-m", "-s", shell, name, NULL);
        else
          execl(cmd, cmd, "-U", "-G", sec_grp, "-c", unconfirmed_user, "-d", home, "-m", "-s", shell, name, NULL);
        syslog(LOG_ERR, "exec of %s failed with errno=%d", cmd, errno);
        return -1;

    // Error
    } else {
        fprintf(stderr, "error forking the child\n");
        return -1;
    }

    return status;
}

static int user_del(const char* name) {
    pid_t pid, w;
    int status = 0;
    int wstatus;
    char cmd[64];

    snprintf(cmd, 63, "%s", USERDEL);

    pid = fork();

    if(pid > 0) {
        do {
            w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
            if (w == -1)
                return -1;
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
        if WIFEXITED(wstatus)
            return WEXITSTATUS(wstatus);
        else
            return -1;

    // Child

    } else if(pid == 0) {

        execl(cmd, cmd, "-r", name, NULL);
        syslog(LOG_ERR, "exec of %s failed with errno=%d", cmd, errno);
        return -1;

    // Error
    } else {
        fprintf(stderr, "error forking the child\n");
        return -1;
    }

    return status;
}

static int user_mod(const char* name, char* sec_grp) {
    pid_t pid, w;
    int status = 0;
    int wstatus;
    char cmd[64];

    snprintf(cmd, 63, "%s", USERMOD);

    pid = fork();

    if(pid > 0) {
        do {
            w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
            if (w == -1)
                return -1;
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
        if WIFEXITED(wstatus)
            return WEXITSTATUS(wstatus);
        else
            return -1;

    // Child

    } else if(pid == 0) {

        execl(cmd, cmd, "-G", sec_grp, "-c", name, name, NULL);
        syslog(LOG_ERR, "exec of %s failed with errno=%d", cmd, errno);
        return -1;

    // Error
    } else {
        fprintf(stderr, "error forking the child\n");
        return -1;
    }

    return status;
}

int parse_nss_config(RADIUS_NSS_CONF_B * conf, char * prog,
    char * file_buf, int file_buf_sz, int * errnop, int * plockfd) {

      /* Slurp the whole file.
       */
    int ncfd = -1;
    int ret = 0;
    int i = 0, remain;
    struct stat sb;
    char errbuf[128];
    int flags;
    char * scanpos, * line;
    int use_default_rnm = 1, bad_rnm = 0;
    int unconfirmed_disallow = 0;

    memset((char *)conf, 0, sizeof(*conf));
    conf->prog = prog;
    conf->allow_anonymous = 1;
    conf->unconfirmed_ageout = UNCONFIRMED_AGEOUT_DEFAULT;
    conf->unconfirmed_clear_limit = UNCONFIRMED_CLEAR_LIMIT_DEFAULT;

      /* Read the file.
       */
    if (((ncfd = open(RADIUS_NSS_CONF, O_RDONLY)) == -1)
        || (fstat(ncfd, &sb) == -1)
        || (((flags = fcntl(ncfd, F_GETFL, 0)) == -1) && ((flags = 0) != 0))
        || (fcntl(ncfd, F_SETFL, flags | O_NONBLOCK) == -1)) {

        if (errnop)
            *errnop = errno;
        ret = 1;
        errbuf[0] = 0; strerror_r(errno, errbuf, sizeof(errbuf));
        syslog( LOG_WARNING, "%s: %s", prog, errbuf);
        goto parse_nss_config_exit;
    }

      /* The maximum file size is 1 less than the buffer, to allow space for
       * a NULL byte in the case where the last line has no \n, \r, \l char.
       * (which could have been substituted with a NULL).
       */
    if (sb.st_size >= file_buf_sz) {
        syslog( LOG_WARNING, "%s: size greater than %d. Ignoring",
            prog, file_buf_sz - 1);
        goto parse_nss_config_exit;
    }

    if ((i = read(ncfd, file_buf, file_buf_sz)) != sb.st_size) {
        syslog( LOG_WARNING, "%s: read %d of %ld. Ignoring", prog,
            i, sb.st_size);
        goto parse_nss_config_exit;
    }

      /* Parse each line from file.
       */

    scanpos = file_buf;
    remain = sb.st_size;
    while((line = parse_line(conf, file_buf, &scanpos, &remain)) != NULL) {

        if (strncmp(line, "debug=", 6) == 0) {

            if (strncmp(&(line[6]), "on", 2) == 0) {

          /* Handle "debug".
           */

                conf->debug = 1;

            } else if (strncmp(&(line[6]), "trace", 5) == 0) {

                conf->debug = 1;
                conf->trace = 1;

            }

        } else if (strncmp(line, "user_priv=", 10) == 0) {

          /* Handle "user_priv"
           */

            use_default_rnm = 0;
            if (parse_user(conf, line)) {
                bad_rnm = 1;
            }

        } else if (strncmp(line, "many_to_one=", 12) == 0) {

          /* Handle "many_to_one"
           */

            if ((strncmp(&(line[12]), "y", 2) == 0) ||
                (strncmp(&(line[12]), "ye", 3) == 0) ||
                (strncmp(&(line[12]), "yes", 4) == 0)) {

                conf->many_to_one = 1;

            } else if (strncmp(&(line[12]), "a", 1) == 0) {

                conf->many_to_one = 0;
                syslog( LOG_WARNING, "%s: a(nonymous) is same as n(o) option:"
                    " \"%s\"", prog, line);

            } else if ((strncmp(&(line[12]), "n", 2) == 0) ||
                       (strncmp(&(line[12]), "no", 3) == 0)) {

                conf->many_to_one = 0;

            } else {

                syslog( LOG_WARNING, "%s: Ignorning \"%s\"", prog, line);

            }

        } else if (strncmp(line, "unconfirmed_regexp=", 19) == 0) {

            conf->unconfirmed_regexp = &(line[19]);

        } else if (strncmp(line, "unconfirmed_ageout=", 19) == 0) {

            conf->unconfirmed_ageout = atoi(&(line[19]));

        } else if (strncmp(line, "unconfirmed_disallow=", 21) == 0) {

            if ((strncmp(&(line[21]), "y", 2) == 0) ||
                (strncmp(&(line[21]), "ye", 3) == 0) ||
                (strncmp(&(line[21]), "yes", 4) == 0)) {

                unconfirmed_disallow = 1;

            } else if ((strncmp(&(line[21]), "n", 2) == 0) ||
                       (strncmp(&(line[21]), "no", 3) == 0)) {

                unconfirmed_disallow = 0;

            } else {
                syslog( LOG_WARNING, "%s: Ignorning \"%s\"", prog, line);
            }

        } else {

            syslog( LOG_WARNING, "%s: Ignoring \"%s\"", prog, line);
        }
     }

     if (unconfirmed_disallow) {

         conf->allow_anonymous = 0;

     }


parse_nss_config_exit:

    if (ncfd != -1) {

        if (flock(ncfd, LOCK_EX|LOCK_NB) == 0) {
            if (conf->debug)
                syslog( LOG_DEBUG, "%s: %d: Unconfirmed: lock success",
                    prog, (int) getpid());
        } else {
            conf->allow_anonymous = 0;
            if (conf->debug)
                syslog( LOG_DEBUG, "%s: %d: Unconfirmed: locked out",
                    prog, (int) getpid());
        }

        if (plockfd) {
            *plockfd = ncfd;
        } else {
            close(ncfd); /* This should release the lock, if we placed one */
        }
    }


      /* Fix up rnm.
       */

    if (use_default_rnm || bad_rnm)
        init_rnm(conf);

    for ( i = 1; i < RADIUS_MAX_MPL; i++) {
        if ((conf->rnm)[i].gecos == NULL) {
            (conf->rnm)[i] = (conf->rnm)[i-1];
        }
    }

    return ret;
}

/* Releases any memory.
 * Closes any fds.
 */
int unparse_nss_config(RADIUS_NSS_CONF_B * conf, int * errnop, int * plockfd) {

        /* If the caller had a lock in parse_nss_config(),
         * we should now release that lock.
         */
    if (plockfd && (*plockfd != -1)) {
        if (flock(*plockfd, LOCK_UN|LOCK_NB) == 0) {
            if (conf->debug)
                syslog( LOG_DEBUG, "%s: %d: Unconfirmed: unlock success",
                    conf->prog, (int) getpid());
        } else {
            syslog( LOG_ERR, "%s: %d: Unconfirme: unlock fail",
                conf->prog, (int) getpid());
        }
        close(*plockfd);
        *plockfd = -1;
    }
    return 0;
}

static int radius_getpwnam_r_cleanup(int status, FILE * fp) {
    if (fp)
        fclose(fp);
    return status;
}

int radius_getpwnam_r(char * prog, const char * name,
    struct passwd * pwd, char * buf, int buflen, struct passwd ** result) {

    FILE *fp;
    int status;

    if ((fp = fopen(ETC_PASSWD, "r")) == NULL) {
        syslog(LOG_ERR, "%s: fopen(\"/etc/passwd\") failed\n", prog);
        return radius_getpwnam_r_cleanup(STATUS_ENOENT, fp);
    }

    while((status = fgetpwent_r(fp, pwd, buf, buflen, result)) == 0) {
        if (    (*result)
             && (*result)->pw_name
             && (strcmp((*result)->pw_name, name) == 0)) {
            return radius_getpwnam_r_cleanup(status, fp);
        }
    }

    syslog(LOG_INFO, "%s: User %s not found in local db: %d\n", prog, name,
        status);

    return radius_getpwnam_r_cleanup(status, fp);
}


static int radius_update_user_cleanup(int status) {
    return status;
}

int radius_update_user(RADIUS_NSS_CONF_B * conf, const char * user, int mpl) {

    char buf[BUFLEN];
    struct passwd pw, *result = NULL;
    RADIUS_NSS_MPL * rnm = NULL;
    int status;

    /* Verify uid is not in the reserved range (<=1000).
     */

    if ((status = radius_getpwnam_r(conf->prog, user, &pw, buf, sizeof(buf),
           &result)) != 0) {

        /* /bin/login does NSS after PAM.
         */
        status = radius_create_user(conf, user, mpl, RADIUS_CONFIRMED);
        return radius_update_user_cleanup(status);
    }

    if (result->pw_uid <= 1000) {
        syslog(LOG_INFO, "%s: Not changing \"%s\", uid(%d) <= 1000",
            conf->prog, user, result->pw_uid);
        return radius_update_user_cleanup(STATUS_EPERM);
    }

    syslog(LOG_INFO, "%s: Adjusting Supplementary Groups for \"%s\"",
        conf->prog, user);

    rnm = &((conf->rnm)[mpl-1]);

    if (conf->trace)
        dump_rnm(mpl, rnm, "update");

    if(0 != user_mod(user, rnm->groups)) {
      syslog(LOG_ERR, "%s: %s %s failed", conf->prog, USERMOD, user);
        return -1;
    }
    return 0;
}

int radius_create_user(RADIUS_NSS_CONF_B * conf, const char * user, int mpl,
    int unconfirmed) {

    char buf[BUFLEN] = {0};
    RADIUS_NSS_MPL * rnm = &((conf->rnm)[mpl-1]);

    if (conf->trace)
        dump_rnm(mpl, rnm, "create");

    if(strlen(user) > 32) {
        syslog(LOG_ERR, "%s: Username too long", conf->prog);
        return -1;
    }

    syslog(LOG_INFO, "%s: Creating user \"%s\"", conf->prog, user);

    char sgid[10] = {0};
    char home[64] = {0};
    snprintf(sgid, 10, "%d", rnm->gid);
    snprintf(home, 63, "/home/%s", user);

    snprintf(buf, sizeof(buf), "Unconfirmed-%ld", time(NULL));

    if(0 != user_add(user, sgid, rnm->groups, rnm->gecos, home, rnm->shell, unconfirmed ? buf : user, conf->many_to_one)) {
      syslog(LOG_ERR, "%s: %s %s failed", conf->prog, USERADD, user);

        return -1;
    }
    return 0;
}

int radius_delete_user(RADIUS_NSS_CONF_B * conf, const char * user) {

    syslog(LOG_INFO, "%s: Deleting user \"%s\"", conf->prog, user);
    if(0 != user_del(user)) {
      syslog(LOG_ERR, "%s: %s %s failed", conf->prog, USERDEL, user);

        return -1;
    }
    return 0;
}

int radius_clear_unconfirmed_users_cleanup(int status, FILE * fp) {
    if (fp)
        fclose(fp);
    return status;
}

int radius_clear_unconfirmed_users(RADIUS_NSS_CONF_B * conf)
{
    FILE *fp;
    int status;
    time_t ts, curr = time(NULL);
    struct passwd pw, * pwd = & pw, * result = NULL;
    char buf[BUFLEN];

    if ((fp = fopen(ETC_PASSWD, "r")) == NULL) {
        syslog(LOG_ERR, "%s: fopen(\"/etc/passwd\") failed\n", conf->prog);
        return radius_clear_unconfirmed_users_cleanup(STATUS_ENOENT, fp);
    }

    while((status = fgetpwent_r(fp, pwd, buf, sizeof(buf), &result)) == 0) {
        if (   (result)
            && (strncmp((result)->pw_gecos, "Unconfirmed-", 12) == 0)
            && (ts = atoi(&(((result)->pw_gecos)[12])))
            && ((curr - ts) >= conf->unconfirmed_ageout)) {

            syslog(LOG_INFO, "%s: Deleting unconfirmed user \"%s\"",
                conf->prog, (result)->pw_name);

            return radius_clear_unconfirmed_users_cleanup(
                       radius_delete_user(conf, (result)->pw_name), fp);
        }
    }

    return radius_clear_unconfirmed_users_cleanup(STATUS_ESRCH, fp);
}


int radius_lookup_cache_cleanup( int status, int rafd) {
    if (rafd != -1)
        close(rafd);
    return status;
}

int radius_lookup_cache( char * prog, const char * nam, int * pmpl) {
    int rafd = -1;
    int i;
    char cache_filename[PATH_MAX];
    char file_buf[10];
    struct stat sb;
    int flags;
    int written;
    int mpl;

    *pmpl = RADIUS_MIN_MPL;

    if ((written = snprintf(cache_filename, sizeof(cache_filename), "%s/%s/%s",
        RADIUS_ATTRIBUTE_CACHE_DIR, nam, RADIUS_ATTR_MPL)
          >= sizeof(cache_filename))) {
        syslog( LOG_ERR, "%s: \"%s\": too long.", prog, cache_filename);
        return radius_lookup_cache_cleanup(STATUS_E2BIG, rafd);
    }

      /* Ensure the user's cache exists
       */
    if (((rafd = open(cache_filename, O_RDONLY)) == -1)
        || (fstat(rafd, &sb) == -1)
        || (((flags = fcntl(rafd, F_GETFL, 0)) == -1) && ((flags = 0) != 0))
        || (fcntl(rafd, F_SETFL, flags | O_NONBLOCK) == -1)) {
        syslog( LOG_INFO, "%s: \"%s\": Absent.", prog, cache_filename);
        return radius_lookup_cache_cleanup(STATUS_ENOENT, rafd);
    }

      /* Read the privilege attribute file.
       */

    if (sb.st_size >= sizeof(file_buf)) {
        syslog( LOG_WARNING, "%s: size %ld greater than %ld. Ignoring",
            cache_filename, sb.st_size, sizeof(file_buf)-1);
        return radius_lookup_cache_cleanup(STATUS_EFBIG, rafd);
    }

    if ((i = read(rafd, file_buf, sizeof(file_buf))) != sb.st_size) {
        syslog( LOG_WARNING, "%s: read %d of %ld. Ignoring", prog,
            i, sb.st_size);
        return radius_lookup_cache_cleanup(STATUS_EIO, rafd);
    }

    file_buf[sb.st_size] = 0;
    mpl = atoi(file_buf);

    if (((RADIUS_MIN_MPL <= mpl) && (mpl <= RADIUS_MAX_MPL)))
        *pmpl = mpl;

    return radius_lookup_cache_cleanup(0, rafd);
}

int radius_copy_pw( RADIUS_NSS_CONF_B * conf, struct passwd * res,
    const char * nam, struct passwd * pwd,
    char * buffer, size_t buflen, int * errnop) {

    int ret = 0;
    size_t namlen, gecoslen, dirlen, shelllen;

    size_t totallen = (namlen = strlen(res->pw_name)) + 1
                 + (gecoslen = strlen(res->pw_gecos)) + 1
                 + (dirlen = strlen(res->pw_dir)) + 1
                 + (shelllen = strlen(res->pw_shell)) + 1
                 + 1 + 1;

    if (totallen > buflen) {
        if (errnop)
            *errnop = ERANGE;
        ret = 1;
        if (conf->debug)
            syslog(LOG_INFO, "(%s->%s): %s: getpwnam_r ERANGE %ld < %ld\n",
                conf->prog, nam, res->pw_name, buflen, totallen);
        return ret;
    }

    memcpy( buffer, res->pw_name, namlen + 1);
    pwd->pw_name = buffer;
    buffer += (namlen + 1);

    memcpy( buffer, "*", 1 + 1);
    pwd->pw_passwd = buffer;
    buffer += (1 + 1);

    pwd->pw_uid = res->pw_uid;

    pwd->pw_gid = res->pw_gid;

    memcpy( buffer, res->pw_gecos, gecoslen + 1);
    pwd->pw_gecos = buffer;
    buffer += (gecoslen + 1);

    memcpy( buffer, res->pw_dir, dirlen + 1);
    pwd->pw_dir = buffer;
    buffer += (dirlen + 1);

    memcpy( buffer, res->pw_shell, shelllen + 1);
    pwd->pw_shell = buffer;
    buffer += (shelllen + 1);

    return ret;
}


static int is_sshd_lookup_exit(int status, int fd, regex_t * re) {
    if (fd != -1)
        close(fd);

    if (re)
        regfree(re);

    return status;
}


int is_sshd_lookup(RADIUS_NSS_CONF_B * conf, const char * name) {
    pid_t pid = getpid();
    int fd, i;
    regex_t regex, * re = NULL;
    int reg_ret;


#define PROC_FILENAME_LEN 128
#define CMDLINE_SZ        128

    char proc_file[PROC_FILENAME_LEN];
    char cmdline[CMDLINE_SZ], * colon;
    char expected[CMDLINE_SZ];
    char accepted[CMDLINE_SZ];
    char errbuf[CMDLINE_SZ];

    snprintf(proc_file, sizeof(proc_file), "/proc/%ld/cmdline", (long) pid);
    snprintf(expected, sizeof(expected), ": %s [priv]", name);
    snprintf(accepted, sizeof(accepted), ": [accepted]");

    if ((fd = open(proc_file, O_RDONLY)) == -1) {

        syslog( LOG_WARNING, "%s: open(%s) failed: errno %d",
            conf->prog, proc_file, errno);
        return is_sshd_lookup_exit(0, fd, re);
    }

    if ((i = read(fd, cmdline, sizeof(cmdline)-1)) <= 0) {
        syslog( LOG_WARNING, "%s: %s: read(%d) failed: errno %d. Ignoring",
            conf->prog, proc_file, fd, errno);
        return is_sshd_lookup_exit(0, fd, re);
    }

    if (conf->unconfirmed_regexp) {

        if ((reg_ret = regcomp(&regex, conf->unconfirmed_regexp,
                REG_EXTENDED|REG_NOSUB))) {

            errbuf[0] = 0;
            regerror(reg_ret, &regex, errbuf, sizeof(errbuf));
            syslog( LOG_ERR, "%s: %s: regcomp() failed: %s", conf->prog,
                conf->unconfirmed_regexp, errbuf);

        } else {

            re = &regex;

        }
    }

    if (re) {

        if (!(reg_ret = regexec(re, cmdline, 0, NULL, 0))) {
            syslog( LOG_INFO, "%s: %s: Lookup %s", conf->prog, cmdline, name);
            return is_sshd_lookup_exit(1, fd, re);
        }

    } else {

        cmdline[i] = 0;
        colon = strchr(cmdline, ':');

        if (colon && ((0 == strncmp(expected, colon, sizeof(expected) - 1)) ||
                (0 == strncmp(accepted, colon, sizeof(accepted) - 1)))) {
            syslog( LOG_INFO, "%s: %s: Lookup %s", conf->prog, cmdline, name);
            return is_sshd_lookup_exit(1, fd, re);
        }
    }

    if (conf->debug)
        syslog( LOG_DEBUG, "%s: %s: Non sshd Lookup %s",
            conf->prog, cmdline, name);

    return is_sshd_lookup_exit(0, fd, re);
}

