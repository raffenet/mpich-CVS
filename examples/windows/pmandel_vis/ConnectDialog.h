#pragma once


// CConnectDialog dialog

class CConnectDialog : public CDialog
{
	DECLARE_DYNAMIC(CConnectDialog)

public:
	CConnectDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConnectDialog();

// Dialog Data
	enum { IDD = IDD_CONNECT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    int m_nPort;
    CString m_pszHost;
};
