// kill.cpp : Defines the entry point for the console application.
//

#include <string>
#include <list>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "NTProcessInfo.h"

#include <process.h>
#include <Tlhelp32.h>
#include <shellapi.h>

struct ProcessInfo
{
	unsigned Id;
	std::string Name;
	std::string CmdLine;
};

std::string GetProcessCmdLine(unsigned pid)
{
	smPROCESSINFO pi;
	if(!sm_GetNtProcessInfo(pid, &pi)) return "";
	return pi.szCmdLine;
}

bool CreateProcessSnapshot(std::list<ProcessInfo> &snapshot)
{
	snapshot.clear();

	ProcessInfo pi;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) 
	{
		printf("Error: CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS) failed!");
		return false;
	}
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hSnapshot, &pe))
	{
		do 
		{
			pi.Id = pe.th32ProcessID;
			pi.Name = pe.szExeFile;
			pi.CmdLine = GetProcessCmdLine(pi.Id);
			//printf("%s\t\t%s", pi.Name.c_str(), pi.CmdLine.c_str());
			snapshot.push_back(pi);
		} while (Process32Next(hSnapshot, &pe));
	}
	CloseHandle(hSnapshot);
	return snapshot.size() > 0;
}

bool KillProcessId(unsigned pid, std::string &reason)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL)
	{
		printf("OpenProcess ERROR: %d", GetLastError());
		reason = "OpenProcess error";
		return false;
	}
	if(!TerminateProcess(hProcess, 1))
	{
		printf("TerminateProcess ERROR: %d", GetLastError());
		reason = "TerminateProcess error";
		CloseHandle(hProcess);
		return false;
	}
	CloseHandle(hProcess);
	return true;
}

bool KillProcess(std::string keys[], int keycount, std::string &reason)
{
	std::list<ProcessInfo> snapshot;
	if (!CreateProcessSnapshot(snapshot))
	{
		reason = "snapshot error";
		return false;
	}

	bool found = true;
	std::list<ProcessInfo>::const_iterator it;
	for (it = snapshot.begin(); it != snapshot.end(); it++)
	{
		found = true;
		for (int i=0; i<keycount; i++)
		{
			if (std::string::npos == it->CmdLine.find(keys[i]))
			{
				found = false;
				break;
			}
		}
		if (found)
		{
			return KillProcessId(it->Id, reason);
		}
	}
	reason = "not found";
	return false;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if(!sm_EnableTokenPrivilege(SE_DEBUG_NAME))
	{
		printf("Failed to EnableTokenPrivilege");
		return false;
	}
	HMODULE hNtDll = sm_LoadNTDLLFunctions();
	if(hNtDll == NULL)
	{
		printf("Failed to load NT dll functions");
		return false;
	}

	std::string reason;
	std::string keys[100];

	if (argc>1)
	{
		for (int i=1; i<argc&&i<100; i++) 
		{
			keys[i-1] = argv[i];
		}
		KillProcess(keys, argc-1, reason);
		printf("Reason: %s.\n", reason.c_str());
	}

	if (hNtDll != NULL) sm_FreeNTDLLFunctions(hNtDll);
	return 0;
}

