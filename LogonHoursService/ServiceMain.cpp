#include "stdafx.h"

#include "config.h"
#include "Logger.h"
#include "WorkerThread.h"

SERVICE_STATUS          g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE   g_StatusHandle  = NULL;


VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    LOG_DEBUG(__func__) << "Entry";

    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        LOG_DEBUG(__func__) << "SERVICE_CONTROL_STOP Request";

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks neccesary to stop the service here
         */

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            LOG_ERROR(__func__) << "SetServiceStatus returned error";
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_WorkerData.hStopEvent);

        break;

    default:
        break;
    }

    LOG_DEBUG(__func__) << "Exit";
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
    DWORD Status = E_FAIL;

    LOG_DEBUG(__func__) << "Entry";

    g_StatusHandle = RegisterServiceCtrlHandler(_T(SERVICE_NAME), ServiceCtrlHandler);

    if (g_StatusHandle == NULL)
    {
        LOG_ERROR(__func__) << "RegisterServiceCtrlHandler failed";
    }
    else
    {
        // Tell the service controller we are starting
        ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
        g_ServiceStatus.dwServiceType       = SERVICE_WIN32_OWN_PROCESS;
        g_ServiceStatus.dwControlsAccepted  = 0;
        g_ServiceStatus.dwCurrentState      = SERVICE_START_PENDING;
        g_ServiceStatus.dwWin32ExitCode     = 0;
        g_ServiceStatus.dwServiceSpecificExitCode = 0;
        g_ServiceStatus.dwCheckPoint        = 0;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            LOG_ERROR(__func__) << "SetServiceStatus(SERVICE_START_PENDING) failed";
        }

        /*
         * Perform tasks neccesary to start the service here
         */
        LOG_DEBUG(__func__) << "Performing Service Start Operations";

        // Create stop event to wait on later.
        g_WorkerData.hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (g_WorkerData.hStopEvent == NULL)
        {
            LOG_ERROR(__func__) << "CreateEvent() for hStopEvent failed";

            g_ServiceStatus.dwControlsAccepted = 0;
            g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            g_ServiceStatus.dwWin32ExitCode = GetLastError();
            g_ServiceStatus.dwCheckPoint = 1;

            if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
            {
                LOG_ERROR(__func__) << "SetServiceStatus(SERVICE_STOPPED) failed";
            }
        }
        else
        {

            // Tell the service controller we are started
            g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
            g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
            g_ServiceStatus.dwWin32ExitCode = 0;
            g_ServiceStatus.dwCheckPoint = 0;

            if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
            {
                LOG_ERROR(__func__) << "SetServiceStatus(SERVICE_RUNNING) failed";
            }

            // Start the thread that will perform the main task of the service
            const HANDLE hThread = CreateThread(NULL, 0, WorkerThread, &g_WorkerData, 0, NULL);
            if (hThread)
            {
                LOG_DEBUG(__func__) << "Waiting for Worker Thread to finish";

                // Wait until our worker thread exits effectively signaling that the service needs to stop
                WaitForSingleObject(hThread, INFINITE);

                LOG_DEBUG(__func__) << "Worker Thread is over";
            }
            else
                LOG_ERROR(__func__) << "CreateThread failed!";

            // Perform any cleanup tasks
            LOG_DEBUG(__func__) << "Performing Cleanup Operations";

            CloseHandle(g_WorkerData.hStopEvent);

            g_ServiceStatus.dwControlsAccepted = 0;
            g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            g_ServiceStatus.dwWin32ExitCode = 0;
            g_ServiceStatus.dwCheckPoint = 3;

            if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
            {
                LOG_DEBUG(__func__) << "SetServiceStatus(SERVICE_STOPPED) failed";
            }
        }
    }

    LOG_DEBUG(__func__) << "Exit";
}
