#include "stdafx.h"

#include "config.h"
#include "Logger.h"
#include "ServiceMain.h"
#include "ServiceManager.h"
#include "WorkerThread.h"

#include "version.h"

#pragma comment(lib, "ws2_32.lib")                      // Needed for log4cpp if PropertyConfigurator is used

void Main()
{
    LOG_DEBUG(__func__) << "Entry";

    // Create stop event to wait on later.
    g_WorkerData.hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_WorkerData.hStopEvent == NULL)
    {
        LOG_ERROR(__func__) << "CreateEvent() for hStopEvent failed";
    }
    else
    {
        // Start the thread that will perform the main task
        const HANDLE hThread = CreateThread(NULL, 0, WorkerThread, &g_WorkerData, 0, NULL);
        if (hThread)
        {
            LOG_DEBUG(__func__) << "Waiting for Worker Thread to finish";

            // Wait until our worker thread exits
            WaitForSingleObject(hThread, INFINITE);

            LOG_DEBUG(__func__) << "Worker Thread is over";

            // Perform any cleanup tasks
            LOG_DEBUG(__func__) << "Performing Cleanup Operations";
        }
        else
            LOG_ERROR(__func__) << "CreateThread failed!";

        CloseHandle(g_WorkerData.hStopEvent);
    }

    LOG_DEBUG(__func__) << "Exit";
}

#ifdef _CONSOLE
int _tmain(int argc, TCHAR* argv[])
#else
int APIENTRY _tWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
#endif
{
    OutputDebugString(_T(SERVICE_NAME) _T(": Start\n"));

    log4cpp::Category& root = log4cpp::Category::getRoot();

    ////////////////////////////////////////
    // Configigure log4cpp
    ////////////////////////////////////////
    char config_path[_MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, config_path, _countof(config_path));
    strcat_s(config_path, ".log4cpp");
    try
    {
        log4cpp::PropertyConfigurator::configure(config_path);
    }
    catch (const log4cpp::ConfigureFailure& /*ex*/)
    {
        config_path[0] = 0;
/*
#ifdef _CONSOLE
        std::cerr
            << ex.what()
            << " [log4cpp::ConfigureFailure catched] while reading "
            << file_log4cpp_init
            << std::endl;
#endif
*/

        // Default configuration for log4cpp
#ifdef _DEBUG
        root.setPriority(log4cpp::Priority::DEBUG);
#else
        root.setPriority(log4cpp::Priority::INFO);
#endif

    }

    const log4cpp::AppenderSet& set = root.getAllAppenders();
    if (set.empty())            // if no appenders are specifiec in the config file ...
    {
        char log_path[MAX_PATH];
        ExpandEnvironmentStringsA("%TEMP%\\" SERVICE_NAME ".log", log_path, _countof(log_path));
#if 1
        if (log4cpp::Appender* const appender = new log4cpp::RollingFileAppender("logfile", log_path))
#else
        if (log4cpp::Appender* const appender = new log4cpp::FileAppender("logfile", log_path))
#endif
        {
            log4cpp::PatternLayout* const layout = new log4cpp::PatternLayout();
            // Refer to http://www.cplusplus.com/reference/ctime/strftime/ for format specification
            layout->setConversionPattern("%d{%Y.%m.%d %H:%M:%S,%l} [%p] [%t] %c: %m%n");
            appender->setLayout(layout);
            root.addAppender(appender);
        }
        if (log4cpp::Appender* const appender = new log4cpp::Win32DebugAppender("debugger"))
        {
            log4cpp::PatternLayout* const layout = new log4cpp::PatternLayout();
            // Refer to http://www.cplusplus.com/reference/ctime/strftime/ for format specification
            layout->setConversionPattern("%d{%H:%M:%S,%l} [%p] [%t] %c: %m%n");
            appender->setLayout(layout);
            root.addAppender(appender);
        }
    }

    LOG_INFO(__func__) << "========================================";
    LOG_INFO(__func__) << "Log initialized successfully";
    if (*config_path)
        LOG_INFO(__func__) << "Loaded " << config_path;

#ifndef _CONSOLE
    LOG_INFO(__func__) << "Length of command line: " << _tcslen(lpCmdLine);
#endif

    LOG_INFO(__func__) << "Version: " << VERSION_STRING;
    LOG_INFO(__func__) << "Build  : " << __DATE__ << ' ' << __TIME__;
#if defined(_MSC_FULL_VER)
    LOG_INFO(__func__) << "_MSC_FULL_VER: "
        << _MSC_FULL_VER / 10000000 << '.'
        << (_MSC_FULL_VER % 10000000) / 100000 << '.'
        << _MSC_FULL_VER % 100000;
#elif defined(_MSC_VER)
    LOG_INFO(__func__) << "_MSC_VER: "
        << _MSC_VER / 100 << '.'
        << _MSC_VER % 100;
#endif


#ifdef _CONSOLE
    if (argc > 1)
    {
#endif
        // Let's handle the command line
#ifdef _CONSOLE
        if (_tcsicmp(argv[1], CMD_LINE_INSTALL) == 0)
#else
        if (_tcsstr(lpCmdLine, CMD_LINE_INSTALL))
#endif
        {
            CServiceManager scm;
            SERVICEINFO info;
            wchar_t path[_MAX_PATH] = { 0 };
            GetModuleFileNameW(NULL, path, _countof(path));
            _tcscat_s(path, _T(" "));
            _tcscat_s(path, CMD_LINE_SERVICE);
            info.lpBinaryPathName   = path;
            info.lpDisplayName      = _T(SERVICE_FULL_NAME);
            info.lpServiceName      = _T(SERVICE_NAME);
            if (scm.Create(info))
            {
#ifdef _CONSOLE
                _tprintf(_T("Service %s installed successfully!"), _T(SERVICE_NAME));
#endif
#ifdef SERVICE_DESC
                if (_tcslen(_T(SERVICE_DESC)))
                    scm.SetDescription(_T(SERVICE_NAME), _T(SERVICE_DESC));
#endif
            }
            else
            {
#ifdef _CONSOLE
                _tprintf(_T("ERROR: Cannot install service %s!"), _T(SERVICE_NAME));
#endif
            }
        }
#ifdef _CONSOLE
        else if (_tcsicmp(argv[1], CMD_LINE_SERVICE) == 0)
#else
        else if (_tcsstr(lpCmdLine, CMD_LINE_SERVICE))
#endif
        {
            static const SERVICE_TABLE_ENTRY ServiceTable[] =
            {
                { _T(SERVICE_NAME), ServiceMain },
                { NULL,             NULL}
            };

            LOG_DEBUG(__func__) << "Calling StartServiceCtrlDispatcher...";
            if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
            {
                const DWORD dwLastErr = GetLastError();
                LOG_EMERG(__func__) << "StartServiceCtrlDispatcher returned error " << std::hex << dwLastErr;
                return dwLastErr;
            }
        }
#ifdef _CONSOLE
        else if (_tcsicmp(argv[1], CMD_LINE_UNINSTALL) == 0)
#else
        else if (_tcsstr(lpCmdLine, CMD_LINE_UNINSTALL))
#endif
        {
            CServiceManager scm;
            if (scm.Delete(_T(SERVICE_NAME)))
            {
#ifdef _CONSOLE
                _tprintf(_T("Service %s uninstalled successfully!"), _T(SERVICE_NAME));
#endif
            }
            else
            {
#ifdef _CONSOLE
                _tprintf(_T("ERROR: Cannot uninstall service %s!"), _T(SERVICE_NAME));
#endif
            }
        }
#ifdef _CONSOLE
    }
#endif
    else
        Main();

    LOG_DEBUG(__func__) << "Exit";
    return 0;
}
