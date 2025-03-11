/**
 * System-Assigned Credentials (SAC)
 *
 * This NSS module is only needed when users are logged into the system
 * with an authentication protocol such as RADIUS or TACACS+. These
 * protocols were not designed for UNIX-like systems and do not provide
 * a standard way of defining UNIX credentials (UID and GID). In
 * particular, RADIUS doesn't separate the authorization function from
 * authentication, which makes it impossible to retrieve user credentials
 * separate from the authentication process. In other words, in order to
 * get the UID associated with a given user (i.e. getpwnam) one needs
 * to provide the password for that user. This simply doesn't work well
 * with Linux. In fact, sshd calls getpwnam() before it even tries to
 * authenticate the user and if this API cannot find the credentials the
 * login will fail.
 *
 * This NSS module solves this problem by automatically allocating
 * credentials (UID and primary GID) to users that are not found by any
 * other NSS methods. We call these credentials "System-Assigned
 * Credentials" or SAC for short. These credentials are only allocated
 * temporarily and won't become permanent until the user has passed
 * authentication. If the user fails to authenticate, then the temporary
 * credentials get deleted. Once the user has successfully authenticated,
 * the credentials become permanent in the /etc/passwd file.
 *
 * There are 3 components to SAC. The SAC NSS module, the Host Account
 * Management Daemon (hamd), and the RADIUS and/or TACACS+ PAM modules.
 *
 * System overview:
 *
 * The following is a highly simplified representation of what happens
 * when a user logs into the system over SSH.
 *
 * ssh   sshd           NSS-SAC     hamd            PAM      RADIUS/TACACS+
 *  |     .                .          .              .           Server
 *  |                      .          .              .              .
 *  +---->+                .          .              .              .
 *        |                           .              .              .
 *  . [1] +--getpwnam()--->+          .              .              .
 *  .                      |                         .              .
 *  .     .                +---DBus-->+              .              .
 *  .     .                .          |              .              .
 *  .     .                .       useradd() [2]     .              .
 *  .     .                .      temporarily        .              .
 *  .     .                .    to /etc/passwd       .              .
 *  .     .                           |              .              .
 *  .     .                +<--DBus---+              .              .
 *  .                      |          .              .              .
 *  . [3] +<--(uid, gid)---+          .              .              .
 *  .     |                                                         .
 *  .     +-pam_autheticate()----------------------->+ [4]          .
 *  .     .                                          |
 *  .     .                           .              +---auth------>+
 *  .     .                           .                             |
 *  .     .                           .              +<--pass/fail--+ [5]
 *  .     .                               DBus:      |              .
 *  .     .                           +<--pass/fail--+              .
 *  .     .                           |              .              .
 *  .     .                [6] persist/remove        .              .
 *  .     .                   from /etc/passwd       .
 *  .     .                           |
 *  .     .                           +--DBus:done-->+
 *  .                                                |
 *  .     +<---------------success/fail--------------+ [7]
 *  .     |
 *  . [8] +----exec(shell) / exit()
 *  .
 *
 *  [1] When an ssh session is initiated, one of the first things that sshd
 *      does is to get the credentials (uid/gid) for the user that is
 *      trying to log in. This is done by calling getpwnam(username), which
 *      invokes the NSS modules configured in /etc/nsswitch.conf in the
 *      order they appear in the file. libnss_sac should be configured as
 *      the last method in /etc/nsswitch.conf. When all other NSS methods
 *      have failed to find credentials for the user, libnss_sac gets
 *      invoked. The NSS SAC module then invokes hamd over DBus to
 *      temporarily allocate credentials to the user.
 *
 *  [2] The daemon hamd creates temporary credentials for the user. We use
 *      the GECOS filed to mark the credentials as "unconfirmed", which
 *      simply means that the user hasn't been authenticated yet. The GECOS
 *      will also have the PID of the process that initiated the session.
 *      The PID can then be used later to identify "unconfirmed"
 *      credentials that are no longer valid. That is, if hamd finds an
 *      entry marked as "unconfirmed", but the PID associted to that entry
 *      no longer exists, then hamd will conclude that this user was never
 *      confirmed and can safely be removed from the /etc/passwd file.
 *
 *  [3] After the temporary credentials are returned to sshd, sshd can
 *      start the authentication phase using pam_authenticate().
 *
 *  [4] pam_authenticate() triggers the PAM layer library to query the
 *      user's password and verify it against the different PAM modules
 *      configured in /etc/pam.d/[application]. In this example
 *      [application] would be the /etc/pam.d/sshd. In that file there may
 *      be several authentication methods that will be tried including
 *      local (i.e. pam_unix), RADIUS, and/or TACACS+. For the sake of this
 *      example we will refer to RADIUS and/or TACACS+ servers as a AAA
 *      server. At this point the PAM module will contact the AAA server
 *      to authenticate the user.
 *
 *  [5] The AAA server verifies the user provided password with what it has
 *      in its DB. Depending on whether the password matches the AAA server
 *      returns pass or fail to the PAM module.
 *
 *  [6] If authentication is successful, the PAM module contacts hamd to
 *      confirm the user and make the credentials permanent in /etc/passwd.
 *      This simply means that the "unconfirmed" marker in the GECOS field
 *      can be cleared. If authentication has failed, the PAM module will
 *      contact hamd to tell it to remove the user credentials from
 *      /etc/passwd.
 *
 *  [7] The PAM module finally returns success/fail to sshd
 *
 *  [8] Upon successful authentication sshd will start the shell configured
 *      in the passwd struct returned by getpwnam() earlier. This has been
 *      set by hamd. By default, hamd will have set it to start a kish
 *      shell, but hamd is configurable so a different could be configured
 *      if need be (see hamd code for details).
 *
 * Restricting applications that are allowed to invoke SAC
 * =======================================================
 * For additional security, only certain applications will be allowed to
 * automatically create credentials. Those are applications that are
 * typically used for login such as sshd, login, su, etc. This is
 * configurable through file /etc/sonic/hamd/libnss_sac.conf (see macro
 * SAC_CONFIG_FILE below).
 *
 * This code is compiled as libnss_sac.so.2. In order to use it one must
 * use the keyword "sac" in the NSS configuration file (i.e.
 * /etc/nsswitch.conf) as follows:
 *
 * /etc/nsswitch.conf
 * ==================
 *
 *     passwd:   compat sac    <- To support getpwnam()
 */

#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif
#include <nss.h>                     /* NSS_STATUS_SUCCESS, NSS_STATUS_NOTFOUND */
#include <syslog.h>                  /* syslog() */
#include <stdio.h>                   /* fopen(), fclose() */
#include <errno.h>                   /* errno, program_invocation_name */
#include <pwd.h>                     /* uid_t, struct passwd */
#include <sys/stat.h>                /* stat() */
#include <sys/types.h>               /* getpid() */
#include <unistd.h>                  /* getpid(), access() */
#include <string.h>                  /* strdup() */
#include <fcntl.h>                   /* open(), O_RDONLY */
#include <limits.h>                  /* LINE_MAX */

#include <string>                    /* std::string */
#include <vector>                    /* std::vector */
#include <algorithm>                 /* std::find() */

#include "prototypes.h"              /* _nss_sac_getpwnam_r() */
#include "sac_proxy.h"               /* sac_proxy_c */
#include "../shared/utils.h"         /* strneq(), startswith(), cpy2buf(), join() */

#define SAC_CONFIG_FILE  "/etc/sonic/hamd/libnss_sac.conf"
#define SAC_ENABLE_FILE  "/etc/sonic/hamd/libnss_sac.enable" // Presence of this file enables SAC

//#define WITH_PYTHON

static FILE                      * log_p   = nullptr;
static bool                        verbose = false;
static char                      * cmdline_p = program_invocation_name;
static std::vector<std::string>    default_programs = { "sshd", "login", "su" };
static std::vector<std::string>    programs(default_programs);
#ifdef WITH_PYTHON
static std::vector<std::string>    default_pyscripts;
static std::vector<std::string>    pyscripts(default_pyscripts);
#endif


#define SYSLOG(LEVEL, args...) \
do \
{ \
    if (verbose) \
    { \
        if (log_p != nullptr) fprintf(log_p, args); \
        else                  syslog(LEVEL, args);  \
    } \
} while (0)


static bool sac_enabled()
{
    // The presence of the file SAC_ENABLE_FILE
    // indicates whether SAC is enabled.
    return access(SAC_ENABLE_FILE, F_OK) != -1;
}

static char * kv_strip(char * p)
{
    #define  WHITESPACE " \t\n\r"

    p[strcspn(p, "\n\r")] = '\0';

    p += strspn(p, WHITESPACE);            // Remove leading newline and spaces
    if (*p == '#' || *p == '\0')           // Skip comments and empty lines
    {
        *p = '\0';
        return p;
    }
    p[strcspn(p, "\n\r")] = '\0';          // Remove trailing newline chars

    // Delete trailing comments (including spaces/tabs that precede the #)
    char * s = &p[strcspn(p, "#")];
    *s-- = '\0';
    while ((s >= p) && ((*s == ' ') || (*s == '\t')))
    {
        *s-- = '\0';
    }

    return p;
}

static char * kv_keymatch(const char * s, const char * key)
{
    char * value = startswith(s, key);
    if (NULL != value)
    {
        switch (*value)
        {
        case ' ':  // Make sure
        case '\t': // key is a whole word.
        case '=':  // I.e. it should be followed by spaces, tabs, or a equal sign.
            value += strspn(value, " \t="); // Skip leading spaces, tabs, and equal sign (=)
            return value;
        default:;
        }
    }

    return NULL;
}

/**
 * @brief Extract cmdline from /proc/self/cmdline. This is only needed for
 *        debugging purposes
 */
static void read_cmdline()
{
    const char *fn = "/proc/self/cmdline";
    struct stat  st;
    if (stat(fn, &st) != 0) return;

    int fd = open(fn, O_RDONLY);
    if (fd == -1) return;

    char      buffer[LINE_MAX] = {0};
    size_t    sz = sizeof(buffer);
    size_t    n  = 0;
    for (;;)
    {
        ssize_t r = read(fd, &buffer[n], sz - n);
        if (r == -1)
        {
            if (errno == EINTR) continue;
            break;
        }
        n += r;
        if (n == sz) break; // filled the buffer
        if (r == 0)  break; // EOF
    }
    close(fd);

    if (n)
    {
        if (n == sz) n--;
        buffer[n] = '\0';
        size_t i = n;
        while (i--)
        {
            int c = buffer[i];
            if ((c < ' ') || (c > '~')) buffer[i] = ' ';
        }

        // Delete trailing spaces, tabs, and newline chars.
        char *p = &buffer[strcspn(buffer, "\n\r")];
        while ((p >= buffer) && (*p == ' ' || *p == '\t' || *p == '\0'))
        {
            *p-- = '\0';
        }
    }

    cmdline_p = strdup(buffer);
}

/**
 * @brief Get configuration parameters for this module. Parameters are
 *        retrieved from the file #SAC_CONFIG_FILE
 */
static void read_config()
{
    FILE *file = fopen(SAC_CONFIG_FILE, "re");
    if (file)
    {
        char    line[LINE_MAX];
        char  * p;
        char  * s;

        std::vector<std::string> new_programs;
#ifdef WITH_PYTHON
        std::vector<std::string> new_pyscripts;
#endif

        while (NULL != (p = fgets(line, sizeof line, file)))
        {
            // Clean up string by removing leading/trailing blanks and new line
            // characters. Also eliminate trailing comments, if any.
            p = kv_strip(p);
            if (*p == '\0') continue; // Check that there is still something left in the string

            if (NULL != (s = kv_keymatch(p, "debug")))
            {
                if (strneq(s, "yes", 3)) verbose = true;
            }
            else if (NULL != (s = kv_keymatch(p, "log")))
            {
                if (*s != '\0')
                {
                    if (log_p != nullptr)
                    {
                        if ((log_p != stderr) && (log_p != stdout))
                        {
                            fclose(log_p);
                        }
                        log_p = nullptr;
                    }
                    log_p = fopen(s, "w");
                }
            }
            if (NULL != (s = kv_keymatch(p, "programs")))
            {
                // 'programs' is a list of comma-separated program names.
                // So we need to split the list into its components
                std::vector<std::string> prog_names = split_any(s, ", \t");
                for (auto &prog : prog_names) new_programs.push_back(trim(prog));

            }
#ifdef WITH_PYTHON
            if (NULL != (s = kv_keymatch(p, "python_scripts")))
            {
                // 'python_scripts' is a list of comman-separated python script names.
                // So we need to split the list into its components
                std::vector<std::string> scripts = split_any(s, ", \t");
                for (auto & script : scripts)
                new_pyscripts.push_back(trim(script));
            }
#endif
        }

        fclose(file);

        programs  = new_programs.empty() ? default_programs  : new_programs;
#ifdef WITH_PYTHON
        pyscripts = new_pyscripts.empty() ? default_pyscripts : new_pyscripts;
#endif

        if (verbose)
        {
            SYSLOG(LOG_DEBUG, "NSS sac: verbose       = true\n");
            SYSLOG(LOG_DEBUG, "NSS sac: programs      = [%s]\n", join(programs.cbegin(), programs.cend()).c_str());
#ifdef WITH_PYTHON
            SYSLOG(LOG_DEBUG, "NSS sac: pyscripts     = [%s]\n", join(pyscripts.cbegin(), pyscripts.cend()).c_str());
#endif
        }
    }

    if (verbose)
    {
        read_cmdline();
        SYSLOG(LOG_DEBUG, "NSS sac: cmdline       = %s\n", cmdline_p);
        SYSLOG(LOG_DEBUG, "NSS sac: sac_enabled() = %s\n", true_false(sac_enabled()));
        SYSLOG(LOG_DEBUG, "NSS sac: program       = %s\n", program_invocation_short_name);
        SYSLOG(LOG_DEBUG, "NSS sac: config file   = %s\n", SAC_CONFIG_FILE);
    }
}

static DBus::Connection  & get_dbusconn()
{
    static DBus::Connection  * conn_p = nullptr;
    if (conn_p == nullptr)
    {
        // DBus::BusDispatcher is a "main loop" construct that
        // handles (i.e. dispatched) DBus messages. This should
        // be defined as a singleton to avoid memory leaks.
        static DBus::BusDispatcher dispatcher;

        // DBus::default_dispatcher must be initialized before DBus::Connection.
        DBus::default_dispatcher = &dispatcher;

        static DBus::Connection conn = DBus::Connection::SystemBus();

        conn_p = &conn;
    }

    return *conn_p;
}

/**
 * @brief This NSS module is only needed when we want to login users with a
 *        AAA method such as RADIUS or TACACS+. This means that only the
 *        programs used to log into the system should be allowed to
 *        proceed. Depending on the method used to log into the system we
 *        can expect one of the following program names:
 *
 *           Login method      Program name
 *           ================  ============
 *           ssh               sshd
 *           telnet, console   login
 *           bash's su         su
 *
 * @return bool: true if the calling program is one of the few programs
 *         used to let AAA users log into the system. false otherwise.
 */
static bool is_program_allowed_to_alloc_creds()
{
#ifdef WITH_PYTHON
    // Look for python scripts that are allowed to invoke SAC
    if (strneq(program_invocation_short_name, "python", strlen("python")))
    {
        for (auto & pyscript : pyscripts)
        {
            if (NULL != strstr(cmdline, pyscript.c_str()))
            return true;
        }

        return false;
    }
#endif

    // Check if program_invocation_short_name is in programs list.
    return std::find(programs.cbegin(), programs.cend(), program_invocation_short_name) != programs.cend();
}

/**
 * @brief Scan file (@fname) looking for user. If found, return a pointer
 *        to a "struct passwd" containing all the data related to user.
 *
 * @param fname E.g. /etc/passwd
 * @param user The user we're looking for
 *
 * @return If user found, return a pointer to a struct passwd.
 */
static struct passwd* fgetpwname(const char *fname, const char *user)
{
    struct passwd *pwd = NULL;
    FILE          *f   = fopen(fname, "re");
    if (f)
    {
        struct passwd *ent;
        while (NULL != (ent = fgetpwent(f)))
        {
            if (streq(ent->pw_name, user))
            {
                pwd = ent;
                break;
            }
        }
        fclose(f);
    }

    return pwd;
}


/**
 * @brief Fill structure pointed by result with the data contained in the
 *        structure pointed to by pwd.
 *
 * @param pwd     Pointer to the source of the data to transfer to result
 * @param name    User name
 * @param result  Pointer to destination where data get copied
 * @param buffer  Pointer to a chunk of memory where strings can be
 *                allocated from
 * @param buflen  Size of buffer
 * @param errnop  Pointer to where errno code can be written
 *
 * @return If there is not enough memory in buffer to copy the strings
 *         return NSS_STATUS_TRYAGAIN. Otherwise return NSS_STATUS_SUCCESS.
 */
static nss_status fill_result(struct passwd * pwd,
                              const char    * name,
                              struct passwd * result,
                              char          * buffer,
                              size_t          buflen,
                              int           * errnop)
{
    size_t name_l   = strlen(name) + 1; /* +1 to include NUL terminating char */
    size_t dir_l    = strlen(pwd->pw_dir) + 1;
    size_t shell_l  = strlen(pwd->pw_shell) + 1;
    size_t passwd_l = strlen(pwd->pw_passwd) + 1;
    if (buflen < (name_l + shell_l + dir_l + passwd_l))
    {
        if (errnop) *errnop = ENOMEM;
        return NSS_STATUS_TRYAGAIN;
    }

    result->pw_uid    = pwd->pw_uid;
    result->pw_gid    = pwd->pw_gid;
    result->pw_name   = buffer;
    result->pw_dir    = cpy2buf(result->pw_name,   name,           name_l);
    result->pw_shell  = cpy2buf(result->pw_dir,    pwd->pw_dir,    dir_l);
    result->pw_passwd = cpy2buf(result->pw_shell,  pwd->pw_shell,  shell_l);
    result->pw_gecos  = cpy2buf(result->pw_passwd, pwd->pw_passwd, passwd_l);

    // Check if there is enough room left in buffer for GECOS.
    // If not just, just assign "AAA user" as default.
    size_t gecos_l = strlen(pwd->pw_gecos) + 1;
    if ((size_t)((buffer + buflen) - result->pw_gecos) >= gecos_l) cpy2buf(result->pw_gecos, pwd->pw_gecos, gecos_l);
    else result->pw_gecos = (char *)"AAA user";

    if (errnop) *errnop = 0;

    return NSS_STATUS_SUCCESS;
}


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Automatically create user credentials for AAA authenticated
 *        users.
 *
 * @param name      User name.
 * @param result    Where to write the result
 * @param buffer    Buffer used as a temporary pool where we can save
 *                  strings.
 * @param buflen    Size of memory pointed to by buffer
 * @param errnop    Where to return the errno
 *
 * @return NSS_STATUS_SUCCESS on success, NSS_STATUS_NOTFOUND otherwise.
 */
enum nss_status _nss_sac_getpwnam_r(const char    * name,
                                    struct passwd * result,
                                    char          * buffer,
                                    size_t          buflen,
                                    int           * errnop)
{
    if (!sac_enabled()) return NSS_STATUS_NOTFOUND;

    // Just to be sure, let's check if user is already in /etc/passwd
    struct passwd *pwd = fgetpwname("/etc/passwd", name);
    if (pwd)
    {
        if (verbose) SYSLOG(LOG_DEBUG, "NSS sac: _nss_sac_getpwnam_r() - User \"%s\": user found in /etc/passwd\n", name);
        return fill_result(pwd, name, result, buffer, buflen, errnop);
    }

    // Only allow certain programs to automatically allocated credentials.
    // The list of programs is configurable through /etc/

    bool program_allowed = is_program_allowed_to_alloc_creds();
    if (verbose) SYSLOG(LOG_DEBUG, "NSS sac: _nss_sac_getpwnam_r() - User \"%s\": invoked from \"%s\" with creds UID=%u GID=%u. program_allowed=%s\n",
                        name, cmdline_p, getuid(), getgid(), program_allowed ? "yes" : "no");

    /* We should only let login programs continue. */
    if (!program_allowed) return NSS_STATUS_NOTFOUND;

    try
    {
        // Create the DBus interface
        sac_proxy_c  sac(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);

        std::string  errmsg = sac.add_unconfirmed_user(name, getpid());
        bool ok = errmsg.empty();
        if (ok)
        {
            pwd = fgetpwname("/etc/passwd", name);
            if (pwd)
            {
                return fill_result(pwd, name, result, buffer, buflen, errnop);
            }
        }

        if (verbose) SYSLOG(LOG_DEBUG, "NSS sac: _nss_sac_getpwnam_r() - User \"%s\": Exiting with Try Again due to: %s, pwd=%p\n",
                            name, errmsg.c_str(), pwd);
    }
    catch (DBus::Error &ex)
    {
        SYSLOG(LOG_ERR, "NSS sac: _nss_sac_getpwnam_r() - User \"%s\": Exiting with Try Again due to: %s\n",
               name, ex.what());
    }

    *errnop = EBUSY;
    return NSS_STATUS_TRYAGAIN;
}

/**
 * @brief Initalize module singletons on entry.
 *
 *        cmdline_p contains the command line of the program that invoked
 *        the NSS module. Used for debug purposes only.
 */
void __attribute__((constructor)) __module_enter(void)
{
    read_config();
}

/**
 * @brief Module clean up on exit
 */
void __attribute__((destructor)) __module_exit(void)
{
    if ((cmdline_p != nullptr) && (cmdline_p != program_invocation_name))
    {
        free(cmdline_p);
        cmdline_p = nullptr;
    }

    if ((log_p != nullptr) && (log_p != stderr) && (log_p != stdout))
    {
        fclose(log_p);
        log_p = nullptr;
    }
}

#ifdef __cplusplus
}
#endif
