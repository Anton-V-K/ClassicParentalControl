// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef STDAFX_H_LogonHoursManager_
#define STDAFX_H_LogonHoursManager_

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <Windows.h>

#include <LM.h>                         // NetUserGetInfo(), etc.

// C RunTime Header Files
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <tchar.h>

// STL
#include <string>

// ATL
#include <atlbase.h>

// WTL
#include <atlapp.h>
extern CAppModule _Module;
#include <atlcrack.h>                   // MSG_WM_INITDIALOG, etc.
#include <atlctrls.h>                   // CToolTipCtrl
// #define _ATL_USE_DDX_FLOAT
#include <atlddx.h>
#include <atlframe.h>                   // CUpdateUI
#include <atlwin.h>

#include <Common/Logger.h>

#endif
