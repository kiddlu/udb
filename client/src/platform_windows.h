#ifndef _PLATFORM_WINDOWS_H
#define _PLATFORM_WINDOWS_H

#include <atlstr.h>
#include <cfgmgr32.h>
#include <comsvcs.h>
#include <dshow.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include <setupapi.h>
#include <stdio.h>
#include <strmif.h>
#include <vidcap.h>
#include <windows.h>
#include <string>

#define MAX_ID_LEN (MAX_DEVICE_ID_LEN)

typedef struct
{
    unsigned short vid;
    unsigned short pid;
    unsigned short busnum;  // HUB
    unsigned short devnum;  // PORT

    unsigned char usbclass;
    unsigned char backend;

    wchar_t dev_sn[MAX_ID_LEN];
    wchar_t inst[MAX_ID_LEN];
} usb_dev_t;

typedef struct
{
    // public
    unsigned char  devclass;
    unsigned short prot_size;

    // uvc
    GUID          PROPSETID_VIDCAP_EXTENSION_UNIT;
    IKsControl   *pKsControl;
    int           nNodeId;
    unsigned char selector;

    // cdc
    HANDLE hSerialPort;

    int                net_fd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
    WSADATA            net_wsaData;
} udb_ctrl_handle_t;

#define ENABLE_PLATFORM_WINDOWS

int usleep(unsigned int useconds);

#endif
