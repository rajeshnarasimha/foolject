#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "ReloadService.h"
#include "ServiceHelper.h"

CReloadService::CReloadService()
	: CNTService(TEXT("ReloadService"), TEXT("ReloadService"))
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


void CReloadService::Run(DWORD dwArgc, LPTSTR * ppszArgv) {
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
	
	int count = 0;
	HANDLE hSvr = ::CreateNamedPipe("\\\\.\\PIPE\\NEED_RELOAD\\",
		PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, 4096, 4096, 1, NULL);
	if (hSvr == INVALID_HANDLE_VALUE)
	{
		_tprintf(TEXT("CreateNamePipe Failed\n"));
	}

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
		if (ConnectNamedPipe(hSvr,NULL))
		{
			BYTE bRead;
			DWORD dwRead;
			CServiceHelper sh;
			DWORD service_status;
			ReadFile(hSvr,&bRead,1,&dwRead,NULL);
			if(bRead == 'Y' || bRead =='z')
			{
				count++;
				_tprintf(TEXT("ID:%03d  ...GOT\n"), count);
				Sleep(3000);
				sh.SetServiceName("ClientService");
				sh.QueryStatus(service_status);
				if (service_status == SERVICE_STOPPED)
				{	
					int ret = sh.Start();
					if (ret == false)
						_tprintf(TEXT("start Client service FAILED\n"));
					else
						_tprintf(TEXT("Client service RESTARTED\n"));
				}	
			}
		}
		else
		{
			_tprintf(TEXT("Error when waiting connected\n"));
		}		
		DisconnectNamedPipe(hSvr);
	}

Stop:
	if(hSvr)
		::CloseHandle(hSvr);
	if( m_hStop )
		::CloseHandle(m_hStop);
	if(m_hPause)
		::CloseHandle(m_hPause);
	if(m_hContinue)
		::CloseHandle(m_hContinue);
}


void CReloadService::Stop() {
	// report to the SCM that we're about to stop

	// TODO: Adjust the number of milliseconds you think
	//		 the stop-operation may take.
	ReportStatus(SERVICE_STOP_PENDING, 5000);

	if( m_hStop )
		::SetEvent(m_hStop);
}


void CReloadService::Pause() {
	ReportStatus(SERVICE_PAUSE_PENDING);

	if(m_hPause)
		::SetEvent(m_hPause);

	// TODO: add additional "pause" code here

	ReportStatus(SERVICE_PAUSED);
}


void CReloadService::Continue() {
	ReportStatus(SERVICE_CONTINUE_PENDING);

	// TODO: add additional "continue" code here

	if(m_hContinue)
		::SetEvent(m_hContinue);

	ReportStatus(SERVICE_RUNNING);
}


void CReloadService::Shutdown() {
	if(m_hStop)
		::SetEvent(m_hStop);

	// TODO: add code here that will be executed when the system shuts down
}
