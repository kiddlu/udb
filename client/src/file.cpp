#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform.h"
#include "udb.h"
#include "utils.h"

void pull_file(int argc, char **argv)
{
    char *local_path = argv[0];
    char *dev_path   = argv[1];

    if (dev_path == NULL || local_path == NULL)
    {
        print_usage();
        return;
    }

    if (dev_index > 0)
    {
        select_usb_dev_by_index(dev_index, &usb_dev);
        get_udb_ctrl_handle_from_usb(&usb_dev, &udb_ctrl);
    }
    else
    {
        get_udb_ctrl_handle_from_net(net_dev, &udb_ctrl);
    }

    printf("from dev: %s\nto local: %s\n", dev_path, local_path);

    FILE *fp = fopen(local_path, "wb");
    if (fp == NULL)
    {
        printf("open %s error\n", local_path);
        return;
    }

    struct timeval tstart, tend;
    gettimeofday(&tstart, NULL);

    unsigned int   get_size = 0;
    unsigned int   cp_size;
    unsigned int   file_size = 0;
    unsigned int  *nisda     = 0;
    unsigned char *write_buf = (unsigned char *)malloc(udb_ctrl.prot_size);
    unsigned char *read_buf  = (unsigned char *)malloc(udb_ctrl.prot_size + 1);

    unsigned long nr_write = udb_ctrl.prot_size;
    unsigned long nr_read  = 0;

    memset(write_buf, 0, udb_ctrl.prot_size);
    memset(read_buf, 0, udb_ctrl.prot_size + 1);

    set_udb_channel_header(write_buf, UDB_PULL_FILE_INIT);

    memcpy(write_buf + 4, dev_path, strlen(dev_path));

    udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
    udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
    dumphex(write_buf, nr_write);
    dumphex(read_buf, nr_read);
#endif

    memcpy(&get_size, read_buf, sizeof(get_size));
    file_size = get_size / 1024;
    printf("File Size: %dKiB, 0x%x\n", get_size / 1024, get_size);

    int i = 0, j = 0;
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

        set_udb_channel_header(write_buf, UDB_PULL_FILE_PULLOUT);

        memcpy(write_buf + 4, &cp_size, sizeof(cp_size));
        udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
        udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
        dumphex(write_buf, nr_write);
        dumphex(read_buf, nr_read);
#endif

        printf("#");
        i++;
        if (i == 64)
        {
            i = 0;
            j++;
            printf("\t(%d/%d)\n", (udb_ctrl.prot_size / 16) * j, file_size);
        }

        fwrite(read_buf, 1, cp_size, fp);

        get_size -= cp_size;

        if (get_size == 0)
        {
            printf("\t(%d/%d)\n", file_size, file_size);
            break;
        }
    }

    gettimeofday(&tend, NULL);
    printf("Time: %lds\n", tend.tv_sec - tstart.tv_sec);
}

void push_file(int argc, char **argv)
{
    char *local_path = argv[0];
    char *dev_path   = argv[1];

    if (dev_path == NULL || local_path == NULL)
    {
        print_usage();
        return;
    }

    if (dev_index > 0)
    {
        select_usb_dev_by_index(dev_index, &usb_dev);
        get_udb_ctrl_handle_from_usb(&usb_dev, &udb_ctrl);
    }
    else
    {
        get_udb_ctrl_handle_from_net(net_dev, &udb_ctrl);
    }

    printf("from local: %s\nto dev: %s\n", local_path, dev_path);

    FILE *fp = fopen(local_path, "rb");
    if (fp == NULL)
    {
        printf("open %s error\n", local_path);
        return;
    }

    struct timeval tstart, tend;
    gettimeofday(&tstart, NULL);

    fseek(fp, 0L, SEEK_END);
    unsigned int file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    printf("File Size: %dKiB, 0x%x\n", file_size / 1024, file_size);

    unsigned char *write_buf = (unsigned char *)malloc(udb_ctrl.prot_size);
    unsigned char *read_buf  = (unsigned char *)malloc(udb_ctrl.prot_size + 1);

    unsigned long nr_write = udb_ctrl.prot_size;
    unsigned long nr_read  = 0;

    memset(write_buf, 0, udb_ctrl.prot_size);
    memset(read_buf, 0, udb_ctrl.prot_size + 1);

    set_udb_channel_header(write_buf, UDB_PUSH_FILE_INIT);

    memcpy(write_buf + 4, &file_size, sizeof(file_size));
    memcpy(write_buf + 8, dev_path, strlen(dev_path));

    udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
    udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
    dumphex(write_buf, nr_write);
    dumphex(read_buf, nr_read);
#endif

    unsigned short cp_size;

    unsigned int left_size = file_size;

    unsigned short PUSH_MAX_PKG_SIZE;

    if (udb_ctrl.prot_size == UDB_PROT_SIZE_DEFAULT)
    {
        PUSH_MAX_PKG_SIZE = udb_ctrl.prot_size - 4 - 1;
    }
    else
    {
        PUSH_MAX_PKG_SIZE = udb_ctrl.prot_size - 4 - 2;
    }

    int i = 0, j = 0;

    for (;;)
    {
        if (left_size < PUSH_MAX_PKG_SIZE)
        {
            cp_size = left_size;
        }
        else
        {
            cp_size = PUSH_MAX_PKG_SIZE;
        }
        memset(write_buf, 0, udb_ctrl.prot_size);
        memset(read_buf, 0, udb_ctrl.prot_size + 1);

        set_udb_channel_header(write_buf, UDB_PUSH_FILE_PROC);
        if (udb_ctrl.prot_size == UDB_PROT_SIZE_DEFAULT)
        {
            unsigned char onebyte_cp_size = (unsigned char)cp_size;

            memcpy(write_buf + 4, &onebyte_cp_size, sizeof(onebyte_cp_size));
            fread(write_buf + 5, onebyte_cp_size, 1, fp);
        }
        else
        {
            memcpy(write_buf + 4, &cp_size, sizeof(cp_size));
            fread(write_buf + 6, cp_size, 1, fp);
        }

        udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
        udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
        dumphex(write_buf, nr_write);
        dumphex(read_buf, nr_read);
#endif

        printf("#");
        i++;
        if (i == 64)
        {
            i = 0;
            j++;
            printf("\t(%d/%d)\n", PUSH_MAX_PKG_SIZE * 64 * j / 1024,
                   file_size / 1024);
        }

        left_size -= cp_size;

        if (left_size == 0)
        {
            printf("\t(%d/%d)\n", file_size / 1024, file_size / 1024);
            break;
        }
    }
    memset(write_buf, 0, udb_ctrl.prot_size);
    memset(read_buf, 0, udb_ctrl.prot_size + 1);

    set_udb_channel_header(write_buf, UDB_PUSH_FILE_FINI);

    udb_ctrl_write(&udb_ctrl, write_buf, nr_write);
    udb_ctrl_read(&udb_ctrl, read_buf, udb_ctrl.prot_size, &nr_read);

#ifdef DEBUG
    dumphex(write_buf, nr_write);
    dumphex(read_buf, nr_read);
#endif

    fclose(fp);

    gettimeofday(&tend, NULL);
    printf("Time: %lds\n", tend.tv_sec - tstart.tv_sec);
}
