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

#pragma   comment(lib,"Strmiids.lib")

//#define DEBUG

#define UVCVID (0x0603)
#define UVCPID (0x8612)

GUID PROPSETID_VIDCAP_EXTENSION_UNIT = { 0xc987a729, 0x59d3, 0x4569, 0x84, 0x67, 0xff, 0x08, 0x49, 0xfc, 0x19, 0xe8 };

#define EU_MAX_PKG_SIZE  64

/****************************************/
enum udb_chan_id {
	UDB_CTRL           = 0x01,

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

void print_usage(const char *program)
{
    printf("usage: %s [command]\n", program);
}


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

    vid = UVCVID;
    pid = UVCPID;

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

    vid = UVCVID;
    pid = UVCPID;

	unsigned char loop = 0;

	set_buf[0] = 'u';
	set_buf[1] = 'd';
	set_buf[2] = 'b';	

	int offset = 4;
	int len = 0;
    for(int i = 0; i < argc; i ++) {
		len = strlen(argv[i]);
		memcpy(set_buf+offset, argv[i], len);
		set_buf[offset+len+1] = ' ';
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

    vid = UVCVID;
    pid = UVCPID;

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

    vid = UVCVID;
    pid = UVCPID;

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
	printf("File Size: %dKiB\n", file_size);

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

	printf("File Size: %d, 0x%x\n", file_size, file_size);

    bool set = false;
	bool get = true;
	unsigned char cp_size;

	USHORT vid, pid;
    ULONG ret;

    memset(set_buf, 0, EU_MAX_PKG_SIZE);
    memset(get_buf, 0, EU_MAX_PKG_SIZE + 1);

    vid = UVCVID;
    pid = UVCPID;

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
			printf("\t(%d/%d)", PUSH_MAX_PKG_SIZE*64*j, file_size);
			GetLocalTime( &sys );   
			printf( "\t%02d:%02d:%02d.%03d\n",sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds);  
		}


		left_size -= cp_size;

		if(left_size == 0) {
			printf("\t(%d/%d)", file_size, file_size);
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

	} else {
		print_usage();
	}

    return 0;
}