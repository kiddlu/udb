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
	UDB_GET_RUNCMD_OUT = 0x03,
	
	/* interactive shell */
	UDB_INIT_SHELL     = 0x04,
	UDB_EXEC_SHELL     = 0x05,
	UDB_PULL_SHELL     = 0x06,
	UDB_CLOSE_SHELL    = 0x07,

	/* get file */

	/* put file */

	/* property */
	UDB_PROP_GET_CMD = 0xA0,

	/*RAW*/
	UDB_RAW        = 0xfe, //eu raw buffer 

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
#ifdef DEBUG
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
#endif
}


UCHAR set_buf[EU_MAX_PKG_SIZE];
UCHAR get_buf[EU_MAX_PKG_SIZE + 1];

void interactive_shell(void)
{
    bool set = false;
	bool get = true;

    USHORT vid, pid;
    ULONG ret;

    memset(set_buf, 0, EU_MAX_PKG_SIZE);
    memset(get_buf, 0, EU_MAX_PKG_SIZE + 1);

    vid = UVCVID;
    pid = UVCPID;

	while (printf("\n> ") && fgets((char *)set_buf + 4, EU_MAX_PKG_SIZE - 4, stdin)) 
	{
		if(strncmp((char *)set_buf + 4, "exit", 4) == 0){
			printf("exit\n");
			exit(0);
		}

		unsigned char loop = 0;

		set_buf[0] = 'u';
		set_buf[1] = 'd';
		set_buf[2] = 'b';

		set_buf[3] = UDB_INIT_SHELL;
		UvcXuCommand(vid, pid, set_buf, &ret, set);			
		
		set_buf[3] = UDB_RUN_CMD;
		dumphex(set_buf, EU_MAX_PKG_SIZE);
		UvcXuCommand(vid, pid, set_buf, &ret, set);
		UvcXuCommand(vid, pid, get_buf, &ret, get);
		dumphex(get_buf, ret);
		
		loop = get_buf[0];
		//printf("\n");

		for(unsigned char i=0;i<loop;i++)
		{
			set_buf[3] = UDB_GET_RUNCMD_OUT;
			set_buf[4] = i;
			
			dumphex(set_buf, EU_MAX_PKG_SIZE);
			UvcXuCommand(vid, pid, set_buf, &ret, set);
			UvcXuCommand(vid, pid, get_buf, &ret, get);
			dumphex(get_buf, ret);
			printf("%s", get_buf);
		}

		//printf("ret %d\n", ret);
		memset(set_buf, 0, EU_MAX_PKG_SIZE);
		memset(get_buf, 0, EU_MAX_PKG_SIZE);

		set_buf[0] = 'u';
		set_buf[1] = 'd';
		set_buf[2] = 'b';
		set_buf[3] = UDB_CLOSE_SHELL;
		UvcXuCommand(vid, pid, set_buf, &ret, set);				
	}
}
void exec_cmd(int argc, char**argv)
{
    bool set = false;
	bool get = true;

    USHORT vid, pid;
    ULONG ret;

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
    for(int i = 1; i < argc; i ++) {
		len = strlen(argv[i]);
		memcpy(set_buf+offset, argv[i], len);
		set_buf[offset+len+1] = ' ';
		offset += (len+1);
		
		if(offset >= 60)
			break;
	}

	set_buf[3] = UDB_RUN_CMD;
	dumphex(set_buf, EU_MAX_PKG_SIZE);
	UvcXuCommand(vid, pid, set_buf, &ret, set);
	UvcXuCommand(vid, pid, get_buf, &ret, get);
	dumphex(get_buf, ret);
	
	loop = get_buf[0];
	//printf("\n");

	for(unsigned char i=0;i<loop;i++)
	{
		set_buf[3] = UDB_GET_RUNCMD_OUT;
		set_buf[4] = i;
		
		dumphex(set_buf, EU_MAX_PKG_SIZE);
		UvcXuCommand(vid, pid, set_buf, &ret, set);
		UvcXuCommand(vid, pid, get_buf, &ret, get);
		dumphex(get_buf, ret);
		printf("%s", get_buf);
	}

	//printf("ret %d\n", ret);
	memset(set_buf, 0, EU_MAX_PKG_SIZE);
	memset(get_buf, 0, EU_MAX_PKG_SIZE);    
}
void print_usage(void)
{
    printf("usage:\n");
    printf("  command: \n");
    printf("    shell            : enter to interactive mode\n");
    printf("    shell [cmd]      : run shell cmd\n");
    printf("    put [local] [dev]: put file to uvc device\n");
    printf("    get [local] [dev]: get file from uvc device:\n");

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
			argc--;
			argv++;
			exec_cmd(argc,argv);
		}
	} else if (!strcmp(argv[1],"put"))  {

	} else if (!strcmp(argv[1],"get"))  {

	} else {
		print_usage();
	}

    return 0;
}