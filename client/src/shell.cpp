#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "udb.h"
#include "utils.h"

void interactive_shell(void)
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

    set_udb_channel_header(write_buf, UDB_RUN_CMD);
    memcpy(write_buf + 4, "pwd\n", 4);

    udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
    udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
    dumphex(write_buf, nr_write);
    dumphex(read_buf, nr_read);
#endif

    unsigned int get_size = 0;
    unsigned int cp_size;
    unsigned int file_size = 0;
    memcpy(&get_size, read_buf, sizeof(get_size));

    for (;;)
    {
        if (get_size < udb_ctrl.prot_size)
        {
            cp_size = get_size;
        }
        else
        {
            cp_size = udb_ctrl.prot_size;
        }
        memset(write_buf, 0, udb_ctrl.prot_size);
        memset(read_buf, 0, udb_ctrl.prot_size + 1);

        set_udb_channel_header(write_buf, UDB_RUN_CMD_PULLOUT);

        memcpy(write_buf + 4, &cp_size, sizeof(cp_size));

        udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
        udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);
#ifdef DEBUG
        dumphex(write_buf, nr_write);
        dumphex(read_buf, nr_read);
#endif

        printf("%s", read_buf);

        get_size -= cp_size;
        if (get_size == 0)
        {
            break;
        }
    }

    printf(">");
    memset(write_buf, 0, udb_ctrl.prot_size);
    memset(read_buf, 0, udb_ctrl.prot_size + 1);

    while (fgets((char *)write_buf + 4, udb_ctrl.prot_size - 4, stdin))
    {
        if (strncmp((char *)write_buf + 4, "exit", 4) == 0)
        {
            printf("exit\n");
            exit(0);
        }

        set_udb_channel_header(write_buf, UDB_RUN_CMD);

        udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
        udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
        dumphex(write_buf, nr_write);
        dumphex(read_buf, nr_read);
#endif
        memcpy(&get_size, read_buf, sizeof(get_size));
        // printf("\n");

        for (;;)
        {
            if (get_size < udb_ctrl.prot_size)
            {
                cp_size = get_size;
            }
            else
            {
                cp_size = udb_ctrl.prot_size;
            }
            memset(write_buf, 0, udb_ctrl.prot_size);
            memset(read_buf, 0, udb_ctrl.prot_size + 1);

            set_udb_channel_header(write_buf, UDB_RUN_CMD_PULLOUT);

            memcpy(write_buf + 4, &cp_size, sizeof(cp_size));
            udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
            udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);
#ifdef DEBUG
            dumphex(write_buf, nr_write);
            dumphex(read_buf, nr_read);
#endif
            printf("%s", read_buf);

            get_size -= cp_size;
            if (get_size == 0)
            {
                break;
            }
        }

        // printf("ret %d\n", ret);
        memset(write_buf, 0, udb_ctrl.prot_size);
        memset(read_buf, 0, udb_ctrl.prot_size + 1);

        set_udb_channel_header(write_buf, UDB_RUN_CMD);

        memcpy(write_buf + 4, "pwd\n", 4);
        udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
        udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);
#ifdef DEBUG
        dumphex(write_buf, nr_write);
        dumphex(read_buf, nr_read);
#endif
        memcpy(&get_size, read_buf, sizeof(get_size));
        // printf("\n");

        for (;;)
        {
            if (get_size < udb_ctrl.prot_size)
            {
                cp_size = get_size;
            }
            else
            {
                cp_size = udb_ctrl.prot_size;
            }
            memset(write_buf, 0, udb_ctrl.prot_size);
            memset(read_buf, 0, udb_ctrl.prot_size + 1);

            set_udb_channel_header(write_buf, UDB_RUN_CMD_PULLOUT);

            memcpy(write_buf + 4, &cp_size, sizeof(cp_size));
            udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
            udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
            dumphex(write_buf, nr_write);
            dumphex(read_buf, nr_read);
#endif
            int idx = (int)strlen((const char *)read_buf) - 1;

            if (read_buf[idx] == '\n')
            {
                read_buf[idx] = 0;
            }

            printf("%s", read_buf);

            get_size -= cp_size;
            if (get_size == 0)
            {
                break;
            }
        }

        printf(">");
        memset(write_buf, 0, udb_ctrl.prot_size);
        memset(read_buf, 0, udb_ctrl.prot_size + 1);
    }
}

void exec_cmd(int argc, char **argv)
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

    set_udb_channel_header(write_buf, UDB_RUN_CMD);

    unsigned int get_size = 0;
    unsigned int cp_size;

    unsigned char loop = 0;

    int offset = 4;
    int len    = 0;
    for (int i = 0; i < argc; i++)
    {
        len = (int)strlen(argv[i]);
        memcpy(write_buf + offset, argv[i], len);
        write_buf[offset + len] = ' ';
        offset += (len + 1);

        if (offset >= 60)
        {
            break;
        }
    }

    udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
    udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
    dumphex(write_buf, nr_write);
    dumphex(read_buf, nr_read);
#endif

    memcpy(&get_size, read_buf, sizeof(get_size));
    // printf("\n");

    for (;;)
    {
        if (get_size < udb_ctrl.prot_size)
        {
            cp_size = get_size;
        }
        else
        {
            cp_size = udb_ctrl.prot_size;
        }

        memset(write_buf, 0, udb_ctrl.prot_size);
        memset(read_buf, 0, udb_ctrl.prot_size);

        set_udb_channel_header(write_buf, UDB_RUN_CMD_PULLOUT);

        memcpy(write_buf + 4, &cp_size, sizeof(cp_size));
        udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
        udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
        dumphex(write_buf, nr_write);
        dumphex(read_buf, nr_read);
#endif

        printf("%s", read_buf);

        get_size -= cp_size;
        if (get_size == 0)
        {
            break;
        }
    }
}

void exec_cmd_wait(int argc, char **argv)
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

    set_udb_channel_header(write_buf, UDB_RUN_CMD_WAIT);

    unsigned char loop = 0;

    int offset = 4;
    int len    = 0;
    for (int i = 0; i < argc; i++)
    {
        len = (int)strlen(argv[i]);
        memcpy(write_buf + offset, argv[i], len);
        write_buf[offset + len] = ' ';
        offset += (len + 1);

        if (offset >= 60)
        {
            break;
        }
    }

    udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
    udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
    dumphex(write_buf, nr_write);
    dumphex(read_buf, nr_read);
#endif
    unsigned char support = 0;
    memcpy(&support, read_buf, sizeof(support));

    if (support != 1)
    {
        // printf("fw not support runcmd wait\n");
        return;
    }

    for (;;)
    {
        memset(write_buf, 0, udb_ctrl.prot_size);
        memset(read_buf, 0, udb_ctrl.prot_size);

        set_udb_channel_header(write_buf, UDB_RUN_CMD_WAIT_PULLOUT);

        udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
        udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
        dumphex(write_buf, nr_write);
        dumphex(read_buf, nr_read);
#endif
        if (strlen((const char *)(read_buf + 1)) != 0)
        {
            printf("%s", read_buf + 1);
        }
        else
        {
            usleep(100 * 1000);
        }

        if (read_buf[0] == 1)
        {
            break;
        }
    }
}

void msh_exec_cmd(int argc, char **argv)
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

    set_udb_channel_header(write_buf, UDB_MSH_CMD);

    unsigned int get_size = 0;
    unsigned int cp_size;

    unsigned char loop = 0;

    int offset = 4;
    int len    = 0;
    for (int i = 0; i < argc; i++)
    {
        len = (int)strlen(argv[i]);
        memcpy(write_buf + offset, argv[i], len);
        write_buf[offset + len] = ' ';
        offset += (len + 1);

        if (offset >= 60)
        {
            break;
        }
    }

    udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
    udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
    dumphex(write_buf, nr_write);
    dumphex(read_buf, nr_read);
#endif

    memcpy(&get_size, read_buf, sizeof(get_size));
    // printf("\n");

    for (;;)
    {
        if (get_size < udb_ctrl.prot_size)
        {
            cp_size = get_size;
        }
        else
        {
            cp_size = udb_ctrl.prot_size;
        }

        memset(write_buf, 0, udb_ctrl.prot_size);
        memset(read_buf, 0, udb_ctrl.prot_size);

        set_udb_channel_header(write_buf, UDB_MSH_CMD_PULLOUT);

        memcpy(write_buf + 4, &cp_size, sizeof(cp_size));
        udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
        udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
        dumphex(write_buf, nr_write);
        dumphex(read_buf, nr_read);
#endif

        printf("%s", read_buf);

        get_size -= cp_size;
        if (get_size == 0)
        {
            break;
        }
    }
}
