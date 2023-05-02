#include "stdafx.h"

#include <Common/LogonHours.h>

#include "config.h"
#include "Logger.h"
#include "WorkerThread.h"

#pragma comment(lib, "Netapi32.lib")                // NetUserGetInfo(), etc.
#pragma comment(lib, "Wtsapi32.lib")                // WTSEnumerateSessions(), etc.

// Converts wide string to multibyte string, allocating memory for new string
LPSTR W2A(LPCWSTR lpwszStrIn)
{
    if (!lpwszStrIn)
        return NULL;
    const size_t nInputStrLen = wcslen(lpwszStrIn);

    // Double NULL termination
    const int nOutputStrLen = WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, NULL, 0, 0, 0) + 2;
    const LPSTR pszOut = new char[nOutputStrLen];

    if (pszOut)
    {
        memset(pszOut, 0x00, nOutputStrLen);
        WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, pszOut, nOutputStrLen, 0, 0);
    }
    return pszOut;
}

WorkerData              g_WorkerData;

void seconds2time(long seconds, long* days, WORD* hours, WORD* mins, WORD* secs)
{
    if (days)
        *days = seconds / (24 * 3600);
    seconds %= (24 * 3600);
    if (hours)
        *hours = WORD(seconds / 3600);
    seconds %= 3600;
    if (mins)
        *mins = WORD(seconds / 60);
    seconds %= 60;
    if (secs)
        *secs = WORD(seconds);
}

WorkerData::WorkerData()
{
    hStopEvent = NULL;
}

DWORD WINAPI WorkerThread(LPVOID lpData)
{
    LOG_DEBUG(__func__) << "Entry";

    WorkerData* const pData = reinterpret_cast<WorkerData*>(lpData);
#ifdef _DEBUG
    enum { TIMEOUT = 5000 };
#else
    enum { TIMEOUT = 10000 };
#endif
    //  Periodically check if the service has been requested to stop
    while (pData->hStopEvent && WaitForSingleObject(pData->hStopEvent, TIMEOUT) != WAIT_OBJECT_0)
    {
        struct Session
        {
            DWORD           id;
            std::wstring    user;
        };
        std::vector<Session> sessions;
        PWTS_SESSION_INFO pSessionInfo;
        DWORD count;
        LOG_DEBUG(__func__) << "Starting WTSEnumerateSessions";
        if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &count))
        {
            for (DWORD i = 0; i < count; ++i)
            {
                const auto session_id = pSessionInfo[i].SessionId;
                LPWSTR pBuffer;
                DWORD bytes;
                LOG_DEBUG(__func__) << "Starting WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, " << session_id  << ", WTSConnectState, ...)";
                if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, session_id, WTSConnectState, &pBuffer, &bytes))
                    continue;
                const WTS_CONNECTSTATE_CLASS session_state = *reinterpret_cast<WTS_CONNECTSTATE_CLASS*>(pBuffer);
                WTSFreeMemory(pBuffer);
                if (session_state != WTSActive)
                    continue;
                LOG_DEBUG(__func__) << "Starting WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, " << session_id << ", WTSUserName, ...)";
                if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, session_id, WTSUserName, &pBuffer, &bytes))
                {
                    if (wcslen(pBuffer))
                    {
                        const std::wstring wusername(pBuffer);
                        const std::unique_ptr<char[]> username(W2A(wusername.c_str()));
                        LOG_DEBUG(__func__) << "WTSQuerySessionInformation: active session " << session_id
                            << " '" << username.get() << '\''
                            ;
                        sessions.push_back({ session_id, wusername });
                    }
                    WTSFreeMemory(pBuffer);
                }
            }
            WTSFreeMemory(pSessionInfo);
        }
        LOG_DEBUG(__func__) << sessions.size() << " active session(s) detected";

        for (const auto session: sessions)
        {
            const auto& wusername = session.user;
            
            LogonHours hours;
            if (hours.InitFrom(wusername.c_str()))
                {
                    if (!hours.All())
                    {
                        const std::unique_ptr<char[]> username(W2A(wusername.c_str()));

                        LOG_DEBUG(__func__) << "checking logon hours for session " << session.id << " '" << username << "'";

                        SYSTEMTIME systime; GetSystemTime(&systime); // UTC/GMT

                        const auto seconds_left = hours.SecondsLeft(EDayOfWeek(systime.wDayOfWeek), systime.wHour, systime.wMinute, systime.wSecond);
                        if (seconds_left >= 0)
                        {
                            long days;
                            WORD hours, minutes, seconds;
                            seconds2time(seconds_left, &days, &hours, &minutes, &seconds);
                            LOG_DEBUG(__func__)
                                << days << "d:"
                                // TODO leading zeros
                                << hours << ":" << minutes << ":" << seconds
                                << " (" << seconds_left << " seconds) left for session " << session.id << " '" << username << "'";
                        }
                        if (0 <= seconds_left && seconds_left <= 60)
                        {
                            LOG_INFO(__func__) << seconds_left << " seconds left for session " << session.id << " '" << username << "'";
                            if (true) // always notify user?
                            {
                                LOG_DEBUG(__func__) << "WTSSendMessage for sesssion " << session.id;
                                TCHAR message[] = _T("Your session will end in 1 minute!");
                                DWORD response;
                                if (!WTSSendMessage(WTS_CURRENT_SERVER_HANDLE, session.id
                                    , _T(SERVICE_FULL_NAME), strlen(SERVICE_FULL_NAME) * sizeof(TCHAR) // in bytes
                                    , message, _tcslen(message) * sizeof(message[0]) // in bytes
                                    , MB_OK | MB_ICONWARNING, 60, &response, FALSE))
                                {
                                    LOG_ERROR(__func__) << "WTSSendMessage failed to send message into session " << session.id;
                                }
                                Sleep(60 * 1000);
                            }
                            else
                            {
                                Sleep(seconds_left * 1000);
                            }

                            LOG_DEBUG(__func__) << "WTSDisconnectSession for sesssion " << session.id;
                            if (WTSDisconnectSession(WTS_CURRENT_SERVER_HANDLE, session.id, FALSE))
                            {
                                LOG_INFO(__func__) << "Session " << session.id << " '" << username << "' disconnected!";
                            }
                            else
                            {
                                const auto err = GetLastError();
                                LOG_ERROR(__func__) << "WTSDisconnectSession for sesssion " << session.id << " failed with error " << err;
                            }
                        }
                    }
                }
                else
                {

                }
        }
    }

    LOG_DEBUG(__func__) << "Exit";

    return ERROR_SUCCESS;
}
