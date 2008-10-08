// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the TELECOMDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// TELECOMDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef TELECOMDLL_EXPORTS
#define TELECOMDLL_API extern "C" __declspec(dllexport)
#else
#define TELECOMDLL_API extern "C" __declspec(dllimport)
#endif

#include "callback.h"
#include "LineState.h"
#include "CallState.h"

// return number of lines, if error return value less than 0
TELECOMDLL_API int  __stdcall tel_Initialize(LOG_PROC logProc, EVENT_HANDLER eventHandler);
TELECOMDLL_API void __stdcall tel_Uninitialize();
TELECOMDLL_API unsigned __stdcall tel_TotalLinesInState(TLineState ls);

TELECOMDLL_API BOOL __stdcall tel_GetState(unsigned line, TLineState *ls, TCallState *cs);
TELECOMDLL_API BOOL __stdcall tel_MakeCall(unsigned line, const char *ANI, const char *DNIS);
TELECOMDLL_API BOOL __stdcall tel_AcceptCall(unsigned line);
TELECOMDLL_API BOOL __stdcall tel_AnswerCall(unsigned line);
TELECOMDLL_API BOOL __stdcall tel_HangupCall(unsigned line);

//if ANY dtmf can abort play, set DTMFabort = NULL
//if NO  dtmf can abort play, set DTMFabort = ""
//if * or #   can abort play, set DTMFabort = "*#"
TELECOMDLL_API BOOL __stdcall tel_PlayFile(unsigned line, const char *filename, const char *DTMFabort);
TELECOMDLL_API BOOL __stdcall tel_StopPlayFile(unsigned line);

