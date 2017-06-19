// PPCS_ClientUI.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CPPCS_ClientUIApp:
// See PPCS_ClientUI.cpp for the implementation of this class
//

class CPPCS_ClientUIApp : public CWinApp
{
public:
	CPPCS_ClientUIApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CPPCS_ClientUIApp theApp;