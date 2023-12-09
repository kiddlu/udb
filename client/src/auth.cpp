#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "udb.h"
#include "utils.h"

void auth(void)
{
    if (dev_index > 0)
    {
        select_usb_dev_by_index(dev_index, &usb_dev);
        get_udb_ctrl_handle_from_usb(&usb_dev, &udb_ctrl);
    }
    else
    {
        get_udb_ctrl_handle_from_net(net_dev, &udb_ctrl);
    }

    unsigned char *write_buf = (unsigned char *)malloc(udb_ctrl.prot_size);
    unsigned char *read_buf  = (unsigned char *)malloc(udb_ctrl.prot_size + 1);

    unsigned long nr_write = udb_ctrl.prot_size;
    unsigned long nr_read  = 0;

    unsigned char auth_cmd[] = {
        '0', '3', '0', '3', '0', '6', '0', '3',
        '0', '3', '0', '7', '0', '7', '0', '1',
    };

    memset(write_buf, 0, udb_ctrl.prot_size);
    memset(read_buf, 0, udb_ctrl.prot_size + 1);

    fprintf(stdout, "UDB build time: " __DATE__ " " __TIME__ "\n");

    set_udb_channel_header(write_buf, UDB_AUTH);
    memcpy(write_buf + 4, auth_cmd, sizeof(auth_cmd));

    udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
    udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

    dumphex(write_buf, nr_write);
    dumphex(read_buf, nr_read);
}
