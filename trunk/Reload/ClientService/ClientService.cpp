#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <time.h>
#include "ClientService.h"


CClientService::CClientService()
	: CNTService(TEXT("ClientService"), TEXT("Client Service"))
	, m_hStop(0)
	, m_hPause(0)
	, m_hContinue(0)
{
	m_dwControlsAccepted = 0;
	m_dwControlsAccepted |= SERVICE_ACCEPT_STOP;
	m_dwControlsAccepted |= SERVICE_ACCEPT_PAUSE_CONTINUE;
	m_dwControlsAccepted |= SERVICE_ACCEPT_SHUTDOWN;
	m_dwServiceType |= SERVICE_INTERACTIVE_PROCESS;
}


void CClientService::Run(DWORD dwArgc, LPTSTR * ppszArgv) {
	// report to the SCM that we're about to start
	ReportStatus(SERVICE_START_PENDING);

	m_hStop = ::CreateEvent(0, TRUE, FALSE, 0);
	m_hPause = ::CreateEvent(0, TRUE, FALSE, 0);
	m_hContinue = ::CreateEvent(0, TRUE, FALSE, 0);

	// TODO: You might do some initialization here.
	//		 Parameter processing for instance ...
	//		 If this initialization takes a long time,
	//		 don't forget to call "ReportStatus()"
	//		 frequently or adjust the number of milliseconds
	//		 in the "ReportStatus()" above.
	
	int i=0;
	srand( (unsigned)time( NULL ) );
	
	// report SERVICE_RUNNING immediately before you enter the main-loop
	// DON'T FORGET THIS!
	ReportStatus(SERVICE_RUNNING);

	// main-loop
	// If the Stop() method sets the event, then we will break out of
	// this loop.
	while( ::WaitForSingleObject(m_hStop, 10) != WAIT_OBJECT_0 ) {
		// if the service got a "pause" request, wait until the user
		// wants to "continue".
		if(::WaitForSingleObject(m_hPause, 5) == WAIT_OBJECT_0) {
			while(::WaitForSingleObject(m_hContinue, 50) != WAIT_OBJECT_0)
				if(::WaitForSingleObject(m_hPause, 50) == WAIT_OBJECT_0)
					goto Stop;
			::ResetEvent(m_hPause);
			::ResetEvent(m_hContinue);
		}

		// TODO: Enter your service's real functionality here
		try
		{			
			i = rand();
			i = i % 800;
			_tprintf(TEXT("%d\n"), i);
			i = 800/i;
		}
		catch( ... )
		{
			HANDLE hClient = CreateFile("\\\\.\\PIPE\\NEED_RELOAD\\",
				GENERIC_WRITE |GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			if(hClient == INVALID_HANDLE_VALUE)
			{
				_tprintf(TEXT("Error open pipe\n"));
			}
			else
			{
				DWORD dwWritten;
				char szSend[10]="Y";
				WriteFile(hClient,szSend,1,&dwWritten,NULL);
				CloseHandle(hClient);
			}
			Stop();
		}
	}

Stop:

	if( m_hStop )
		::CloseHandle(m_hStop);
	if(m_hPause)
		::CloseHandle(m_hPause);
	if(m_hContinue)
		::CloseHandle(m_hContinue);
}


void CClientService::Stop() {
	// report to the SCM that we're about to stop

	// TODO: Adjust the number of milliseconds you think
	//		 the stop-operation may take.
	ReportStatus(SERVICE_STOP_PENDING, 5000);

	if( m_hStop )
		::SetEvent(m_hStop);
}


void CClientService::Pause() {
	ReportStatus(SERVICE_PAUSE_PENDING);

	if(m_hPause)
		::SetEvent(m_hPause);

	// TODO: add additional "pause" code here

	ReportStatus(SERVICE_PAUSED);
}


void CClientService::Continue() {
	ReportStatus(SERVICE_CONTINUE_PENDING);

	// TODO: add additional "continue" code here

	if(m_hContinue)
		::SetEvent(m_hContinue);

	ReportStatus(SERVICE_RUNNING);
}


void CClientService::Shutdown() {
	if(m_hStop)
		::SetEvent(m_hStop);

	// TODO: add code here that will be executed when the system shuts down
}
