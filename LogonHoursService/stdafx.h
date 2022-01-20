// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#include <LM.h>                         // NetUserGetInfo(), etc.
#include <tchar.h>
#include <winsvc.h>
#include <WtsApi32.h>                   // WTSEnumerateSessions(), etc.

//#include <iostream>
#include <string>

//#include <log4cpp/Appender.hh>
#include <log4cpp/Category.hh>
#include <log4cpp/DailyRollingFileAppender.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/PatternLayout.hh>
//#include <log4cpp/Priority.hh>
#include <log4cpp/PropertyConfigurator.hh>
//#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/Win32DebugAppender.hh>

// reference additional headers your program requires here

#ifdef _UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif
