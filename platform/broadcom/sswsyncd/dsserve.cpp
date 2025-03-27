/*
 *   dsserve -d <domain_socket_filename> bcm.user
 *
 * Now connect to bcm.user, e.g. like this:
 *
 *   dsclient <domain_socket_filename> <cmd>
 *
 */

 #include <stdlib.h>
 #include <stdio.h>
 #include <stdarg.h>
 #include <string.h>
 #include <errno.h>

 #include <sys/types.h>
 #include <sys/socket.h>
 #include <sys/un.h>
 #include <syslog.h>
 #include <sys/time.h>
 #include <sys/wait.h>

 #include <errno.h>
 #include <unistd.h>
 #include <signal.h>
 #include <pthread.h>
 #include <pty.h>
 #include <arpa/inet.h>
 #include "dsserve.h"

 static inline void syslog_printf(int priority, const char *format, ...)
 {
     va_list args1, args2;
     va_start(args1, format);
     va_copy(args2, args1); // va_list cannot be used twice, copy it
     vsyslog(priority, format, args1);

     auto f = (priority & LOG_PRIMASK) <= LOG_WARNING ? stderr : stdout;
     fprintf(f, "[%d] ", priority);
     vfprintf(f, format, args2);
     va_end(args2);
     va_end(args1);
 }
 // Quickly replace all syslog() with syslog_printf()
 #define syslog syslog_printf

 /* Network server */
 static volatile int _dsfd = -1;
 static int _server_socket;

 static int
 _setup_domain_socket(const char *sun_path)
 {
     struct sockaddr_un addr;
     int sockfd;

     if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
         syslog(LOG_ERR, "server: can't open stream socket: %s", strerror(errno));
         exit(EXIT_FAILURE);
     }

     /* Set up server address */
     memset((void *) &addr, 0x0, sizeof(addr));
     addr.sun_family = AF_UNIX;
     // Copy 1 char less to make sure the destination is ended with \0
     strncpy(addr.sun_path, sun_path, sizeof(addr.sun_path) - 1);

     /* Remove the domain socket file first */
     unlink(sun_path);
     /* Bind our domain socket */
     if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
         syslog(LOG_ERR, "server: can't bind domain socket: %s", strerror(errno));
         exit(EXIT_FAILURE);
     }

     /* Only process one connection at a time */
     listen(sockfd, 1);

     return sockfd;
 }

 static void *
 _ds2tty(void *arg)
 {
     int fd = *((int *)arg);
     const size_t DATA_SIZE = 1024;
     unsigned char data[DATA_SIZE];
     ssize_t rc;
     struct sockaddr_in addr;
     socklen_t len;

     while (1) {
         if (_dsfd < 0)
         {
             len = sizeof(addr);
             if ((_dsfd = accept(_server_socket, (struct sockaddr *) &addr, &len)) < 0) {
                 syslog(LOG_ERR, "server: can't accept socket: %s", strerror(errno));
                 exit(EXIT_FAILURE);
             }
         }

         rc = read(_dsfd, data, DATA_SIZE);
         if (rc <= 0) {
             if (rc < 0) {
                 /* Broken pipe -- client quit */
                 syslog(LOG_ERR, "_ds2tty broken pipe");
             }
             else {
                 /* Ending connection */
             }
             close(_dsfd);
             _dsfd = -1;
         } else {
             write(fd, data, rc);
             fsync(fd);
         }
     }

     return NULL;
 }

 static void *
 _tty2ds(void *arg)
 {
     int fd = *((int *)arg);
     const size_t DATA_SIZE = 1024;
     unsigned char data[DATA_SIZE];
     ssize_t rc;

     while (1) {
         rc = read(fd, data, DATA_SIZE);
         if (rc <= 0) {
             /* Broken pipe -- app quit */
             syslog(LOG_ERR, "_tty2ds broken pipe");
             close(fd);
             exit(0);
         }
         if (_dsfd >= 0) {
             ssize_t written = write(_dsfd, data, rc);
             // Handle the client exit problem
             if (written < 0) {
                 close(_dsfd);
                 _dsfd = -1;
             }
             fsync(_dsfd);
         }
         else
         {
             /* print orphaned message to the stdout */
             printf("%.*s", (int)rc, data);
             fflush(stdout);
         }
     }
     return NULL;
 }

 static int
 _start_app(char **args, int s)
 {
     pid_t pid;

     if ((pid = fork()) < 0) {
         syslog(LOG_ERR, "fork %s", strerror(errno));
         return -1;
     }
     if (pid > 0) {
         return pid;
     }
     dup2(s, 0);
     dup2(s, 1);
     dup2(s, 2);
     int rc = execv(*args, args);

     syslog(LOG_ERR, "execv program exit, rc=%d, errno=%d: %s\n", rc, errno, strerror(errno));
     exit(0);
 }

 int
 main(int argc, char *argv[])
 {
     int pid;
     int do_fork = 0;
     int rc;
     int ttyfd, appfd;
     pthread_t id;

     auto usage = [=]() {
         const char* prog = argv[0];
         printf("Usage: %s [-d] [-f <sun_path>] <program> [args]\n", prog);
         printf("    -d     Daemon mode\n");
         printf("    -f     Specify the path of unix socket\n");
         printf("Default sun_path: %s\n", DEFAULT_SUN_PATH);
         printf("\n");
         printf("Exit status:\n");
         printf("    0      Both %s and program exit normally in non daemon mode, or %s exits normally in daemon mode\n", prog, prog);
         printf("    1      %s exits with error\n", prog);
         printf("    2      Program exits with non-zero exit status\n");
         printf("    3      Program terminates without an exit status, eg. receiving a signal\n");
         printf("    255    Usage error\n");
         return 255;
     };

     const char *sun_path = DEFAULT_SUN_PATH;
     for (argc--, argv++; argc > 0 && *argv; argc--, argv++) {
         if (!strcmp(*argv, "--help") || !strcmp(*argv, "-h")) {
             usage();
             return 0;
         }
         else if (!strcmp(*argv, "-d")) {
             syslog(LOG_INFO, "daemon mode\n");
             do_fork = 1;
         }
         else if (!strcmp(*argv, "-f")) {
             argc--, argv++;
             if (argc > 1 && *argv) {
                 sun_path = *argv;
             }
             else {
                 fprintf(stderr, "[ERROR] bad domain socket filename\n");
                 return usage();
             }
             syslog(LOG_INFO, "domain socket filename: %s\n", sun_path);
         }
         else break;
     }

     if (do_fork) {
         /* Daemonize */
         if (daemon(1, 1) < 0) {
             syslog(LOG_ERR, "daemon(): %s", strerror(errno));
             exit(EXIT_FAILURE);
         }
     }

     /* Broken pipes are not a problem */
     signal(SIGPIPE, SIG_IGN);

     /* Get a pseudo tty */
     if (openpty(&ttyfd, &appfd, NULL, NULL, NULL) < 0) {
         syslog(LOG_ERR, "open pty: %s", strerror(errno));
         exit(EXIT_FAILURE);
     }

     /* Start the application up with sv[1] as its stdio */
     const char *process_name = argv[0];
     if (process_name == NULL)
     {
         fprintf(stderr, "[ERROR] no program name\n");
         return usage();
     }
     pid = _start_app(argv, appfd);

     /* Setup server */
     _server_socket = _setup_domain_socket(sun_path);

     /* Start proxy for input */
     if ((rc = pthread_create(&id, NULL, _ds2tty, (void *)&ttyfd)) < 0) {
         syslog(LOG_ERR, "pthread_create: %s", strerror(rc));
         exit(EXIT_FAILURE);
     }

     /* Start proxy for output */
     if ((rc = pthread_create(&id, NULL, _tty2ds, (void *)&ttyfd)) < 0) {
         syslog(LOG_ERR, "pthread_create: %s", strerror(rc));
         exit(EXIT_FAILURE);
     }

     /* Wait for our child to exit */
     int status;
     waitpid(pid, &status, 0);
     syslog(LOG_NOTICE, "child %s exited status: %d", process_name, status);

     if (WIFEXITED(status))
     {
         if (WEXITSTATUS(status))
         {
             return 2;
         }
         else
         {
             return 0;
         }
     }
     else
     {
         return 3;
     }
 }
