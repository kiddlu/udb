#ifndef _PLATFORM_LINUX_H
#define _PLATFORM_LINUX_H

#include <arpa/inet.h>
#include <limits.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

enum
{
    DEV_DRIVR_V4L2     = 0x01,
    DEV_DRIVR_USBDEVFS = 0x02,
    DEV_DRIVR_UNKNOW   = 0xff
};

#define DEV_DRIVR_USBDEVFS_TIMEOUT (5000)  // def 1000, 0 means forever

typedef struct
{
    unsigned short vid;
    unsigned short pid;
    unsigned char  busnum;
    unsigned char  devnum;

    unsigned char usbclass;
    unsigned char backend;

    char dev_sn[PATH_MAX];
} usb_dev_t;

typedef struct
{
    unsigned char  devclass;
    unsigned short prot_size;

    int           driver;
    int           fd;
    unsigned char unitid;
    unsigned char selector;
    char          dev_name[PATH_MAX];

    int                net_fd;
    struct sockaddr_in net_server;
} udb_ctrl_handle_t;

#define ENABLE_PLATFORM_LINUX
int print_lsusb_list(void);

void detach(void);
void attach(void);

void usbreset(void);
void usbmon(void);

void capture(int argc, char **argv);
void view(int argc, char **argv);

extern "C" {
int dfu_main(int argc, char **argv);
}

extern "C" {
int uhubctl_main(int argc, char **argv);
}

extern "C" {
int lsusb_main(int argc, char **argv);
}

extern "C" {
int adb_main_func(int argc, char **argv);
}

extern "C" {
int list_upnpc_main(int argc, char **argv);
}
#endif
