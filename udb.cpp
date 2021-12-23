#define _CRT_SECURE_NO_WARNINGS

#include <comsvcs.h>
#include <atlstr.h>
#include <vidcap.h>
#include <windows.h>
#include <ksproxy.h>
#include <strmif.h>
#include <dshow.h>
#include <ks.h>
#include <ksmedia.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <setupapi.h>

#pragma   comment(lib,"Strmiids.lib")
#pragma   comment(lib,"setupapi.lib")

//#define DEBUG

GUID PROPSETID_VIDCAP_EXTENSION_UNIT = { 0xc987a729, 0x59d3, 0x4569, 0x84, 0x67, 0xff, 0x08, 0x49, 0xfc, 0x19, 0xe8 };

#define EU_MAX_PKG_SIZE  64

/****************************************/
enum udb_chan_id {
	UDB_AUTH           = 0x01,

	/* run cmd */
	UDB_RUN_CMD        = 0x02,
	UDB_RUN_CMD_PULLOUT = 0x03,

	/* pull file */
	UDB_PULL_FILE_INIT        = 0x11,
	UDB_PULL_FILE_PULLOUT     = 0x12,

	/* push file */
	UDB_PUSH_FILE_INIT      = 0x21,
	UDB_PUSH_FILE_PROC      = 0x22,
	UDB_PUSH_FILE_FINI		= 0x23,

	/* property */

	/*RAW*/
	UDB_RAWSET        = 0xfd, //eu raw buffer 
	UDB_RAWGET        = 0xfe, //eu raw buffer 

	NONE_CHAN      = 0xff
};


/*=============================================================================*/

void dumphex(void *data, uint32_t size)
{
	char ascii[17];
	unsigned int i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

static void ListupUVCDev(USHORT *vid, USHORT *pid)
{
	GUID class_guid[1];
	unsigned long required_size;
	bool ret;
	HDEVINFO devinfo_set = NULL;
	SP_DEVINFO_DATA devinfo_data;
	unsigned int member_index = 0;
	int find = 0;

	devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);

	// Get ClassGuid from ClassName for PORTS class
	ret = SetupDiClassGuidsFromName(L"Camera", (LPGUID)&class_guid, 1,
		                          &required_size);
	if (ret != true) {
		printf("SetupDiClassGuidsFromName Error\n");
		goto cleanup;
	}

	// Get class devices
	devinfo_set = SetupDiGetClassDevs(&class_guid[0], NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);

	if (devinfo_set) {
		// Enumerate devices
		member_index = 0;
		while (SetupDiEnumDeviceInfo(devinfo_set, member_index++, &devinfo_data)) {
			char friendly_name[MAX_PATH];
			char hardware_id[MAX_PATH];
			char message[MAX_PATH];
			unsigned long req_size = 0;
			unsigned long prop_type;
			unsigned long type = REG_SZ;
			int port_nb;
			HKEY hKey = NULL;

			// Get friendly_name
			ret = SetupDiGetDeviceRegistryProperty(devinfo_set,
			                                        &devinfo_data,
			                                        SPDRP_FRIENDLYNAME,
			                                        &prop_type,
			                                        (LPBYTE)friendly_name,
			                                        sizeof(friendly_name),
			                                        &req_size);

            ret = SetupDiGetDeviceRegistryProperty(devinfo_set,
													&devinfo_data,
													SPDRP_HARDWAREID,
													&prop_type,
													(LPBYTE)hardware_id,
													sizeof(hardware_id),
													&req_size);

			wprintf(L"%s \t\t %s\n", (wchar_t *)friendly_name, (wchar_t *)hardware_id);
			//dumphex(hardware_id, MAX_PATH);
			*vid = (hardware_id[16] - '0') * 4096 + (hardware_id[18] - '0') * 256 + (hardware_id[20] - '0') * 16 + (hardware_id[22] - '0');
			*pid = (hardware_id[34] - '0') * 4096 + (hardware_id[36] - '0') * 256 + (hardware_id[38] - '0') * 16 + (hardware_id[40] - '0');
			//printf("vid 0x%04x pid 0x%04x\n", *vid, *pid);
			if(*vid == 0x0603 && *pid == 0x8612) {
				find = 1;
				break;
			}
		}
		if(find == 0) {
			printf("Can't find UVC Dev\n");
			exit(-1);
		}
	}
cleanup:
// Destroy device info list
	SetupDiDestroyDeviceInfoList(devinfo_set);
}



HRESULT FindExtensionNode(IKsTopologyInfo* pIksTopologyInfo, GUID extensionGuid, DWORD* pNodeId)
{
	DWORD numberOfNodes;
	HRESULT hResult = S_FALSE;

	hResult = pIksTopologyInfo->get_NumNodes(&numberOfNodes);
	if (SUCCEEDED(hResult))
	{
		DWORD i;
		GUID nodeGuid;
		for (i = 0; i < numberOfNodes; i++)
		{
			if (SUCCEEDED(pIksTopologyInfo->get_NodeType(i, &nodeGuid)))
			{
				if (IsEqualGUID(extensionGuid, nodeGuid))
				{  // Found the extension node 
					*pNodeId = i;
					hResult = S_OK;
					break;
				}
			}
		}

		if (i == numberOfNodes)
		{ // Did not find the node 
			hResult = S_FALSE;
		}
	}
	return hResult;
}

BOOL GetNodeId(IBaseFilter* pCaptureFilter, int* pNodeId)
{

	IKsTopologyInfo* pKsToplogyInfo;
	HRESULT hResult;
	DWORD dwNode;

	if (!pCaptureFilter)
		return FALSE;

	hResult = pCaptureFilter->QueryInterface(
		__uuidof(IKsTopologyInfo),
		(void**)(&pKsToplogyInfo));
	if (S_OK == hResult)
	{
		hResult = FindExtensionNode(pKsToplogyInfo, KSNODETYPE_DEV_SPECIFIC, &dwNode); //KSNODETYPE_DEV_SPECIFIC PROPSETID_VIDCAP_EXTENSION_UNIT
		pKsToplogyInfo->Release();

		if (S_OK == hResult)
		{
			*pNodeId = dwNode;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL GetVideoCaptureFilter(IBaseFilter** piFilter, USHORT usVid, USHORT usPid)
{
	HRESULT hr;
	ICreateDevEnum* piCreateDevEnum;
	CString s;
	BOOL bSuccess = FALSE;

	CoInitialize(NULL);

	HRESULT hResult = CoCreateInstance(
		CLSID_SystemDeviceEnum,
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(piCreateDevEnum),
		reinterpret_cast<void**>(&piCreateDevEnum));
	if (SUCCEEDED(hResult))
	{
		IEnumMoniker* piEnumMoniker;
		hResult = piCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &piEnumMoniker, 0);
		piCreateDevEnum->Release();

		if (S_OK == hResult)
			hResult = piEnumMoniker->Reset();

		if (S_OK == hResult)
		{
			// Enumerate KS devices
			ULONG cFetched;
			IMoniker* piMoniker;
			CString strDevicePath;
			CString strMatch;

			while ((hResult = piEnumMoniker->Next(1, &piMoniker, &cFetched)) == S_OK)
			{

				IPropertyBag* pBag = 0;
				hr = piMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
				if (SUCCEEDED(hr))
				{
					VARIANT var;
					var.vt = VT_BSTR;
					hr = pBag->Read(_T("DevicePath"), &var, NULL); //FriendlyName
					if (hr == NOERROR)
					{
						strDevicePath = var.bstrVal;
						SysFreeString(var.bstrVal);
						strDevicePath.MakeUpper();

						strMatch.Format(_T("VID_%04X&PID_%04X"), usVid, usPid);
						if (strDevicePath.Find(strMatch) != -1)
						{
							hResult = piMoniker->BindToObject(
								NULL,
								NULL,
								IID_IBaseFilter,
								(void**)(piFilter));
							pBag->Release();
							piMoniker->Release();

							if (SUCCEEDED(hResult))
							{
								bSuccess = TRUE;
							}
							break;
						}
					}
					pBag->Release();
				}
				piMoniker->Release();
			}

			piEnumMoniker->Release();
		}
	}

	return bSuccess;
}

BOOL GetIKsControl(IBaseFilter* pCaptureFilter, IKsControl** ppKsControl)
{
	if (!pCaptureFilter)
		return FALSE;

	HRESULT	hr;
	hr = pCaptureFilter->QueryInterface(__uuidof(IKsControl),
		(void**)ppKsControl);

	if (hr != S_OK)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL UvcXuCommand(USHORT usVid, USHORT usPid, UCHAR *buffer, ULONG *BytesReturned, bool get)
{
	BOOL bSuccess = FALSE;
	IKsControl* pKsControl = NULL;
	IBaseFilter* pCaptureFilter = NULL;
	int nNodeId;
	KSP_NODE ExtensionProp;
	HRESULT hr;

	if (!GetVideoCaptureFilter(&pCaptureFilter, usVid, usPid))
	{
		printf("GetVideoCaptureFilter failed\n");
		goto Exit_UvcSwitch;
	}

	if (!GetNodeId(pCaptureFilter, &nNodeId))
	{
		printf("GetNodeId failed\n");
		goto Exit_UvcSwitch;
	}

	if (!GetIKsControl(pCaptureFilter, &pKsControl))
	{
		printf("GetIKsControl failed\n");
		goto Exit_UvcSwitch;
	}

	ExtensionProp.Property.Set = PROPSETID_VIDCAP_EXTENSION_UNIT;
	ExtensionProp.Property.Id = KSPROPERTY_EXTENSION_UNIT_CONTROL;
	ExtensionProp.NodeId = nNodeId;
	//printf("nNodeId is %d\n", nNodeId);

    if (get)
        ExtensionProp.Property.Flags = KSPROPERTY_TYPE_GET |
            KSPROPERTY_TYPE_TOPOLOGY;
    else
        ExtensionProp.Property.Flags = KSPROPERTY_TYPE_SET |
            KSPROPERTY_TYPE_TOPOLOGY;

	hr = pKsControl->KsProperty((PKSPROPERTY)&ExtensionProp, sizeof(ExtensionProp), (PVOID)buffer, EU_MAX_PKG_SIZE, BytesReturned);

	if (hr != S_OK)
	{
		printf("KsProperty failed\n");
		goto Exit_UvcSwitch;
	}

	bSuccess = TRUE;

Exit_UvcSwitch:
	if (pKsControl)
	{
		pKsControl->Release();
	}
	if (pCaptureFilter)
	{
		pCaptureFilter->Release();
	}

	return bSuccess;
}


unsigned char set_buf[EU_MAX_PKG_SIZE];
unsigned char get_buf[EU_MAX_PKG_SIZE + 1];

void interactive_shell(void)
{
    bool set = false;
	bool get = true;

    USHORT vid, pid;
    ULONG ret;

	unsigned int get_size = 0;
	unsigned int cp_size;	

    memset(set_buf, 0, EU_MAX_PKG_SIZE);
    memset(get_buf, 0, EU_MAX_PKG_SIZE + 1);

	ListupUVCDev(&vid, &pid);

	set_buf[0] = 'u';
	set_buf[1] = 'd';
	set_buf[2] = 'b';
	set_buf[3] = UDB_RUN_CMD;

	memcpy(set_buf + 4, "pwd\n", 4);
	UvcXuCommand(vid, pid, set_buf, &ret, set);
	UvcXuCommand(vid, pid, get_buf, &ret, get);

#ifdef DEBUG
			dumphex(set_buf, EU_MAX_PKG_SIZE);
			dumphex(get_buf, ret);
#endif

	memcpy(&get_size, get_buf, sizeof(get_size));

	//printf("\n");

	for( ; ; )
	{
		if(get_size < EU_MAX_PKG_SIZE) {
			cp_size = get_size;
		} else {
			cp_size = EU_MAX_PKG_SIZE;
		}
		memset(set_buf, 0, EU_MAX_PKG_SIZE);
		set_buf[0] = 'u';
		set_buf[1] = 'd';
		set_buf[2] = 'b';
		set_buf[3] = UDB_RUN_CMD_PULLOUT;
		memcpy(set_buf + 4, &cp_size,sizeof(cp_size));
		UvcXuCommand(vid, pid, set_buf, &ret, set);
		UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
			dumphex(set_buf, EU_MAX_PKG_SIZE);
			dumphex(get_buf, ret);
#endif
		int idx = strlen((const char *)get_buf) - 1;
		
		if(get_buf[idx] == '\n') {
			get_buf[idx] = 0;
		}
		printf("%s", get_buf);
		
		get_size -= cp_size;
		if(get_size == 0)
			break;
	}
	printf(">");
	memset(set_buf, 0, EU_MAX_PKG_SIZE);
	memset(get_buf, 0, EU_MAX_PKG_SIZE);

	while (fgets((char *)set_buf + 4, EU_MAX_PKG_SIZE - 4, stdin)) 
	{
		if(strncmp((char *)set_buf + 4, "exit", 4) == 0){
			printf("exit\n");
			exit(0);
		}
		set_buf[0] = 'u';
		set_buf[1] = 'd';
		set_buf[2] = 'b';	
		set_buf[3] = UDB_RUN_CMD;
		UvcXuCommand(vid, pid, set_buf, &ret, set);
		UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
			dumphex(set_buf, EU_MAX_PKG_SIZE);
			dumphex(get_buf, ret);
#endif
		memcpy(&get_size, get_buf, sizeof(get_size));
		//printf("\n");

		for( ; ; )
		{
			if(get_size < EU_MAX_PKG_SIZE) {
				cp_size = get_size;
			} else {
				cp_size = EU_MAX_PKG_SIZE;
			}
			memset(set_buf, 0, EU_MAX_PKG_SIZE);
			set_buf[0] = 'u';
			set_buf[1] = 'd';
			set_buf[2] = 'b';
			set_buf[3] = UDB_RUN_CMD_PULLOUT;
			memcpy(set_buf + 4, &cp_size,sizeof(cp_size));
			UvcXuCommand(vid, pid, set_buf, &ret, set);
			UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
			dumphex(set_buf, EU_MAX_PKG_SIZE);
			dumphex(get_buf, ret);
#endif
			printf("%s", get_buf);

			get_size -= cp_size;
			if(get_size == 0)
				break;
		}


		//printf("ret %d\n", ret);
		memset(set_buf, 0, EU_MAX_PKG_SIZE);
		memset(get_buf, 0, EU_MAX_PKG_SIZE);

		set_buf[0] = 'u';
		set_buf[1] = 'd';
		set_buf[2] = 'b';
		set_buf[3] = UDB_RUN_CMD;
	
		memcpy(set_buf + 4, "pwd\n", 4);
		UvcXuCommand(vid, pid, set_buf, &ret, set);
		UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
			dumphex(set_buf, EU_MAX_PKG_SIZE);
			dumphex(get_buf, ret);
#endif	
		memcpy(&get_size, get_buf, sizeof(get_size));
		//printf("\n");
	
		for( ; ; )
		{
			if(get_size < EU_MAX_PKG_SIZE) {
				cp_size = get_size;
			} else {
				cp_size = EU_MAX_PKG_SIZE;
			}
			memset(set_buf, 0, EU_MAX_PKG_SIZE);
			set_buf[0] = 'u';
			set_buf[1] = 'd';
			set_buf[2] = 'b';	
			set_buf[3] = UDB_RUN_CMD_PULLOUT;
			memcpy(set_buf + 4, &cp_size,sizeof(cp_size));
			UvcXuCommand(vid, pid, set_buf, &ret, set);
			UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
			dumphex(set_buf, EU_MAX_PKG_SIZE);
			dumphex(get_buf, ret);
#endif
			int idx = strlen((const char *)get_buf) - 1;
			
			if(get_buf[idx] == '\n') {
				get_buf[idx] = 0;
			}

			printf("%s", get_buf);
			
			get_size -= cp_size;
			if(get_size == 0)
				break;
		}

		printf(">");
		memset(set_buf, 0, EU_MAX_PKG_SIZE);
		memset(get_buf, 0, EU_MAX_PKG_SIZE);
	}
}
void exec_cmd(int argc, char**argv)
{
    bool set = false;
	bool get = true;

    USHORT vid, pid;
    ULONG ret;

	unsigned int get_size = 0;
	unsigned int cp_size;

    memset(set_buf, 0, EU_MAX_PKG_SIZE);
    memset(get_buf, 0, EU_MAX_PKG_SIZE + 1);

	ListupUVCDev(&vid, &pid);

	unsigned char loop = 0;

	set_buf[0] = 'u';
	set_buf[1] = 'd';
	set_buf[2] = 'b';	

	int offset = 4;
	int len = 0;
    for(int i = 0; i < argc; i ++) {
		len = strlen(argv[i]);
		memcpy(set_buf+offset, argv[i], len);
		set_buf[offset+len] = ' ';
		offset += (len+1);
		
		if(offset >= 60)
			break;
	}

	set_buf[3] = UDB_RUN_CMD;

	UvcXuCommand(vid, pid, set_buf, &ret, set);
	UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
	dumphex(set_buf, EU_MAX_PKG_SIZE);
	dumphex(get_buf, ret);
#endif	
	memcpy(&get_size, get_buf, sizeof(get_size));
	//printf("\n");

	for( ; ; )
	{
		if(get_size < EU_MAX_PKG_SIZE) {
			cp_size = get_size;
		} else {
			cp_size = EU_MAX_PKG_SIZE;
		}
		memset(set_buf, 0, EU_MAX_PKG_SIZE);
		set_buf[0] = 'u';
		set_buf[1] = 'd';
		set_buf[2] = 'b';	
		set_buf[3] = UDB_RUN_CMD_PULLOUT;
		memcpy(set_buf + 4, &cp_size,sizeof(cp_size));
		UvcXuCommand(vid, pid, set_buf, &ret, set);
		UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
		dumphex(set_buf, EU_MAX_PKG_SIZE);
		dumphex(get_buf, ret);
#endif
		int idx = strlen((const char *)get_buf) - 1;
		
		if(get_buf[idx] == '\n') {
			get_buf[idx] = 0;
		}

		printf("%s", get_buf);
		
		get_size -= cp_size;
		if(get_size == 0)
			break;
	}
	//printf("ret %d\n", ret);
	memset(set_buf, 0, EU_MAX_PKG_SIZE);
	memset(get_buf, 0, EU_MAX_PKG_SIZE);    
}

void raw_set(int argc, char**argv)
{
    bool set = false;
	bool get = true;

    USHORT vid, pid;
    ULONG ret;

    memset(set_buf, 0, EU_MAX_PKG_SIZE);
    memset(get_buf, 0, EU_MAX_PKG_SIZE + 1);

	ListupUVCDev(&vid, &pid);

    for(int i = 0; i < argc; i ++) {
		//printf("argc:%d\t %s,%d\n", i,argv[i], strlen(argv[i]));
		set_buf[i] = (unsigned char)strtoul(argv[i], NULL, 16);
	}

	UvcXuCommand(vid, pid, set_buf, &ret, set);
	UvcXuCommand(vid, pid, get_buf, &ret, get);
	dumphex(set_buf, EU_MAX_PKG_SIZE);
	dumphex(get_buf, ret);
}

void print_usage(void)
{
    printf("usage:\n");
    printf("  command: \n");
	printf("    auth             : auth to ctrl dev\n");

    printf("    shell            : enter to interactive mode\n");
    printf("    shell [cmd]      : run shell cmd\n");

    printf("    push [local] [dev]: push file to uvc device\n");
    printf("    pull [local] [dev]: pull file from uvc device\n");

    printf("    rawset [0x11] [0x22] ...: set raw eu\n");
    printf("    rawget                  : get raw eu\n");

}

void pull_file(int argc, char**argv)
{
	char *local_path = argv[0];
	char *dev_path = argv[1];
	
	if(dev_path == NULL || local_path == NULL)
	{
		print_usage();
		return;
	}
	
	printf("from dev: %s\nto local: %s\n", dev_path, local_path);


	FILE *fp = fopen(local_path, "wb");
	if(fp == NULL)
	{
		printf("open %s error\n", local_path);
		return;
	}

    bool set = false;
	bool get = true;
	
	unsigned int get_size = 0;
	unsigned int cp_size;
	unsigned int file_size = 0;
	USHORT vid, pid;
    ULONG ret;

    memset(set_buf, 0, EU_MAX_PKG_SIZE);
    memset(get_buf, 0, EU_MAX_PKG_SIZE + 1);

	ListupUVCDev(&vid, &pid);

	set_buf[0] = 'u';
	set_buf[1] = 'd';
	set_buf[2] = 'b';	
	set_buf[3] = UDB_PULL_FILE_INIT;
	memcpy(set_buf + 4, dev_path, strlen(dev_path));
	UvcXuCommand(vid, pid, set_buf, &ret, set);
	UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
	dumphex(set_buf, EU_MAX_PKG_SIZE);
	dumphex(get_buf, ret);
#endif
	memcpy(&get_size, get_buf, sizeof(get_size));
	file_size = get_size/1024;
	printf("File Size: %dKiB, 0x%x\n", get_size/1024, get_size);

	int i = 0,j = 0;
	SYSTEMTIME sys;
	for( ; ; )
	{
		if(get_size < EU_MAX_PKG_SIZE) {
			cp_size = get_size;
		} else {
			cp_size = EU_MAX_PKG_SIZE;
		}
		memset(set_buf, 0, EU_MAX_PKG_SIZE);
		set_buf[0] = 'u';
		set_buf[1] = 'd';
		set_buf[2] = 'b';	
		set_buf[3] = UDB_PULL_FILE_PULLOUT;
		memcpy(set_buf + 4, &cp_size, sizeof(cp_size));
		UvcXuCommand(vid, pid, set_buf, &ret, set);
		UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
		dumphex(set_buf, EU_MAX_PKG_SIZE);
		dumphex(get_buf, ret);
#endif

		printf("#");
		i++;
		if(i == 64)
		{
			i = 0;
			j++;
			printf("\t(%d/%d)", 4*j, file_size);
			GetLocalTime( &sys );   
			printf( "\t%02d:%02d:%02d.%03d\n",sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds);  
		}

		fwrite(get_buf, 1, cp_size, fp);

		get_size -= cp_size;

		if(get_size == 0) {
			printf("\t(%d/%d)", file_size, file_size);
			GetLocalTime( &sys );   
			printf( "\t%02d:%02d:%02d.%03d\n",sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds);  
			break;
		}
	}

	memset(set_buf, 0, EU_MAX_PKG_SIZE);
	memset(get_buf, 0, EU_MAX_PKG_SIZE);    
}



void push_file(int argc, char**argv)
{
	char *local_path = argv[0];
	char *dev_path = argv[1];

	if(dev_path == NULL || local_path == NULL)
	{
		print_usage();
		return;
	}

	printf("from local: %s\nto dev: %s\n", local_path, dev_path);


	FILE *fp = fopen(local_path, "rb");
	if(fp == NULL)
	{
		printf("open %s error\n", local_path);
		return;
	}

    fseek(fp, 0L, SEEK_END);
    unsigned int file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

	printf("File Size: %dKiB, 0x%x\n", file_size/1024, file_size);

    bool set = false;
	bool get = true;
	unsigned char cp_size;

	USHORT vid, pid;
    ULONG ret;

    memset(set_buf, 0, EU_MAX_PKG_SIZE);
    memset(get_buf, 0, EU_MAX_PKG_SIZE + 1);

	ListupUVCDev(&vid, &pid);

	set_buf[0] = 'u';
	set_buf[1] = 'd';
	set_buf[2] = 'b';	
	set_buf[3] = UDB_PUSH_FILE_INIT;
	memcpy(set_buf + 4, &file_size, sizeof(file_size));
	memcpy(set_buf + 8, dev_path, strlen(dev_path));
	UvcXuCommand(vid, pid, set_buf, &ret, set);
	UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
	dumphex(set_buf, EU_MAX_PKG_SIZE);
	dumphex(get_buf, ret);
#endif

	unsigned int left_size = file_size;
	unsigned char PUSH_MAX_PKG_SIZE = EU_MAX_PKG_SIZE  - 4 - 1;
	int i = 0,j = 0;
	SYSTEMTIME sys;

	for( ; ; )
	{
		if(left_size < PUSH_MAX_PKG_SIZE) {
			cp_size = left_size;
		} else {
			cp_size = PUSH_MAX_PKG_SIZE;
		}
		memset(set_buf, 0, EU_MAX_PKG_SIZE);
		set_buf[0] = 'u';
		set_buf[1] = 'd';
		set_buf[2] = 'b';	
		set_buf[3] = UDB_PUSH_FILE_PROC;
		memcpy(set_buf + 4, &cp_size, sizeof(cp_size));
		fread(set_buf + 5, cp_size, 1, fp);
		UvcXuCommand(vid, pid, set_buf, &ret, set);
		UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
		dumphex(set_buf, EU_MAX_PKG_SIZE);
		dumphex(get_buf, ret);
#endif

		printf("#");
		i++;
		if(i == 64)
		{
			i = 0;
			j++;
			printf("\t(%d/%d)", PUSH_MAX_PKG_SIZE*64*j /1024, file_size/1024);
			GetLocalTime( &sys );   
			printf( "\t%02d:%02d:%02d.%03d\n",sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds);  
		}


		left_size -= cp_size;

		if(left_size == 0) {
			printf("\t(%d/%d)", file_size/1024, file_size/1024);
			GetLocalTime( &sys );   
			printf( "\t%02d:%02d:%02d.%03d\n",sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds);  
			break;
		}
	}
    memset(set_buf, 0, EU_MAX_PKG_SIZE);
    memset(get_buf, 0, EU_MAX_PKG_SIZE + 1);

	set_buf[0] = 'u';
	set_buf[1] = 'd';
	set_buf[2] = 'b';	
	set_buf[3] = UDB_PUSH_FILE_FINI;
	UvcXuCommand(vid, pid, set_buf, &ret, set);
	UvcXuCommand(vid, pid, get_buf, &ret, get);
#ifdef DEBUG
		dumphex(set_buf, EU_MAX_PKG_SIZE);
		dumphex(get_buf, ret);
#endif

	fclose(fp);

}

void auth(int argc, char**argv)
{
    bool set = false;
	bool get = true;

    USHORT vid, pid;
    ULONG ret;

    memset(set_buf, 0, EU_MAX_PKG_SIZE);
    memset(get_buf, 0, EU_MAX_PKG_SIZE + 1);

	ListupUVCDev(&vid, &pid);

	set_buf[0] = 'u';
	set_buf[1] = 'd';
	set_buf[2] = 'b';	
	set_buf[3] = UDB_AUTH;

	set_buf[4] = '0';
	set_buf[5] = '3';
	set_buf[6] = '0';
	set_buf[7] = '3';
	set_buf[8] = '0';
	set_buf[9] = '6';
	set_buf[10] = '0';
	set_buf[11] = '3';
	set_buf[12] = '0';
	set_buf[13] = '3';
	set_buf[14] = '0';
	set_buf[15] = '7';
	set_buf[16] = '0';
	set_buf[17] = '7';
	set_buf[18] = '0';
	set_buf[19] = '1';

	UvcXuCommand(vid, pid, set_buf, &ret, set);
	UvcXuCommand(vid, pid, get_buf, &ret, get);
	dumphex(set_buf, EU_MAX_PKG_SIZE);
	dumphex(get_buf, ret);
}

int main(int argc, char**argv)
{
	if(argc<=1) {
		print_usage();
		return 0;
	}
	if(!strcmp(argv[1],"shell")) {
		if(argc == 2) {
			interactive_shell();
		} else {
			argc -= 2;
			argv += 2;
			exec_cmd(argc,argv);
		}
	} else if (!strcmp(argv[1],"push"))  {
		argc -= 2;
		argv += 2;
		push_file(argc,argv);
	} else if (!strcmp(argv[1],"pull"))  {
		argc -= 2;
		argv += 2;
		pull_file(argc,argv);
	} else if (!strcmp(argv[1],"rawset"))  {
		argc -= 2;
		argv += 2;
		raw_set(argc,argv);
	} else if (!strcmp(argv[1],"rawget"))  {
	} else if (!strcmp(argv[1],"auth"))  {
		argc -= 2;
		argv += 2;
		auth(argc,argv);
	} else {
		print_usage();
	}

    return 0;
}