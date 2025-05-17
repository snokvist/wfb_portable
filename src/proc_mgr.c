#define _POSIX_C_SOURCE 200809L
#include "proc_mgr.h"
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static void child_sig(int sig) { (void)sig; }

int proc_spawn_and_watch(const char *cmdline)
{
    struct sigaction sa = { .sa_handler = child_sig };
    sigaction(SIGCHLD, &sa, NULL);

    for (;;) {
        pid_t pid = fork();
        if (pid == 0) {
            /* child */
            execl("/bin/sh", "sh", "-c", cmdline, (char*)NULL);
            _exit(127);   /* exec failed */
        }
        if (pid < 0) { perror("fork"); return -1; }

        int status;
        waitpid(pid, &status, 0);
        fprintf(stderr, "[proc_mgr] '%s' exited (status=%d); restart in 4s\n",
                cmdline, status);
        sleep(4);
    }
}
