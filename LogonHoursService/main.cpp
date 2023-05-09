#include "stdafx.h"

#include "config.h"
#include "Logger.h"
#include "ServiceMain.h"
#include "ServiceManager.h"
#include "WorkerThread.h"

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

    LogMain(SERVICE_NAME);

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
