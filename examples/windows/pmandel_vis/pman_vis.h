// pman_vis.h : main header file for the pman_vis application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


// Cpman_visApp:
// See pman_vis.cpp for the implementation of this class
//

class Cpman_visApp : public CWinApp
{
public:
	Cpman_visApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern Cpman_visApp theApp;