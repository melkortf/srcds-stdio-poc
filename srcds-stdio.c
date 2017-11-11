#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

const char *TMUX_EXEC = "/usr/bin/tmux";
const char *SRCDS_EXEC = "./srcds_linux";
const char *TMUX_SESSION_NAME = "srcds";
const char *FIFO_PATH = "/tmp/srcds-stdio";
const int CMD_SIZE = 256;

static int tmux_create_session(const char *name)
{
    char cmd[CMD_SIZE];
    snprintf(cmd, CMD_SIZE, "%s new -d -s %s", TMUX_EXEC, name);

    int ret = system(cmd);
    if (ret)
        return ret;

    pid_t pid = getpid();

    snprintf(cmd, CMD_SIZE, "%s pipe-pane -t %s '/usr/bin/cat >> /proc/%d/fd/1'", TMUX_EXEC, name, pid);
    ret = system(cmd);

    return ret;
}

static int tmux_send_keys(const char *session_name, const char* keys)
{
    char cmd[CMD_SIZE];
    snprintf(cmd, CMD_SIZE, "%s send-keys -t %s '%s' C-m", TMUX_EXEC, session_name, keys);
    printf("[ %s ]\n", cmd);

    return system(cmd);
}

static int tmux_kill_session(const char *name)
{
    char cmd[CMD_SIZE];
    snprintf(cmd, CMD_SIZE, "%s kill-session -t %s", TMUX_EXEC, name);

    return system(cmd);
}

/**
 * Pass arguments to srcds_linux.
 */
static char* build_cmd(int argc, char **argv)
{
    char *cmd = malloc(CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "%s ", SRCDS_EXEC);

    for (int i = 1; i < argc; ++i) {
        strcat(cmd, argv[i]);
        strcat(cmd, " ");
    }

    strcat(cmd, "; echo 1 > ");
    strcat(cmd, FIFO_PATH);
    return cmd;
}

int main(int argc, char **argv)
{
    if (access(TMUX_EXEC, F_OK) == -1) {
        fprintf(stderr, "tmux not found\n");
        return -1;
    }

    if (access(SRCDS_EXEC, F_OK) == -1) {
        fprintf(stderr, "srcds_linux not found; "
                "did you forget to cd to your TF2 server installation directory?\n");
        return -1;
    }

    int ret = tmux_create_session(TMUX_SESSION_NAME);
    if (ret) {
        fprintf(stderr, "Could not create tmux session");
        return ret;
    }

    ret = mkfifo(FIFO_PATH, 0666);
    if (ret) {
        fprintf(stderr, "Could not create fifo at %s", FIFO_PATH);
        return ret;
    }

    char *cmd = build_cmd(argc, argv);
    printf("%s\n", cmd);

    ret = tmux_send_keys(TMUX_SESSION_NAME, cmd);
    if (ret) {
        fprintf(stderr, "Could not send keys to session: %s", TMUX_SESSION_NAME);
        return ret;
    }

    sleep(30);
    ret = tmux_send_keys(TMUX_SESSION_NAME, "quit");
    if (ret) {
        fprintf(stderr, "Could not send keys to session: %s", TMUX_SESSION_NAME);
        return ret;
    }

    char buf[16];
    int fd = open(FIFO_PATH, O_RDONLY);
    read(fd, buf, 16);
    close(fd);
    unlink(FIFO_PATH);
    sleep(1);

    printf("\n");

    ret = tmux_kill_session(TMUX_SESSION_NAME);
    if (ret) {
        fprintf(stderr, "Could not kill session: %s", TMUX_SESSION_NAME);
        return ret;
    }

    return 0;
}
