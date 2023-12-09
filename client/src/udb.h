#ifndef _UDB_H
#define _UDB_H

#include "config.h"
#include "platform.h"

/****************************************/
enum udb_chan_id
{
    UDB_AUTH = 0x01,

    /* run cmd */
    UDB_RUN_CMD         = 0x02,
    UDB_RUN_CMD_PULLOUT = 0x03,

    UDB_RUN_CMD_WAIT         = 0x04,
    UDB_RUN_CMD_WAIT_PULLOUT = 0x05,

    /* mcu cmd */
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
    UDB_RAWSET = 0xfd,  // raw buffer
    UDB_RAWGET = 0xfe,

    NONE_CHAN = 0xff
};

static void set_udb_channel_header(unsigned char *buf, unsigned char chan_id)
{
    buf[0] = 'u';
    buf[1] = 'd';
    buf[2] = 'b';
    buf[3] = chan_id;

    return;
}

void print_usage(void);

extern int               dev_index;
extern char             *net_dev;
extern usb_dev_t         usb_dev;
extern udb_ctrl_handle_t udb_ctrl;

#endif
