// detect_fullscreenDlg.h : header file
//

#pragma once


// Cdetect_fullscreenDlg dialog
class Cdetect_fullscreenDlg : public CDialog
{
// Construction
public:
	Cdetect_fullscreenDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_DETECT_FULLSCREEN_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	LRESULT WindowProc(UINT msg, WPARAM wp, LPARAM lp);

	BOOL RegisterAccessBar(HWND hwndAccessBar, BOOL fRegister);
	void PASCAL AppBarQuerySetPos(UINT uEdge, LPRECT lprc, PAPPBARDATA pabd) ;
	void AppBarCallback(HWND hwndAccessBar, UINT uNotifyMsg, LPARAM lParam);
	void PASCAL AppBarPosChanged(PAPPBARDATA pabd);

	int g_uSide;
	BOOL g_fAppRegistered;
};

#define MSG_APPBAR_MSGID WM_APP+100
