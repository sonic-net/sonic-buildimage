/**
 * This is an NSS module. It allows applications running in containers to
 * access the user account information on the host. A service on the host
 * called "hamd" for Host Account Management Daemon is contacted over DBus
 * to retrieve user account information.
 *
 * This code is compiled as libnss_ham.so.2. In order to use it one must
 * use the keyword "ham" in the NSS configuration file (i.e.
 * /etc/nsswitch.conf) as follows:
 *
 * /etc/nsswitch.conf
 * ==================
 *
 *     passwd: compat ham  <- To support getpwnam(), getpwuid(), getpwent()
 *     group:  compat ham  <- To support getgrnam(), getgrgid(), getgrent()
 *     shadow: compat ham  <- To support getspnam()

 * The Naming Scheme of the NSS Modules
 * ====================================
 *
 * The name of each function consists of various parts:
 *
 *     _nss_[service]_[function]
 *
 * [service] of course corresponds to the name of the module this function
 * is found in. The [function] part is derived from the interface function
 * in the C library itself. If the user calls the function gethostbyname()
 * and the [service] used is "files" the function
 *
 *     _nss_files_gethostbyname_r
 *
 * in the module
 *
 *     libnss_files.so.2
 *
 * is used. You see, what is explained above in not the whole truth. In
 * fact the NSS modules only contain reentrant versions of the lookup
 * functions. I.e., if the user would call the gethostbyname_r function
 * this also would end in the above function. For all user interface
 * functions the C library maps this call to a call to the reentrant
 * function. For reentrant functions this is trivial since the interface is
 * (nearly) the same. For the non-reentrant version the library keeps
 * internal buffers which are used to replace the user supplied
 * buffer.
 *
 * I.e., the reentrant functions can have counterparts. No service module
 * is forced to have functions for all databases and all kinds to access
 * them. If a function is not available it is simply treated as if the
 * function would return unavail (see Actions in the NSS configuration).
 *
 * Error codes
 * ===========
 *
 * In case the interface function has to return an error it is important
 * that the correct error code is stored in *errnop. Some return status
 * values have only one associated error code, others have more.
 *
 * enum nss_status      errno   Description
 * ===================  ======= ==========================================
 * NSS_STATUS_TRYAGAIN  EAGAIN  One of the functions used ran temporarily
 *                              out of resources or a service is
 *                              currently not available.
 *
 *                      ERANGE  The provided buffer is not large enough.
 *                              The function should be called again with a
 *                              larger buffer.
 *
 * NSS_STATUS_UNAVAIL   ENOENT  A necessary input file cannot be found.
 *
 * NSS_STATUS_NOTFOUND  ENOENT  The requested entry is not available.
 *
 * NSS_STATUS_NOTFOUND  SUCCESS There are no entries. Use this to avoid
 *                              returning errors for inactive services
 *                              which may be enabled at a later time. This
 *                              is not the same as the service being
 *                              temporarily unavailable.
 */

#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif
#include <nss.h>                             /* NSS_STATUS_SUCCESS, NSS_STATUS_NOTFOUND */
#include <pwd.h>                             /* uid_t, struct passwd */
#include <grp.h>                             /* gid_t, struct group */
#include <shadow.h>                          /* struct spwd */
#include <sys/stat.h>                        /* stat() */
#include <sys/types.h>                       /* getuid(), getgid() */
#include <unistd.h>                          /* getuid(), getgid() */
#include <errno.h>                           /* errno, program_invocation_name */
#include <string.h>                          /* strdup() */
#include <fcntl.h>                           /* open(), O_RDONLY */
#include <stdio.h>                           /* fopen(), fclose() */
#include <limits.h>                          /* LINE_MAX */
#include <stdint.h>                          /* uint32_t */
#include <syslog.h>                          /* syslog() */
#include <sys/mman.h>                        /* memfd_create() */

#include <string>                            /* std::string */
#include <vector>                            /* std::vector */
#include <numeric>                           /* std::accumulate() */
#include <mutex>                             /* std::mutex, std::lock_guard */

#include "../shared/utils.h"                 /* strneq(), startswith(), cpy2buf(), join() */

#include "../shared/missing-memfd_create.h"  /* memfd_create() if missing from sys/mman.h */
#include "name_service_proxy.h"              /* name_service_proxy_c */
#include "SYSLOG.h"                          /* SYSLOG(), SYSLOG_CONDITIONAL */


//##############################################################################
// Local configuration parameters
static FILE * log_p     = nullptr;                 // Optional log file where log messages can be saved. The default is to log to syslog.
static bool   verbose   = false;                   // For debug only.
static char * cmdline_p = program_invocation_name; // For debug only.

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
 *        debugging purposes (i.e. when verbose==true)
 */
static void read_cmdline()
{
    cmdline_p = program_invocation_name;

    const char * fn = "/proc/self/cmdline";
    struct stat  st;
    if (stat(fn, &st) != 0)
        return;

    int fd = open(fn, O_RDONLY);
    if (fd == -1)
        return;

    char      buffer[LINE_MAX] = {0};
    size_t    sz = sizeof(buffer);
    size_t    n  = 0;
    for(;;)
    {
        ssize_t r = read(fd, &buffer[n], sz-n);
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
        char * p = &buffer[strcspn(buffer, "\n\r") - 1];
        while ((p >= buffer) && (*p == ' ' || *p == '\t' || *p == '\0'))
        {
            *p-- = '\0';
        }
    }

    cmdline_p = strdup(buffer);

    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "cmdline   = %s\n", cmdline_p);
}

/**
 * @brief Get configuration parameters for this module. Parameters are
 *        retrieved from the file "etc/sonic/hamd/libnss_ham.conf"
 */
static void read_config()
{
    bool                debug = false;
    std::string         log_file;
    static std::string  current_log_file;

    FILE * file = fopen("/etc/sonic/hamd/libnss_ham.conf", "re");
    if (file)
    {
        #define   WHITESPACE " \t\n\r"
        char      line[LINE_MAX];
        char    * p;
        char    * s;

        while (NULL != (p = fgets(line, sizeof line, file)))
        {
            // Clean up string by removing leading/trailing blanks and new line
            // characters. Also eliminate trailing comments, if any.
            p = kv_strip(p);
            if (*p == '\0') continue; // Check that there is still something left in the string

            if (NULL != (s = kv_keymatch(p, "debug")))
            {
                if (strneq(s, "yes", 3))
                    debug = true;
            }
            else if (NULL != (s = kv_keymatch(p, "log")))
            {
                if (*s != '\0')
                    log_file = s;
            }
        }

        fclose(file);
    }

    if (log_file != current_log_file)
    {
        if (log_p != nullptr)
        {
            if ((log_p != stderr) && (log_p != stdout))
                fclose(log_p);

            log_p = nullptr;
        }

        if (!log_file.empty())
        {
            log_p = fopen(log_file.c_str(), "w");
            if (log_p != nullptr)
                current_log_file = log_file;
        }
    }

    verbose = debug;

    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "debug     = true\n");
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "log       = %s\n", log_file.c_str());
}

/**
 * @brief Retrieve current system monotonic clock as a 64-bit count in
 *        nano-sec.
 */
static uint64_t get_now_nsec()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1000000000ULL) + now.tv_nsec;
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


#ifdef __cplusplus
extern "C" {
#endif
/*##############################################################################
 *                                  _      _    ____ ___
 *  _ __   __ _ ___ _____      ____| |    / \  |  _ \_ _|___
 * | '_ \ / _` / __/ __\ \ /\ / / _` |   / _ \ | |_) | |/ __|
 * | |_) | (_| \__ \__ \\ V  V / (_| |  / ___ \|  __/| |\__ \
 * | .__/ \__,_|___/___/ \_/\_/ \__,_| /_/   \_\_|  |___|___/
 * |_|
 *
 *############################################################################*/

/**
 * @brief   Fill structure pointed to by @result with the data contained in
 *          the structure pointed to by @ham_data_r.
 *
 * @details The string fields pointed to by the members of the passwd
 *          structure are stored in @buffer of size buflen. In case @buffer
 *          has insufficient memory to hold the strings of struct passwd,
 *          the ENOMEM error will be reported.
 *
 * @param   fn_p         Name of the calling function
 * @param   msec         How long it took to process the request
 * @param   success      Whether the request was successful
 * @param   pw_name_r    Value to be saved to result->pw_name
 * @param   pw_passwd_r  Value to be saved to result->pw_passwd
 * @param   pw_uid       Value to be saved to result->pw_uid
 * @param   pw_gid       Value to be saved to result->pw_gid
 * @param   pw_gecos_r   Value to be saved to result->pw_gecos
 * @param   pw_dir_r     Value to be saved to result->pw_dir
 * @param   pw_shell_r   Value to be saved to result->pw_shell
 * @param   result       Pointer to destination where data gets copied
 * @param   buffer       Pointer to memory where strings can be stored
 * @param   buflen       Size of buffer
 * @param   errnop       Pointer to where errno code can be written
 *
 * @return - If no entry was found, return NSS_STATUS_NOTFOUND and set
 *           errno=ENOENT.
 *         - If @buffer has insufficient memory to hold the strings of
 *           struct passwd then return NSS_STATUS_TRYAGAIN and set
 *           errno=ENOMEM.
 *         - Otherwise return NSS_STATUS_SUCCESS and set errno to 0.
 */
static enum nss_status pw_fill_result(const char           * fn_p,
                                      double                 msec,
                                      bool                   success,
                                      const std::string    & pw_name_r,
                                      const std::string    & pw_passwd_r,
                                      uid_t                  pw_uid,
                                      gid_t                  pw_gid,
                                      const std::string    & pw_gecos_r,
                                      const std::string    & pw_dir_r,
                                      const std::string    & pw_shell_r,
                                      struct passwd        * result,
                                      char                 * buffer,
                                      size_t                 buflen,
                                      int                  * errnop)
{
    if (!success)
    {
        SYSLOG(LOG_ERR, "%s() - [%u:%u] cmdline=\"%s\" exec time=%.3f ms. Failed!\n", fn_p, getuid(), getgid(), cmdline_p, msec);
        *errnop = ENOENT;
        return NSS_STATUS_NOTFOUND;
    }

    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "%s() - [%u:%u] cmdline=\"%s\" exec time=%.3f ms. Success! pw_name=\"%s\", pw_passwd=\"%s\", pw_uid=%u, pw_gid=%u, pw_gecos=\"%s\", pw_dir=%s, pw_shell=\"%s\"\n",
                       fn_p, getuid(), getgid(), cmdline_p, msec, pw_name_r.c_str(), pw_passwd_r.c_str(), pw_uid, pw_gid, pw_gecos_r.c_str(), pw_dir_r.c_str(), pw_shell_r.c_str());

    size_t name_l   = pw_name_r.length()   + 1; /* +1 to include NUL terminating char */
    size_t dir_l    = pw_dir_r.length()    + 1;
    size_t shell_l  = pw_shell_r.length()  + 1;
    size_t passwd_l = pw_passwd_r.length() + 1;
    size_t gecos_l  = pw_gecos_r.length()  + 1;
    if (buflen < (name_l + shell_l + dir_l + passwd_l + gecos_l))
    {
        SYSLOG(LOG_ERR, "%s() - [%u:%u] cmdline=\"%s\" not enough memory for struct passwd data\n", fn_p, getuid(), getgid(), cmdline_p);

        if (errnop) *errnop = ENOMEM;
        return NSS_STATUS_TRYAGAIN;
    }

    result->pw_uid    = pw_uid;
    result->pw_gid    = pw_gid;
    result->pw_name   = buffer;
    result->pw_dir    = cpy2buf(result->pw_name,   pw_name_r.c_str(),   name_l);
    result->pw_shell  = cpy2buf(result->pw_dir,    pw_dir_r.c_str(),    dir_l);
    result->pw_passwd = cpy2buf(result->pw_shell,  pw_shell_r.c_str(),  shell_l);
    result->pw_gecos  = cpy2buf(result->pw_passwd, pw_passwd_r.c_str(), passwd_l);
    cpy2buf(result->pw_gecos, pw_gecos_r.c_str(), gecos_l);

    if (errnop) *errnop = 0;

    return NSS_STATUS_SUCCESS;
}

/**
 * @brief Retrieve passwd info from Host Account Management daemon (hamd).
 *
 * @param name      User name.
 * @param result    Where to write the result
 * @param buffer    Buffer used as a temporary pool where we can save
 *                  strings.
 * @param buflen    Size of memory pointed to by buffer
 * @param errnop    Where to return the errno
 *
 * @return NSS_STATUS_SUCCESS, NSS_STATUS_NOTFOUND, or NSS_STATUS_TRYAGAIN.
 */
enum nss_status _nss_ham_getpwnam_r(const char    * name,
                                    struct passwd * result,
                                    char          * buffer,
                                    size_t          buflen,
                                    int           * errnop)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_getpwnam_r() - [%u:%u] cmdline=\"%s\" name=\"%s\"\n",
                       getuid(), getgid(), cmdline_p, name);

    uint64_t  before_nsec = verbose ? get_now_nsec() : 0ULL;

    ::DBus::Struct <
       bool,        /* _1: success   */
       std::string, /* _2: pw_name   */
       std::string, /* _3: pw_passwd */
       uint32_t,    /* _4: pw_uid    */
       uint32_t,    /* _5: pw_gid    */
       std::string, /* _6: pw_gecos  */
       std::string, /* _7: pw_dir    */
       std::string  /* _8: pw_shell  */
    >  ham_data;

    try
    {
        name_service_proxy_c  ns(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);
        ham_data = ns.getpwnam(name);
    }
    catch (DBus::Error & ex)
    {
        SYSLOG(LOG_CRIT, "_nss_ham_getpwnam_r() - [%u:%u] cmdline=\"%s\" Exception %s\n", getuid(), getgid(), cmdline_p, ex.what());
        *errnop = ENOENT;
        return NSS_STATUS_NOTFOUND;
    }

    double duration_msec = verbose ? (get_now_nsec() - before_nsec)/1000000.0 : 0.0;

    return pw_fill_result("_nss_ham_getpwnam_r",
                          duration_msec,
                          ham_data._1,
                          ham_data._2,
                          ham_data._3,
                          ham_data._4,
                          ham_data._5,
                          ham_data._6,
                          ham_data._7,
                          ham_data._8,
                          result,
                          buffer,
                          buflen,
                          errnop);
}

/**
 * @brief Retrieve passwd info from Host Account Management daemon (hamd).
 *
 * @param uid       User ID.
 * @param result    Where to write the result
 * @param buffer    Buffer used as a temporary pool where we can save
 *                  strings.
 * @param buflen    Size of memory pointed to by buffer
 * @param errnop    Where to return the errno
 *
 * @return NSS_STATUS_SUCCESS, NSS_STATUS_NOTFOUND, or NSS_STATUS_TRYAGAIN.
 */
enum nss_status _nss_ham_getpwuid_r(uid_t           uid,
                                    struct passwd * result,
                                    char          * buffer,
                                    size_t          buflen,
                                    int           * errnop)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_getpwuid_r() - [%u:%u] cmdline=\"%s\" uid=%u\n",
                       getuid(), getgid(), cmdline_p, uid);

    uint64_t  before_nsec = verbose ? get_now_nsec() : 0ULL;

    ::DBus::Struct <
       bool,        /* _1: success   */
       std::string, /* _2: pw_name   */
       std::string, /* _3: pw_passwd */
       uint32_t,    /* _4: pw_uid    */
       uint32_t,    /* _5: pw_gid    */
       std::string, /* _6: pw_gecos  */
       std::string, /* _7: pw_dir    */
       std::string  /* _8: pw_shell  */
    >  ham_data;

    try
    {
        name_service_proxy_c  ns(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);
        ham_data = ns.getpwuid(uid);
    }
    catch (DBus::Error & ex)
    {
        SYSLOG(LOG_CRIT, "_nss_ham_getpwuid_r() - [%u:%u] cmdline=\"%s\" Exception %s\n", getuid(), getgid(), cmdline_p, ex.what());
        *errnop = ENOENT;
        return NSS_STATUS_NOTFOUND;
    }

    double duration_msec = verbose ? (get_now_nsec() - before_nsec)/1000000.0 : 0.0;

    return pw_fill_result("_nss_ham_getpwuid_r",
                          duration_msec,
                          ham_data._1,
                          ham_data._2,
                          ham_data._3,
                          ham_data._4,
                          ham_data._5,
                          ham_data._6,
                          ham_data._7,
                          ham_data._8,
                          result,
                          buffer,
                          buflen,
                          errnop);
}

/**
 * @brief This function prepares the service for following operations (i.e.
 *        _nss_ham_endpwent and _nss_ham_getpwent_r). This function
 *        contacts the hamd service to retrieve the contents of the
 *        /etc/passwd file on the host and caches the contents to a local
 *        memory file.
 *
 * @return The return value should be NSS_STATUS_SUCCESS or according to
 *         the table above in case of an error (see NSS Modules Interface:
 *         https://www.gnu.org/software/libc/manual/html_node/NSS-Modules-Interface.html#NSS-Modules-Interface).
 */
static FILE        * passwd_fp = nullptr;
static std::mutex    passwd_mtx;  // protects passwd_fp
enum nss_status _nss_ham_setpwent(void)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_setpwent() - [%u:%u] cmdline=\"%s\"\n", getuid(), getgid(), cmdline_p);

    uint64_t  before_nsec = verbose ? get_now_nsec() : 0ULL;

    const std::lock_guard<std::mutex> lock(passwd_mtx);
    if (passwd_fp == nullptr)
    {
        std::string contents;

        try
        {
            name_service_proxy_c  ns(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);
            contents = ns.getpwcontents();
        }
        catch (DBus::Error & ex)
        {
            SYSLOG(LOG_CRIT, "_nss_ham_setpwent() - [%u:%u] cmdline=\"%s\" Exception %s\n", getuid(), getgid(), cmdline_p, ex.what());
            return NSS_STATUS_TRYAGAIN;
        }

        int fd = memfd_create("passwd", MFD_CLOEXEC);
        if (fd == -1)
        {
            SYSLOG(LOG_ERR, "_nss_ham_setpwent() - [%u:%u] cmdline=\"%s\" Failed to create passwd cache file. errno=%d (%s)\n",
                   getuid(), getgid(), cmdline_p, errno, strerror(errno));
            return NSS_STATUS_TRYAGAIN;
        }

        // Convert File Descriptor to a "FILE *". This is needed for fgetpwent()
        passwd_fp = fdopen(fd, "w+");
        if (passwd_fp == nullptr)
        {
            SYSLOG(LOG_ERR, "_nss_ham_setpwent() - [%u:%u] cmdline=\"%s\" fdopen() failed. errno=%d (%s)\n",
                   getuid(), getgid(), cmdline_p, errno, strerror(errno));
            return NSS_STATUS_TRYAGAIN;
        }

        fwrite(contents.c_str(), contents.length(), 1, passwd_fp);
    }

    rewind(passwd_fp);

    double duration_msec = verbose ? (get_now_nsec() - before_nsec)/1000000.0 : 0.0;
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_setpwent() - [%u:%u] cmdline=\"%s\" exec time=%.3f ms. Success!\n", getuid(), getgid(), cmdline_p, duration_msec);

    return NSS_STATUS_SUCCESS;
}

/**
 * @brief This function indicates that the caller is done with the cached
 *        contents of the host's /etc/passwd.
 *
 * @return There normally is no return value other than NSS_STATUS_SUCCESS.
 */
enum nss_status _nss_ham_endpwent(void)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_endpwent() - [%u:%u] cmdline=\"%s\"\n", getuid(), getgid(), cmdline_p);

    const std::lock_guard<std::mutex> lock(passwd_mtx);

    if (passwd_fp != nullptr)
    {
        fclose(passwd_fp);
        passwd_fp = nullptr;
    }

    return NSS_STATUS_SUCCESS;
}

/**
 * @brief Since this function will be called several times in a row to
 *        retrieve one entry after the other it must keep some kind of
 *        state. But this also means the functions are not really
 *        reentrant. They are reentrant only in that simultaneous calls to
 *        this function will not try to write the retrieved data in the
 *        same place; instead, it writes to the structure pointed to by the
 *        @result parameter. But the calls share a common state and in the
 *        case of a file access this means they return neighboring entries
 *        in the file.
 *
 *        The buffer of length @buflen pointed to by @buffer can be used
 *        for storing some additional data for the result. It is not
 *        guaranteed that the same buffer will be passed for the next call
 *        of this function. Therefore one must not misuse this buffer to
 *        save some state information from one call to another.
 *
 *        Before the function returns with a failure code, the
 *        implementation should store the value of the local errno variable
 *        in the variable pointed to be @errnop. This is important to
 *        guarantee the module working in statically linked programs. The
 *        stored value must not be zero.
 *
 * @param result  Where to save result
 * @param buffer  Memory used to store additional data (e.g. strings) that
 *                cannot fit in result.
 * @param buflen  Size of @buffer
 * @param errnop  Where to save the errno code.
 *
 * @return The function shall return NSS_STATUS_SUCCESS as long as there
 *         are more entries. When the last entry was read it should return
 *         NSS_STATUS_NOTFOUND. When the buffer given as an argument is too
 *         small for the data to be returned NSS_STATUS_TRYAGAIN should be
 *         returned. When the service was not formerly initialized by a
 *         call to @_nss_ham_setpwent() all return values allowed for this
 *         function can also be returned here.
 */
enum nss_status _nss_ham_getpwent_r(struct passwd * result, char * buffer, size_t buflen, int * errnop)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_getpwent_r() - [%u:%u] cmdline=\"%s\"\n", getuid(), getgid(), cmdline_p);

    struct passwd * ent = nullptr;
    uint64_t        before_nsec = verbose ? get_now_nsec() : 0ULL;

    do { // protected section begin
        const std::lock_guard<std::mutex> lock(passwd_mtx);

        if (passwd_fp == nullptr)
        {
            SYSLOG(LOG_ERR, "_nss_ham_getpwent_r() - [%u:%u] cmdline=\"%s\" passwd_fp=NULL\n", getuid(), getgid(), cmdline_p);
            if (errnop != nullptr) *errnop = EIO;
            return NSS_STATUS_TRYAGAIN;
        }

        if (NULL == (ent = fgetpwent(passwd_fp)))
            return NSS_STATUS_NOTFOUND; // no more entries
    } while(0); // protected section end

    double duration_msec = verbose ? (get_now_nsec() - before_nsec)/1000000.0 : 0.0;

    return pw_fill_result("_nss_ham_getpwent_r",
                          duration_msec,
                          true,
                          ent->pw_name,
                          ent->pw_passwd,
                          ent->pw_uid,
                          ent->pw_gid,
                          ent->pw_gecos,
                          ent->pw_dir,
                          ent->pw_shell,
                          result,
                          buffer,
                          buflen,
                          errnop);
}

/*##############################################################################
 *                                   _    ____ ___
 *   __ _ _ __ ___  _   _ _ __      / \  |  _ \_ _|___
 *  / _` | '__/ _ \| | | | '_ \    / _ \ | |_) | |/ __|
 * | (_| | | | (_) | |_| | |_) |  / ___ \|  __/| |\__ \
 *  \__, |_|  \___/ \__,_| .__/  /_/   \_\_|  |___|___/
 *  |___/                |_|
 *
 *############################################################################*/

/**
 * @brief   Fill structure pointed to by @result with the data contained in
 *          the structure pointed to by @ham_data_r.
 *
 * @details The string fields pointed to by the members of the group
 *          structure are stored in @buffer of size buflen. In case @buffer
 *          has insufficient memory to hold the strings of struct group,
 *          the ENOMEM error will be reported.
 *
 * @param   fn_p         Name of the calling function
 * @param   msec         How long it took to process the request
 * @param   success      Whether the request was successful
 * @param   gr_name_r    Value to be saved to result->gr_name
 * @param   gr_passwd_r  Value to be saved to result->gr_passwd
 * @param   gr_gid       Value to be saved to result->gr_gid
 * @param   gr_mem_r     Value to be saved to result->gr_mem
 * @param   result       Pointer to destination where data gets copied
 * @param   buffer       Pointer to memory where strings can be stored
 * @param   buflen       Size of buffer
 * @param   errnop       Pointer to where errno code can be written
 *
 * @return - If no entry was found, return NSS_STATUS_NOTFOUND and set
 *           errno=ENOENT.
 *         - If @buffer has insufficient memory to hold the strings of
 *           struct passwd then return NSS_STATUS_TRYAGAIN and set
 *           errno=ENOMEM.
 *         - Otherwise return NSS_STATUS_SUCCESS and set errno to 0.
 */
static enum nss_status gr_fill_result(const char                         * fn_p,
                                      double                               msec,
                                      bool                                 success,
                                      const std::string                  & gr_name_r,
                                      const std::string                  & gr_passwd_r,
                                      gid_t                                gr_gid,
                                      const std::vector< std::string >   & gr_mem_r,
                                      struct group                       * result,
                                      char                               * buffer,
                                      size_t                               buflen,
                                      int                                * errnop)
{
    if (!success)
    {
        SYSLOG(LOG_ERR, "%s() - [%u:%u] cmdline=\"%s\" exec time=%.3f ms. Failed!\n", fn_p, getuid(), getgid(), cmdline_p, msec);
        *errnop = ENOENT;
        return NSS_STATUS_NOTFOUND;
    }

    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "%s() - [%u:%u] cmdline=\"%s\" exec time=%.3f ms. Success! gr_name=\"%s\", pw_passwd=\"%s\", gr_gid=%u, pw_mem=[\"%s\"]\n",
                       fn_p, getuid(), getgid(), cmdline_p, msec, gr_name_r.c_str(), gr_passwd_r.c_str(), gr_gid, join(gr_mem_r.begin(), gr_mem_r.end(), "\", \"").c_str());

    size_t name_l    = gr_name_r.length()   + 1; /* +1 to include NUL terminating char */
    size_t passwd_l  = gr_passwd_r.length() + 1;
    size_t array_l   = sizeof(char *) * (gr_mem_r.size() + 1); /* +1 for a nullptr sentinel */
    size_t strings_l = std::accumulate(gr_mem_r.begin(), gr_mem_r.end(), 0, [](size_t sum, const std::string& elem) {return sum + elem.length() + 1;}); /* +1 to include NUL terminating char */
    if (buflen < (name_l + passwd_l + array_l + strings_l))
    {
        SYSLOG(LOG_ERR, "%s() - [%u:%u] cmdline=\"%s\" not enough memory for struct group data\n", fn_p, getuid(), getgid(), cmdline_p);

        if (errnop) *errnop = ENOMEM;
        return NSS_STATUS_TRYAGAIN;
    }

    result->gr_gid    = gr_gid;
    result->gr_mem    = (char  **)buffer;
    result->gr_mem[0] = buffer + array_l;
    for (unsigned i = 0; i < gr_mem_r.size(); i++)
        result->gr_mem[i+1] = cpy2buf(result->gr_mem[i], gr_mem_r[i].c_str(), gr_mem_r[i].length() + 1);

    result->gr_name = result->gr_mem[gr_mem_r.size()];
    result->gr_mem[gr_mem_r.size()] = nullptr; // sentinel

    result->gr_passwd = cpy2buf(result->gr_name, gr_name_r.c_str(), name_l);
    cpy2buf(result->gr_passwd, gr_passwd_r.c_str(), passwd_l);

    if (errnop) *errnop = 0;

    return NSS_STATUS_SUCCESS;
}

/**
 * @brief Retrieve group info from Host Account Management daemon (hamd).
 *
 * @param name      Group name.
 * @param result    Where to write the result
 * @param buffer    Buffer used as a temporary pool where we can save
 *                  strings.
 * @param buflen    Size of memory pointed to by buffer
 * @param errnop    Where to return the errno
 *
 * @return NSS_STATUS_SUCCESS, NSS_STATUS_NOTFOUND, or NSS_STATUS_TRYAGAIN.
 */
enum nss_status _nss_ham_getgrnam_r(const char    * name,
                                    struct group  * result,
                                    char          * buffer,
                                    size_t          buflen,
                                    int           * errnop)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_getgrnam_r() - [%u:%u] cmdline=\"%s\" name=\"%s\"\n",
                       getuid(), getgid(), cmdline_p, name);

    uint64_t  before_nsec = verbose ? get_now_nsec() : 0ULL;

    ::DBus::Struct <
       bool,                      /* _1: success   */
       std::string,               /* _2: gr_name   */
       std::string,               /* _3: gr_passwd */
       uint32_t,                  /* _4: gr_gid    */
       std::vector< std::string > /* _5: gr_mem    */
    >  ham_data;

    try
    {
        name_service_proxy_c  ns(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);
        ham_data = ns.getgrnam(name);
    }
    catch (DBus::Error & ex)
    {
        SYSLOG(LOG_CRIT, "_nss_ham_getgrnam_r() - [%u:%u] cmdline=\"%s\" Exception %s\n",
               getuid(), getgid(), cmdline_p, ex.what());
        *errnop = ENOENT;
        return NSS_STATUS_NOTFOUND;
    }

    double duration_msec = verbose ? (get_now_nsec() - before_nsec)/1000000.0 : 0.0;

    return gr_fill_result("_nss_ham_getgrnam_r",
                          duration_msec,
                          ham_data._1, /* success   */
                          ham_data._2, /* gr_name   */
                          ham_data._3, /* gr_passwd */
                          ham_data._4, /* gr_gid    */
                          ham_data._5, /* gr_mem    */
                          result,
                          buffer,
                          buflen,
                          errnop);
}

/**
 * @brief Retrieve group info from Host Account Management daemon (hamd).
 *
 * @param gid       Group ID.
 * @param result    Where to write the result
 * @param buffer    Buffer used as a temporary pool where we can save
 *                  strings.
 * @param buflen    Size of memory pointed to by buffer
 * @param errnop    Where to return the errno
 *
 * @return NSS_STATUS_SUCCESS, NSS_STATUS_NOTFOUND, or NSS_STATUS_TRYAGAIN.
 */
enum nss_status _nss_ham_getgrgid_r(gid_t           gid,
                                    struct group  * result,
                                    char          * buffer,
                                    size_t          buflen,
                                    int           * errnop)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_getgrgid_r() - [%u:%u] cmdline=\"%s\" gid=%u\n",
                       getuid(), getgid(), cmdline_p, gid);

    uint64_t  before_nsec = verbose ? get_now_nsec() : 0ULL;

    ::DBus::Struct <
       bool,                      /* _1: success   */
       std::string,               /* _2: gr_name   */
       std::string,               /* _3: gr_passwd */
       uint32_t,                  /* _4: gr_gid    */
       std::vector< std::string > /* _5: gr_mem    */
    >  ham_data;

    try
    {
        name_service_proxy_c  ns(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);
        ham_data = ns.getgrgid(gid);
    }
    catch (DBus::Error & ex)
    {
        SYSLOG(LOG_CRIT, "_nss_ham_getgrgid_r() - [%u:%u] cmdline=\"%s\" Exception %s\n", getuid(), getgid(), cmdline_p, ex.what());
        *errnop = ENOENT;
        return NSS_STATUS_NOTFOUND;
    }

    double duration_msec = verbose ? (get_now_nsec() - before_nsec)/1000000.0 : 0.0;

    return gr_fill_result("_nss_ham_getgrgid_r",
                          duration_msec,
                          ham_data._1, /* success   */
                          ham_data._2, /* gr_name   */
                          ham_data._3, /* gr_passwd */
                          ham_data._4, /* gr_gid    */
                          ham_data._5, /* gr_mem    */
                          result,
                          buffer,
                          buflen,
                          errnop);
}

/**
 * @brief This function prepares the service for following operations (i.e.
 *        _nss_ham_endgrent and _nss_ham_getgrent_r). This function
 *        contacts the hamd service to retrieve the contents of the
 *        /etc/group file on the host and caches the contents to a local
 *        memory file.
 *
 * @return The return value should be NSS_STATUS_SUCCESS or according to
 *         the table above in case of an error (see NSS Modules Interface:
 *         https://www.gnu.org/software/libc/manual/html_node/NSS-Modules-Interface.html#NSS-Modules-Interface).
 */
static FILE        * group_fp = nullptr;
static std::mutex    group_mtx;  // protects group_fp
enum nss_status _nss_ham_setgrent(void)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_setgrent() - [%u:%u] cmdline=\"%s\"\n", getuid(), getgid(), cmdline_p);

    uint64_t  before_nsec = verbose ? get_now_nsec() : 0ULL;

    const std::lock_guard<std::mutex> lock(group_mtx);
    if (group_fp == nullptr)
    {
        std::string contents;

        try
        {
            name_service_proxy_c  ns(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);
            contents = ns.getgrcontents();
        }
        catch (DBus::Error & ex)
        {
            SYSLOG(LOG_CRIT, "_nss_ham_setgrent() - [%u:%u] cmdline=\"%s\" Exception %s\n", getuid(), getgid(), cmdline_p, ex.what());
            return NSS_STATUS_TRYAGAIN;
        }

        SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_setgrent() - [%u:%u] cmdline=\"%s\" received %lu bytes from hamd's getgrcontents()\n", getuid(), getgid(), cmdline_p, contents.size());

        int fd = memfd_create("group", MFD_CLOEXEC);
        if (fd == -1)
        {
            SYSLOG(LOG_ERR, "_nss_ham_setgrent() - [%u:%u] cmdline=\"%s\" Failed to create passwd cache file. errno=%d (%s)\n",
                   getuid(), getgid(), cmdline_p, errno, strerror(errno));
            return NSS_STATUS_TRYAGAIN;
        }

        // Convert File Descriptor to a "FILE *". This is needed for fgetpwent()
        group_fp = fdopen(fd, "w+");
        if (group_fp == nullptr)
        {
            SYSLOG(LOG_ERR, "_nss_ham_setgrent() - [%u:%u] cmdline=\"%s\" fdopen() failed. errno=%d (%s)\n",
                   getuid(), getgid(), cmdline_p, errno, strerror(errno));
            return NSS_STATUS_TRYAGAIN;
        }

        if (fwrite(contents.c_str(), contents.length(), 1, group_fp) < 1)
        {
            SYSLOG(LOG_ERR, "_nss_ham_setgrent() - [%u:%u] cmdline=\"%s\" Failed to write to temporary file. errno=%d (%s)\n",
                   getuid(), getgid(), cmdline_p, errno, strerror(errno));
        }
    }

    rewind(group_fp);

    double duration_msec = verbose ? (get_now_nsec() - before_nsec)/1000000.0 : 0.0;
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_setgrent() - [%u:%u] cmdline=\"%s\" exec time=%.3f ms. Success!\n", getuid(), getgid(), cmdline_p, duration_msec);

    return NSS_STATUS_SUCCESS;
}

/**
 * @brief This function indicates that the caller is done with the cached
 *        contents of the host's /etc/group.
 *
 * @return There normally is no return value other than NSS_STATUS_SUCCESS.
 */
enum nss_status _nss_ham_endgrent(void)
{

    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_endgrent() - [%u:%u] cmdline=\"%s\"\n", getuid(), getgid(), cmdline_p);

    const std::lock_guard<std::mutex> lock(group_mtx);

    if (group_fp != nullptr)
    {
        fclose(group_fp);
        group_fp = nullptr;
    }
    return NSS_STATUS_SUCCESS;
}

/**
 * @brief Since this function will be called several times in a row to
 *        retrieve one entry after the other it must keep some kind of
 *        state. But this also means the functions are not really
 *        reentrant. They are reentrant only in that simultaneous calls to
 *        this function will not try to write the retrieved data in the
 *        same place; instead, it writes to the structure pointed to by the
 *        @result parameter. But the calls share a common state and in the
 *        case of a file access this means they return neighboring entries
 *        in the file.
 *
 *        The buffer of length @buflen pointed to by @buffer can be used
 *        for storing some additional data for the result. It is not
 *        guaranteed that the same buffer will be passed for the next call
 *        of this function. Therefore one must not misuse this buffer to
 *        save some state information from one call to another.
 *
 *        Before the function returns with a failure code, the
 *        implementation should store the value of the local errno variable
 *        in the variable pointed to be @errnop. This is important to
 *        guarantee the module working in statically linked programs. The
 *        stored value must not be zero.
 *
 * @param result  Where to save result
 * @param buffer  Memory used to store additional data (e.g. strings) that
 *                cannot fit in result.
 * @param buflen  Size of @buffer
 * @param errnop  Where to save the errno code.
 *
 * @return The function shall return NSS_STATUS_SUCCESS as long as there
 *         are more entries. When the last entry was read it should return
 *         NSS_STATUS_NOTFOUND. When the buffer given as an argument is too
 *         small for the data to be returned NSS_STATUS_TRYAGAIN should be
 *         returned. When the service was not formerly initialized by a
 *         call to @_nss_ham_setgrent() all return values allowed for this
 *         function can also be returned here.
 */
enum nss_status _nss_ham_getgrent_r(struct group * result, char * buffer, size_t buflen, int * errnop)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_getgrent() - [%u:%u] cmdline=\"%s\"\n", getuid(), getgid(), cmdline_p);

    struct group * ent = nullptr;
    uint64_t       before_nsec = verbose ? get_now_nsec() : 0ULL;

    do { // protected section begin
        const std::lock_guard<std::mutex> lock(group_mtx);

        if (group_fp == nullptr)
        {
            SYSLOG(LOG_ERR, "_nss_ham_getgrent_r() - [%u:%u] cmdline=\"%s\" group_fp=NULL\n", getuid(), getgid(), cmdline_p);
            if (errnop != nullptr) *errnop = EIO;
            return NSS_STATUS_TRYAGAIN;
        }

        if (NULL == (ent = fgetgrent(group_fp)))
        {
            SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_getgrent() - [%u:%u] cmdline=\"%s\" reached end of entries\n", getuid(), getgid(), cmdline_p);
            return NSS_STATUS_NOTFOUND; // no more entries
        }
    } while(0); // protected section end

    double duration_msec = verbose ? (get_now_nsec() - before_nsec)/1000000.0 : 0.0;

    std::vector < std::string > gr_mem;
    for (unsigned i = 0; ent->gr_mem[i] != NULL; i++)
        gr_mem.push_back(ent->gr_mem[i]);

    return gr_fill_result("_nss_ham_getgrent_r",
                          duration_msec,
                          true,
                          ent->gr_name,
                          ent->gr_passwd,
                          ent->gr_gid,
                          gr_mem,
                          result,
                          buffer,
                          buflen,
                          errnop);
}

/*##############################################################################
 *      _               _                    _    ____ ___
 *  ___| |__   __ _  __| | _____      __    / \  |  _ \_ _|___
 * / __| '_ \ / _` |/ _` |/ _ \ \ /\ / /   / _ \ | |_) | |/ __|
 * \__ \ | | | (_| | (_| | (_) \ V  V /   / ___ \|  __/| |\__ \
 * |___/_| |_|\__,_|\__,_|\___/ \_/\_/   /_/   \_\_|  |___|___/
 *
 *############################################################################*/

/**
 * @brief Invoke Host Account Management Daemon (hamd) over DBus to
 *        retrieve shadow password information for user @name. Upon receipt
 *        of hamd data, fill structure pointed to by @result.
 *
 * @details The string fields pointed to by the members of the spwd
 *          structure are stored in @buffer of size buflen. In case @buffer
 *          has insufficient memory to hold the strings of struct spwd,
 *          the ENOMEM error will be reported.
 *
 * @param name    User name
 * @param result  Pointer to destination where data gets copied
 * @param buffer  Pointer to memory where strings can be stored
 * @param buflen  Size of buffer
 * @param errnop  Pointer to where errno code can be written
 *
 * @return - If no entry was found, return NSS_STATUS_NOTFOUND and set
 *           errno=ENOENT.
 *         - If @buffer has insufficient memory to hold the strings of
 *           struct passwd then return NSS_STATUS_TRYAGAIN and set
 *           errno=ENOMEM.
 *         - Otherwise return NSS_STATUS_SUCCESS and set errno to 0.
 */
enum nss_status _nss_ham_getspnam_r(const char    * name,
                                    struct spwd   * result,
                                    char          * buffer,
                                    size_t          buflen,
                                    int           * errnop)
{
    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_getspnam_r() - [%u:%u] cmdline=\"%s\" name=\"%s\"\n",
                       getuid(), getgid(), cmdline_p, name);

    uint64_t  before_nsec = verbose ? get_now_nsec() : 0ULL;

    ::DBus::Struct <
        bool,        /* _1:  success   */
        std::string, /* _2:  sp_namp   */
        std::string, /* _3:  sp_pwdp   */
        int32_t,     /* _4:  sp_lstchg */
        int32_t,     /* _5:  sp_min    */
        int32_t,     /* _6:  sp_max    */
        int32_t,     /* _7:  sp_warn   */
        int32_t,     /* _8:  sp_inact  */
        int32_t,     /* _9:  sp_expire */
        uint32_t     /* _10: sp_flag   */
    > ham_data;

    try
    {
        name_service_proxy_c  ns(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);
        ham_data = ns.getspnam(name);
    }
    catch (DBus::Error & ex)
    {
        SYSLOG(LOG_CRIT, "_nss_ham_getspnam_r() - [%u:%u] cmdline=\"%s\" Exception %s\n", getuid(), getgid(), cmdline_p, ex.what());
        *errnop = ENOENT;
        return NSS_STATUS_NOTFOUND;
    }

    double duration_msec = verbose ? (get_now_nsec() - before_nsec)/1000000.0 : 0.0;
    bool   success       = ham_data._1;

    if (!success)
    {
        SYSLOG(LOG_ERR, "_nss_ham_getspnam_r() - [%u:%u] cmdline=\"%s\" exec time=%.3f ms. Failed!\n", getuid(), getgid(), cmdline_p, duration_msec);
        *errnop = ENOENT;
        return NSS_STATUS_NOTFOUND;
    }

    std::string   & sp_namp_r = ham_data._2;
    std::string   & sp_pwdp_r = ham_data._3;
    long            sp_lstchg = ham_data._4;
    long            sp_min    = ham_data._5;
    long            sp_max    = ham_data._6;
    long            sp_warn   = ham_data._7;
    long            sp_inact  = ham_data._8;
    long            sp_expire = ham_data._9;
    unsigned long   sp_flag   = ham_data._10;

    SYSLOG_CONDITIONAL(verbose, LOG_DEBUG, "_nss_ham_getspnam_r() - [%u:%u] cmdline=\"%s\" exec time=%.3f ms. Success! sp_namp=\"%s\", sp_pwdp=\"%s\", sp_lstchg=%ld, sp_min=%ld, sp_max=%ld, sp_warn=%ld, sp_inact=%ld, sp_expire=%ld, sp_flag=%lu\n",
                       getuid(), getgid(), cmdline_p, duration_msec, sp_namp_r.c_str(), sp_pwdp_r.c_str(), sp_lstchg, sp_min, sp_max, sp_warn, sp_inact, sp_expire, sp_flag);

    size_t sp_namp_l = sp_namp_r.length() + 1; /* +1 to include NUL terminating char */
    size_t sp_pwdp_l = sp_pwdp_r.length() + 1;
    if (buflen < (sp_namp_l + sp_pwdp_l))
    {
        SYSLOG(LOG_ERR, "_nss_ham_getspnam_r() - [%u:%u] cmdline=\"%s\" not enough memory for struct spwd data\n", getuid(), getgid(), cmdline_p);

        if (errnop) *errnop = ENOMEM;
        return NSS_STATUS_TRYAGAIN;
    }

    result->sp_namp = buffer;
    result->sp_pwdp = cpy2buf(result->sp_namp, sp_namp_r.c_str(), sp_namp_l);
    cpy2buf(result->sp_pwdp, sp_pwdp_r.c_str(), sp_pwdp_l);
    result->sp_lstchg = sp_lstchg;
    result->sp_min    = sp_min;
    result->sp_max    = sp_max;
    result->sp_warn   = sp_warn;
    result->sp_inact  = sp_inact;
    result->sp_expire = sp_expire;
    result->sp_flag   = sp_flag;

    if (errnop) *errnop = 0;

    return NSS_STATUS_SUCCESS;
}

//##############################################################################

/**
 * @brief Initalize module singletons on entry.
 */
void __attribute__((constructor)) __module_enter(void)
{
    read_config();
    if (verbose)
        read_cmdline();
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
