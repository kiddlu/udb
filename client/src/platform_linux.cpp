
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "list.h"
#include "udb.h"
#include "utils.h"

#include "platform_linux.h"

#include "linux/libuvc.h"
#include "linux/v4l2.h"

#define USB_CTRL_IN  0xa1
#define USB_CTRL_OUT 0x21

/*
int sysusb_get_dir_path(uint8_t bnum, uint8_t dnum, char *path)
{
    DIR *dir;
    struct dirent *dirent;
    FILE *file;
    char filename[PATH_MAX];
    char line[1024];
    int ret = -1;
    uint8_t busnum, devnum;

    if(path == NULL)
    {
        return ret;
    }

    dir = opendir("/sys/bus/usb/devices");
    if (!dir) {
        return ret;
    }

    while ((dirent = readdir(dir))) {
        if (dirent->d_name[0]=='.')
            continue;
        sprintf(filename, "/sys/bus/usb/devices/%s/busnum", dirent->d_name);
        file = fopen(filename, "r");
        if (!file)
            continue;
        memset(line, 0, 1024);
        if (fgets(line, 1023,file) == NULL) {
            fclose(file);
            continue;
        }
        fclose(file);
        busnum = strtoul(line, NULL, 10);

        sprintf(filename, "/sys/bus/usb/devices/%s/devnum", dirent->d_name);
        file = fopen(filename, "r");
        if (!file)
            continue;
        memset(line, 0, 1024);
        if (fgets(line, 1023,file)==NULL) {
            fclose(file);
            continue;
        }
        fclose(file);
        devnum = strtoul(line, NULL, 10);

        //printf("busnum %d, devnum %d, path %s\n", busnum, devnum,
dirent->d_name);

        if(busnum == bnum && devnum == dnum) {
            sprintf(path, "/sys/bus/usb/devices/%s/", dirent->d_name);
            ret = 0;
            break;
        }
    }

    closedir(dir);
    return ret;
}
*/

int read_sysfs_node(char *path, char *buf, int size)
{
    int ret = 0;

    if (buf == NULL && size == 0)
    {
        return -1;
    }

    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        return -1;
    }

    memset(buf, 0, size);

    ret = fread(buf, size, 1, fp);

    fclose(fp);

    buf[strlen(buf) - 1] = 0;
    ret -= 1;

    return ret;
}

#define V4L2_DEVICES_ID_MAX 255

static int get_v4l2_index(unsigned char busnr, unsigned char devnr)
{
    char cam_path[PATH_MAX];
    char interface_path[PATH_MAX];
    char dev_path[PATH_MAX];
    char sys_node[PATH_MAX];
    char line[PATH_MAX];

    memset(cam_path, 0x00, PATH_MAX);
    memset(interface_path, 0x00, PATH_MAX);
    memset(dev_path, 0x00, PATH_MAX);
    memset(sys_node, 0x00, PATH_MAX);
    memset(line, 0x00, PATH_MAX);

    int           index = 0, j = 0, k = 0;
    unsigned char busnum = 0, devnum = 0;

    for (index = 0; index < V4L2_DEVICES_ID_MAX; index++)
    {
        memset(cam_path, 0x00, PATH_MAX);
        sprintf(cam_path, "/sys/class/video4linux/video%d", index);
        int fd = open(cam_path, O_RDONLY);
        if (fd < 0)
        {
            // printf("open camera path [ /dev/video%d ] failed.\n", index);
            continue;
        }
        else
        {
            close(fd);
            fd = -1;
            readlink(cam_path, interface_path, PATH_MAX);
            for (j = 0, k = 0; j < strlen(interface_path); j++)
            {
                if (interface_path[j] == '/')
                {
                    k++;
                }
            }
            int slen = strlen("/sys/class/video4linux/");
            int flag = k;
            memcpy(dev_path, "/sys/class/video4linux/", slen);
            for (j = 0, k = 0; j < strlen(interface_path); j++)
            {
                if (interface_path[j] == '/')
                {
                    k++;
                }
                if (k == flag - 2)
                {
                    break;
                }
                else
                {
                    dev_path[slen + j] = interface_path[j];
                }
            }
            snprintf(sys_node, strlen(dev_path) + 8, "%s/busnum", dev_path);
            read_sysfs_node(sys_node, line, 1024);
            busnum = strtoul(line, NULL, 10);

            snprintf(sys_node, strlen(dev_path) + 8, "%s/devnum", dev_path);
            read_sysfs_node(sys_node, line, 1024);
            devnum = strtoul(line, NULL, 10);
            // printf("busnum %d devnum %d\n", busnum, devnum);
            // printf("cam_path %s\n", cam_path);
            // printf("interface_path %s\n", interface_path);
            // printf("dev_path %s\n", dev_path);
            // printf("sys_node %s\n", sys_node);
            // printf("line %s\n", line);

            if (devnum == devnr && busnum == busnr)
            {
                return index;
            }
        }
    }
    return -1;
}

bool uvc_detach_kernel_driver(usb_dev_t *usb_dev)
{
    bool bRet = false;
    char devpath[PATH_MAX];
    memset(devpath, 0, PATH_MAX);
    int fd;

    int video_index = get_v4l2_index(usb_dev->busnum, usb_dev->devnum);

    if (video_index < 0)
    {
        exit(0);
    }

    sprintf(devpath, "/dev/bus/usb/%03d/%03d", usb_dev->busnum,
            usb_dev->devnum);

    fd = open(devpath, O_RDWR);
    if (fd < 0)
    {
        printf("Can't open uvc dev %s, pls check permission\n", devpath);
        exit(-1);
    }

    struct usbdevfs_getdriver getdrv;
    int                       r;

    getdrv.interface = 0;
    r                = ioctl(fd, USBDEVFS_GETDRIVER, &getdrv);
    if (!r)
    {
        // detach from kernel driver
        struct usbdevfs_ioctl operate;
        operate.data       = NULL;
        operate.ifno       = 0;
        operate.ioctl_code = USBDEVFS_DISCONNECT;
        r                  = ioctl(fd, USBDEVFS_IOCTL, &operate);
        if (r)
        {
            printf("detach kernel driver failed error %d errno %d", r, errno);
            return bRet;
        }
    }

    printf("Detach uvc /dev/video%d from kernel\n", video_index);

    bRet = true;
    return bRet;
}

bool uvc_attach_kernel_driver(usb_dev_t *usb_dev)
{
    bool bRet = false;
    char devpath[PATH_MAX];
    memset(devpath, 0, PATH_MAX);
    int fd;

    printf("Attach uvc /dev/bus/usb/%03d/%03d to kernel\n", usb_dev->busnum,
           usb_dev->devnum);

    sprintf(devpath, "/dev/bus/usb/%03d/%03d", usb_dev->busnum,
            usb_dev->devnum);

    fd = open(devpath, O_RDWR);
    if (fd < 0)
    {
        printf("Can't open UVC Dev %s, pls check permission\n", devpath);
        exit(-1);
    }

    struct usbdevfs_getdriver getdrv;
    int                       r;

    getdrv.interface = 0;
    r                = ioctl(fd, USBDEVFS_GETDRIVER, &getdrv);
    if (r)  // error
    {
        // attach from kernel driver
        struct usbdevfs_ioctl operate;
        operate.data       = NULL;
        operate.ifno       = 0;
        operate.ioctl_code = USBDEVFS_CONNECT;
        r                  = ioctl(fd, USBDEVFS_IOCTL, &operate);
        if (!r)
        {
            printf("attach kernel driver failed error %d errno %d", r, errno);
            return bRet;
        }
    }

    usleep(300 * 1000);

    int video_index = get_v4l2_index(usb_dev->busnum, usb_dev->devnum);
    if (video_index >= 0)
    {
        printf("Attach uvc /dev/video%d to kernel\n", video_index);
    }

    bRet = true;
    return bRet;
}

int convert_sn_to_index(char *sn)
{
    DIR           *dir;
    struct dirent *dirent;
    FILE          *file;
    char           filename[PATH_MAX];
    char           line[1024];
    int            find = 0;

    int ret = -1;

    unsigned short vid, pid;
    unsigned char  busnum;
    unsigned char  devnum;

    char sysfs_product[1024];
    char sysfs_sn[1024];

    dir = opendir("/sys/bus/usb/devices");
    if (!dir)
    {
        printf("sysfs not mount?\n");
        exit(-1);
    }

    while ((dirent = readdir(dir)))
    {
        if (dirent->d_name[0] == '.')
        {
            continue;
        }
        sprintf(filename, "/sys/bus/usb/devices/%s/idVendor", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        vid = strtoul(line, NULL, 16);

        sprintf(filename, "/sys/bus/usb/devices/%s/idProduct", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        pid = strtoul(line, NULL, 16);

        if (in_support_list(vid, pid))
        {
            find++;
            sprintf(filename, "/sys/bus/usb/devices/%s/busnum", dirent->d_name);
            read_sysfs_node(filename, line, 1024);
            busnum = strtoul(line, NULL, 10);

            sprintf(filename, "/sys/bus/usb/devices/%s/devnum", dirent->d_name);
            read_sysfs_node(filename, line, 1024);
            devnum = strtoul(line, NULL, 10);

            sprintf(filename, "/sys/bus/usb/devices/%s/serial", dirent->d_name);
            read_sysfs_node(filename, sysfs_sn, 1024);
            if (strncmp(sysfs_sn, sn, strlen(sn)) == 0)
            {
                break;
            }
        }
    }

    if (find == 0)
    {
        printf("Can't find udb dev\n");
        exit(-1);
    }

    closedir(dir);
    return find;
}

bool uvc_hal_write(udb_ctrl_handle_t *udb_handle,
                   unsigned char     *buffer,
                   unsigned long      size)
{
    bool bRet = false;
    bRet      = true;

    if (udb_handle->driver == DEV_DRIVR_USBDEVFS)
    {
        struct usbdevfs_ctrltransfer control;

        control.bRequestType = USB_CTRL_OUT;
        control.bRequest     = 0x01;
        control.wValue       = udb_handle->selector << 8;
        control.wIndex       = udb_handle->unitid << 8;
        control.wLength      = size;
        control.timeout      = DEV_DRIVR_USBDEVFS_TIMEOUT; /* in milliseconds */
        control.data         = buffer;
        ioctl(udb_handle->fd, USBDEVFS_CONTROL, &control);
    }
    else if (udb_handle->driver == DEV_DRIVR_V4L2)
    {
        struct uvc_xu_control_query cq;

        cq.query    = V4L2_UVC_SET_CUR;
        cq.selector = udb_handle->selector;
        cq.unit     = udb_handle->unitid;
        cq.size     = size;
        cq.data     = buffer;

        ioctl(udb_handle->fd, UVCIOC_CTRL_QUERY, &cq);
    }

    return bRet;
}

bool uvc_hal_read(udb_ctrl_handle_t *udb_handle,
                  unsigned char     *buffer,
                  unsigned long      size,
                  unsigned long     *BytesReturned)
{
    bool bRet = false;
    bRet      = true;
    if (udb_handle->driver == DEV_DRIVR_USBDEVFS)
    {
        struct usbdevfs_ctrltransfer control;

        control.bRequestType = USB_CTRL_IN;
        control.bRequest     = 0x81;
        control.wValue       = udb_handle->selector << 8;
        control.wIndex       = udb_handle->unitid << 8;
        control.wLength      = size;
        control.timeout      = DEV_DRIVR_USBDEVFS_TIMEOUT; /* in milliseconds */
        control.data         = buffer;
        *BytesReturned = ioctl(udb_handle->fd, USBDEVFS_CONTROL, &control);
    }
    else if (udb_handle->driver == DEV_DRIVR_V4L2)
    {
        struct uvc_xu_control_query cq;

        cq.query    = V4L2_UVC_GET_CUR;
        cq.selector = udb_handle->selector;
        cq.unit     = udb_handle->unitid;
        cq.size     = size;
        cq.data     = buffer;

        ioctl(udb_handle->fd, UVCIOC_CTRL_QUERY, &cq);
        *BytesReturned = size;
    }

    return bRet;
}

bool net_hal_write(udb_ctrl_handle_t *udb_handle,
                   unsigned char     *buffer,
                   unsigned long      size)
{
    sendto(udb_handle->net_fd, buffer, size, 0,
           (struct sockaddr *)(&udb_handle->net_server),
           sizeof(struct sockaddr_in));

    return true;
}

bool net_hal_read(udb_ctrl_handle_t *udb_handle,
                  unsigned char     *buffer,
                  unsigned long      size,
                  unsigned long     *BytesReturned)
{
    socklen_t addrlen;

    *BytesReturned =
        recvfrom(udb_handle->net_fd, buffer, size, 0,
                 (struct sockaddr *)(&udb_handle->net_server), &addrlen);

    return true;
}

bool udb_ctrl_write(udb_ctrl_handle_t *udb_handle,
                    unsigned char     *buffer,
                    unsigned long      size)
{
    switch (udb_handle->devclass)
    {
        case USB_CLASS_UVC:
            return uvc_hal_write(udb_handle, buffer, size);
        case NET_CLASS_UDP:
            return net_hal_write(udb_handle, buffer, size);
        case USB_CLASS_CDC:
        case USB_CLASS_HID:
        default:
            printf("Can not udb ctrl write\n");
            break;
    }

    return false;
}

bool udb_ctrl_read(udb_ctrl_handle_t *udb_handle,
                   unsigned char     *buffer,
                   unsigned long      size,
                   unsigned long     *BytesReturned)
{
    switch (udb_handle->devclass)
    {
        case USB_CLASS_UVC:
            return uvc_hal_read(udb_handle, buffer, size, BytesReturned);
        case NET_CLASS_UDP:
            return net_hal_read(udb_handle, buffer, size, BytesReturned);
        case USB_CLASS_CDC:
        case USB_CLASS_HID:
        default:
            printf("Can not udb ctrl write\n");
            break;
    }

    return false;
}

void select_usb_dev_by_index(int index, usb_dev_t *usb_dev)
{
    DIR           *dir;
    struct dirent *dirent;
    FILE          *file;
    char           filename[PATH_MAX];
    char           line[1024];
    int            find = 0;

    int ret = -1;

    unsigned short vid, pid;
    unsigned char  busnum;
    unsigned char  devnum;

    char sysfs_product[1024];
    char sysfs_sn[1024];

    dir = opendir("/sys/bus/usb/devices");
    if (!dir)
    {
        printf("sysfs not mount?\n");
        return;
    }

    while ((dirent = readdir(dir)))
    {
        if (dirent->d_name[0] == '.')
        {
            continue;
        }
        sprintf(filename, "/sys/bus/usb/devices/%s/idVendor", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        vid = strtoul(line, NULL, 16);

        sprintf(filename, "/sys/bus/usb/devices/%s/idProduct", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        pid = strtoul(line, NULL, 16);

        if (in_support_list(vid, pid))
        {
            find++;
            sprintf(filename, "/sys/bus/usb/devices/%s/busnum", dirent->d_name);
            read_sysfs_node(filename, line, 1024);
            busnum = strtoul(line, NULL, 10);

            sprintf(filename, "/sys/bus/usb/devices/%s/devnum", dirent->d_name);
            read_sysfs_node(filename, line, 1024);
            devnum = strtoul(line, NULL, 10);
            if (index == find && usb_dev != NULL)
            {
                usb_dev->vid      = vid;
                usb_dev->pid      = pid;
                usb_dev->busnum   = busnum;
                usb_dev->devnum   = devnum;
                usb_dev->usbclass = get_support_list_usbclass(vid, pid);
                usb_dev->backend  = get_support_list_backend(vid, pid);

                memset(usb_dev->dev_sn, 0, PATH_MAX);
                sprintf(filename, "/sys/bus/usb/devices/%s/serial",
                        dirent->d_name);
                read_sysfs_node(filename, usb_dev->dev_sn, PATH_MAX);

                break;
            }
        }
    }

    if (find == 0)
    {
        printf("Can't find udb dev\n");
        exit(-1);
    }
    closedir(dir);
    return;
}

bool get_udb_ctrl_handle_from_usb(usb_dev_t         *usb_dev,
                                  udb_ctrl_handle_t *udb_handle)
{
    bool bRet = false;
    char devpath[PATH_MAX];
    memset(devpath, 0, PATH_MAX);
    int fd;

    int video_index = get_v4l2_index(usb_dev->busnum, usb_dev->devnum);
    if (video_index >= 0)
    {
        udb_handle->driver = DEV_DRIVR_V4L2;
        sprintf(devpath, "/dev/video%d", video_index);

        fd = open(devpath, O_RDWR);
        if (fd < 0)
        {
            printf("Can't open uvc dev %s, pls check permission\n", devpath);
            exit(-1);
        }
        udb_handle->devclass = usb_dev->usbclass;
        udb_handle->fd       = fd;
        udb_handle->unitid =
            get_support_list_unitid(usb_dev->vid, usb_dev->pid);
        udb_handle->selector =
            get_support_list_selector(usb_dev->vid, usb_dev->pid);
        udb_handle->prot_size =
            get_support_list_prot_size(usb_dev->vid, usb_dev->pid);
        sprintf(udb_handle->dev_name, "/dev/video%d", video_index);
    }
    else
    {
        udb_handle->driver = DEV_DRIVR_USBDEVFS;

        sprintf(devpath, "/dev/bus/usb/%03d/%03d", usb_dev->busnum,
                usb_dev->devnum);

        fd = open(devpath, O_RDWR);
        if (fd < 0)
        {
            printf("Can't open uvc dev %s, pls check permission\n", devpath);
            exit(-1);
        }

        struct usbdevfs_getdriver getdrv;
        int                       r;
        getdrv.interface = 0;
        r                = ioctl(fd, USBDEVFS_GETDRIVER, &getdrv);
        if (!r)
        {
            perror("USBDEVFS_GETDRIVER");
            printf(
                "Can't get usbdevfs driver, some one get it??\nKernel V4L2 or "
                "Userspace\n\n");
            // return bRet;
            exit(0);
        }
        // TO DO claim interface
        udb_handle->devclass = usb_dev->usbclass;
        udb_handle->fd       = fd;
        udb_handle->unitid =
            get_support_list_unitid(usb_dev->vid, usb_dev->pid);
        udb_handle->selector =
            get_support_list_selector(usb_dev->vid, usb_dev->pid);
        udb_handle->prot_size =
            get_support_list_prot_size(usb_dev->vid, usb_dev->pid);
        sprintf(udb_handle->dev_name, "/dev/bus/usb/%03d/%03d", usb_dev->busnum,
                usb_dev->devnum);
    }

    bRet = true;
    return bRet;
}

bool get_udb_ctrl_handle_from_net(char *hostip, udb_ctrl_handle_t *udb_handle)
{
    bool bRet = false;
    int  fd;

    struct hostent    *hp; /* holds IP address of server */
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("socket udp failed!");
        exit(-1);
    }

    bzero((char *)&local_addr, sizeof(local_addr));
    local_addr.sin_family      = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port        = htons(0);

    if (bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        printf("bind udp socket failed!");
        exit(-1);
    }

    bzero((char *)&remote_addr, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port   = htons(UDB_NET_UDP_SERVER_PORT);

    hp = gethostbyname(hostip);
    if (hp == 0)
    {
        printf("could not obtain address of %s\n", hostip);
        return (-1);
    }

    bcopy(hp->h_addr_list[0], (caddr_t)&remote_addr.sin_addr, hp->h_length);

    // copy net handle
    udb_handle->devclass  = NET_CLASS_UDP;
    udb_handle->net_fd    = fd;
    udb_handle->prot_size = UDB_PROT_SIZE_NET_UDP;
    memcpy(&(udb_handle->net_server), &remote_addr, sizeof(remote_addr));

    bRet = true;
    return bRet;
}

int print_connected_list(void)
{
    DIR           *dir;
    struct dirent *dirent;
    FILE          *file;
    char           filename[PATH_MAX];
    char           line[1024];
    int            find = 0;

    int ret = -1;

    unsigned short vid, pid;
    unsigned char  busnum;
    unsigned char  devpath;
    unsigned char  devnum;
    int            video_index;

    char sysfs_product[1024];
    char sysfs_sn[1024];

    dir = opendir("/sys/bus/usb/devices");
    if (!dir)
    {
        printf("sysfs not mount?\n");
        return ret;
    }

    while ((dirent = readdir(dir)))
    {
        if (dirent->d_name[0] == '.')
        {
            continue;
        }
        sprintf(filename, "/sys/bus/usb/devices/%s/idVendor", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        vid = strtoul(line, NULL, 16);

        sprintf(filename, "/sys/bus/usb/devices/%s/idProduct", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        pid = strtoul(line, NULL, 16);

        if (in_support_list_all(vid, pid, USB_CLASS_UVC))
        {
            find++;
            sprintf(filename, "/sys/bus/usb/devices/%s/busnum", dirent->d_name);
            read_sysfs_node(filename, line, 1024);
            busnum = strtoul(line, NULL, 10);

            sprintf(filename, "/sys/bus/usb/devices/%s/devpath",
                    dirent->d_name);
            read_sysfs_node(filename, line, 1024);
            devpath = strtoul(line, NULL, 10);

            sprintf(filename, "/sys/bus/usb/devices/%s/devnum", dirent->d_name);
            read_sysfs_node(filename, line, 1024);
            devnum = strtoul(line, NULL, 10);

            sprintf(filename, "/sys/bus/usb/devices/%s/product",
                    dirent->d_name);
            read_sysfs_node(filename, sysfs_product, 1024);

            sprintf(filename, "/sys/bus/usb/devices/%s/serial", dirent->d_name);
            read_sysfs_node(filename, sysfs_sn, 1024);

            video_index = get_v4l2_index(busnum, devnum);
            if (video_index >= 0)
            {
                printf(
                    "%02d %s \t [Bus %03d Port %03d Dev %03d ID %04X:%04X] \t "
                    "SN:%s \t "
                    "[video%d]\n",
                    find, sysfs_product, busnum, devpath, devnum, vid, pid,
                    sysfs_sn, video_index);
            }
            else
            {
                printf(
                    "%02d %s \t [Bus %03d Port %03d Dev %03d ID %04X:%04X] \t "
                    "SN:%s \t "
                    "[usbfs]\n",
                    find, sysfs_product, busnum, devpath, devnum, vid, pid,
                    sysfs_sn);
            }
        }
    }
    if (find == 0)
    {
        printf("Can't find UVC Dev\n");
        exit(-1);
    }
    closedir(dir);
    return 0;
}

int print_lsusb_list(void)
{
    DIR           *dir;
    struct dirent *dirent;
    FILE          *file;
    char           filename[PATH_MAX];
    char           line[1024];

    int ret = -1;

    unsigned short vid, pid;
    unsigned char  busnum;
    unsigned char  devpath;
    unsigned char  devnum;
    unsigned int   speed;

    char sysfs_product[1024];
    char sysfs_sn[1024];

    dir = opendir("/sys/bus/usb/devices");
    if (!dir)
    {
        printf("sysfs not mount?\n");
        return ret;
    }

    while ((dirent = readdir(dir)))
    {
        if (dirent->d_name[0] == '.')
        {
            continue;
        }

        memset(line, 0, 1024);
        sprintf(filename, "/sys/bus/usb/devices/%s/idVendor", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        vid = strtoul(line, NULL, 16);
        if (vid == 0)
        {
            continue;
        }

        memset(line, 0, 1024);
        sprintf(filename, "/sys/bus/usb/devices/%s/idProduct", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        pid = strtoul(line, NULL, 16);

        memset(line, 0, 1024);
        sprintf(filename, "/sys/bus/usb/devices/%s/busnum", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        busnum = strtoul(line, NULL, 10);

        memset(line, 0, 1024);
        sprintf(filename, "/sys/bus/usb/devices/%s/devpath", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        devpath = strtoul(line, NULL, 10);

        memset(line, 0, 1024);
        sprintf(filename, "/sys/bus/usb/devices/%s/devnum", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        devnum = strtoul(line, NULL, 10);

        memset(line, 0, 1024);
        sprintf(filename, "/sys/bus/usb/devices/%s/speed", dirent->d_name);
        read_sysfs_node(filename, line, 1024);
        speed = strtoul(line, NULL, 10);

        memset(sysfs_product, 0, 1024);
        sprintf(filename, "/sys/bus/usb/devices/%s/product", dirent->d_name);
        read_sysfs_node(filename, sysfs_product, 1024);

        memset(sysfs_sn, 0, 1024);
        sprintf(filename, "/sys/bus/usb/devices/%s/serial", dirent->d_name);
        read_sysfs_node(filename, sysfs_sn, 1024);

        printf(
            "[Bus %03d Port %03d Dev %03d ID %04X:%04X Sp:%5d]  [%s] \t "
            "SN:%s\n",
            busnum, devpath, devnum, vid, pid, speed, sysfs_product, sysfs_sn);
    }

    closedir(dir);
    return 0;
}

void detach(void)
{
    if (dev_index > 0)
    {
        select_usb_dev_by_index(dev_index, &usb_dev);
        get_udb_ctrl_handle_from_usb(&usb_dev, &udb_ctrl);
    }

    if (usb_dev.usbclass == USB_CLASS_UVC)
    {
        uvc_detach_kernel_driver(&usb_dev);
    }
}

void attach(void)
{
    if (dev_index > 0)
    {
        select_usb_dev_by_index(dev_index, &usb_dev);
        get_udb_ctrl_handle_from_usb(&usb_dev, &udb_ctrl);
    }

    if (usb_dev.usbclass == USB_CLASS_UVC)
    {
        uvc_attach_kernel_driver(&usb_dev);
    }
}

void usbreset(void)
{
    if (dev_index > 0)
    {
        select_usb_dev_by_index(dev_index, &usb_dev);
        get_udb_ctrl_handle_from_usb(&usb_dev, &udb_ctrl);
    }

    int  fd;
    char dev_path[PATH_MAX];

    snprintf(dev_path, sizeof(dev_path) - 1, "/dev/bus/usb/%03d/%03d",
             usb_dev.busnum, usb_dev.devnum);

    printf("Resetting %s ... ", usb_dev.dev_sn);

    fd = open(dev_path, O_WRONLY);
    if (fd > -1)
    {
        if (ioctl(fd, USBDEVFS_RESET, 0) < 0)
        {
            printf("failed [%s]\n", strerror(errno));
        }
        else
        {
            printf("ok\n");
        }

        close(fd);
    }
    else
    {
        printf("can't open [%s]\n", strerror(errno));
    }
    return;
}

void usbmon(void)
{
    if (dev_index > 0)
    {
        select_usb_dev_by_index(dev_index, &usb_dev);
        get_udb_ctrl_handle_from_usb(&usb_dev, &udb_ctrl);
    }

    FILE *fp;
    char  dev_path[PATH_MAX];

    snprintf(dev_path, sizeof(dev_path) - 1, "/sys/kernel/debug/usb/usbmon/%uu",
             usb_dev.busnum);

    // printf("Starting monitor %s on bus %d  ... ", usb_dev.dev_sn,
    // usb_dev.busnum);

    fp = fopen(dev_path, "r");
    if (fileno(fp) > -1)
    {
        char buf[8192];
        while (fgets(buf, sizeof(buf), fp) != NULL)
        {
            fprintf(stdout, "%s", buf);
            fflush(stdout);
        };
        fclose(fp);
    }
    else
    {
        printf("can't open [%s]: %s\n", dev_path, strerror(errno));
        printf("sudo modprobe usbmon\n");
    }
    return;
}

void capture(int argc, char **argv)
{
    if (dev_index > 0)
    {
        select_usb_dev_by_index(dev_index, &usb_dev);
        get_udb_ctrl_handle_from_usb(&usb_dev, &udb_ctrl);
    }

    if (udb_ctrl.devclass != USB_CLASS_UVC)
    {
        return;
    }

    if (udb_ctrl.driver == DEV_DRIVR_USBDEVFS)
    {
        libuvc_handle_t libuvc_hdl;

        libuvc_hdl.width  = 640;
        libuvc_hdl.height = 400;
        libuvc_hdl.format = UVC_FRAME_FORMAT_MJPEG;

        libuvc_hdl.dump_path = NULL;
        libuvc_hdl.dump_fp   = NULL;
        libuvc_hdl.img_print = 0;

        int c;
        while ((c = getopt(argc, argv, "w:h:m:d:")) != EOF)
        {
            switch (c)
            {
                case 'w':
                    libuvc_hdl.width = atoi(optarg);
                    break;

                case 'h':
                    libuvc_hdl.height = atoi(optarg);
                    break;

                case 'm':
                    if (1 == atoi(optarg))
                    {
                        libuvc_hdl.format = UVC_FRAME_FORMAT_YUYV;
                    }
                    break;

                case 'd':
                    libuvc_hdl.dump_path = optarg;
                    break;

                default:
                    break;
            }
        }

        if (libuvc_hdl.format == UVC_FRAME_FORMAT_MJPEG)
        {
            printf("MJPEG Stream Cap W:%d H:%d\n", libuvc_hdl.width,
                   libuvc_hdl.height);
        }
        else
        {
            printf("YUYV  Stream Cap W:%d H:%d\n", libuvc_hdl.width,
                   libuvc_hdl.height);
        }

        if (libuvc_hdl.dump_path != NULL)
        {
            libuvc_hdl.dump_fp = fopen(libuvc_hdl.dump_path, "wb");
        }

        libuvc_init_device(&libuvc_hdl);
        libuvc_start_capturing(&libuvc_hdl);
        libuvc_cap_loop(&libuvc_hdl);
    }
    else if (udb_ctrl.driver == DEV_DRIVR_V4L2)
    {
        v4l2_handle_t v4l2_hdl;

        v4l2_hdl.width  = 640;
        v4l2_hdl.height = 400;
        v4l2_hdl.format = V4L2_PIX_FMT_MJPEG;

        v4l2_hdl.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        v4l2_hdl.dump_path = NULL;
        v4l2_hdl.dump_fp   = NULL;
        v4l2_hdl.img_print = 0;

        int c;
        while ((c = getopt(argc, argv, "w:h:m:d:")) != EOF)
        {
            switch (c)
            {
                case 'w':
                    v4l2_hdl.width = atoi(optarg);
                    break;

                case 'h':
                    v4l2_hdl.height = atoi(optarg);
                    break;

                case 'm':
                    if (1 == atoi(optarg))
                    {
                        v4l2_hdl.format = V4L2_PIX_FMT_YUYV;
                    }
                    break;

                case 'd':
                    v4l2_hdl.dump_path = optarg;
                    break;

                default:
                    break;
            }
        }

        if (v4l2_hdl.format == V4L2_PIX_FMT_MJPEG)
        {
            printf("MJPEG Stream Cap W:%d H:%d\n", v4l2_hdl.width,
                   v4l2_hdl.height);
        }
        else
        {
            printf("YUYV  Stream Cap W:%d H:%d\n", v4l2_hdl.width,
                   v4l2_hdl.height);
        }

        v4l2_hdl.fd       = udb_ctrl.fd;
        v4l2_hdl.dev_name = udb_ctrl.dev_name;

        if (v4l2_hdl.dump_path != NULL)
        {
            v4l2_hdl.dump_fp = fopen(v4l2_hdl.dump_path, "wb");
        }

        v4l2_init_device(&v4l2_hdl);
        v4l2_start_capturing(&v4l2_hdl);
        v4l2_cap_loop(&v4l2_hdl);
    }
}

void view(int argc, char **argv)
{
    if (dev_index > 0)
    {
        select_usb_dev_by_index(dev_index, &usb_dev);
        get_udb_ctrl_handle_from_usb(&usb_dev, &udb_ctrl);
    }

    if (udb_ctrl.devclass != USB_CLASS_UVC)
    {
        return;
    }

    if (udb_ctrl.driver == DEV_DRIVR_USBDEVFS)
    {
        libuvc_handle_t libuvc_hdl;

        libuvc_hdl.vid    = usb_dev.vid;
        libuvc_hdl.pid    = usb_dev.pid;
        libuvc_hdl.dev_sn = usb_dev.dev_sn;

        libuvc_hdl.width  = 640;
        libuvc_hdl.height = 400;
        libuvc_hdl.fps    = 30;
        libuvc_hdl.format = UVC_FRAME_FORMAT_YUYV;

        libuvc_hdl.img_compat           = 0;
        libuvc_hdl.img_print_batch_mode = 0;

        int c;
        while ((c = getopt(argc, argv, "cnrb")) != EOF)
        {
            switch (c)
            {
                case 'c':
                    libuvc_hdl.format = UVC_FRAME_FORMAT_MJPEG;
                    break;
                case 'n':
                    libuvc_hdl.img_compat = 1;
                    break;
                case 'r':
                    libuvc_hdl.img_resize = 1;
                    break;
                case 'b':
                    libuvc_hdl.img_print_batch_mode = 1;
                    break;
                default:
                    break;
            }
        }

        libuvc_hdl.dump_path = NULL;
        libuvc_hdl.dump_fp   = NULL;

        libuvc_hdl.img_print = 1;

        /* terminal 4*8 one pixel */
        if (libuvc_hdl.img_resize == 1)
        {
            libuvc_hdl.opt_width  = 0;
            libuvc_hdl.opt_height = 0;
        }
        else
        {
            libuvc_hdl.opt_width  = 640;
            libuvc_hdl.opt_height = 400;
        }

        libuvc_init_device(&libuvc_hdl);
        libuvc_start_capturing(&libuvc_hdl);
        libuvc_cap_loop(&libuvc_hdl);
    }
    else if (udb_ctrl.driver == DEV_DRIVR_V4L2)
    {
        v4l2_handle_t v4l2_hdl;

        v4l2_hdl.width  = 640;
        v4l2_hdl.height = 400;
        v4l2_hdl.format = V4L2_PIX_FMT_YUYV;

        v4l2_hdl.buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        v4l2_hdl.img_compat           = 0;
        v4l2_hdl.img_print_batch_mode = 0;

        int c;
        while ((c = getopt(argc, argv, "cnrb")) != EOF)
        {
            switch (c)
            {
                case 'c':
                    v4l2_hdl.format = V4L2_PIX_FMT_MJPEG;
                    break;
                case 'n':
                    v4l2_hdl.img_compat = 1;
                    break;
                case 'r':
                    v4l2_hdl.img_resize = 1;
                    break;
                case 'b':
                    v4l2_hdl.img_print_batch_mode = 1;
                    break;
                default:
                    break;
            }
        }
        v4l2_hdl.fd       = udb_ctrl.fd;
        v4l2_hdl.dev_name = udb_ctrl.dev_name;

        v4l2_hdl.dump_path = NULL;
        v4l2_hdl.dump_fp   = NULL;

        v4l2_hdl.img_print = 1;

        /* terminal 4*8 one pixel */
        if (v4l2_hdl.img_resize == 1)
        {
            v4l2_hdl.opt_width  = 0;
            v4l2_hdl.opt_height = 0;
        }
        else
        {
            v4l2_hdl.opt_width  = 640;
            v4l2_hdl.opt_height = 400;
        }

        v4l2_init_device(&v4l2_hdl);
        v4l2_start_capturing(&v4l2_hdl);
        v4l2_cap_loop(&v4l2_hdl);
    }
}
