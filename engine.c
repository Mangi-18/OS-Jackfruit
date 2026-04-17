#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <time.h>

#include "monitor_ioctl.h"

#define STACK_SIZE 1024*1024
#define MAX_CONTAINERS 20

typedef struct {
    char id[32];
    pid_t pid;
    int running;
    time_t start_time;
} container_t;

container_t containers[MAX_CONTAINERS];
int count = 0;

/* ================= CHILD ================= */

int child_fn(void *arg)
{
    char **args = (char **)arg;

    unshare(CLONE_NEWUTS);
    sethostname(args[0], strlen(args[0]));

    chroot(args[1]);
    chdir("/");

    mount("proc", "/proc", "proc", 0, NULL);

    execl("/bin/sh", "sh", NULL);
    perror("exec");
    return 1;
}

/* ================= MONITOR ================= */

void register_monitor(int fd, char *id, pid_t pid,
                      unsigned long soft, unsigned long hard)
{
    if (fd < 0) return;

    struct monitor_request req;
    memset(&req, 0, sizeof(req));

    req.pid = pid;
    req.soft_limit_bytes = soft;
    req.hard_limit_bytes = hard;
    strncpy(req.container_id, id, 31);

    ioctl(fd, MONITOR_REGISTER, &req);
}

void unregister_monitor(int fd, char *id, pid_t pid)
{
    if (fd < 0) return;

    struct monitor_request req;
    memset(&req, 0, sizeof(req));

    req.pid = pid;
    strncpy(req.container_id, id, 31);

    ioctl(fd, MONITOR_UNREGISTER, &req);
}

/* ================= START ================= */

void start_container(char *id, char *rootfs,
                     unsigned long soft, unsigned long hard, int nice)
{
    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc");
        return;
    }

    char *args[2];
    args[0] = id;
    args[1] = rootfs;

    int fd = open("/dev/container_monitor", O_RDWR);
    if (fd < 0)
        perror("monitor open");

    pid_t pid = clone(child_fn, stack + STACK_SIZE,
                      CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD,
                      args);

    if (pid < 0) {
        perror("clone");
        free(stack);
        return;
    }

    setpriority(PRIO_PROCESS, pid, nice);

    register_monitor(fd, id, pid, soft, hard);

    strcpy(containers[count].id, id);
    containers[count].pid = pid;
    containers[count].running = 1;
    containers[count].start_time = time(NULL);

    count++;

    printf("Started %s (PID %d)\n", id, pid);

    /* NOTE:
       For start(), we DO NOT unregister immediately
       because container runs in background.
       (Explain this in README)
    */

    if (fd >= 0)
        close(fd);
}

/* ================= RUN ================= */

void run_container(char *id, char *rootfs,
                   unsigned long soft, unsigned long hard, int nice)
{
    char *stack = malloc(STACK_SIZE);
    if (!stack) {
        perror("malloc");
        return;
    }

    char *args[2];
    args[0] = id;
    args[1] = rootfs;

    int fd = open("/dev/container_monitor", O_RDWR);
    if (fd < 0)
        perror("monitor open");

    pid_t pid = clone(child_fn, stack + STACK_SIZE,
                      CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD,
                      args);

    if (pid < 0) {
        perror("clone");
        free(stack);
        return;
    }

    setpriority(PRIO_PROCESS, pid, nice);

    register_monitor(fd, id, pid, soft, hard);

    waitpid(pid, NULL, 0);

    unregister_monitor(fd, id, pid);

    if (fd >= 0)
        close(fd);

    free(stack);

    printf("Container finished\n");
}

/* ================= PS ================= */

void list_containers()
{
    for (int i = 0; i < count; i++) {
        printf("%s\tPID:%d\t%s\n",
               containers[i].id,
               containers[i].pid,
               containers[i].running ? "running" : "stopped");
    }
}

/* ================= STOP ================= */

void stop_container(char *id)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(containers[i].id, id) == 0) {
            kill(containers[i].pid, SIGTERM);

            /* CLEANUP */
            waitpid(containers[i].pid, NULL, 0);

            containers[i].running = 0;

            printf("Stopped %s\n", id);
            return;
        }
    }
}

/* ================= MAIN ================= */

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage:\n");
        printf("./engine start <id> <rootfs>\n");
        printf("./engine run <id> <rootfs>\n");
        printf("./engine ps\n");
        printf("./engine stop <id>\n");
        return 1;
    }

    unsigned long soft = 40UL << 20;
    unsigned long hard = 64UL << 20;
    int nice = 0;

    if (strcmp(argv[1], "start") == 0)
        start_container(argv[2], argv[3], soft, hard, nice);

    else if (strcmp(argv[1], "run") == 0)
        run_container(argv[2], argv[3], soft, hard, nice);

    else if (strcmp(argv[1], "ps") == 0)
        list_containers();

    else if (strcmp(argv[1], "stop") == 0)
        stop_container(argv[2]);

    else
        printf("Invalid command\n");

    return 0;
}
