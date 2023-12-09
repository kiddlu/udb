#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "platform_windows.h"
#include "list.h"
#include "utils.h"

#include "config.h"

#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "ws2_32.lib")

// need vid/pid/sn to bus/port
static void get_usb_location(usb_dev_t *usb_dev)
{
    GUID            class_guid[1];
    unsigned long   required_size;
    bool            ret;
    HDEVINFO        devinfo_set = NULL;
    SP_DEVINFO_DATA devinfo_data;
    unsigned int    member_index = 0;
    int             find         = 0;

    devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);

    ret = SetupDiClassGuidsFromName(L"USB", (LPGUID)&class_guid, 1,
                                    &required_size);
    if (ret != true)
    {
        printf("SetupDiClassGuidsFromName Error\n");
        goto cleanup;
    }

    // Get class devices
    devinfo_set = SetupDiGetClassDevs(&class_guid[0], NULL, NULL,
                                      DIGCF_PRESENT | DIGCF_PROFILE);

    if (devinfo_set)
    {
        // Enumerate devices
        member_index = 0;
        while (
            SetupDiEnumDeviceInfo(devinfo_set, member_index++, &devinfo_data))
        {
            char           device_path[MAX_ID_LEN];
            char           device_location[MAX_ID_LEN];
            char           usbidstr[5];
            unsigned long  req_size = 0;
            unsigned long  prop_type;
            unsigned long  type = REG_SZ;
            unsigned short vid  = 0;
            unsigned short pid  = 0;
            HKEY           hKey = NULL;

            ret = SetupDiGetDeviceInstanceId(devinfo_set, &devinfo_data,
                                             (PWSTR)device_path,
                                             sizeof(device_path), &req_size);

            // dumphex(hardware_id, MAX_PATH);
            usbidstr[0] = device_path[16];
            usbidstr[1] = device_path[18];
            usbidstr[2] = device_path[20];
            usbidstr[3] = device_path[22];
            usbidstr[4] = 0;
            vid         = (unsigned short)strtoul(usbidstr, NULL, 16);

            usbidstr[0] = device_path[34];
            usbidstr[1] = device_path[36];
            usbidstr[2] = device_path[38];
            usbidstr[3] = device_path[40];
            usbidstr[4] = 0;
            pid         = (unsigned short)strtoul(usbidstr, NULL, 16);

            // printf("vid 0x%04x pid 0x%04x\n", vid, pid);
            if (usb_dev->vid == vid && usb_dev->pid == pid)
            {
                if (wcsncmp(usb_dev->dev_sn, (wchar_t *)device_path + 22,
                            wcslen(((wchar_t *)device_path + 22))) == 0)
                {
                    ret = SetupDiGetDeviceRegistryProperty(
                        devinfo_set, &devinfo_data, SPDRP_LOCATION_INFORMATION,
                        &prop_type, (LPBYTE)device_location,
                        sizeof(device_location), &req_size);
                    usbidstr[0] = device_location[12];
                    usbidstr[1] = device_location[14];
                    usbidstr[2] = device_location[16];
                    usbidstr[3] = device_location[18];
                    usbidstr[4] = 0;
                    usb_dev->devnum =
                        (unsigned short)strtoul(usbidstr, NULL, 10);

                    usbidstr[0] = device_location[32];
                    usbidstr[1] = device_location[34];
                    usbidstr[2] = device_location[36];
                    usbidstr[3] = device_location[38];
                    usbidstr[4] = 0;
                    usb_dev->busnum =
                        (unsigned short)strtoul(usbidstr, NULL, 10);
                    // wprintf(L"device_path %s\n", (wchar_t *)device_path);
                    // wprintf(L"device_location %s\n", (wchar_t
                    // *)device_location); printf("dev bus %d, port %d\n",
                    // usb_dev->busnum, usb_dev->devnum);
                }
            }
        }
    }
cleanup:
    // Destroy device info list
    SetupDiDestroyDeviceInfoList(devinfo_set);
}

static HRESULT uvc_handle_find_extension_node(IKsTopologyInfo *pIksTopologyInfo,
                                              GUID             extensionGuid,
                                              DWORD           *pNodeId)
{
    DWORD   numberOfNodes;
    HRESULT hRet = S_FALSE;

    hRet = pIksTopologyInfo->get_NumNodes(&numberOfNodes);
    if (SUCCEEDED(hRet))
    {
        DWORD i;
        GUID  nodeGuid;
        for (i = 0; i < numberOfNodes; i++)
        {
            if (SUCCEEDED(pIksTopologyInfo->get_NodeType(i, &nodeGuid)))
            {
                if (IsEqualGUID(extensionGuid, nodeGuid))
                {  // Found the extension node
                    *pNodeId = i;
                    hRet     = S_OK;
                    break;
                }
            }
        }

        if (i == numberOfNodes)
        {  // Did not find the node
            hRet = S_FALSE;
        }
    }
    return hRet;
}

static bool uvc_handle_get_node_id(IBaseFilter *pCaptureFilter, int *pNodeId)
{

    IKsTopologyInfo *pKsToplogyInfo;
    HRESULT          hRet;
    DWORD            dwNode;

    if (!pCaptureFilter)
        return false;

    hRet = pCaptureFilter->QueryInterface(__uuidof(IKsTopologyInfo),
                                          (void **)(&pKsToplogyInfo));
    if (S_OK == hRet)
    {
        hRet = uvc_handle_find_extension_node(
            pKsToplogyInfo, KSNODETYPE_DEV_SPECIFIC,
            &dwNode);  // KSNODETYPE_DEV_SPECIFIC
                       // PROPSETID_VIDCAP_EXTENSION_UNIT
        pKsToplogyInfo->Release();

        if (S_OK == hRet)
        {
            *pNodeId = dwNode;
            return TRUE;
        }
    }
    return false;
}

static bool uvc_handle_get_vcap_filter(IBaseFilter  **piFilter,
                                       unsigned short usVid,
                                       unsigned short usPid,
                                       wchar_t       *inst)
{
    HRESULT         hr;
    ICreateDevEnum *piCreateDevEnum;
    bool            bRet = false;

    CoInitialize(NULL);

    HRESULT hRet = CoCreateInstance(
        CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        __uuidof(piCreateDevEnum), reinterpret_cast<void **>(&piCreateDevEnum));
    if (SUCCEEDED(hRet))
    {
        IEnumMoniker *piEnumMoniker;
        hRet = piCreateDevEnum->CreateClassEnumerator(
            CLSID_VideoInputDeviceCategory, &piEnumMoniker, 0);
        piCreateDevEnum->Release();

        if (S_OK == hRet)
            hRet = piEnumMoniker->Reset();

        if (S_OK == hRet)
        {
            // Enumerate KS devices
            unsigned long cFetched;
            IMoniker     *piMoniker;
            CString       strDevicePath;
            CString       strMatch;
            CString       strInst;

            for (unsigned int i = 0; i < wcslen(inst); i++)
                strInst.AppendChar(inst[i]);

            while ((hRet = piEnumMoniker->Next(1, &piMoniker, &cFetched)) ==
                   S_OK)
            {

                IPropertyBag *pBag = 0;
                hr = piMoniker->BindToStorage(0, 0, IID_IPropertyBag,
                                              (void **)&pBag);
                if (SUCCEEDED(hr))
                {
                    VARIANT var;
                    var.vt = VT_BSTR;
                    hr     = pBag->Read(_T("DevicePath"), &var,
                                        NULL);  // FriendlyName
                    if (hr == NOERROR)
                    {
                        strDevicePath = var.bstrVal;
                        SysFreeString(var.bstrVal);
                        strDevicePath.MakeUpper();
                        strMatch.Format(_T("VID_%04X&PID_%04X"), usVid, usPid);
                        // printf("DevPath  %S\n", strDevicePath.GetBuffer(0));
                        // printf("strMatch  %S\n", strMatch.GetBuffer(0));
                        // printf("strInst   %S\n", strInst.GetBuffer(0));

                        if (strDevicePath.Find(strMatch) != -1)
                        {
                            if (strDevicePath.Find(strInst) != -1)
                            {
                                hRet = piMoniker->BindToObject(
                                    NULL, NULL, IID_IBaseFilter,
                                    (void **)(piFilter));
                                pBag->Release();
                                piMoniker->Release();

                                if (SUCCEEDED(hRet))
                                {
                                    // printf("True\n");
                                    bRet = TRUE;
                                }
                                break;
                            }
                        }
                    }
                    pBag->Release();
                }
                piMoniker->Release();
            }

            piEnumMoniker->Release();
        }
    }

    return bRet;
}

static bool uvc_handle_get_iks_ctrl(IBaseFilter *pCaptureFilter,
                                    IKsControl **ppKsControl)
{
    if (!pCaptureFilter)
        return false;

    HRESULT hr;
    hr = pCaptureFilter->QueryInterface(__uuidof(IKsControl),
                                        (void **)ppKsControl);

    if (hr != S_OK)
    {
        return false;
    }

    return true;
}

static bool uvc_hal_get_ctrl_handle(usb_dev_t         *usb_dev,
                                    udb_ctrl_handle_t *udb_handle)
{
    bool bRet = false;

    IBaseFilter *pCaptureFilter = NULL;
    IKsControl  *pKsControl     = NULL;
    int          nNodeId        = 0;

    if (!uvc_handle_get_vcap_filter(&pCaptureFilter, usb_dev->vid, usb_dev->pid,
                                    usb_dev->inst))
    {
        printf("uvc_handle_get_vcap_filter failed\n");
        goto Exit_Uvc;
    }

    if (!uvc_handle_get_node_id(pCaptureFilter, &nNodeId))
    {
        printf("uvc_handle_get_node_id failed\n");
        goto Exit_Uvc;
    }
    // printf("nNodeId is %d\n", nNodeId);

    if (!uvc_handle_get_iks_ctrl(pCaptureFilter, &pKsControl))
    {
        printf("uvc_handle_get_iks_ctrl failed\n");
        goto Exit_Uvc;
    }

    udb_handle->pKsControl = pKsControl;
    udb_handle->nNodeId    = nNodeId;
    udb_handle->PROPSETID_VIDCAP_EXTENSION_UNIT =
        *((GUID *)get_support_list_guid(usb_dev->vid, usb_dev->pid));
    udb_handle->selector =
        get_support_list_selector(usb_dev->vid, usb_dev->pid);
    udb_handle->prot_size =
        get_support_list_prot_size(usb_dev->vid, usb_dev->pid);
    udb_handle->devclass = usb_dev->usbclass;

    bRet = TRUE;

    return bRet;

Exit_Uvc:
    if (pKsControl)
    {
        pKsControl->Release();
    }
    if (pCaptureFilter)
    {
        pCaptureFilter->Release();
    }
    return bRet;
}

static bool uvc_hal_write(udb_ctrl_handle_t *udb_handle,
                          unsigned char     *buffer,
                          unsigned long      size)
{
    bool bRet = false;

    KSP_NODE ExtensionProp;
    HRESULT  hr;

    unsigned long BytesReturned;

    if (udb_handle->pKsControl == NULL)
    {
        printf("pKsControl NULL\n");
        return bRet;
    }

    ExtensionProp.Property.Set = udb_handle->PROPSETID_VIDCAP_EXTENSION_UNIT;
    ExtensionProp.Property.Id  = udb_handle->selector;
    // KSPROPERTY_EXTENSION_UNIT_INFO = 0x00,
    // KSPROPERTY_EXTENSION_UNIT_CONTROL = 0x01;
    ExtensionProp.NodeId = udb_handle->nNodeId;
    ExtensionProp.Property.Flags =
        KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;

    hr = udb_handle->pKsControl->KsProperty(
        (PKSPROPERTY)&ExtensionProp, sizeof(ExtensionProp), (PVOID)buffer, size,
        &BytesReturned);

    if (hr != S_OK)
    {
        printf("KsProperty failed\n");
        return bRet;
    }

    bRet = TRUE;

    return bRet;
}

static bool uvc_hal_read(udb_ctrl_handle_t *udb_handle,
                         unsigned char     *buffer,
                         unsigned long      size,
                         unsigned long     *BytesReturned)
{
    bool bRet = false;

    KSP_NODE ExtensionProp;
    HRESULT  hr;

    if (udb_handle->pKsControl == NULL)
    {
        printf("pKsControl NULL\n");
        return bRet;
    }

    ExtensionProp.Property.Set = udb_handle->PROPSETID_VIDCAP_EXTENSION_UNIT;
    ExtensionProp.Property.Id  = udb_handle->selector;
    // KSPROPERTY_EXTENSION_UNIT_INFO = 0x00,
    // KSPROPERTY_EXTENSION_UNIT_CONTROL = 0x01;
    ExtensionProp.NodeId = udb_handle->nNodeId;
    ExtensionProp.Property.Flags =
        KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;

    hr = udb_handle->pKsControl->KsProperty((PKSPROPERTY)&ExtensionProp,
                                            sizeof(ExtensionProp),
                                            (PVOID)buffer, size, BytesReturned);

    if (hr != S_OK)
    {
        printf("KsProperty failed\n");
        return bRet;
    }

    bRet = TRUE;

    return bRet;
}

static int cdc_handle_get_port_num(unsigned short vid,
                                   unsigned short pid,
                                   wchar_t       *inst)
{
    GUID            class_guid[1];
    unsigned long   required_size;
    bool            ret;
    HDEVINFO        devinfo_set = NULL;
    SP_DEVINFO_DATA devinfo_data;
    unsigned int    member_index = 0;

    int  port_num = -1;
    char port_num_str[4];

    devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);

    // Get ClassGuid from ClassName for PORTS class
    ret = SetupDiClassGuidsFromName(L"Ports", (LPGUID)&class_guid, 1,
                                    &required_size);
    if (ret != true)
    {
        printf("SetupDiClassGuidsFromName Error\n");
        goto cleanup;
    }

    // Get class devices
    devinfo_set = SetupDiGetClassDevs(&class_guid[0], NULL, NULL,
                                      DIGCF_PRESENT | DIGCF_PROFILE);

    if (devinfo_set)
    {
        // Enumerate devices
        member_index = 0;
        while (
            SetupDiEnumDeviceInfo(devinfo_set, member_index++, &devinfo_data))
        {
            char          port_name[MAX_ID_LEN];
            char          device_path[MAX_ID_LEN];
            char         *cur_inst = device_path + 56;
            unsigned long req_size = 0;
            unsigned long type     = REG_SZ;
            HKEY          hKey     = NULL;

            ret = SetupDiGetDeviceInstanceId(devinfo_set, &devinfo_data,
                                             (PWSTR)device_path,
                                             sizeof(device_path), &req_size);

            // wprintf(L"device_path %s\n", (wchar_t *)cur_inst);
            // wprintf(L"inst %s\n", (wchar_t *)inst);

            if (wcsncmp(inst, (wchar_t *)cur_inst, wcslen(inst)) != 0)
            {
                continue;
            }

            // Open device parameters reg key
            hKey =
                SetupDiOpenDevRegKey(devinfo_set, &devinfo_data,
                                     DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            if (hKey)
            {
                // Qurey for portname
                req_size = sizeof(port_name);
                RegQueryValueEx(hKey, L"PortName", 0, &type,
                                (unsigned char *)&port_name, &req_size);

                // Close reg key
                RegCloseKey(hKey);
            }

            port_num_str[0] = port_name[6];
            port_num_str[1] = port_name[8];
            port_num_str[2] = port_name[10];
            // wprintf(L"port_name %s\n", (wchar_t *)port_name);
            // printf("port_num_str %s\n", port_num_str);
            port_num = strtoul(port_num_str, NULL, 10);
            break;
        }
    }

cleanup:
    SetupDiDestroyDeviceInfoList(devinfo_set);

    return port_num;
}

static bool cdc_handle_get_port_hdl(int port, HANDLE *pSerialPort)
{
    HANDLE hCom = NULL;

    int combuf_size = 8192;

    wchar_t portname[255];

    memset(portname, 0, sizeof(portname));

    swprintf_s(portname, L"\\\\.\\COM%d", port);

    hCom = CreateFile(portname, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                      OPEN_EXISTING, 0, NULL);

    if (hCom == (HANDLE)-1)
    {
        return false;
    }

    if (!SetupComm(hCom, combuf_size, combuf_size))
    {
        return false;
    }

    COMMTIMEOUTS TimeOuts;
    TimeOuts.ReadIntervalTimeout         = 0;
    TimeOuts.ReadTotalTimeoutMultiplier  = 0;
    TimeOuts.ReadTotalTimeoutConstant    = 1000;
    TimeOuts.WriteTotalTimeoutMultiplier = 0;
    TimeOuts.WriteTotalTimeoutConstant   = 0;

    SetCommTimeouts(hCom, &TimeOuts);

    DCB p;
    memset(&p, 0, sizeof(p));

    if (!GetCommState(hCom, &p))
    {
        printf("GetCommState error\n");
        return false;
    }

    // p.BaudRate = 115200;
    p.fDtrControl = 1;
    p.fRtsControl = 1;

    if (!SetCommState(hCom, &p))
    {
        printf("SetCommState error\n");
        return false;
    }

    PurgeComm(hCom, PURGE_TXCLEAR | PURGE_RXCLEAR);

    memcpy(pSerialPort, &hCom, sizeof(hCom));

    return true;
}

static bool cdc_hal_get_ctrl_handle(usb_dev_t         *usb_dev,
                                    udb_ctrl_handle_t *udb_handle)
{
    bool bRet = false;

    int port =
        cdc_handle_get_port_num(usb_dev->vid, usb_dev->pid, usb_dev->inst);

    cdc_handle_get_port_hdl(port, &(udb_handle->hSerialPort));

    udb_handle->devclass = USB_CLASS_CDC;

    udb_handle->prot_size =
        get_support_list_prot_size(usb_dev->vid, usb_dev->pid);

    bRet = true;

    return bRet;
}

static bool cdc_hal_write(udb_ctrl_handle_t *udb_handle,
                          unsigned char     *buffer,
                          unsigned long      size)
{
    bool bRet = false;

    HANDLE hCom = udb_handle->hSerialPort;

    DWORD dwBytesWrite = size;

    BOOL bWriteStat = WriteFile(hCom, buffer, size, &dwBytesWrite, NULL);
    if (!bWriteStat)
    {
        bRet = false;
    }
    else
    {
        bRet = true;
    }

    return bRet;
}

static bool cdc_hal_read(udb_ctrl_handle_t *udb_handle,
                         unsigned char     *buffer,
                         unsigned long      size,
                         unsigned long     *BytesReturned)
{
    bool bRet = false;

    HANDLE hCom = udb_handle->hSerialPort;

    DWORD wCount    = size;
    BOOL  bReadStat = ReadFile(hCom, buffer, wCount, BytesReturned, NULL);
    if (!bReadStat)
    {
        printf("cdc read error\n");
        return false;
    }

    bRet = true;

    return bRet;
}

bool net_hal_write(udb_ctrl_handle_t *udb_handle,
                   unsigned char     *buffer,
                   unsigned long      size)
{
    sendto(udb_handle->net_fd, (const char *)buffer, size, 0,
           (struct sockaddr *)(&udb_handle->remote_addr),
           sizeof(struct sockaddr_in));

    return true;
}

bool net_hal_read(udb_ctrl_handle_t *udb_handle,
                  unsigned char     *buffer,
                  unsigned long      size,
                  unsigned long     *BytesReturned)
{
    int addrlen = sizeof(struct sockaddr);

    int recv_len = 0;

    recv_len =
        recvfrom(udb_handle->net_fd, (char *)buffer, size, 0,
                 (struct sockaddr *)(&udb_handle->remote_addr), &addrlen);

    if (recv_len < 0)
    {
        *BytesReturned = 0;
    }
    else
    {
        *BytesReturned = recv_len;
    }

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
        case USB_CLASS_CDC:
            return cdc_hal_write(udb_handle, buffer, size);
        case NET_CLASS_UDP:
            return net_hal_write(udb_handle, buffer, size);
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
        case USB_CLASS_CDC:
            return cdc_hal_read(udb_handle, buffer, size, BytesReturned);
        case NET_CLASS_UDP:
            return net_hal_read(udb_handle, buffer, size, BytesReturned);
        case USB_CLASS_HID:
        default:
            printf("Can not udb ctrl write\n");
            break;
    }

    return false;
}

void select_usb_dev_by_index(int index, usb_dev_t *usb_dev)
{
    bool            ret;
    HDEVINFO        devinfo_set = NULL;
    SP_DEVINFO_DATA devinfo_data;
    unsigned int    member_index = 0;
    int             find         = 0;

    devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
    // Get class devices
    devinfo_set = SetupDiGetClassDevs(
        NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT | DIGCF_PROFILE);
    if (devinfo_set)
    {
        // Enumerate devices
        member_index = 0;
        while (
            SetupDiEnumDeviceInfo(devinfo_set, member_index++, &devinfo_data))
        {
            char           device_path[MAX_ID_LEN];
            char           parent_id[MAX_ID_LEN];
            char           device_class[MAX_ID_LEN];
            char           usbidstr[5];
            unsigned long  req_size = 0;
            unsigned long  prop_type;
            unsigned long  type           = REG_SZ;
            unsigned short vid            = 0;
            unsigned short pid            = 0;
            unsigned char  usbclass       = 0;
            HKEY           hKey           = NULL;
            DEVINST        devinst_parent = 0;

            ret = SetupDiGetDeviceRegistryProperty(
                devinfo_set, &devinfo_data, SPDRP_CLASS, &prop_type,
                (LPBYTE)device_class, sizeof(device_class), &req_size);

            ret = SetupDiGetDeviceInstanceId(devinfo_set, &devinfo_data,
                                             (PWSTR)device_path,
                                             sizeof(device_path), &req_size);

            usbidstr[0] = device_path[16];
            usbidstr[1] = device_path[18];
            usbidstr[2] = device_path[20];
            usbidstr[3] = device_path[22];
            usbidstr[4] = 0;
            vid         = (unsigned short)strtoul(usbidstr, NULL, 16);

            usbidstr[0] = device_path[34];
            usbidstr[1] = device_path[36];
            usbidstr[2] = device_path[38];
            usbidstr[3] = device_path[40];
            usbidstr[4] = 0;
            pid         = (unsigned short)strtoul(usbidstr, NULL, 16);

            if (wcsncmp((const wchar_t *)device_class, L"Camera",
                        wcslen(L"Camera")) == 0)
            {
                usbclass = USB_CLASS_UVC;
            }
            else if (wcsncmp((const wchar_t *)device_class, L"Image",
                             wcslen(L"Image")) == 0)
            {
                usbclass = USB_CLASS_UVC;
            }
            else if (wcsncmp((const wchar_t *)device_class, L"Ports",
                             wcslen(L"Ports")) == 0)
            {
                usbclass = USB_CLASS_CDC;
            }
            else if (wcsncmp((const wchar_t *)device_class, L"HIDClass",
                             wcslen(L"HIDClass")) == 0)
            {
                usbclass = USB_CLASS_HID;
            }

            // printf("vid 0x%04x pid 0x%04x\n", vid, pid);
            // wprintf(L"device class %s\n", (wchar_t *)device_class);
            if (in_support_list_all(vid, pid, usbclass) == true)
            {
                find++;
                if (index == find && usb_dev != NULL)
                {

                    CONFIGRET cr =
                        CM_Get_Parent(&devinst_parent, devinfo_data.DevInst, 0);
                    cr = CM_Get_Device_ID(devinst_parent, (PWSTR)parent_id,
                                          MAX_ID_LEN, 0);

                    usb_dev->vid      = vid;
                    usb_dev->pid      = pid;
                    usb_dev->usbclass = get_support_list_usbclass(vid, pid);
                    usb_dev->backend  = get_support_list_backend(vid, pid);
                    memcpy(usb_dev->dev_sn, parent_id + 44, MAX_ID_LEN);
                    memcpy(usb_dev->inst, device_path + 56, MAX_ID_LEN);
                    get_usb_location(usb_dev);

                    break;
                }
            }
        }
        if (find == 0)
        {
            printf("Can't find udb dev\n");
            exit(-1);
        }
    }

    SetupDiDestroyDeviceInfoList(devinfo_set);

    return;
}

bool get_udb_ctrl_handle_from_usb(usb_dev_t         *usb_dev,
                                  udb_ctrl_handle_t *udb_handle)
{
    switch (usb_dev->usbclass)
    {
        case USB_CLASS_UVC:
            return uvc_hal_get_ctrl_handle(usb_dev, udb_handle);
        case USB_CLASS_CDC:
            return cdc_hal_get_ctrl_handle(usb_dev, udb_handle);
        case USB_CLASS_HID:
        default:
            printf("Can not get usb ctrl handle\n");
            break;
    }

    return false;
}

bool get_udb_ctrl_handle_from_net(char *hostip, udb_ctrl_handle_t *udb_handle)
{
    bool bRet = false;

    WSAStartup(MAKEWORD(2, 2), &(udb_handle->net_wsaData));

    udb_handle->net_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udb_handle->net_fd == INVALID_SOCKET)
    {
        printf("socket udp failed!");
        exit(-1);
    }

    memset((char *)&(udb_handle->local_addr), 0, sizeof(struct sockaddr_in));
    udb_handle->local_addr.sin_family           = AF_INET;
    udb_handle->local_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    udb_handle->local_addr.sin_port             = htons(0);

    if (bind(udb_handle->net_fd, (struct sockaddr *)&(udb_handle->local_addr),
             sizeof(struct sockaddr_in)) < 0)
    {
        printf("bind udp socket failed!");
        exit(-1);
    }

    memset((char *)&(udb_handle->remote_addr), 0, sizeof(struct sockaddr_in));
    udb_handle->remote_addr.sin_family = AF_INET;
    udb_handle->remote_addr.sin_port   = htons(UDB_NET_UDP_SERVER_PORT);
    udb_handle->remote_addr.sin_addr.S_un.S_addr = inet_addr(hostip);

    udb_handle->devclass  = NET_CLASS_UDP;
    udb_handle->prot_size = UDB_PROT_SIZE_NET_UDP;

    int nTimeout = 100;
    setsockopt(udb_handle->net_fd, SOL_SOCKET, SO_SNDTIMEO,
               (const char *)&nTimeout, sizeof(nTimeout));
    setsockopt(udb_handle->net_fd, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&nTimeout, sizeof(nTimeout));

    bRet = true;
    return bRet;
}

int print_connected_list(void)
{
    bool            ret;
    HDEVINFO        devinfo_set = NULL;
    SP_DEVINFO_DATA devinfo_data;
    unsigned int    member_index = 0;
    int             find         = 0;

    devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
    // Get class devices
    devinfo_set = SetupDiGetClassDevs(
        NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT | DIGCF_PROFILE);
    if (devinfo_set)
    {
        // Enumerate devices
        member_index = 0;
        while (
            SetupDiEnumDeviceInfo(devinfo_set, member_index++, &devinfo_data))
        {
            char           friendly_name[MAX_ID_LEN];
            char           device_path[MAX_ID_LEN];
            char           parent_id[MAX_ID_LEN];
            char           device_class[MAX_ID_LEN];
            char           usbidstr[5];
            unsigned long  req_size = 0;
            unsigned long  prop_type;
            unsigned long  type           = REG_SZ;
            unsigned short vid            = 0;
            unsigned short pid            = 0;
            unsigned char  usbclass       = 0;
            HKEY           hKey           = NULL;
            DEVINST        devinst_parent = 0;

            ret = SetupDiGetDeviceRegistryProperty(
                devinfo_set, &devinfo_data, SPDRP_CLASS, &prop_type,
                (LPBYTE)device_class, sizeof(device_class), &req_size);

            ret = SetupDiGetDeviceInstanceId(devinfo_set, &devinfo_data,
                                             (PWSTR)device_path,
                                             sizeof(device_path), &req_size);
            // dumphex(hardware_id, MAX_ID_LEN);
            usbidstr[0] = device_path[16];
            usbidstr[1] = device_path[18];
            usbidstr[2] = device_path[20];
            usbidstr[3] = device_path[22];
            usbidstr[4] = 0;
            vid         = (unsigned short)strtoul(usbidstr, NULL, 16);

            usbidstr[0] = device_path[34];
            usbidstr[1] = device_path[36];
            usbidstr[2] = device_path[38];
            usbidstr[3] = device_path[40];
            usbidstr[4] = 0;
            pid         = (unsigned short)strtoul(usbidstr, NULL, 16);

            if (wcsncmp((const wchar_t *)device_class, L"Camera",
                        wcslen(L"Camera")) == 0)
            {
                usbclass = USB_CLASS_UVC;
            }
            else if (wcsncmp((const wchar_t *)device_class, L"Image",
                             wcslen(L"Image")) == 0)
            {
                usbclass = USB_CLASS_UVC;
            }
            else if (wcsncmp((const wchar_t *)device_class, L"Ports",
                             wcslen(L"Ports")) == 0)
            {
                usbclass = USB_CLASS_CDC;
            }
            else if (wcsncmp((const wchar_t *)device_class, L"HIDClass",
                             wcslen(L"HIDClass")) == 0)
            {
                usbclass = USB_CLASS_HID;
            }

            // printf("vid 0x%04x pid 0x%04x\n", vid, pid);
            // wprintf(L"device class %s\n", (wchar_t *)device_class);
            if (in_support_list_all(vid, pid, usbclass) == true)
            {
                find++;

                ret = SetupDiGetDeviceRegistryProperty(
                    devinfo_set, &devinfo_data, SPDRP_FRIENDLYNAME, &prop_type,
                    (LPBYTE)friendly_name, sizeof(friendly_name), &req_size);

                CONFIGRET cr =
                    CM_Get_Parent(&devinst_parent, devinfo_data.DevInst, 0);
                cr = CM_Get_Device_ID(devinst_parent, (PWSTR)parent_id,
                                      MAX_ID_LEN, 0);

                usb_dev_t cur_udev;

                cur_udev.vid = vid;
                cur_udev.pid = pid;
                memcpy(cur_udev.dev_sn, parent_id + 44, MAX_ID_LEN);
                get_usb_location(&cur_udev);
                wprintf(L"%02d %s %s [Hub %03d Port %03d] SN:%s\n", find,
                        (wchar_t *)friendly_name, (wchar_t *)device_path,
                        cur_udev.busnum, cur_udev.devnum, cur_udev.dev_sn);
            }
        }
        if (find == 0)
        {
            printf("Can't find udb dev\n");
            exit(-1);
        }
    }

    SetupDiDestroyDeviceInfoList(devinfo_set);

    return 0;
}

int convert_sn_to_index(char *sn)
{
    bool            ret;
    HDEVINFO        devinfo_set = NULL;
    SP_DEVINFO_DATA devinfo_data;
    unsigned int    member_index = 0;
    int             find         = 0;
    int             dev_index    = 0;

    devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
    // Get class devices
    devinfo_set = SetupDiGetClassDevs(
        NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT | DIGCF_PROFILE);
    if (devinfo_set)
    {
        // Enumerate devices
        member_index = 0;
        while (
            SetupDiEnumDeviceInfo(devinfo_set, member_index++, &devinfo_data))
        {
            char           device_path[MAX_ID_LEN];
            char           parent_id[MAX_ID_LEN];
            char           device_class[MAX_ID_LEN];
            char           usbidstr[5];
            unsigned long  req_size = 0;
            unsigned long  prop_type;
            unsigned long  type           = REG_SZ;
            unsigned short vid            = 0;
            unsigned short pid            = 0;
            unsigned char  usbclass       = 0;
            HKEY           hKey           = NULL;
            DEVINST        devinst_parent = 0;

            ret = SetupDiGetDeviceRegistryProperty(
                devinfo_set, &devinfo_data, SPDRP_CLASS, &prop_type,
                (LPBYTE)device_class, sizeof(device_class), &req_size);

            ret = SetupDiGetDeviceInstanceId(devinfo_set, &devinfo_data,
                                             (PWSTR)device_path,
                                             sizeof(device_path), &req_size);

            usbidstr[0] = device_path[16];
            usbidstr[1] = device_path[18];
            usbidstr[2] = device_path[20];
            usbidstr[3] = device_path[22];
            usbidstr[4] = 0;
            vid         = (unsigned short)strtoul(usbidstr, NULL, 16);

            usbidstr[0] = device_path[34];
            usbidstr[1] = device_path[36];
            usbidstr[2] = device_path[38];
            usbidstr[3] = device_path[40];
            usbidstr[4] = 0;
            pid         = (unsigned short)strtoul(usbidstr, NULL, 16);

            if (wcsncmp((const wchar_t *)device_class, L"Camera",
                        wcslen(L"Camera")) == 0)
            {
                usbclass = USB_CLASS_UVC;
            }
            else if (wcsncmp((const wchar_t *)device_class, L"Image",
                             wcslen(L"Image")) == 0)
            {
                usbclass = USB_CLASS_UVC;
            }
            else if (wcsncmp((const wchar_t *)device_class, L"Ports",
                             wcslen(L"Ports")) == 0)
            {
                usbclass = USB_CLASS_CDC;
            }
            else if (wcsncmp((const wchar_t *)device_class, L"HIDClass",
                             wcslen(L"HIDClass")) == 0)
            {
                usbclass = USB_CLASS_HID;
            }

            if (in_support_list_all(vid, pid, usbclass) == true)
            {
                find++;

                CONFIGRET cr =
                    CM_Get_Parent(&devinst_parent, devinfo_data.DevInst, 0);
                cr = CM_Get_Device_ID(devinst_parent, (PWSTR)parent_id,
                                      MAX_ID_LEN, 0);

                wchar_t tempwsn[MAX_ID_LEN] = {0};
                mbstowcs(tempwsn, sn, strlen(sn));
                // wprintf(L"%d %s\n", find, (wchar_t *)tempwsn);
                // wprintf(L"%02d %s %s  SN:%s\n", find,
                //     (wchar_t *)friendly_name, (wchar_t *)device_path,
                //     (wchar_t *)parent_id + 22);
                if (wcsncmp((wchar_t *)tempwsn, (wchar_t *)parent_id + 22,
                            wcslen(((wchar_t *)parent_id + 22))) == 0)
                {
                    // wprintf(L"Find: %d %s\n", find, (wchar_t *)parent_id +
                    // 22, wcslen(((wchar_t
                    // *)parent_id + 22)));
                    dev_index = find;
                    break;
                }
            }
        }
        if (dev_index == 0)
        {
            printf("Can't find udb dev\n");
            exit(-1);
        }
    }

    SetupDiDestroyDeviceInfoList(devinfo_set);

    return dev_index;
}

#if 0
static bool os_is_win10(void)
{
    static bool already_init = false;
    static bool is_win10     = false;

    if( already_init == true ) {
        return is_win10;
    }

    std::string vname;
    typedef void(__stdcall*NTPROC)(DWORD*, DWORD*, DWORD*);
    HINSTANCE hinst = LoadLibrary(L"ntdll.dll");
    DWORD dwMajor, dwMinor, dwBuildNumber;
    NTPROC proc = (NTPROC)GetProcAddress(hinst, "RtlGetNtVersionNumbers");
    proc(&dwMajor, &dwMinor, &dwBuildNumber);

    already_init = true;

    if (dwMajor == 6 && dwMinor == 3) {
        is_win10 = true;
    } else if (dwMajor == 10 && dwMinor == 0) {
        is_win10 = true;
    } else {
        is_win10 = false;
    }

    return is_win10;
}

void print_os_name(void)
{
    std::string vname;
    typedef void(__stdcall*NTPROC)(DWORD*, DWORD*, DWORD*);
    HINSTANCE hinst = LoadLibrary(L"ntdll.dll");
    DWORD dwMajor, dwMinor, dwBuildNumber;
    NTPROC proc = (NTPROC)GetProcAddress(hinst, "RtlGetNtVersionNumbers");
    proc(&dwMajor, &dwMinor, &dwBuildNumber);
    if (dwMajor == 6 && dwMinor == 3)    //win 8.1
    {
        vname = "Microsoft Windows 8.1";
        printf_s("%s\n", vname.c_str());
        return;
    }
    if (dwMajor == 10 && dwMinor == 0)    //win 10
    {
        vname = "Microsoft Windows 10";
        printf_s("%s\n", vname.c_str());
        return;
    }

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    OSVERSIONINFOEX os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
#pragma warning(disable : 4996)
    if (GetVersionEx((OSVERSIONINFO *)&os))
    {
        switch (os.dwMajorVersion)
        {
        case 4:
            switch (os.dwMinorVersion)
            {
            case 0:
                if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
                    vname ="Microsoft Windows NT 4.0";
                else if (os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
                    vname = "Microsoft Windows 95";
                break;
            case 10:
                vname ="Microsoft Windows 98";
                break;
            case 90:
                vname = "Microsoft Windows Me";
                break;
            }
            break;
        case 5:
            switch (os.dwMinorVersion)
            {
            case 0:
                vname = "Microsoft Windows 2000";
                break;
            case 1:
                vname = "Microsoft Windows XP";
                break;
            case 2:
                if (os.wProductType == VER_NT_WORKSTATION &&
                    info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
                    vname = "Microsoft Windows XP Professional x64 Edition";
                else if (GetSystemMetrics(SM_SERVERR2) == 0)
                    vname = "Microsoft Windows Server 2003";
                else if (GetSystemMetrics(SM_SERVERR2) != 0)
                    vname = "Microsoft Windows Server 2003 R2";
                break;
            }
            break;
        case 6:
            switch (os.dwMinorVersion)
            {
            case 0:
                if (os.wProductType == VER_NT_WORKSTATION)
                    vname = "Microsoft Windows Vista";
                else
                    vname = "Microsoft Windows Server 2008";
                break;
            case 1:
                if (os.wProductType == VER_NT_WORKSTATION)
                    vname = "Microsoft Windows 7";
                else
                    vname = "Microsoft Windows Server 2008 R2";
                break;
            case 2:
                if (os.wProductType == VER_NT_WORKSTATION)
                    vname = "Microsoft Windows 8";
                else
                    vname = "Microsoft Windows Server 2012";
                break;
            }
            break;
        default:
            vname = "unknown";
        }
        printf_s("%s\n", vname.c_str());
    }
    else
        printf_s("unknown os\n");
}

#endif

int usleep(unsigned int useconds)
{
    HANDLE        timer;
    LARGE_INTEGER due;

    due.QuadPart = -(10 * (__int64)useconds);

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &due, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
    return 0;
}