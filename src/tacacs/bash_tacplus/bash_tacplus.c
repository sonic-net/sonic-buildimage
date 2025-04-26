#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Remote user gecos prefix, which been assigned by nss_tacplus */
#define REMOTE_USER_GECOS_PREFIX      "remote_user"

/* Default value for getpwent */
#define DEFAULT_GETPWENT_SIZE_MAX     4096

/* Remote IP address size */
#define REMOTE_ADDRESS_SIZE     64

/* Return value for is_local_user method */
#define IS_LOCAL_USER              0
#define IS_REMOTE_USER             1
#define ERROR_CHECK_LOCAL_USER     2

/* Tacacs+ lib */
#include <libtac/libtac.h>

/* Tacacs+ support lib */
#include <libtac/support.h>

/* Output syslog to mock method when build with UT */
#if defined (BASH_PLUGIN_UT)
#define syslog mock_syslog
#define getpwent_r mock_getpwent_r
#endif

/* Tacacs+ log format */
#define  TACACS_LOG_FORMAT "TACACS+: %s"

/* Tacacs+ config file timestamp string format */
#define  CONFIG_FILE_TIME_STAMP_FORMAT "%d.%m.%Y %H:%M:%S"

/* Tacacs+ config file timestamp string length */
#define  CONFIG_FILE_TIME_STAMP_LEN  100

#define GET_ENV_VARIABLE_OK                 0
#define GET_ENV_VARIABLE_NOT_FOUND          1
#define GET_ENV_VARIABLE_INCORRECT_FORMAT   2
#define GET_ENV_VARIABLE_NOT_ENOUGH_BUFFER  3
#define GET_REMOTE_ADDRESS_OK               0
#define GET_REMOTE_ADDRESS_FAILED           1

/*
    Convert log to a string because va args resoursive issue:
    http://www.c-faq.com/varargs/handoff.html
*/
#define GENERATE_LOG_FROM_VA(logBufferName)                 \
    char logBufferName[512];                                \
    va_list args;                                           \
    va_start(args, format);                                 \
    vsnprintf(logBufferName, sizeof(logBufferName), format, args);  \
    va_end(args);

/* Config file path */
const char *tacacs_config_file = "/etc/tacplus_nss.conf";

/* Unknown user name */
const char *unknown_username = "UNKNOWN";

/* Config file attribute */
struct stat config_file_attr;

/* Tacacs server config data */
typedef struct {
    struct addrinfo *address;
    const char *key;
} tacacs_server_t;

/* Tacacs control flag */
int tacacs_ctrl;

/*
 * Output error message.
 */
void output_error(const char *format, ...)
{
    GENERATE_LOG_FROM_VA(logBuffer);

    if (tacacs_ctrl & PAM_TAC_DEBUG) {
        fprintf(stderr, TACACS_LOG_FORMAT, logBuffer);
    }

    syslog(LOG_ERR, TACACS_LOG_FORMAT, logBuffer);
}

/*
 * Output debug message.
 */
void output_debug(const char *format, ...)
{
    if ((tacacs_ctrl & PAM_TAC_DEBUG) == 0) {
        return;
    }

    GENERATE_LOG_FROM_VA(logBuffer);
    fprintf(stderr, TACACS_LOG_FORMAT, logBuffer);
    syslog(LOG_DEBUG, TACACS_LOG_FORMAT, logBuffer);
}


/*
 * Send authorization message.
 * This method based on send_auth_msg in https://github.com/daveolson53/tacplus-auth/blob/master/tacplus-auth.c
 */
int send_authorization_message(
    int tac_fd,
    const char *user,
    const char *tty,
    const char *remote,
    uint16_t taskid,
    const char *cmd,
    char **args,
    int argc)
{
    char buf[128];
    struct tac_attrib *attr;
    int retval;
    struct areply re;
    int i;

    attr=(struct tac_attrib *)xcalloc(1, sizeof(struct tac_attrib));

    snprintf(buf, sizeof buf, "%hu", taskid);
    tac_add_attrib(&attr, "task_id", buf);
    tac_add_attrib(&attr, "protocol", "ssh");
    tac_add_attrib(&attr, "service", "shell");

    tac_add_attrib(&attr, "cmd", (char*)cmd);

    for(i=1; i<argc; i++) {
        // TACACS protocol allow max 255 bytes per argument. 'cmd-arg' will take 7 bytes.
        char tbuf[248];
        const char *arg;
        if(strlen(args[i]) >= sizeof(tbuf)) {
            snprintf(tbuf, sizeof tbuf, "%s", args[i]);
            arg = tbuf;
        }
        else {
            arg = args[i];
        }

        tac_add_attrib(&attr, "cmd-arg", (char *)arg);
    }

    re.msg = NULL;
    output_debug("send authorizatiom message with user: %s, tty: %s, remote: %s\n", user, tty, remote);
    retval = tac_author_send(tac_fd, (char *)user, (char *)tty, (char *)remote, attr);
    output_debug("authorization result: %d\n", retval);

    if(retval < 0) {
        output_error("send of authorization message failed: %s\n", strerror(errno));
    }
    else {
        retval = tac_author_read(tac_fd, &re);
        if (retval < 0) {
            output_debug("authorization response failed: %d\n", retval);
        }
        else if(re.status == AUTHOR_STATUS_PASS_ADD ||
                    re.status == AUTHOR_STATUS_PASS_REPL) {
            retval = 0;
        }
        else  {
            output_debug("command not authorized (%d)\n", re.status);
            retval = 1;
        }
    }

    tac_free_attrib(&attr);
    if(re.msg != NULL) {
        free(re.msg);
    }

    return retval;
}

/*
 * Send tacacs authorization request.
 * This method based on send_tacacs_auth in https://github.com/daveolson53/tacplus-auth/blob/master/tacplus-auth.c
 */
int tacacs_authorization(
    const char *user,
    const char *tty,
    const char *remote,
    const char *cmd,
    char **args,
    int argc)
{
    int result = 1, server_idx, server_fd, connected_servers=0;
    uint16_t task_id = (uint16_t)getpid();

    for(server_idx = 0; server_idx < tac_srv_no; server_idx++) {
        server_fd = tac_connect_single(tac_srv[server_idx].addr, tac_srv[server_idx].key, tac_source_addr, tac_timeout, __vrfname);
        if(server_fd < 0) {
            // connect to tacacs server failed
            output_error("Failed to connecting to %s to request authorization for %s: %s\n", tac_ntop(tac_srv[server_idx].addr->ai_addr), cmd, strerror(errno));
            continue;
        }

        // increase connected servers
        connected_servers++;
        result = send_authorization_message(server_fd, user, tty, remote, task_id, cmd, args, argc);
        close(server_fd);
        if(result) {
            // authorization failed
            output_debug("%s not authorized from %s\n", cmd, tac_ntop(tac_srv[server_idx].addr->ai_addr));
        }
        else {
            // authorization successed
            output_debug("%s authorized from %s\n", cmd, tac_ntop(tac_srv[server_idx].addr->ai_addr));
            break;
        }
    }

    // can't connect to any server
    if(!connected_servers) {
        result = -2;
        output_error("Failed to connect to TACACS server(s)\n");
    }

    return result;
}

/*
 * Get environment variable first part by name and delimiters
 */
int get_environment_variable_first_part(char* dst, socklen_t size, const char* name, const char* delimiters)
{
    memset(dst, 0, size);

    const char* variable = getenv(name);
    if (variable == NULL) {
        output_debug("Can't get environment variable %s, errno=%d", name, errno);
        return GET_ENV_VARIABLE_NOT_FOUND;
    }

    char* context = NULL;
    char* first_part = strtok_r((char *)variable, delimiters, &context);
    if (first_part == NULL) {
        output_debug("Can't split %s by delimiters %s", variable, delimiters);
        return GET_ENV_VARIABLE_INCORRECT_FORMAT;
    }

    int first_part_len = strlen(first_part);
    if (first_part_len >= size) {
        output_debug("Dest buffer size %d not enough for %s", size, first_part);
        return GET_ENV_VARIABLE_NOT_ENOUGH_BUFFER;
    }

    snprintf(dst, size, "%s", first_part);
    output_debug("Remote address=%s", dst);
    return GET_ENV_VARIABLE_OK;
}

/*
 * Get current SSH session remote address from environment variable
 */
int get_remote_address(char* dst, socklen_t size)
{
    // SSHD will create environment variable SSH_CONNECTION after user session created.
    if (get_environment_variable_first_part(dst, size, "SSH_CONNECTION", " ") == GET_ENV_VARIABLE_OK) {
        return GET_REMOTE_ADDRESS_OK;
    }

    // Before user session created, SSHD will create environment variable SSH_CLIENT_IPADDR_PORT.
    if (get_environment_variable_first_part(dst, size, "SSH_CLIENT_IPADDR_PORT", " ") == GET_ENV_VARIABLE_OK) {
        return GET_REMOTE_ADDRESS_OK;
    }

    return GET_REMOTE_ADDRESS_FAILED;
}

/*
 * Send authorization request.
 * This method based on build_auth_req in https://github.com/daveolson53/tacplus-auth/blob/master/tacplus-auth.c
 */
int authorization_with_host_and_tty(const char *user, const char *cmd, char **argv, int argc)
{
    // try get host name
    char remote_addr[REMOTE_ADDRESS_SIZE];
    memset(&remote_addr, 0, sizeof(remote_addr));

    int result = get_remote_address(remote_addr, sizeof(remote_addr));
    if ((result != GET_REMOTE_ADDRESS_OK)) {
        snprintf(remote_addr, sizeof(remote_addr), "UNK");
        output_error("Failed to determine remote address, passing %s\n", remote_addr);
    }

    // try get tty name
    char ttyname[64];
    memset(&ttyname, 0, sizeof(ttyname));

    int i;
    for(i=0; i<3; i++) {
        int result;
        if (isatty(i)) {
            result = ttyname_r(i, ttyname, sizeof(ttyname) -1);
            if (result) {
                output_error("Failed to get tty name for fd %d: %s\n", i, strerror(result));
            }
            break;
        }
    }

    if (!ttyname[0]) {
        snprintf(ttyname, sizeof(ttyname), "UNK");
        output_error("Failed to determine tty, passing %s\n", ttyname);
    }

    // send tacacs authorization request
    return tacacs_authorization(user, ttyname, remote_addr, cmd, argv, argc);
}

/*
 * Load tacacs config.
 */
void load_tacacs_config()
{
    // load config file: tacacs_config_file
    tacacs_ctrl = parse_config_file (tacacs_config_file);

    output_debug("tacacs config updated:\n");
    int server_idx;
    for(server_idx = 0; server_idx < tac_srv_no; server_idx++) {
        output_debug("Server %d, address:%s, key length:%d\n", server_idx, tac_ntop(tac_srv[server_idx].addr->ai_addr),strlen(tac_srv[server_idx].key));
    }

    output_debug("TACACS+ control flag: 0x%x\n", tacacs_ctrl);

    if (tacacs_ctrl & AUTHORIZATION_FLAG_TACACS) {
        output_debug("TACACS+ per-command authorization enabled.\n");
    }

    if (tacacs_ctrl & AUTHORIZATION_FLAG_LOCAL) {
        output_debug("Local per-command authorization enabled.\n");
    }

    if (tacacs_ctrl & PAM_TAC_DEBUG) {
        output_debug("TACACS+ debug enabled.\n");
    }
}

/*
 * Load tacacs config.
 */
void check_and_load_changed_tacacs_config()
{
    struct stat attr;
    // get config file stat, check if file changed
    stat(tacacs_config_file, &attr);
    char date[CONFIG_FILE_TIME_STAMP_LEN];
    strftime(date, sizeof(date), CONFIG_FILE_TIME_STAMP_FORMAT, localtime(&(attr.st_mtime)));
    if (difftime(attr.st_mtime, config_file_attr.st_mtime) == 0) {
        output_debug("tacacs config file not change: last modified time: %s.\n", date);
        return;
    }

    output_debug("tacacs config file changed: last modified time: %s.\n", date);

    // config file changed, update file stat and reload config.
    config_file_attr = attr;

    // load config file
    load_tacacs_config();
}

/*
 * Tacacs plugin initialization.
 */
void plugin_init()
{
    // get config file stat, will use this to check config file changed
    stat(tacacs_config_file, &config_file_attr);

    // load config file: tacacs_config_file
    load_tacacs_config();

    output_debug("tacacs plugin initialized.\n");
}

/*
 * Tacacs plugin release.
 */
void plugin_uninit()
{
    output_debug("tacacs plugin un-initialize.\n");
}

/*
 * Check if current user is local user.
 */
int is_local_user(const char *user)
{
    if (user == unknown_username) {
        // for unknown user name, when tacacs enabled, always authorization with tacacs.
        return IS_REMOTE_USER;
    }

    struct passwd pwd;
    struct passwd *ppwd;
    char buf[DEFAULT_GETPWENT_SIZE_MAX];
    int pwdresult;
    int result = ERROR_CHECK_LOCAL_USER;
    setpwent();
    while (1) {
        pwdresult = getpwent_r(&pwd, buf, sizeof(buf), &ppwd);
        if (pwdresult) {
            // no more pw entry
            break;
        }

        if (strcmp(ppwd->pw_name, user) != 0) {
            continue;
        }

        // compare passwd entry, for remote user pw_gecos will start as 'remote_user'
        if (strncmp(ppwd->pw_gecos, REMOTE_USER_GECOS_PREFIX, strlen(REMOTE_USER_GECOS_PREFIX)) == 0) {
            output_debug("user: %s, UID: %d, GECOS: %s is remote user.\n", user, ppwd->pw_uid, ppwd->pw_gecos);
            result = IS_REMOTE_USER;
        }
        else {
            output_debug("user: %s, UID: %d, GECOS: %s is local user.\n", user, ppwd->pw_uid, ppwd->pw_gecos);
            result = IS_LOCAL_USER;
        }
        break;
    }
    endpwent();

    if (result == ERROR_CHECK_LOCAL_USER) {
        output_error("get user information user failed, user: %s not found\n", user);
    }

    return result;
}

/*
 * Get user name.
 */
const char* get_user_name(char *user)
{
    if (user != NULL && strlen(user) != 0) {
        return user;
    }

    // uid is the real user id: https://man7.org/linux/man-pages/man2/geteuid.2.html
    output_debug("Login user name is empty, try get user name by euid.\n");
    uid_t uid = getuid();
    struct passwd* userwd = getpwuid(uid);
    if (userwd != NULL && userwd->pw_name != NULL) {
        return userwd->pw_name;
    }

    // euid is the effective user name, may not match real user id: https://man7.org/linux/man-pages/man2/geteuid.2.html
    output_debug("Login user name is empty, try get user name by euid.\n");
    uid_t euid = geteuid();
    struct passwd* euserwd = getpwuid(euid);
    if (euserwd != NULL && euserwd->pw_name != NULL) {
        return euserwd->pw_name;
    }

    // if can't find user name by both euid or ruid, return UNKNOWN.
    return unknown_username;
}

/*
 * Tacacs authorization.
 */
int on_shell_execve (char *user, int shell_level, char *cmd, char **argv)
{
    const char* user_namd = get_user_name(user);
    output_debug("Authorization parameters:\n");
    output_debug("    Shell level: %d\n", shell_level);
    output_debug("    Current user: %s\n", user_namd);
    output_debug("    Command full path: %s\n", cmd);
    output_debug("    Parameters:\n");
    char **parameter_array_pointer = argv;
    int argc = 0;
    while (*parameter_array_pointer != NULL) {
        // output parameter
        output_debug("        %s\n", *parameter_array_pointer);

        // move to next parameter
        parameter_array_pointer++;
        argc++;
    }

    if (shell_level > 2) {
        // when shell_level > 1, it's a recursive command in shell script.
        output_debug("Recursive command %s ignored.\n", cmd);
        return 0;
    }

    // reload config file when tacacs config changed
    check_and_load_changed_tacacs_config();

    int check_local_user_result = is_local_user(user_namd);
    if (check_local_user_result != IS_REMOTE_USER) {
        /*
            Return 0 to check with linux permission control in following 2 scenario:
                1: ERROR_CHECK_LOCAL_USER: check if user is local user failed because can't get user information.
                        In this case, as failback, check with linux permission control.
                2: IS_LOCAL_USER: user login as local user.
                        In this case, tacacs authorization disabled for local user.
        */
        output_debug("ignore TACACS+ authorization for current user, check with local permission.\n");
        return 0;
    }

    if (tacacs_ctrl & AUTHORIZATION_FLAG_TACACS) {
        output_debug("start TACACS+ authorization for command %s with given arguments\n", cmd);
        int ret = authorization_with_host_and_tty(user_namd, cmd, argv, argc);
        switch (ret) {
            case 0:
            break;
            case -2:
                // -2 means no servers, so not authorized
                fprintf(stdout, "%s not authorized by TACACS+ with given arguments, not executing\n", cmd);
            break;
            default:
                // when command reject by server, authorization will failed immediately
                fprintf(stdout, "%s authorize failed by TACACS+ with given arguments, not executing\n", cmd);
                return ret;
        }

        if ((tacacs_ctrl & AUTHORIZATION_FLAG_LOCAL) == 0) {
            // when local authorization disabled, tacacs authorization failed will block user from run current command
            output_debug("local authorization disabled, TACACS+ authorization result: %d\n", ret);
            return ret;
        }
    }

    // return 0, so bash will continue run user command and will check user permission with linux permission check.
    output_debug("start local authorization for command %s with given arguments\n", cmd);
    return 0;
}
