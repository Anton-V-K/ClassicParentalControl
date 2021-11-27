#pragma once

struct WorkerData
{
    HANDLE hStopEvent;

    WorkerData();
};

extern WorkerData       g_WorkerData;

DWORD WINAPI WorkerThread(LPVOID lpData);
