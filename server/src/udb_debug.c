#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <utmp.h>

#include <pthread.h>

uint32_t os_time_get_boot_ms(void)
{
    uint32_t        ret;
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC, &time);
    // printf("CLOCK_MONOTONIC %d, %d\n", time.tv_sec, time.tv_nsec);

    ret = time.tv_sec * 1000 + time.tv_nsec / 1000000;

    return ret;
}

#include "udb_debug.h"

#include "rlog.h"

#define UDB_PROT_SIZE_DEFAULT   (64)
#define UDB_PROT_SIZE_USB1_CTRL (64)
#define UDB_PROT_SIZE_USB1_BULK (64)
#define UDB_PROT_SIZE_USB2_CTRL (64)
#define UDB_PROT_SIZE_USB2_BULK (512)
#define UDB_PROT_SIZE_USB3_BULK (1024)
#define UDB_PROT_SIZE_NET_UDP   (1470)

static int UDB_PROT_SIZE = (UDB_PROT_SIZE_DEFAULT);

/****************************************/
typedef enum
{
    UDB_AUTH = 0x01,

    /* run cmd */
    UDB_RUN_CMD         = 0x02,
    UDB_RUN_CMD_PULLOUT = 0x03,

    UDB_RUN_CMD_WAIT         = 0x04,
    UDB_RUN_CMD_WAIT_PULLOUT = 0x05,

    UDB_MSH_CMD         = 0x06,
    UDB_MSH_CMD_PULLOUT = 0x07,

    /* pull file */
    UDB_PULL_FILE_INIT    = 0x11,
    UDB_PULL_FILE_PULLOUT = 0x12,

    /* push file */
    UDB_PUSH_FILE_INIT = 0x21,
    UDB_PUSH_FILE_PROC = 0x22,
    UDB_PUSH_FILE_FINI = 0x23,

    /* property */

    /*RAW*/
    UDB_RAWSET = 0xfd,  // eu raw buffer
    UDB_RAWGET = 0xfe,  // eu raw buffer

    NONE_CHAN = 0xff
} udb_chan_id_e;

#define UDB_MAX_PKG_COUNT (1024 * 10)
#define UDB_BUF_SIZE      ((UDB_PROT_SIZE_DEFAULT) * (UDB_MAX_PKG_COUNT))

#define UDB_CMD_CHANNEL_ID_OFFSET  (3)
#define UDB_CMD_PKG_CONTENT_OFFSET (4)

/*
void udb_dumphex(void *data, uint32_t size)
{
        char ascii[17];
        unsigned int i, j;
        ascii[16] = '\0';
        for (i = 0; i < size; ++i) {
                printf("%02X ", ((unsigned char*)data)[i]);
                if (((unsigned char*)data)[i] >= ' ' && ((unsigned
char*)data)[i] <= '~') { ascii[i % 16] = ((unsigned char*)data)[i]; } else {
                        ascii[i % 16] = '.';
                }
                if ((i+1) % 8 == 0 || i+1 == size) {
                        printf(" ");
                        if ((i+1) % 16 == 0) {
                                printf("|  %s \n", ascii);
                        } else if (i+1 == size) {
                                ascii[(i+1) % 16] = '\0';
                                if ((i+1) % 16 <= 8) {
                                        printf(" ");
                                }
                                for (j = (i+1) % 16; j < 16; ++j) {
                                        printf("   ");
                                }
                                printf("|  %s \n", ascii);
                        }
                }
        }
}
*/
/*============================================================================*/

#define ARG_MAX_COUNT 256 /* max number of arguments to a command */
/* Type declarations */
struct command
{                              /* a struct that stores a parsed command */
    int   argc;                /* number of arguments in the comand */
    char *name;                /* name of the command */
    char *argv[ARG_MAX_COUNT]; /* the arguments themselves */
};

struct commands
{                              /* struct to store a command pipeline */
    int             cmd_count; /* number of commands in the pipeline */
    struct command *cmds[];    /* the commands themselves */
};

static int is_blank(char *input)
{
    int n = (int)strlen(input);
    int i;

    for (i = 0; i < n; i++)
    {
        if (!isspace(input[i]))
            return 0;
    }
    return 1;
}

static int is_multi_cmd(char *input)
{
    int n = (int)strlen(input);
    int i;

    for (i = 0; i < n; i++)
    {
        if ((input[i]) == ';')
            return 1;
    }
    return 0;
}

/* Parses a single command into a command struct.
 * Allocates memory for keeping the struct and the caller is responsible
 * for freeing up the memory
 */
static struct command *parse_command(char *input)
{
    int   tokenCount = 0;
    char *token;

    /* allocate memory for the cmd structure */
    struct command *cmd =
        calloc(sizeof(struct command) + ARG_MAX_COUNT * sizeof(char *), 1);

    if (cmd == NULL)
    {
        fprintf(stderr, "error: memory alloc error\n");
        exit(EXIT_FAILURE);
    }

    /* get token by splitting on whitespace */
    token = strtok(input, " ");

    while (token != NULL && tokenCount < ARG_MAX_COUNT)
    {
        cmd->argv[tokenCount++] = token;
        token                   = strtok(NULL, " ");
    }
    cmd->name = cmd->argv[0];
    cmd->argc = tokenCount;
    return cmd;
}

/* Parses a command with pipes into a commands* structure.
 * Allocates memory for keeping the struct and the caller is responsible
 * for freeing up the memory
 */
static struct commands *parse_commands_with_pipes(char *input)
{
    int              commandCount = 0;
    int              i            = 0;
    char            *token;
    char            *saveptr;
    char            *c = input;
    struct commands *cmds;

    while (*c != '\0')
    {
        if (*c == '|')
            commandCount++;
        c++;
    }

    commandCount++;

    cmds = calloc(
        sizeof(struct commands) + commandCount * sizeof(struct command *), 1);

    if (cmds == NULL)
    {
        fprintf(stderr, "error: memory alloc error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok_r(input, "|", &saveptr);
    while (token != NULL && i < commandCount)
    {
        cmds->cmds[i++] = parse_command(token);
        token           = strtok_r(NULL, "|", &saveptr);
    }

    cmds->cmd_count = commandCount;
    return cmds;
}

/* Returns whether a command is a built-in. As of now
 * one of [exit, cd, history]
 */
static int check_built_in(struct command *cmd)
{
    return strcmp(cmd->name, "exit") == 0 || strcmp(cmd->name, "cd") == 0;
}

// return 0,not build-in
// return 1, run build-in ok
// return -1,run build-in err
// build-in cmd must be very single
static int uvc_runcmd_input_handle(char *cmd)
{
    int ret;

    char *input = malloc(strlen(cmd));
    memcpy(input, cmd, strlen(cmd));
    input[strlen(cmd) - 1] = 0x0;

    if (strlen(input) > 0 && !is_blank(input) && !is_multi_cmd(input) &&
        input[0] != '|')
    {
        char *linecopy = strdup(input);

        struct commands *commands = parse_commands_with_pipes(input);

        for (int i = 0; i < commands->cmd_count; i++)
        {
            char  *name = commands->cmds[i]->name;
            int    argc = commands->cmds[i]->argc;
            char **argv = commands->cmds[i]->argv;
            // printf("cmd[%d]: %s", i, name);
            for (int j = 0; j < argc; j++)
            {
                // printf(" argv:%d %s size:%d", j, argv[j],strlen(argv[j]));
            }
            // printf("\n");
        }

        if (commands->cmd_count > 1)
        {
            return 0;
        }
        else if (check_built_in(commands->cmds[0]))
        {
            // now we handle it
            struct command *cmd = commands->cmds[0];

            if (strcmp(cmd->name, "exit") == 0)
            {
                printf("exit?\n");
                return 1;
            }

            if (strcmp(cmd->name, "cd") == 0)
            {
                if (cmd->argc < 2)
                {
                    return -1;
                }
                int cmd_len = strlen(cmd->argv[1]);
                if (cmd->argv[1][cmd_len - 1] == '\n')
                {
                    cmd->argv[1][cmd_len - 1] = 0;
                }
                ret = chdir(cmd->argv[1]);
                if (ret != 0)
                {
                    fprintf(stderr, "error: unable to change dir\n");
                    return -1;
                }
                return 1;
            }
        }
        free(linecopy);
    }
    free(input);
    return 0;
}
/*============================================================================*/

/********************************************************************/
/*
 *  popen.c     Written by W. Richard Stevens
 */

static pid_t *childpid = NULL;
/* ptr to array allocated at run-time */
static int maxfd; /* from our open_max(), {Prog openmax} */

static FILE *uvc_popen(char *cmdstring, const char *type, int force_timeout)
{
    int   i, pfd[2];
    pid_t pid;
    FILE *fp;

    /* only allow "r" or "w" */
    if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0)
    {
        errno = EINVAL; /* required by POSIX.2 */
        return (NULL);
    }

    if (childpid == NULL)
    { /* first time through */
        /* allocate zeroed out array for child pids */
        /* OPEN_MAX is canceled after linux-2.6.24, and become a part of
        RLIMIT_NOFILE. Execute shell 'ulimit -a' to get more information */
        struct rlimit limit;
        if (getrlimit(RLIMIT_NOFILE, &limit) == -1)
        {
            perror("getrlimit fail");
            return (NULL);
        }
        maxfd = limit.rlim_cur;
        // print("rlimit = %d.\n", maxfd);
        if ((childpid = calloc(maxfd, sizeof(pid_t))) == NULL)
            return (NULL);
    }

    if (pipe(pfd) < 0)
        return (NULL); /* errno set by pipe() */

#ifdef LOW_MEM_MODE
    if ((pid = vfork()) < 0)
#else
    if ((pid = fork()) < 0)
#endif
        return (NULL); /* errno set by fork() */
    else if (pid == 0)
    { /* child */
        if (*type == 'r')
        {
            close(pfd[0]);
            if (pfd[1] != STDOUT_FILENO)
            {
                dup2(pfd[1], STDOUT_FILENO);
                dup2(pfd[1], STDERR_FILENO);
                close(pfd[1]);
            }
        }
        else
        {
            close(pfd[1]);
            if (pfd[0] != STDIN_FILENO)
            {
                dup2(pfd[0], STDIN_FILENO);
                close(pfd[0]);
            }
        }
        /* close all descriptors in childpid[] */
        for (i = 0; i < maxfd; i++)
            if (childpid[i] > 0)
                close(i);
        if (force_timeout == 1)
        {
            char *cmd = cmdstring;
            if (cmd[0] == '-')
            {  // no timeout
                execlp("/bin/sh", "sh", "-c", cmd + 1, (char *)0);
            }
            else
            {
                execlp("timeout", "timeout", "3", "sh", "-c", cmd, (char *)0);
            }
        }
        else
        {
            execlp("/bin/sh", "sh", "-c", cmdstring, (char *)0);
        }
        _exit(127);
    }
    /* parent */
    if (*type == 'r')
    {
        close(pfd[1]);
        if ((fp = fdopen(pfd[0], type)) == NULL)
            return (NULL);
    }
    else
    {
        close(pfd[0]);
        if ((fp = fdopen(pfd[1], type)) == NULL)
            return (NULL);
    }
    childpid[fileno(fp)] = pid; /* remember child pid for this fd */
    return (fp);
}

static int uvc_pclose(FILE *fp)
{

    int   fd, stat;
    pid_t pid;

    if (childpid == NULL)
        return (-1); /* popen() has never been called */

    if (fp == NULL)
        return (-1); /* fp != NULL !!! */

    fd = fileno(fp);
    if ((pid = childpid[fd]) == 0)
        return (-1); /* fp wasn't opened by popen() */

    childpid[fd] = 0;
    if (fclose(fp) == EOF)
        return (-1);

    while (waitpid(pid, &stat, 0) < 0)
        if (errno != EINTR)
            return (-1); /* error other than EINTR from waitpid() */

    return (stat); /* return child's termination status */
}

/********************************************************************/

/*
runcmd：        malloc buf, runcmd, get output to memory
runcmd_pullout: send mem data to host,when finish, free mem
*/
#ifdef LOW_MEM_MODE
//#define RUMCMD_STATIC_BUF
#else
#define RUMCMD_STATIC_BUF
#endif

#ifdef RUMCMD_STATIC_BUF
char runcmd_buf[UDB_BUF_SIZE];
#endif

struct udb_runcmd_unit
{
    char        *pullout;
    unsigned int total_size;
    unsigned int used_size;
    unsigned int cp_offset;
} udb_runcmd_ctrl;

struct udb_runcmd_wait_unit
{
    char *pullout;

    unsigned char single_instance;

    unsigned char cmd_finish;
    unsigned char pull_finish;

    unsigned int total_size;
    unsigned int used_size;
    unsigned int cp_offset;

    unsigned int tm_last_update;
} udb_runcmd_wait_ctrl;

static pthread_mutex_t udb_runcmd_wait_ctrl_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
init：file_len, path, mode,malloc
pull: pull data to memory
fini: write mem data to file(need??)
*/
struct udb_pullfile_unit
{
    char        *pullout;
    unsigned int used_size;
    unsigned int cp_offset;
} udb_pullfile_ctrl;

/*
init：file_len, path, mode,malloc
push: push data to memory
fini: write mem data to file(need??)
*/
struct udb_pushfile_unit
{
    FILE        *fp;
    unsigned int file_len;
    unsigned int already_wlen;
} udb_pushfile_ctrl;

static void udb_debug_runcmd(char *buf, char *resp, int *resp_len)
{
    FILE        *fp;
    unsigned int nr = 0;
    int          ret;

    char                   *cmd   = buf + UDB_CMD_PKG_CONTENT_OFFSET;
    struct udb_runcmd_unit *pctrl = &udb_runcmd_ctrl;
#ifdef RUMCMD_STATIC_BUF
    pctrl->pullout    = (char *)&runcmd_buf;
    pctrl->total_size = sizeof(runcmd_buf);
#else
    pctrl->pullout    = malloc(UDB_PROT_SIZE * UDB_MAX_PKG_COUNT);
    pctrl->total_size = (UDB_PROT_SIZE * UDB_MAX_PKG_COUNT);
#endif
    // printf("cmd is %s\n",cmd);

    if (cmd == NULL)
    {
        printf("cmd is NULL!\n");
        return;
    }

    // dumphex(buf, UDB_PROT_SIZE);

    ret = uvc_runcmd_input_handle((char *)cmd);
    if (ret > 0)
    {
        char *str = "\n";
        printf("%s", str);
        nr = strlen(str);
        memcpy(pctrl->pullout, str, nr);
        goto exec_finish;
    }
    else if (ret < 0)
    {
        printf("run build-in cmd Err, continue\n");
    }

    if ((fp = uvc_popen((char *)cmd, "r", 1)) == NULL)
    {
        printf("popen error %s: cmd: %s\n", strerror(errno), (char *)cmd);
        memset(pctrl->pullout, 0, pctrl->total_size);
        return;
    }
    else
    {
        memset(pctrl->pullout, 0, pctrl->total_size);
        nr = fread(pctrl->pullout, 1, pctrl->total_size - 1, fp);
        uvc_pclose(fp);
    }

exec_finish:
    // printf("nr is %d\n", nr);
    // printf("#############\n");
    // printf("%s\n", pctrl->pullout);
    // printf("#############\n");

    *((unsigned int *)resp) = nr;
    *resp_len               = UDB_PROT_SIZE;

    pctrl->used_size = nr;
    pctrl->cp_offset = 0;
    return;
}

static void udb_debug_runcmd_pullout(char *buf, char *resp, int *resp_len)
{
    unsigned int cp_size =
        *((unsigned int *)(buf + UDB_CMD_PKG_CONTENT_OFFSET));

    struct udb_runcmd_unit *pctrl = &udb_runcmd_ctrl;

    memset(resp, 0, UDB_PROT_SIZE);

    if (pctrl->cp_offset + cp_size > pctrl->used_size)
    {
        // printf("what?\n");
        *resp_len = UDB_PROT_SIZE;
    }
    else
    {
        memcpy(resp, pctrl->pullout + pctrl->cp_offset, cp_size);
        *resp_len = UDB_PROT_SIZE;
        pctrl->cp_offset += cp_size;
        if (pctrl->cp_offset == pctrl->used_size)
        {
            // finish output
            pctrl->used_size  = 0;
            pctrl->cp_offset  = 0;
            pctrl->total_size = 0;
#ifdef RUMCMD_STATIC_BUF
            memset(runcmd_buf, 0, sizeof(runcmd_buf));
#else
            free(pctrl->pullout);
#endif
        }
    }
}

static void *udb_debug_runcmd_wait_thread(void *arg)
{
    FILE *fp;
    char *cmd = (char *)arg;
    char  thread_cmd[UDB_PROT_SIZE];
    char  buffer[1024];
    int   len;

    udb_runcmd_wait_ctrl.used_size  = 0;
    udb_runcmd_wait_ctrl.cmd_finish = 0;

    memset(buffer, 0, sizeof(buffer));
    memset(thread_cmd, 0, UDB_PROT_SIZE);
    memcpy(thread_cmd, cmd, strlen(cmd));

    // printf("runcmd_wait_thread [%s]\n", thread_cmd);
    pthread_setname_np(pthread_self(), "udb runcmd");

    if ((fp = uvc_popen((char *)thread_cmd, "r", 0)) == NULL)
    {
        printf("popen error: %s/n", strerror(errno));
        goto exit_handle;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        unsigned int tm_curr = os_time_get_boot_ms();
        unsigned int tm_diff =
            tm_curr > udb_runcmd_wait_ctrl.tm_last_update
                ? (tm_curr - udb_runcmd_wait_ctrl.tm_last_update)
                : (udb_runcmd_wait_ctrl.tm_last_update - tm_curr);
        if (tm_diff > 300)
        {
            printf("udb: [%s] timeout\n", thread_cmd);
            uvc_pclose(fp);
            printf("udb: [%s] close done\n", thread_cmd);
            goto exit_handle;
        }

        len = strlen(buffer) + 1;
        // printf("%s", buffer);
        memcpy(udb_runcmd_wait_ctrl.pullout + udb_runcmd_wait_ctrl.used_size,
               buffer, len);
        udb_runcmd_wait_ctrl.used_size += len;
    };

    uvc_pclose(fp);

    udb_runcmd_wait_ctrl.cmd_finish = 1;
    // printf("udb_runcmd_wait_ctrl.used_size %d\n",
    // udb_runcmd_wait_ctrl.used_size); dumphex(udb_runcmd_wait_ctrl.pullout,
    // udb_runcmd_wait_ctrl.used_size);

exit_handle:
    pthread_mutex_lock(&udb_runcmd_wait_ctrl_mutex);
    udb_runcmd_wait_ctrl.single_instance = 0;
    pthread_mutex_unlock(&udb_runcmd_wait_ctrl_mutex);

    return NULL;
}

static void udb_debug_runcmd_wait(char *buf, char *resp, int *resp_len)
{
    FILE        *fp;
    unsigned int nr = 0;
    int          ret;

    char                        *cmd   = buf + UDB_CMD_PKG_CONTENT_OFFSET;
    struct udb_runcmd_wait_unit *pctrl = &udb_runcmd_wait_ctrl;

    if (pctrl->single_instance == 1)
    {
        resp[0]   = 0;  // no support flag
        *resp_len = UDB_PROT_SIZE;
        printf("runcmd: [%s] error\n", cmd);
        printf("runcmd: only support single instance\n");
        return;
    }

#ifdef RUMCMD_STATIC_BUF
    pctrl->pullout    = (char *)runcmd_buf;
    pctrl->total_size = sizeof(runcmd_buf);
#else
    pctrl->pullout    = malloc(UDB_PROT_SIZE * UDB_MAX_PKG_COUNT);
    pctrl->total_size = (UDB_PROT_SIZE * UDB_MAX_PKG_COUNT);
#endif

    if (cmd == NULL)
    {
        printf("cmd is NULL!\n");
        resp[0]   = 0;
        *resp_len = UDB_PROT_SIZE;
        return;
    }

    pthread_mutex_lock(&udb_runcmd_wait_ctrl_mutex);
    pctrl->single_instance = 1;
    pthread_mutex_unlock(&udb_runcmd_wait_ctrl_mutex);

    pctrl->tm_last_update = os_time_get_boot_ms();

    pthread_t thread_id;

    pthread_create(&thread_id, NULL, (void *)udb_debug_runcmd_wait_thread,
                   (void *)cmd);

    pthread_detach(thread_id);

    resp[0]   = 1;  // support flag
    *resp_len = UDB_PROT_SIZE;

    pctrl->cp_offset = 0;

    return;
}

static void udb_debug_runcmd_wait_pullout(char *buf, char *resp, int *resp_len)
{
    unsigned int cp_size = UDB_PROT_SIZE - sizeof(unsigned char);  // state

    struct udb_runcmd_wait_unit *pctrl = &udb_runcmd_wait_ctrl;

    memset(resp, 0, UDB_PROT_SIZE);

    pctrl->pull_finish = 0;

    if (pctrl->cmd_finish == 1)
    {
        if (pctrl->cp_offset + cp_size > pctrl->used_size)
        {
            pctrl->pull_finish = 1;
            cp_size            = pctrl->used_size - pctrl->cp_offset;
        }
    }
    else
    {  // not finish
        if (pctrl->cp_offset + cp_size > pctrl->used_size)
        {
            cp_size = pctrl->used_size - pctrl->cp_offset;
        }
    }

    resp[0]   = pctrl->pull_finish;
    *resp_len = UDB_PROT_SIZE;

    if (cp_size >= pctrl->used_size - pctrl->cp_offset)
    {
        cp_size = pctrl->used_size - pctrl->cp_offset;
    }

    if (cp_size > strlen((char *)(pctrl->pullout + pctrl->cp_offset)) + 1)
    {
        cp_size = strlen((char *)(pctrl->pullout + pctrl->cp_offset)) + 1;
    }

    if (cp_size != 0)
    {
        memcpy(resp + 1, pctrl->pullout + pctrl->cp_offset, cp_size);
        // printf("cp_size is %d\n", cp_size);
        // dumphex(presp + 1, cp_size);
        pctrl->cp_offset += cp_size;
    }

    if (pctrl->pull_finish == 1)
    {
        // finish output
        pctrl->used_size  = 0;
        pctrl->cp_offset  = 0;
        pctrl->total_size = 0;

#ifdef RUMCMD_STATIC_BUF
        memset(runcmd_buf, 0, sizeof(runcmd_buf));
#else
        free(pctrl->pullout);
#endif
    }

    pctrl->tm_last_update = os_time_get_boot_ms();

    return;
}

static void udb_debug_pullfile_init(char *buf, char *resp, int *resp_len)
{
    FILE        *fp;
    unsigned int nr = 0;
    struct stat  st;

    const char               *path  = (char *)(buf + 4);
    struct udb_pullfile_unit *pctrl = &udb_pullfile_ctrl;

    if ((fp = fopen(path, "r")) == NULL)
    {
        printf("Error opening input file %s\n", path);
        pctrl->used_size = 0;
        pctrl->cp_offset = 0;
        pctrl->pullout   = NULL;
    }
    else
    {
        fstat(fileno(fp), &st);
        pctrl->used_size = (unsigned int)st.st_size;
        printf("%s size is %d\n", path, (int)st.st_size);
        pctrl->cp_offset = 0;
        if (pctrl->pullout != NULL)
        {
            free(pctrl->pullout);
        }
        pctrl->pullout = malloc(st.st_size);

        int ret = fread(pctrl->pullout, 1, st.st_size, fp);
        fclose(fp);
    }

    *((unsigned int *)resp) = pctrl->used_size;
    *resp_len               = UDB_PROT_SIZE;
}

static void udb_debug_pullfile_pullout(char *buf, char *resp, int *resp_len)
{
    unsigned int cp_size =
        *((unsigned int *)(buf + UDB_CMD_PKG_CONTENT_OFFSET));

    memset(resp, 0, UDB_PROT_SIZE);

    struct udb_pullfile_unit *pctrl = &udb_pullfile_ctrl;

    if (pctrl->cp_offset + cp_size > pctrl->used_size)
    {
        // printf("what?\n");
        *resp_len = UDB_PROT_SIZE;
    }
    else
    {
        memcpy(resp, pctrl->pullout + pctrl->cp_offset, cp_size);
        *resp_len = UDB_PROT_SIZE;
        pctrl->cp_offset += cp_size;

        if (pctrl->cp_offset == pctrl->used_size)
        {
            // finish output
            // printf("finish?\n");
            pctrl->used_size = 0;
            pctrl->cp_offset = 0;
            free(pctrl->pullout);
            pctrl->pullout = NULL;
        }
    }
}

static void udb_debug_pushfile_init(char *buf, char *resp, int *resp_len)
{
    unsigned int file_len = 0;

    struct udb_pushfile_unit *pctrl = &udb_pushfile_ctrl;

    if (pctrl->fp != NULL)
    {
        fclose(pctrl->fp);
    }

    const char *path = (char *)(buf + 8);

    if ((pctrl->fp = fopen(path, "wb")) == NULL)
    {
        printf("Error opening output file %s\n", path);
        resp[0]   = 'I';
        resp[1]   = 'N';
        resp[2]   = 'I';
        resp[3]   = 'T';
        resp[4]   = 'N';
        resp[5]   = 'O';
        *resp_len = UDB_PROT_SIZE;
    }
    else
    {
        file_len = *((unsigned int *)(buf + UDB_CMD_PKG_CONTENT_OFFSET));
        printf("%s size is %d, 0x%x\n", path, file_len, file_len);
        resp[0]   = 'I';
        resp[1]   = 'N';
        resp[2]   = 'I';
        resp[3]   = 'T';
        resp[4]   = 'O';
        resp[5]   = 'K';
        *resp_len = UDB_PROT_SIZE;
    }
}

static void udb_debug_pushfile_proc(char *buf, char *resp, int *resp_len)
{
    unsigned int nr = 0;
    int          ret;

    struct udb_pushfile_unit *pctrl = &udb_pushfile_ctrl;

    if (UDB_PROT_SIZE == UDB_PROT_SIZE_DEFAULT)
    {
        unsigned char wrlen = buf[UDB_CMD_PKG_CONTENT_OFFSET];
        fwrite(buf + 5, wrlen, 1, pctrl->fp);
    }
    else
    {
        unsigned short wrlen =
            *(unsigned short *)(buf + UDB_CMD_PKG_CONTENT_OFFSET);
        fwrite(buf + 6, wrlen, 1, pctrl->fp);
    }

    resp[0]   = 'P';
    resp[1]   = 'R';
    resp[2]   = 'O';
    resp[3]   = 'C';
    resp[4]   = 'O';
    resp[5]   = 'K';
    *resp_len = UDB_PROT_SIZE;
}

static void udb_debug_pushfile_fini(char *buf, char *resp, int *resp_len)
{
    unsigned int nr = 0;
    int          ret;

    struct udb_pushfile_unit *pctrl = &udb_pushfile_ctrl;

    fclose(pctrl->fp);
    pctrl->fp = NULL;

    resp[0]   = 'F';
    resp[1]   = 'I';
    resp[2]   = 'N';
    resp[3]   = 'I';
    resp[4]   = 'O';
    resp[5]   = 'K';
    *resp_len = UDB_PROT_SIZE;
}

static int udb_auth_ok = 0;

static void udb_debug_auth(char *buf, char *resp, int *resp_len)
{
    if (strncmp((buf + 4), "0303060303070701", 16) == 0)
    {
        udb_auth_ok = 1;
        resp[0]     = 'A';
        resp[1]     = 'U';
        resp[2]     = 'T';
        resp[3]     = 'H';
        resp[4]     = 'O';
        resp[5]     = 'K';
        *resp_len   = UDB_PROT_SIZE;
    }
    else
    {
        resp[0]   = 'A';
        resp[1]   = 'U';
        resp[2]   = 'T';
        resp[3]   = 'H';
        resp[4]   = 'N';
        resp[5]   = 'O';
        *resp_len = UDB_PROT_SIZE;
    }
}

char mshcmd_buf[512];

struct udb_mshcmd_unit
{
    char        *pullout;
    unsigned int total_size;
    unsigned int used_size;
    unsigned int cp_offset;
} udb_mshcmd_ctrl;

static void udb_debug_msh_cmd_help(char *buf, char *resp, int *resp_len)
{
    struct udb_mshcmd_unit *pctrl = &udb_mshcmd_ctrl;

    const char *help_msg =
        "not support msh cmd\n"
        "\n";

    int nr = strlen(help_msg);

    memcpy(pctrl->pullout, help_msg, nr);

    pctrl->used_size = nr;
    pctrl->cp_offset = 0;

    *((unsigned int *)resp) = nr;
    *resp_len               = UDB_PROT_SIZE;

    return;
}

static void udb_debug_msh_cmd_log(char *buf, char *resp, int *resp_len)
{
    struct udb_mshcmd_unit *pctrl = &udb_mshcmd_ctrl;

    int nr = rlog_size();

    rlog_dump(pctrl->pullout, nr, 0);

    pctrl->used_size = nr;

    pctrl->cp_offset = 0;

    *((unsigned int *)resp) = nr;
    *resp_len               = UDB_PROT_SIZE;
}

static void udb_debug_msh(char *buf, char *resp, int *resp_len)
{
    char *cmd = (buf + UDB_CMD_PKG_CONTENT_OFFSET);

    struct udb_mshcmd_unit *pctrl = &udb_mshcmd_ctrl;

    pctrl->pullout    = (char *)mshcmd_buf;
    pctrl->total_size = sizeof(mshcmd_buf);

    memset(pctrl->pullout, 0, pctrl->total_size);

    if (*cmd == 0x00)
    {
        udb_debug_msh_cmd_help(buf, resp, resp_len);
    }
    else
    {
        if (strncmp(cmd, "log", 3) == 0)
        {
            udb_debug_msh_cmd_log(buf, resp, resp_len);
        }
    }

    *resp_len = UDB_PROT_SIZE;

    return;
}

static void udb_debug_msh_pullout(char *buf, char *resp, int *resp_len)
{
    unsigned int cp_size =
        *((unsigned int *)(buf + UDB_CMD_PKG_CONTENT_OFFSET));

    struct udb_mshcmd_unit *pctrl = &udb_mshcmd_ctrl;

    memset(resp, 0, UDB_PROT_SIZE);

    if (pctrl->cp_offset + cp_size > pctrl->used_size)
    {
        *resp_len = UDB_PROT_SIZE;
    }
    else
    {
        memcpy(resp, pctrl->pullout + pctrl->cp_offset, cp_size);
        *resp_len = UDB_PROT_SIZE;
        pctrl->cp_offset += cp_size;
        if (pctrl->cp_offset == pctrl->used_size)
        {
            // finish output
            pctrl->used_size  = 0;
            pctrl->cp_offset  = 0;
            pctrl->total_size = 0;

            memset(mshcmd_buf, 0, sizeof(mshcmd_buf));
        }
    }
}

void udb_debug_bridge(char layer, char *cmd, char *resp, int *resp_len)
{
    switch (layer)
    {
        case UDB_LAYER_DEF:
            UDB_PROT_SIZE = UDB_PROT_SIZE_DEFAULT;
            break;
        case UDB_LAYER_UVC_HS:
            UDB_PROT_SIZE = UDB_PROT_SIZE_USB2_CTRL;
            break;
        case UDB_LAYER_CDC_HS:
            UDB_PROT_SIZE = UDB_PROT_SIZE_USB2_BULK;
            break;
        case UDB_LAYER_NET:
            UDB_PROT_SIZE = UDB_PROT_SIZE_NET_UDP;
            break;
        default:
            UDB_PROT_SIZE = UDB_PROT_SIZE_DEFAULT;
    }

    char *udb_cmd = cmd;

    char chan_id = udb_cmd[UDB_CMD_CHANNEL_ID_OFFSET];

    // printf("chan_id os %x\n", chan_id);
    if (chan_id == UDB_AUTH)
    {
        udb_debug_auth(cmd, resp, resp_len);
        return;
    }
    if (!udb_auth_ok)
    {
        return;
    }

    switch (chan_id)
    {
        case UDB_RUN_CMD:
            udb_debug_runcmd(cmd, resp, resp_len);
            break;
        case UDB_RUN_CMD_PULLOUT:
            udb_debug_runcmd_pullout(cmd, resp, resp_len);
            break;
        case UDB_RUN_CMD_WAIT:
            udb_debug_runcmd_wait(cmd, resp, resp_len);
            break;
        case UDB_RUN_CMD_WAIT_PULLOUT:
            udb_debug_runcmd_wait_pullout(cmd, resp, resp_len);
            break;

        case UDB_PULL_FILE_INIT:
            udb_debug_pullfile_init(cmd, resp, resp_len);
            break;
        case UDB_PULL_FILE_PULLOUT:
            udb_debug_pullfile_pullout(cmd, resp, resp_len);
            break;

        case UDB_PUSH_FILE_INIT:
            udb_debug_pushfile_init(cmd, resp, resp_len);
            break;
        case UDB_PUSH_FILE_PROC:
            udb_debug_pushfile_proc(cmd, resp, resp_len);
            break;
        case UDB_PUSH_FILE_FINI:
            udb_debug_pushfile_fini(cmd, resp, resp_len);
            break;
        case UDB_MSH_CMD:
            udb_debug_msh(cmd, resp, resp_len);
            break;
        case UDB_MSH_CMD_PULLOUT:
            udb_debug_msh_pullout(cmd, resp, resp_len);
            break;
        default:
            memset(resp, 0, UDB_PROT_SIZE);
            *resp_len = UDB_PROT_SIZE;
            printf("unknow cmd\n");
    }

    return;
}
