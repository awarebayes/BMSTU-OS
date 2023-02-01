#include "apue.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>

#define _GNU_SOURCE

#define LOCKFILE "/var/run/chroot_daemon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    return fcntl(fd, F_SETLK, &fl);
}

int already_running(void)
{
    int fd;
    char buf[16];
    fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);

    if (fd < 0)
    {
        syslog(LOG_ERR, "Cannot open %s: %s",
               LOCKFILE, strerror(errno));
        exit(1);
    }

    if (lockfile(fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }

        syslog(LOG_ERR, "Cannot set a lock on %s: %s",
               LOCKFILE, strerror(errno));

        exit(1);
    }

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return 0;
}

void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        err_quit("%s: it is impossible to get the maximum descriptor number ", cmd);

    if ((pid = fork()) < 0)
        err_quit("%s: fork function call error", cmd);
    else if (pid != 0)
        exit(0);

    setsid();

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) < 0)
        err_quit("%s: cannot ignore SIGHUP signal", cmd);

    if (chdir("/") < 0)
        err_quit("%s: cannot be made the current working directory /", cmd);

    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    
    for (i = 0; i < rl.rlim_max; i++)
        close(i);

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    openlog(cmd, LOG_CONS, LOG_DAEMON);

    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "invalid file descriptors %d %d %d", fd0, fd1, fd2);

        exit(1);
    }
}

int main(void)
{
    time_t cur_time;
    daemonize("getlogd");
    char *username = getlogin();

    if (!username)
        syslog(LOG_NOTICE, "Process hasn't username\n");
    else
        syslog(LOG_NOTICE, "Username: %s\n", username);
    
    while (1)
    {
        cur_time = time(NULL);
        syslog(LOG_NOTICE, "Time: %s", ctime(&cur_time));
        sleep(10);
    }

    return 0;
}