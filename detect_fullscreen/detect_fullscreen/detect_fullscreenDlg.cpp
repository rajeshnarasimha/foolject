// detect_fullscreenDlg.cpp : implementation file
//

#include "stdafx.h"
#include "detect_fullscreen.h"
#include "detect_fullscreenDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// Cdetect_fullscreenDlg dialog




Cdetect_fullscreenDlg::Cdetect_fullscreenDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cdetect_fullscreenDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cdetect_fullscreenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cdetect_fullscreenDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// Cdetect_fullscreenDlg message handlers

BOOL Cdetect_fullscreenDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	RegisterAccessBar(m_hWnd, TRUE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Cdetect_fullscreenDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void Cdetect_fullscreenDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR Cdetect_fullscreenDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT Cdetect_fullscreenDlg::WindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
	if (MSG_APPBAR_MSGID == msg)
	{
		switch((UINT)wp)
		{
		case ABN_FULLSCREENAPP:
			{
				if (TRUE == (BOOL)lp)
				{
					TRACE(TEXT("full\n"));
					//KAppBarMsg::m_bFullScreen = TRUE;
				}
				else
				{
					TRACE(TEXT("not full\n"));
					//KAppBarMsg::m_bFullScreen = FALSE;
				}
			}
			break;
		default:
			break;
		}
	}
	return CDialog::WindowProc(msg, wp, lp);
}

// RegisterAccessBar - registers or unregisters an appbar. 
// Returns TRUE if successful, or FALSE otherwise. 
// hwndAccessBar - handle to the appbar 
// fRegister - register and unregister flag 
// 
// Global variables 
//     g_uSide - screen edge (defaults to ABE_TOP) 
//     g_fAppRegistered - flag indicating whether the bar is registered 

BOOL Cdetect_fullscreenDlg::RegisterAccessBar(HWND hwndAccessBar, BOOL fRegister) 
{ 
	APPBARDATA abd; 

	// Specify the structure size and handle to the appbar. 
	abd.cbSize = sizeof(APPBARDATA); 
	abd.hWnd = hwndAccessBar; 

	if (fRegister) { 

		// Provide an identifier for notification messages. 
		abd.uCallbackMessage = MSG_APPBAR_MSGID;

		// Register the appbar. 
		if (!SHAppBarMessage(ABM_NEW, &abd)) 
			return FALSE; 

		g_uSide = ABE_TOP;       // default edge 
		g_fAppRegistered = TRUE; 

	} else { 

		// Unregister the appbar. 
		SHAppBarMessage(ABM_REMOVE, &abd); 
		g_fAppRegistered = FALSE; 
	} 

	return TRUE; 

}


// AppBarQuerySetPos - sets the size and position of an appbar. 
// uEdge - screen edge to which the appbar is to be anchored 
// lprc - current bounding rectangle of the appbar 
// pabd - address of the APPBARDATA structure with the hWnd and 
//     cbSize members filled 

void PASCAL Cdetect_fullscreenDlg::AppBarQuerySetPos(UINT uEdge, LPRECT lprc, PAPPBARDATA pabd) 
{ 
	//int iHeight = 0; 
	//int iWidth = 0; 

	//pabd->rc = *lprc; 
	//pabd->uEdge = uEdge; 

	//// Copy the screen coordinates of the appbar's bounding 
	//// rectangle into the APPBARDATA structure. 
	//if ((uEdge == ABE_LEFT) || 
	//	(uEdge == ABE_RIGHT)) { 

	//		iWidth = pabd->rc.right - pabd->rc.left; 
	//		pabd->rc.top = 0; 
	//		pabd->rc.bottom = GetSystemMetrics(SM_CYSCREEN); 

	//} else { 

	//	iHeight = pabd->rc.bottom - pabd->rc.top; 
	//	pabd->rc.left = 0; 
	//	pabd->rc.right = GetSystemMetrics(SM_CXSCREEN); 

	//} 

	//// Query the system for an approved size and position. 
	//SHAppBarMessage(ABM_QUERYPOS, pabd); 

	//// Adjust the rectangle, depending on the edge to which the 
	//// appbar is anchored. 
	//switch (uEdge) { 

	//	case ABE_LEFT: 
	//		pabd->rc.right = pabd->rc.left + iWidth; 
	//		break; 

	//	case ABE_RIGHT: 
	//		pabd->rc.left = pabd->rc.right - iWidth; 
	//		break; 

	//	case ABE_TOP: 
	//		pabd->rc.bottom = pabd->rc.top + iHeight; 
	//		break; 

	//	case ABE_BOTTOM: 
	//		pabd->rc.top = pabd->rc.bottom - iHeight; 
	//		break; 

	//} 

	//// Pass the final bounding rectangle to the system. 
	//SHAppBarMessage(ABM_SETPOS, pabd); 

	//// Move and size the appbar so that it conforms to the 
	//// bounding rectangle passed to the system. 
	//MoveWindow(pabd->hWnd, pabd->rc.left, pabd->rc.top, 
	//	pabd->rc.right - pabd->rc.left, 
	//	pabd->rc.bottom - pabd->rc.top, TRUE); 

}


// AppBarCallback - processes notification messages sent by the system. 
// hwndAccessBar - handle to the appbar 
// uNotifyMsg - identifier of the notification message 
// lParam - message parameter 

void Cdetect_fullscreenDlg::AppBarCallback(HWND hwndAccessBar, UINT uNotifyMsg, 
					LPARAM lParam) 
{ 
	//APPBARDATA abd; 
	//UINT uState; 

	//abd.cbSize = sizeof(abd); 
	//abd.hWnd = hwndAccessBar; 

	//switch (uNotifyMsg) { 
	//	case ABN_STATECHANGE: 

	//		// Check to see if the taskbar's always-on-top state has 
	//		// changed and, if it has, change the appbar's state 
	//		// accordingly. 
	//		uState = SHAppBarMessage(ABM_GETSTATE, &abd); 

	//		SetWindowPos(hwndAccessBar, 
	//			(ABS_ALWAYSONTOP & uState) ? HWND_TOPMOST : HWND_BOTTOM, 
	//			0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); 

	//		break; 

	//	case ABN_FULLSCREENAPP: 

	//		// A full-screen application has started, or the last full- 
	//		// screen application has closed. Set the appbar's 
	//		// z-order appropriately. 
	//		if (lParam) { 

	//			SetWindowPos(hwndAccessBar, 
	//				(ABS_ALWAYSONTOP & uState) ? HWND_TOPMOST : HWND_BOTTOM, 
	//				0, 0, 0, 0, 
	//				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); 

	//		} 

	//		else { 

	//			uState = SHAppBarMessage(ABM_GETSTATE, &abd); 

	//			if (uState & ABS_ALWAYSONTOP) 
	//				SetWindowPos(hwndAccessBar, HWND_TOPMOST, 
	//				0, 0, 0, 0, 
	//				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); 

	//		} 

	//	case ABN_POSCHANGED: 

	//		// The taskbar or another appbar has changed its 
	//		// size or position. 
	//		AppBarPosChanged(&abd); 
	//		break; 

	//} 
}


// AppBarPosChanged - adjusts the appbar's size and position. 
// pabd - address of an APPBARDATA structure that contains information 
//     used to adjust the size and position 

void PASCAL Cdetect_fullscreenDlg::AppBarPosChanged(PAPPBARDATA pabd) 
{ 
	//RECT rc; 
	//RECT rcWindow; 
	//int iHeight; 
	//int iWidth; 

	//rc.top = 0; 
	//rc.left = 0; 
	//rc.right = GetSystemMetrics(SM_CXSCREEN); 
	//rc.bottom = GetSystemMetrics(SM_CYSCREEN); 

	//GetWindowRect(pabd->hWnd, &rcWindow); 
	//iHeight = rcWindow.bottom - rcWindow.top; 
	//iWidth = rcWindow.right - rcWindow.left; 

	//switch (g_uSide) { 

	//	case ABE_TOP: 
	//		rc.bottom = rc.top + iHeight; 
	//		break; 

	//	case ABE_BOTTOM: 
	//		rc.top = rc.bottom - iHeight; 
	//		break; 

	//	case ABE_LEFT: 
	//		rc.right = rc.left + iWidth; 
	//		break; 

	//	case ABE_RIGHT: 
	//		rc.left = rc.right - iWidth; 
	//		break; 
	//} 

	//AppBarQuerySetPos(g_uSide, &rc, pabd); 
}

