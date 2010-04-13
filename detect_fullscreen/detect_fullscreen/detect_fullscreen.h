// detect_fullscreen.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Cdetect_fullscreenApp:
// See detect_fullscreen.cpp for the implementation of this class
//

class Cdetect_fullscreenApp : public CWinApp
{
public:
	Cdetect_fullscreenApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Cdetect_fullscreenApp theApp;