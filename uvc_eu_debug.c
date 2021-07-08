#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include <sys/ioctl.h>
#include <utmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <ctype.h>
#include <inttypes.h> 
#include <stdint.h>

#include "uvc_eu_debug.h"

#define UDB_RUNCMD_FORCE_TIMEOUT

typedef struct udb_response_t {
	unsigned char data[64];
	unsigned char length;
}udb_resp;


/****************************************/
enum udb_chan_id {
	UDB_AUTH           = 0x01,

	/* run cmd */
	UDB_RUN_CMD        = 0x02,
	UDB_RUN_CMD_PULLOUT = 0x03,


	/* pull file */
	UDB_PULL_FILE_INIT      = 0x11,
	UDB_PULL_FILE_PULLOUT   = 0x12,

	/* push file */
	UDB_PUSH_FILE_INIT      = 0x21,
	UDB_PUSH_FILE_PROC      = 0x22,
	UDB_PUSH_FILE_FINI		= 0x23,

	/* property */

	/*RAW*/
	UDB_RAWSET        = 0xfd, //eu raw buffer 
	UDB_RAWGET        = 0xfe, //eu raw buffer 

	NONE_CHAN      = 0xff
};


#define EU_MAX_PKG_SIZE    (64)
#define EU_MAX_PKG_COUNT   (1024 * 4)
#define EU_BUF_SIZE        ((EU_MAX_PKG_SIZE)*(EU_MAX_PKG_COUNT))

#define EU_CMD_CHANNEL_ID_OFFSET       (3)
#define EU_CMD_PKG_CONTENT_OFFSET      (4)


void udb_dumphex(void *data, uint32_t size)
{
	char ascii[17];
	unsigned int i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
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

/*============================================================================*/

#define ARG_MAX_COUNT    EU_MAX_PKG_SIZE     /* max number of arguments to a command */
/* Type declarations */
struct command {		   /* a struct that stores a parsed command */
	int argc;                  /* number of arguments in the comand */
	char *name;                /* name of the command */
	char *argv[ARG_MAX_COUNT]; /* the arguments themselves */
};

struct commands {                  /* struct to store a command pipeline */
	int cmd_count;             /* number of commands in the pipeline */
	struct command *cmds[];    /* the commands themselves */
};

static int is_blank(char *input)
{
	int n = (int) strlen(input);
	int i;

	for (i = 0; i < n; i++) {
		if (!isspace(input[i]))
			return 0;
	}
	return 1;
}

static int is_multi_cmd(char *input)
{
	int n = (int) strlen(input);
	int i;

	for (i = 0; i < n; i++) {
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
	int tokenCount = 0;
	char *token;

	/* allocate memory for the cmd structure */
	struct command *cmd = calloc(sizeof(struct command) +
				     ARG_MAX_COUNT * sizeof(char *), 1);

	if (cmd == NULL) {
		fprintf(stderr, "error: memory alloc error\n");
		exit(EXIT_FAILURE);
	}

	/* get token by splitting on whitespace */
	token = strtok(input, " ");

	while (token != NULL && tokenCount < ARG_MAX_COUNT) {
		cmd->argv[tokenCount++] = token;
		token = strtok(NULL, " ");
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
	int commandCount = 0;
	int i = 0;
	char *token;
	char *saveptr;
	char *c = input;
	struct commands *cmds;

	while (*c != '\0') {
		if (*c == '|')
			commandCount++;
		c++;
	}

	commandCount++;

	cmds = calloc(sizeof(struct commands) +
		      commandCount * sizeof(struct command *), 1);

	if (cmds == NULL) {
		fprintf(stderr, "error: memory alloc error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok_r(input, "|", &saveptr);
	while (token != NULL && i < commandCount) {
		cmds->cmds[i++] = parse_command(token);
		token = strtok_r(NULL, "|", &saveptr);
	}

	cmds->cmd_count = commandCount;
	return cmds;
}

/* Returns whether a command is a built-in. As of now
 * one of [exit, cd, history]
 */
static int check_built_in(struct command *cmd)
{
	return strcmp(cmd->name, "exit") == 0 ||
		strcmp(cmd->name, "cd") == 0;
}

//return 0,not build-in
//return 1, run build-in ok
//return -1,run build-in err
//build-in cmd must be very single
static int uvc_runcmd_input_handle(char *cmd)
{
	int ret;
	
	char *input = malloc(strlen(cmd));
	memcpy(input, cmd, strlen(cmd));
	input[strlen(cmd) - 1] = 0x0;

	if (strlen(input) > 0 && !is_blank(input) && !is_multi_cmd(input) && input[0] != '|') {
		char *linecopy = strdup(input);

		struct commands *commands =
			parse_commands_with_pipes(input);

		for(int i=0;i<commands->cmd_count;i++) {
			char *name = commands->cmds[i]->name;
			int argc = commands->cmds[i]->argc;
			char **argv = commands->cmds[i]->argv;
			//printf("cmd[%d]: %s", i, name);
			for(int j=0;j<argc;j++) {
				//printf(" argv:%d %s size:%d", j, argv[j],strlen(argv[j]));
			}
			//printf("\n");
		}

		if (commands->cmd_count > 1) {
			return 0;
		} else if(check_built_in(commands->cmds[0])) {
			//now we handle it
			struct command *cmd = commands->cmds[0];

			if (strcmp(cmd->name, "exit") == 0){
				printf("exit?\n");
				return 1;
			}
			
			if (strcmp(cmd->name, "cd") == 0) {
				if(cmd->argc < 2) {
					return -1;
				} 
				int cmd_len = strlen(cmd->argv[1]);
				if(cmd->argv[1][cmd_len - 1] == '\n'){
					cmd->argv[1][cmd_len - 1] = 0;
				}
				ret = chdir(cmd->argv[1]);
				if (ret != 0) {
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

static pid_t    *childpid = NULL;  
                        /* ptr to array allocated at run-time */  
static int      maxfd;  /* from our open_max(), {Prog openmax} */  

static FILE *  
uvc_popen(const char *cmdstring, const char *type)  
{  
    int     i, pfd[2];  
    pid_t   pid;  
    FILE    *fp;  
  
            /* only allow "r" or "w" */  
    if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0) {  
        errno = EINVAL;     /* required by POSIX.2 */  
        return(NULL);  
    }  
  
    if (childpid == NULL) {     /* first time through */  
                /* allocate zeroed out array for child pids */  
        /* OPEN_MAX is canceled after linux-2.6.24, and become a part of RLIMIT_NOFILE. 
        Execute shell 'ulimit -a' to get more information */
        struct rlimit limit;
        if(getrlimit(RLIMIT_NOFILE, &limit) == -1)
        {
            perror("getrlimit fail");
            return(NULL);
        }
        maxfd = limit.rlim_cur;
        //print("rlimit = %d.\n", maxfd); 
        if ( (childpid = calloc(maxfd, sizeof(pid_t))) == NULL)  
            return(NULL);  
    }  
  
    if (pipe(pfd) < 0)  
        return(NULL);   /* errno set by pipe() */  
  
    if ( (pid = fork()) < 0)  
        return(NULL);   /* errno set by fork() */  
    else if (pid == 0) {                            /* child */  
        if (*type == 'r') {  
            close(pfd[0]);  
            if (pfd[1] != STDOUT_FILENO) {  
                dup2(pfd[1], STDOUT_FILENO);
				dup2(pfd[1], STDERR_FILENO);  
                close(pfd[1]);  
            }  
        } else {  
            close(pfd[1]);  
            if (pfd[0] != STDIN_FILENO) {  
                dup2(pfd[0], STDIN_FILENO);  
                close(pfd[0]);  
            }  
        }  
            /* close all descriptors in childpid[] */  
        for (i = 0; i < maxfd; i++)  
            if (childpid[ i ] > 0)  
                close(i);  
#ifdef UDB_RUNCMD_FORCE_TIMEOUT //MUST be gnu timeout coreutils
		execlp("timeout" , "timeout", "3", "sh", "-c" ,cmdstring, (char *) 0);
#else
        execlp("/bin/sh" , "sh", "-c", cmdstring, (char *) 0);  
#endif
        _exit(127);  
    }  
                                /* parent */  
    if (*type == 'r') {  
        close(pfd[1]);  
        if ( (fp = fdopen(pfd[0], type)) == NULL)  
            return(NULL);  
    } else {  
        close(pfd[0]);  
        if ( (fp = fdopen(pfd[1], type)) == NULL)  
            return(NULL);  
    }  
    childpid[fileno(fp)] = pid; /* remember child pid for this fd */  
    return(fp);  
}  
  
static int  
uvc_pclose(FILE *fp)  
{  
  
    int     fd, stat;  
    pid_t   pid;  
  
    if (childpid == NULL)  
        return(-1);     /* popen() has never been called */  
  
    fd = fileno(fp);  
    if ( (pid = childpid[fd]) == 0)  
        return(-1);     /* fp wasn't opened by popen() */  
  
    childpid[fd] = 0;  
    if (fclose(fp) == EOF)  
        return(-1);  
  
    while (waitpid(pid, &stat, 0) < 0)  
        if (errno != EINTR)  
            return(-1); /* error other than EINTR from waitpid() */  
  
    return(stat);   /* return child's termination status */  
}


/********************************************************************/


/*
runcmd£º        malloc buf, runcmd, get output to memory
runcmd_pullout: send mem data to host,when finish, free mem
*/
#define RUMCMD_STATICBUF

#ifdef RUMCMD_STATIC_BUF
unsigned char runcmd_buf[EU_BUF_SIZE];
#endif

struct udb_runcmd_unit {
	unsigned char *pullout;
	unsigned int total_size;
	unsigned int used_size;
	unsigned int cp_offset;
}udb_runcmd_ctrl;

/*
init£ºfile_len, path, mode,malloc
pull: pull data to memory
fini: write mem data to file(need??)
*/
struct udb_pullfile_unit {
	unsigned char *pullout;
	unsigned int  used_size;
	unsigned int  cp_offset;
}udb_pullfile_ctrl;


/*
init£ºfile_len, path, mode,malloc
push: push data to memory
fini: write mem data to file(need??)
*/
struct udb_pushfile_unit {
	FILE          *fp;
	unsigned int  file_len;
	unsigned int  already_wlen;
}udb_pushfile_ctrl;


static void uvc_eu_debug_runcmd(unsigned char *buf, udb_resp *resp)
{
	FILE *fp;
	unsigned int nr = 0;
	int ret;
	
	unsigned char  *presp = &resp->data[0];
	unsigned char  *plen = &resp->length;
	
	unsigned char *cmd = buf + EU_CMD_PKG_CONTENT_OFFSET;
	struct udb_runcmd_unit *pctrl = &udb_runcmd_ctrl;
#ifdef RUMCMD_STATIC_BUF
	pctrl->pullout = &runcmd_buf;
	pctrl->total_size = sizeof(runcmd_buf);
#else
	pctrl->pullout = malloc(EU_MAX_PKG_SIZE * EU_MAX_PKG_COUNT);
	pctrl->total_size = (EU_MAX_PKG_SIZE * EU_MAX_PKG_COUNT);
#endif
	//printf("cmd is %s\n",cmd);

	if (cmd == NULL){
		printf("cmd is NULL!\n");
		return;
	}

	//udb_dumphex(buf, EU_MAX_PKG_SIZE);

	ret = uvc_runcmd_input_handle((char *)cmd);
	if (ret > 0) {
		char *str = "\n";
		printf("%s", str);
		nr = strlen(str);
		memcpy(pctrl->pullout, str, nr);
		goto exec_finish;
	} else if (ret < 0) {
		printf("run build-in cmd Err, continue\n");
	}
	
	if((fp = uvc_popen((char *)cmd, "r")) == NULL){
		perror("popen");
		printf("popen error: %s/n", strerror(errno));
		return;
	} else {
		memset(pctrl->pullout, 0, pctrl->total_size);
        nr = fread(pctrl->pullout, 1, pctrl->total_size - 1, fp);
		uvc_pclose(fp);
    }

exec_finish:
	//printf("nr is %d\n", nr);
	//printf("#############\n");
	//printf("%s\n", pctrl->pullout);
	//printf("#############\n");

	*((unsigned int *)presp) = nr;
	*plen   = sizeof(nr);

	pctrl->used_size = nr;
	pctrl->cp_offset = 0;
	return;
}

static void uvc_eu_debug_runcmd_pullout(unsigned char *buf, udb_resp *resp)
{
	unsigned int cp_size = *((unsigned int *)(buf + EU_CMD_PKG_CONTENT_OFFSET));
	unsigned char  *presp = &resp->data[0];
	unsigned char  *plen = &resp->length;
	struct udb_runcmd_unit *pctrl = &udb_runcmd_ctrl;
	
	memset(presp, 0, EU_MAX_PKG_SIZE);

	if(pctrl->cp_offset + cp_size > pctrl->used_size)
	{
		printf("what?\n");
		*plen   = 0;
	} else {
		memcpy(presp, pctrl->pullout + pctrl->cp_offset, cp_size);
		*plen   = cp_size;
		pctrl->cp_offset += cp_size;
		if(pctrl->cp_offset == pctrl->used_size)
		{
			//finish output
			pctrl->used_size = 0;
			pctrl->cp_offset = 0;
			pctrl->total_size = 0;
#ifdef RUMCMD_STATIC_BUF
			memset(runcmd_buf, 0, sizeof(runcmd_buf));
#else
			free(pctrl->pullout);
#endif
		}
	}
}

static void uvc_eu_debug_pullfile_init(unsigned char *buf, udb_resp *resp)
{
	FILE *fp;
	unsigned int nr = 0;
	int ret;
	struct stat st;

	unsigned char  *presp = &resp->data[0];
	unsigned char  *plen = &resp->length;
	
	const char *path = (char *)(buf + 4);
	struct udb_pullfile_unit *pctrl = &udb_pullfile_ctrl;

	if ((fp = fopen(path, "r")) == NULL) {
		printf( "Error opening input file %s\n", path);
		pctrl->used_size = 0;
		pctrl->cp_offset = 0;
		pctrl->pullout = NULL;
	} else {
		fstat(fileno(fp), &st);
		pctrl->used_size = (unsigned int)st.st_size;
		printf("%s size is %d\n",path, st.st_size);
		pctrl->cp_offset = 0;
		if(pctrl->pullout != NULL){
			free(pctrl->pullout);
		}
		pctrl->pullout = malloc(st.st_size);
	
		fread(pctrl->pullout, 1, st.st_size, fp);
		fclose(fp);
	}

	*((unsigned int *)presp) = pctrl->used_size;
	*plen	= sizeof(unsigned int);
}

static void uvc_eu_debug_pullfile_pullout(unsigned char *buf, udb_resp *resp)
{
		unsigned int cp_size = *((unsigned int *)(buf + EU_CMD_PKG_CONTENT_OFFSET));
		unsigned char  *presp = &resp->data[0];
		unsigned char  *plen = &resp->length;
		
		memset(presp, 0, EU_MAX_PKG_SIZE);

		struct udb_pullfile_unit *pctrl = &udb_pullfile_ctrl;

		if(pctrl->cp_offset + cp_size > pctrl->used_size)
		{
			printf("what?\n");
			*plen	= 0;
		} else {
			memcpy(presp, pctrl->pullout + pctrl->cp_offset, cp_size);
			*plen	= cp_size;
			pctrl->cp_offset += cp_size;
			if(pctrl->cp_offset == pctrl->used_size)
			{
				//finish output
				//printf("finish?\n");
				pctrl->used_size = 0;
				pctrl->cp_offset = 0;
				free(pctrl->pullout);
				pctrl->pullout =NULL;
			}
		}

}

static void uvc_eu_debug_pushfile_init(unsigned char *buf, udb_resp *resp)
{
	unsigned int file_len = 0;

	unsigned char  *presp = &resp->data[0];
	unsigned char  *plen = &resp->length;

	struct udb_pushfile_unit *pctrl = &udb_pushfile_ctrl;

	if(pctrl->fp != NULL){
		fclose(pctrl->fp);
	}

	const char *path = (char *)(buf + 8);

	if ((pctrl->fp = fopen(path, "wb")) == NULL) {
		printf( "Error opening output file %s\n", path);
		presp[0] = 'I';
		presp[1] = 'N';
		presp[2] = 'I';
		presp[3] = 'T';
		presp[4] = 'N';
		presp[5] = 'O';
		*plen	= 6;

	} else {
		file_len = *((unsigned int*)(buf + EU_CMD_PKG_CONTENT_OFFSET));
		printf("%s size is %d, 0x%x\n",path, file_len, file_len);
		presp[0] = 'I';
		presp[1] = 'N';
		presp[2] = 'I';
		presp[3] = 'T';
		presp[4] = 'O';
		presp[5] = 'K';
		*plen	= 6;
	}
}

static void uvc_eu_debug_pushfile_proc(unsigned char *buf, udb_resp *resp)
{
	unsigned int nr = 0;
	int ret;

	unsigned char  *presp = &resp->data[0];
	unsigned char  *plen = &resp->length;

	struct udb_pushfile_unit *pctrl = &udb_pushfile_ctrl;

	unsigned char wrlen = buf[EU_CMD_PKG_CONTENT_OFFSET];

	fwrite(buf +5,wrlen, 1, pctrl->fp);

	presp[0] = 'P';
	presp[1] = 'R';
	presp[2] = 'O';
	presp[3] = 'C';
	presp[4] = 'O';
	presp[5] = 'K';
	*plen	= 6;

}

static void uvc_eu_debug_pushfile_fini(unsigned char *buf, udb_resp *resp)
{
	unsigned int nr = 0;
	int ret;

	unsigned char  *presp = &resp->data[0];
	unsigned char  *plen = &resp->length;

	struct udb_pushfile_unit *pctrl = &udb_pushfile_ctrl;

	fclose(pctrl->fp);
	pctrl->fp = NULL;

	presp[0] = 'F';
	presp[1] = 'I';
	presp[2] = 'N';
	presp[3] = 'I';
	presp[4] = 'O';
	presp[5] = 'K';
	*plen	= 6;

}

static int udb_auth_ok = 0;

static void uvc_eu_debug_auth(unsigned char *buf, udb_resp *resp)
{
	unsigned char  *presp = &resp->data[0];
	unsigned char  *plen = &resp->length;

	if(strncmp(buf + 4, "0303060303070701", 16) == 0){
		udb_auth_ok = 1;
		presp[0] = 'A';
		presp[1] = 'U';
		presp[2] = 'T';
		presp[3] = 'H';
		presp[4] = 'O';
		presp[5] = 'K';
		*plen	= 6;
	} else {
		presp[0] = 'A';
		presp[1] = 'U';
		presp[2] = 'T';
		presp[3] = 'H';
		presp[4] = 'N';
		presp[5] = 'O';
		*plen	= 6;
	}
}


void uvc_eu_debug_bridge(void *cmd,  void *respond)
{


	unsigned char *udb_cmd = (unsigned char *)cmd;
	udb_resp *resp = (udb_resp *)respond;

	unsigned char chan_id = udb_cmd[EU_CMD_CHANNEL_ID_OFFSET];

	//printf("chan_id os %x\n", chan_id);
	if(chan_id == UDB_AUTH) {
		uvc_eu_debug_auth(cmd, resp);
		return;
	}
	if(!udb_auth_ok){
		return;
	}

	switch(chan_id){
	case UDB_RUN_CMD:
		uvc_eu_debug_runcmd(cmd, resp);
	break;
	case UDB_RUN_CMD_PULLOUT:
		uvc_eu_debug_runcmd_pullout(cmd, resp);
	break;

	case UDB_PULL_FILE_INIT:
		uvc_eu_debug_pullfile_init(cmd, resp);
	break;
	case UDB_PULL_FILE_PULLOUT:
		uvc_eu_debug_pullfile_pullout(cmd, resp);
	break;

	case UDB_PUSH_FILE_INIT:
		uvc_eu_debug_pushfile_init(cmd, resp);
	break;
	case UDB_PUSH_FILE_PROC:
		uvc_eu_debug_pushfile_proc(cmd, resp);
	break;
	case UDB_PUSH_FILE_FINI:
		uvc_eu_debug_pushfile_fini(cmd, resp);
	break;

	default:
		printf("unknow cmd\n");
	}

	return;
}
