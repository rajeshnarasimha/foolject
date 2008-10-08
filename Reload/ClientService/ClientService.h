#if !defined(AFX_CLIENTSERVICE_H__95504C68_F50F_400A_9953_42BC85147560__INCLUDED_)
#define AFX_CLIENTSERVICE_H__95504C68_F50F_400A_9953_42BC85147560__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include "NTService.h"



class CClientService : public CNTService {
	public:	// construction
		CClientService();

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

#endif // !defined(AFX_CLIENTSERVICE_H__95504C68_F50F_400A_9953_42BC85147560__INCLUDED_)
