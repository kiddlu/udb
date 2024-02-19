#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "utils.h"

#include "udb.h"

#include "auth.h"
#include "file.h"
#include "list.h"
#include "raw.h"
#include "shell.h"

#include "config.h"

void print_usage(void)
{
    printf("Build time: " __DATE__ " " __TIME__ "\n");

    printf("usage:\n");
    printf("  command: \n");
    printf("    list             : list all devs\n");

    printf("    auth             : auth to ctrl dev\n");

#ifdef ENABLE_PLATFORM_LINUX
    printf("    lsusb            : list all usb devs\n");
    printf("    dfu              : dfu util for dl/recovery\n");
    printf("    adb              : adb for debug\n");
    printf("    upnp             : upnp for search net devs\n");
    printf("    attach           : attach kernel drv\n");
    printf("    detach           : detach kernel drv\n");
    printf("    usbreset         : reset usb driver\n");
    printf("    usbmon           : mon kernel usb bus\n");
    printf("    cap [-w][-h][-m][-d] : stream capture\n");
    printf("        width height mode[0-mjpeg/1-yuy] dump[path]\n");
    printf("    view [-c][-b][-n][-r]: view stream on terminal\n");
    printf("        color batch compat resize\n");
#endif

    printf("    shell            : enter to interactive mode\n");
    printf("    shell [cmd]      : run shell cmd\n");
    printf("    runcmd [cmd]     : run cmd and wait for exit\n");

    printf("    msh [cmd]        : mcu run shell cmd\n");

    printf("    push [local] [dev]: push file to uvc device\n");
    printf("    pull [local] [dev]: pull file from uvc device\n");

    printf("    rawset [0x11] [0x22] ...: set raw usb ctrl\n");
    printf("    rawget                  : get raw usb ctrl\n");

    printf("    [ -d num ]       : select dev on list\n");
    printf("    [ -s sn ]        : select dev on list by sn\n");
    printf("    [ -i ipaddr ]    : select dev on inet by udp\n");
}

// global val
int               dev_index = 1;
char             *net_dev   = NULL;
usb_dev_t         usb_dev;
udb_ctrl_handle_t udb_ctrl;

int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        print_usage();
        return 0;
    }

    if (!strcmp(argv[1], "-i"))
    {
        if (argc < 4)
        {
            print_usage();
            return 0;
        }
        dev_index = 0;
        net_dev   = argv[2];
        argc -= 2;
        argv += 2;
        goto main_handle;
    }

    if (!strcmp(argv[1], "-d"))
    {
        dev_index = strtoul(argv[2], NULL, 10);
        if (dev_index < 1)
        {
            dev_index = 1;
        }
        argc -= 2;
        argv += 2;
        goto main_handle;
    }

    if (!strcmp(argv[1], "-s"))
    {
        dev_index = convert_sn_to_index(argv[2]);
        if (dev_index < 1)
        {
            dev_index = 1;
        }
        argc -= 2;
        argv += 2;
        goto main_handle;
    }

    if (!strcmp(argv[1], "list"))
    {
        printf("Support USB list\n");
        print_support_list();
        printf("\nConnected USB list\n");
        print_connected_list();
        return 0;
    }

main_handle:
    if (0)
    {
        // no nothing
    }
#ifdef ENABLE_PLATFORM_LINUX
    else if (!strcmp(argv[1], "detach"))
    {
        detach();
    }
    else if (!strcmp(argv[1], "attach"))
    {
        attach();
    }
    else if (!strcmp(argv[1], "usbreset"))
    {
        usbreset();
    }
    else if (!strcmp(argv[1], "usbmon"))
    {
        usbmon();
    }
    else if (!strcmp(argv[1], "cap"))
    {
        argc -= 1;
        argv += 1;
        capture(argc, argv);
    }
    else if (!strcmp(argv[1], "view"))
    {
        argc -= 1;
        argv += 1;
        view(argc, argv);
    }
    else if (!strcmp(argv[1], "dfu"))
    {
        argc -= 1;
        argv += 1;
        //dfu_main(argc, argv);
    }
    else if (!strcmp(argv[1], "hub"))
    {
        argc -= 1;
        argv += 1;
        //uhubctl_main(argc, argv);
    }
    else if (!strcmp(argv[1], "lsusb"))
    {
        // print_lsusb_list();
        argc -= 1;
        argv += 1;
        lsusb_main(argc, argv);

        if ((argc > 1) && (strncmp(argv[1], "-t", 2) == 0))
        {
            print_lsusb_list();
        }
    }
    else if (!strcmp(argv[1], "adb"))
    {
        argc -= 1;
        argv += 1;
        //adb_main_func(argc, argv);
    }
    else if (!strcmp(argv[1], "upnp"))
    {
        argc -= 1;
        argv += 1;
        //list_upnpc_main(argc, argv);
    }
#endif  // ENABLE_PLATFORM_LINUX
    else if (!strcmp(argv[1], "auth"))
    {
        auth();
    }
    else if (!strcmp(argv[1], "push"))
    {
        argc -= 2;
        argv += 2;
        push_file(argc, argv);
    }
    else if (!strcmp(argv[1], "pull"))
    {
        argc -= 2;
        argv += 2;
        pull_file(argc, argv);
    }
    else if (!strcmp(argv[1], "shell"))
    {
        if (argc == 2)
        {
            interactive_shell();
        }
        else
        {
            argc -= 2;
            argv += 2;
            exec_cmd(argc, argv);
        }
    }
    else if (!strcmp(argv[1], "runcmd"))
    {
        argc -= 2;
        argv += 2;
        exec_cmd_wait(argc, argv);
    }
    else if (!strcmp(argv[1], "msh"))
    {
        argc -= 2;
        argv += 2;
        msh_exec_cmd(argc, argv);
    }
    else if (!strcmp(argv[1], "rawset"))
    {
        argc -= 2;
        argv += 2;
        raw_set(argc, argv);
    }
    else if (!strcmp(argv[1], "rawget"))
    {
        // TO DO
        printf("not support right now\n");
    }
    else
    {
        print_usage();
    }

    fflush(stdout);

    return 0;
}
