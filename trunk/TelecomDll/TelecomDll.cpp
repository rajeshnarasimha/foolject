// TelecomDll.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "TelecomDll.h"
#include "Telecom.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


TELECOMDLL_API int  __stdcall tel_Initialize(LOG_PROC logProc, EVENT_HANDLER eventHandler)
{
    tel = new TTelecom;
    if(logProc == NULL || eventHandler == NULL) return -1;
    if(!tel->initialize(logProc, eventHandler)) return -2;
    return tel->NumOfLines;
}

TELECOMDLL_API void __stdcall tel_Uninitialize()
{
    if(tel != NULL)
    {
        tel->closeLines();
        tel->uninitialize();
        delete tel;
        tel = NULL;
    }
}

TELECOMDLL_API unsigned __stdcall tel_TotalLinesInState(TLineState ls)
{
    return tel->TotalLinesInState[ls];
}

TELECOMDLL_API BOOL __stdcall tel_GetState(unsigned line, TLineState *ls, TCallState *cs)
{
    if(line >= tel->NumOfLines) return FALSE;

    if(ls != NULL) *ls = tel->Lines[line].State;
    if(cs != NULL) *cs = tel->Lines[line].CallState;
    return TRUE;
}

TELECOMDLL_API BOOL __stdcall tel_MakeCall(unsigned line, const char *ANI, const char *DNIS)
{
    if(line >= tel->NumOfLines) return FALSE;
    return tel->Lines[line].makeCall(ANI, DNIS);
}

TELECOMDLL_API BOOL __stdcall tel_AcceptCall(unsigned line)
{
    if(line >= tel->NumOfLines) return FALSE;
    return tel->Lines[line].acceptCall();
}

TELECOMDLL_API BOOL __stdcall tel_AnswerCall(unsigned line)
{
    if(line >= tel->NumOfLines) return FALSE;
    return tel->Lines[line].answerCall();
}

TELECOMDLL_API BOOL __stdcall tel_HangupCall(unsigned line)
{
    if(line >= tel->NumOfLines) return FALSE;
    return tel->Lines[line].hangupCall();
}

TELECOMDLL_API BOOL __stdcall tel_PlayFile(unsigned line, const char *filename, const char *DTMFabort)
{
    if(line >= tel->NumOfLines) return FALSE;
    return tel->Lines[line].playFile(filename, DTMFabort);
}

TELECOMDLL_API BOOL __stdcall tel_StopPlayFile(unsigned line)
{
    if(line >= tel->NumOfLines) return FALSE;
    return tel->Lines[line].stopPlay();
}
