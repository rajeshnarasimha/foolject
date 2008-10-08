#if !defined(AFX_RELOADSERVICE_H__637CAED9_6620_4ADA_AB92_84C5CE64B923__INCLUDED_)
#define AFX_RELOADSERVICE_H__637CAED9_6620_4ADA_AB92_84C5CE64B923__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include "NTService.h"



class CReloadService : public CNTService {
	public:	// construction
		CReloadService();

	public:	// overridables
		virtual void	Run(DWORD, LPTSTR *);
		virtual void	Pause();
		virtual void	Continue();
		virtual void	Shutdown();
		virtual void	Stop();

	private:
		HANDLE	m_hStop;
		HANDLE	m_hPause;
		HANDLE	m_hContinue;
};

#endif // !defined(AFX_RELOADSERVICE_H__637CAED9_6620_4ADA_AB92_84C5CE64B923__INCLUDED_)
