// ConnectDialog.cpp : implementation file
//

#include "stdafx.h"
#include "pman_vis.h"
#include "ConnectDialog.h"


// CConnectDialog dialog

IMPLEMENT_DYNAMIC(CConnectDialog, CDialog)
CConnectDialog::CConnectDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CConnectDialog::IDD, pParent)
	, m_nPort(0)
	, m_pszHost(_T(""))
{
}

CConnectDialog::~CConnectDialog()
{
}

void CConnectDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_PORT, m_nPort);
    DDX_Text(pDX, IDC_HOSTNAME, m_pszHost);
}


BEGIN_MESSAGE_MAP(CConnectDialog, CDialog)
END_MESSAGE_MAP()


// CConnectDialog message handlers
