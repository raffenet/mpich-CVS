#pragma once


// CBoundsDlg dialog

class CBoundsDlg : public CDialog
{
	DECLARE_DYNAMIC(CBoundsDlg)

public:
	CBoundsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBoundsDlg();

// Dialog Data
	enum { IDD = IDD_BOUNDS_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    double m_xmin;
    double m_ymin;
    double m_xmax;
    double m_ymax;
    int m_max_iter;
};
