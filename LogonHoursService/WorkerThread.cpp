#include "stdafx.h"

#include "config.h"
#include "Logger.h"
#include "WorkerThread.h"

#pragma comment(lib, "Netapi32.lib")                // NetUserGetInfo(), etc.
//#pragma comment(lib, "Secur32.lib")                 // LsaEnumerateLogonSessions(), etc.
#pragma comment(lib, "Wtsapi32.lib")                // WTSEnumerateSessions(), etc.

WorkerData              g_WorkerData;

enum EDayOfWeek
{
    Sun, Mon, Tue, Wed, Thu, Fri, Sat,
};

class LogonHours
{
public:
    LogonHours(PBYTE usri2_logon_hours)
    {
        Init(usri2_logon_hours);
    }
    void Init(PBYTE usri2_logon_hours)
    {
        m_all = true;
        int day     = Sun;
        int hour    = 0;    // 0:00
        for (int p = 0; p < 21; ++p)
        {
            const BYTE chunk = usri2_logon_hours[p];
            BYTE mask = 0b00000001;
            for (int bit = 0; bit < 8; ++bit)
            {
                m_data[day][hour] = (chunk & mask) != 0;
                if (!m_data[day][hour])
                    m_all = false;
                ++hour;
                mask <<= 1;
                if (hour >= 24)
                {
                    ++day;
                    hour = 0;
                }
            }
        }
    }

    bool All() const { return m_all; }
    bool Allowed(EDayOfWeek day, WORD hour) const
    {
        if (Sun <= day && day <= Sat && 0 <= hour && hour <= 23)
            return m_data[day][hour];
        return false;
    }
    // @return seconds left (for the given time) until the allowed logon period expires, or -1 if no restrictions are set
    // @note 0 (zero) is returned if logon isn't allowed during given time
    long SecondsLeft(EDayOfWeek day, WORD hour, WORD minute, WORD second) const
    {
        if (All())
            return -1;
        if (!Allowed(day, hour))
            return 0;
        long left = (60 - second - 1) + 60 * (60 - minute - 1); // the rest of current hour is allowed
        // TODO other hours/days
        return left;
    }

private:
    bool m_all;             // true if no logon restrictions
    bool m_data[7][24];
};

WorkerData::WorkerData()
{
    hStopEvent = NULL;
}

DWORD WINAPI WorkerThread(LPVOID lpData)
{
    USES_CONVERSION;

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
                if (!WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, session_id, WTSConnectState, &pBuffer, &bytes))
                    continue;
                const WTS_CONNECTSTATE_CLASS session_state = *reinterpret_cast<WTS_CONNECTSTATE_CLASS*>(pBuffer);
                WTSFreeMemory(pBuffer);
                if (session_state != WTSActive)
                    continue;
                if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, session_id, WTSUserName, &pBuffer, &bytes))
                {
                    if (wcslen(pBuffer))
                    {
                        const std::wstring username(pBuffer);
                        LOG_DEBUG(__func__) << "WTSQuerySessionInformation: active session " << session_id
                            << ", user '" << T2A(username.c_str()) << '\'';
                        sessions.push_back({ session_id, username });
                    }
                    WTSFreeMemory(pBuffer);
                }
            }
            WTSFreeMemory(pSessionInfo);
        }
        LOG_DEBUG(__func__) << sessions.size() << " active session(s) detected";

#if 0
        ULONG SessionsCount;
        PLUID SessionsList;
        const NTSTATUS status = LsaEnumerateLogonSessions(&SessionsCount, &SessionsList);
        if (status == 0) // SCESTATUS_SUCCESS // STATUS_SUCCESS | 
        {
            std::set<std::wstring> users;
            for (ULONG i = 0; i < SessionsCount; ++i)
            {
                PSECURITY_LOGON_SESSION_DATA pSessionData;
                if (LsaGetLogonSessionData(&SessionsList[i], &pSessionData) == 0) // STATUS_SUCCESS
                {
                    if (Interactive == pSessionData->LogonType)
                    {
                        users.insert(pSessionData->UserName.Buffer);
                    }
                    LsaFreeReturnBuffer(pSessionData);
                }
            }

            for (const auto username: users)
            {
#else
        for (const auto session: sessions)
        {
            const auto& wusername = session.user;
#endif
            
                LPBYTE bufptr;
                // Refer to https://docs.microsoft.com/en-us/windows/win32/api/lmaccess/nf-lmaccess-netusergetinfo
                const NET_API_STATUS result = NetUserGetInfo(NULL, wusername.c_str(), 2, &bufptr);
                if (result == NERR_Success)
                {
                    const USER_INFO_2* const userinfo2 = reinterpret_cast<USER_INFO_2*>(bufptr);
                    const LogonHours hours(userinfo2->usri2_logon_hours);
                    NetApiBufferFree(bufptr);

                    const char* const username = T2A(session.user.c_str());
                    if (!hours.All())
                    {
                        LOG_DEBUG(__func__) << "checking logon hours for session " << session.id << " '" << username << "'";

                        SYSTEMTIME systime; GetSystemTime(&systime); // UTC/GMT

                        const auto seconds_left = hours.SecondsLeft(EDayOfWeek(systime.wDayOfWeek), systime.wHour, systime.wMinute, systime.wSecond);
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
#if 0
            }
            LsaFreeReturnBuffer(SessionsList);
#else
        }
#endif
    }

    LOG_DEBUG(__func__) << "Exit";

    return ERROR_SUCCESS;
}
