// DDEPrint.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DDEPrint.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;

#include "MFCDDE.h"

bool getRegisterKey(HKEY rootKey, char* keyname, char* keyvalue, int buffsize, char* entryname=NULL)
{
	HKEY hKey;
	long ret = ::RegOpenKeyEx(rootKey,keyname,0,KEY_READ,&hKey);
	if(ret != ERROR_SUCCESS)
		return false;
	
	memset(keyvalue,0,buffsize);
	DWORD type = REG_SZ;
	ret = ::RegQueryValueEx(hKey,entryname,NULL,&type,(unsigned char*)keyvalue,(unsigned long*)&buffsize);
	::RegCloseKey(hKey);
	if(ret == ERROR_SUCCESS)
		return true;
	else
		return false;
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.
		//CString strHello;
		//strHello.LoadString(IDS_HELLO);
		//cout << (LPCTSTR)strHello << endl;

		if (argc != 3) {
			cout << (LPCTSTR)"Command line as following: " << endl;
			cout << (LPCTSTR)"  DDEPrint aa.docx printer_name" << endl;
			cout << (LPCTSTR)"  DDEPrint aa.xlsx printer_name" << endl;
			return -1;
		}

		char reg_query[255] = "";
		char* reg_temp;
		char reg_suffix[255] = "";
		
		char dde_app[255] = "";
		char dde_topic[255] = "";
		char dde_cmd[255] = "";

		CString printer_name = (CString)argv[2];
		CString file_name = (CString)argv[1];
		int pos = file_name.ReverseFind('.');
		CString file_suffix = file_name.Mid(pos, file_name.GetLength());

		reg_temp = file_suffix.GetBuffer(255);
		sprintf(reg_query, "%s\\", reg_temp);
		getRegisterKey(HKEY_CLASSES_ROOT, reg_query, reg_suffix, sizeof(reg_suffix));
		cout << reg_query << ", " << reg_suffix << endl;

		sprintf(reg_query, "%s\\shell\\printto\\ddeexec\\", reg_suffix);
		getRegisterKey(HKEY_CLASSES_ROOT, reg_query, dde_cmd, sizeof(dde_cmd));
		cout << reg_query << ", " << dde_cmd << endl;

		sprintf(reg_query, "%s\\shell\\printto\\ddeexec\\Application\\", reg_suffix);
		getRegisterKey(HKEY_CLASSES_ROOT, reg_query, dde_app, sizeof(dde_app));
		cout << reg_query << ", " << dde_app << endl;

		sprintf(reg_query, "%s\\shell\\printto\\ddeexec\\Topic\\", reg_suffix);
		getRegisterKey(HKEY_CLASSES_ROOT, reg_query, dde_topic, sizeof(dde_topic));
		cout << reg_query << ", " << dde_topic << endl;

		CString full_cmd = dde_cmd;
		full_cmd.Replace("%1", file_name.GetBuffer(255));
		full_cmd.Replace("%2", printer_name.GetBuffer(255));

		CDDEClient client;
		CDDEConnection *connection;

		DDEInitialize();
		for (int i=0; i<10; i++) {
			connection = client.MakeConnection("", dde_app, dde_topic);
			if (connection) 
				break;
			else 
				Sleep(500);
		}
		if (connection) {
			connection->Execute(full_cmd);
			connection->Disconnect();
			delete connection;
		} else {
			cout << "MakeConnection failed!" << endl;
			return -2;
		}
		DDECleanUp();

	}

	return nRetCode;
}


