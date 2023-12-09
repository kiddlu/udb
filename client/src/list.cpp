#include <stdbool.h>
#include <stdio.h>

#include "config.h"

#define NOVATEK_NT9856X_UVC_GUID                                              \
    {                                                                         \
        0xc987a729, 0x59d3, 0x4569, 0x84, 0x67, 0xff, 0x08, 0x49, 0xfc, 0x19, \
            0xe8                                                              \
    }

#define USB_SUPPORT_LIST                                                   \
    {0x0603,          0x8612, "iHawk100", USB_CLASS_UVC,                   \
     UDB_BACKEND_STD, 64,     0x01,       0x05,                            \
     USB_XFER_CTRL,   0x00,   0x00,       NOVATEK_NT9856X_UVC_GUID},       \
        {0x0603,          0x0001, "iHawk100", USB_CLASS_UVC,               \
         UDB_BACKEND_STD, 64,     0x01,       0x05,                        \
         USB_XFER_CTRL,   0x00,   0x00,       NOVATEK_NT9856X_UVC_GUID},   \
        {0x0603,          0x1001, "iHawk100V", USB_CLASS_UVC,              \
         UDB_BACKEND_STD, 64,     0x01,        0x05,                       \
         USB_XFER_CTRL,   0x00,   0x00,        NOVATEK_NT9856X_UVC_GUID},  \
        {0x0603,          0x0002, "iHawk100E", USB_CLASS_UVC,              \
         UDB_BACKEND_STD, 64,     0x01,        0x05,                       \
         USB_XFER_CTRL,   0x00,   0x00,        NOVATEK_NT9856X_UVC_GUID},  \
        {0x0603,          0x0004, "iHawk070", USB_CLASS_UVC,               \
         UDB_BACKEND_STD, 64,     0x01,       0x05,                        \
         USB_XFER_CTRL,   0x00,   0x00,       NOVATEK_NT9856X_UVC_GUID},   \
        {0x0603,          0x0005, "iHawk150W", USB_CLASS_UVC,              \
         UDB_BACKEND_STD, 64,     0x01,        0x05,                       \
         USB_XFER_CTRL,   0x00,   0x00,        NOVATEK_NT9856X_UVC_GUID},  \
        {0x0603,          0x1006, "iHawk020", USB_CLASS_UVC,               \
         UDB_BACKEND_STD, 64,     0x01,       0x05,                        \
         USB_XFER_CTRL,   0x00,   0x00,       NOVATEK_NT9856X_UVC_GUID},   \
        {0x0603,          0x0007, "iHawk100S", USB_CLASS_UVC,              \
         UDB_BACKEND_STD, 64,     0x01,        0x05,                       \
         USB_XFER_CTRL,   0x00,   0x00,        NOVATEK_NT9856X_UVC_GUID},  \
        {0x0603,          0x0008, "iHawk100K", USB_CLASS_UVC,              \
         UDB_BACKEND_STD, 64,     0x01,        0x05,                       \
         USB_XFER_CTRL,   0x00,   0x00,        NOVATEK_NT9856X_UVC_GUID},  \
        {0x0603,          0x0009, "iHawk100R", USB_CLASS_UVC,              \
         UDB_BACKEND_STD, 64,     0x01,        0x05,                       \
         USB_XFER_CTRL,   0x00,   0x00,        NOVATEK_NT9856X_UVC_GUID},  \
        {0x0603,          0x000A, "iHawk100Q", USB_CLASS_UVC,              \
         UDB_BACKEND_STD, 64,     0x01,        0x05,                       \
         USB_XFER_CTRL,   0x00,   0x00,        NOVATEK_NT9856X_UVC_GUID},  \
        {0x0603,          0x000B, "iHawk100Q1", USB_CLASS_UVC,             \
         UDB_BACKEND_STD, 64,     0x01,         0x05,                      \
         USB_XFER_CTRL,   0x00,   0x00,         NOVATEK_NT9856X_UVC_GUID}, \
        {0x0603,          0x000C, "iHawk071", USB_CLASS_UVC,               \
         UDB_BACKEND_STD, 64,     0x01,       0x05,                        \
         USB_XFER_CTRL,   0x00,   0x00,       NOVATEK_NT9856X_UVC_GUID},   \
        {0x0603,          0x000D, "iHawk137", USB_CLASS_UVC,               \
         UDB_BACKEND_STD, 64,     0x01,       0x05,                        \
         USB_XFER_CTRL,   0x00,   0x00,       NOVATEK_NT9856X_UVC_GUID},   \
        {0x0603,          0x000E, "iHawk150E", USB_CLASS_UVC,              \
         UDB_BACKEND_STD, 64,     0x01,        0x05,                       \
         USB_XFER_CTRL,   0x00,   0x00,        NOVATEK_NT9856X_UVC_GUID},  \
        {0x0603,          0x000F, "iHawk100R1", USB_CLASS_UVC,             \
         UDB_BACKEND_STD, 64,     0x01,         0x05,                      \
         USB_XFER_CTRL,   0x00,   0x00,         NOVATEK_NT9856X_UVC_GUID}, \
        {0x0603,          0x000F, "iHawk100R0", USB_CLASS_UVC,             \
         UDB_BACKEND_STD, 64,     0x01,         0x05,                      \
         USB_XFER_CTRL,   0x00,   0x00,         NOVATEK_NT9856X_UVC_GUID},

typedef struct
{
    unsigned long  data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char  data4[8];
} guid_t;

static const struct
{
    const unsigned short vid;
    const unsigned short pid;
    const char          *model;
    const unsigned char  usbclass;
    const unsigned char  backend;
    const unsigned short prot_size;

    const unsigned char selector;
    const unsigned char uintid;

    const unsigned char xfer_type;
    const unsigned char ep_write;
    const unsigned char ep_read;

    const guid_t guid;
} usb_support_list[] = {USB_SUPPORT_LIST};

#define FOREACH(x) ((int)(sizeof(x) / sizeof((x)[0])))

bool in_support_list(unsigned short vid, unsigned short pid)
{
    bool ret = false;

    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        if (vid == usb_support_list[i].vid && pid == usb_support_list[i].pid)
        {
            ret = true;
            return ret;
        }
    }

    return ret;
}

bool in_support_list_only_vid(unsigned short vid)
{
    bool ret = false;

    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        if (vid == usb_support_list[i].vid)
        {
            ret = true;
            return ret;
        }
    }

    return ret;
}

bool in_support_list_all(unsigned short vid,
                         unsigned short pid,
                         unsigned char  usbclass)
{
    bool ret = false;

    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        if (vid == usb_support_list[i].vid && pid == usb_support_list[i].pid &&
            usbclass == usb_support_list[i].usbclass)
        {
            ret = true;
            return ret;
        }
    }

    return ret;
}

void *get_support_list_guid(unsigned short vid, unsigned short pid)
{
    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        if (vid == usb_support_list[i].vid && pid == usb_support_list[i].pid)
        {
            return (void *)&(usb_support_list[i].guid);
        }
    }

    return NULL;
}
unsigned char get_support_list_usbclass(unsigned short vid, unsigned short pid)
{
    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        if (vid == usb_support_list[i].vid && pid == usb_support_list[i].pid)
        {
            return (usb_support_list[i].usbclass);
        }
    }

    return 0;
}

unsigned char get_support_list_backend(unsigned short vid, unsigned short pid)
{
    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        if (vid == usb_support_list[i].vid && pid == usb_support_list[i].pid)
        {
            return (usb_support_list[i].backend);
        }
    }

    return 0;
}

unsigned short get_support_list_prot_size(unsigned short vid,
                                          unsigned short pid)
{
    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        if (vid == usb_support_list[i].vid && pid == usb_support_list[i].pid)
        {
            return (usb_support_list[i].prot_size);
        }
    }

    return 0;
}

unsigned char get_support_list_selector(unsigned short vid, unsigned short pid)
{
    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        if (vid == usb_support_list[i].vid && pid == usb_support_list[i].pid)
        {
            return (usb_support_list[i].selector);
        }
    }

    return 0;
}

unsigned char get_support_list_unitid(unsigned short vid, unsigned short pid)
{
    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        if (vid == usb_support_list[i].vid && pid == usb_support_list[i].pid)
        {
            return (usb_support_list[i].uintid);
        }
    }

    return 0;
}

int print_support_list(void)
{
    for (int i = 0; i < FOREACH(usb_support_list); i++)
    {
        printf("vid 0x%04x pid 0x%04x: %s\n", usb_support_list[i].vid,
               usb_support_list[i].pid, usb_support_list[i].model);
    }
    return 0;
}