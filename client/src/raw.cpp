#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "udb.h"
#include "utils.h"

void raw_set(int argc, char **argv)
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

    memset(write_buf, 0, udb_ctrl.prot_size);
    memset(read_buf, 0, udb_ctrl.prot_size + 1);

    for (int i = 0; i < argc; i++)
    {
        // printf("argc:%d\t %s,%d\n", i,argv[i], strlen(argv[i]));
        write_buf[i] = (unsigned char)strtoul(argv[i], NULL, 16);
    }

    udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
    udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

    dumphex(write_buf, nr_write);
    dumphex(read_buf, nr_read);
}
