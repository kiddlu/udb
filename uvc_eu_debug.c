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


#include "uvc_eu_debug.h"

typedef struct udb_response_t {
	unsigned char data[64];
	unsigned char length;
}udb_resp;

enum udb_chan_id {
	CTRL           = 0x01,

	/* exec cmd */
	OPEN_CMD       = 0x02,
	RUN_CMD        = 0x03,
	GET_CMD_OUT    = 0x04,
	CLOSE_CMD      = 0x05,

	/* get file */

	/* put file */

	NONE_CHAN      = 0xff
};


#define EU_MAX_PKG_SIZE    (64)
#define EU_MAX_PKG_COUNT   (128)
#define EU_BUF_SIZE        ((EU_MAX_PKG_SIZE)*(EU_MAX_PKG_COUNT)) //128*64
#define EU_CMD_FLAG_OFFSET (3)
#define EU_CMD_RUN_OFFSET  (4)
#define EU_CMD_GET_OFFSET  (4)

struct udb_cmd_unit {
	int child_pid;
	int term_m;
	int term_s;

	struct winsize win;
	struct termios tt;

} *udb_cmd_ctrl;

//#define CMD_USE_PTY (1)

static char eu_buf[EU_BUF_SIZE];

static void uvc_eu_debug_opencmd(unsigned char *buf, udb_resp *resp)
{
	//printf("open cmd\n");
#ifdef CMD_USE_PTY
#else
#endif
}

static void uvc_eu_debug_runcmd(unsigned char *buf, udb_resp *resp)
{
	int nr = 0;
	int cnt = 0;
	FILE *fp;

	unsigned char *cmd = buf + EU_CMD_RUN_OFFSET;

	//printf("cmd is %s\n",cmd);

	if (cmd == NULL){
		printf("cmd is NULL!\n");
		return;
	}

#ifdef CMD_USE_PTY
#else

	if((fp = popen((char *)cmd, "r")) == NULL){
		perror("popen");
		printf("popen error: %s/n", strerror(errno));
		return;
	} else {
		memset(eu_buf, 0, EU_BUF_SIZE);
        nr = fread(eu_buf, 1, sizeof(eu_buf)-1, fp);
		pclose(fp);
    }
#endif
	//printf("nr is %d\n", nr);
	//printf("cnt is %d\n", nr/EU_MAX_PKG_SIZE + 1);
	//printf("#############\n");
	//printf("%s\n", eu_buf);
	//printf("#############\n");

	unsigned char  *presp = &resp->data[0];
	unsigned char  *plen = &resp->length;
	presp[0] = nr/EU_MAX_PKG_SIZE + 1;
	*plen   = 1;

	return;
}

static void uvc_eu_debug_getcmdout(unsigned char *buf, udb_resp *resp)
{
	unsigned char offset = buf[EU_CMD_GET_OFFSET];
	unsigned char  *presp = &resp->data[0];
	unsigned char  *plen = &resp->length;

	memset(presp, 0, EU_MAX_PKG_SIZE);
	memcpy(presp, eu_buf + offset * EU_MAX_PKG_SIZE, EU_MAX_PKG_SIZE);
	*plen   = EU_MAX_PKG_SIZE;
}

static void uvc_eu_debug_closecmd(unsigned char *buf, udb_resp *resp)
{
	//printf("close cmd\n");
#ifdef CMD_USE_PTY
#else
#endif

}

void uvc_eu_debug_bridge(void *cmd,  void *respond)
{
	unsigned char *udb_cmd = (unsigned char *)cmd;
	udb_resp *resp = (udb_resp *)respond;

	unsigned char chan_id = udb_cmd[EU_CMD_FLAG_OFFSET];

	switch(chan_id){
	case OPEN_CMD:
		uvc_eu_debug_opencmd(cmd, resp);
	break;
	case RUN_CMD:
		uvc_eu_debug_runcmd(cmd, resp);
	break;
	case GET_CMD_OUT:
		uvc_eu_debug_getcmdout(cmd, resp);
	break;
	case CLOSE_CMD:
		uvc_eu_debug_closecmd(cmd, resp);
	break;

	default:
		printf("unknow cmd\n");
	}

	return;
}
