#include "stdafx.h"
#include "LogonHours.h"

#include "Logger.h"

#pragma comment(lib, "Netapi32.lib")                // NetUserGetInfo(), etc.

EDayOfWeek& operator ++(EDayOfWeek& day)
{
    if (day < EDayOfWeek::LAST)
    {
        int value = static_cast<int>(day);
        day = static_cast<EDayOfWeek>(++value);
        return day;
    }
    LOG_ERROR(__func__) << "Cannot increment " << int(day);
    return day;
}

LogonHours::LogonHours()
{
}

LogonHours::LogonHours(PBYTE usri2_logon_hours)
{
    Init(usri2_logon_hours);
}

bool LogonHours::ApplyTo(const char* username) const
{
    const std::wstring wusername(username, username + strlen(username));
    return ApplyTo(wusername.c_str());
}

bool LogonHours::ApplyTo(const wchar_t* username) const
{
    // Each bit in the string represents a unique hour in the week, in Greenwich Mean Time (GMT).
    // The first bit (bit 0, word 0) is Sunday, 0:00 to 0:59; the second bit (bit 1, word 0) is Sunday, 1:00 to 1:59; and so on.
    // (c) https://learn.microsoft.com/en-us/windows/win32/api/lmaccess/ns-lmaccess-user_info_1020
    BYTE usri2_logon_hours[21];
    // By default no restrictions
    memset(usri2_logon_hours, 0xFF, _countof(usri2_logon_hours));
    if (!m_all)
    {
        int pos = 0; // in the array
        int bit = 0; // bit index
        const BYTE mask = 0b00000001;
        for (int day = FIRST; day <= LAST; ++day)
        {
            for (int hour = 0; hour < 24; ++hour)
            {
                if (!m_data[day][hour])
                    usri2_logon_hours[pos] &= ~(mask << bit); // clear bit
                ++bit;
                if (bit >= 8)
                {
                    bit = 0;
                    ++pos; // next chunk
                }
            }
        }
    }
    USER_INFO_1020 info;
    DWORD parm_err;
    info.usri1020_units_per_week = UNITS_PER_WEEK; // NetUserAdd and NetUserSetInfo functions ignore this member
    info.usri1020_logon_hours = usri2_logon_hours;
    if (NetUserSetInfo(NULL, username, 1020, reinterpret_cast<BYTE*>(&info), &parm_err) != NERR_Success)
    {
        LOG_ERROR(__func__) << "NetUserSetInfo() failed with error " << GetLastError();
        return false;
    }
    return true;
}

void LogonHours::Init(PBYTE usri2_logon_hours)
{
    m_all = true;
    int day = Sun;
    int hour = 0;    // 0:00
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

bool LogonHours::InitFrom(const char* username)
{
    const std::wstring wusername(username, username + strlen(username));
    return InitFrom(wusername.c_str());
}

bool LogonHours::InitFrom(const wchar_t* username)
{
    LPBYTE bufptr;
    // Refer to https://docs.microsoft.com/en-us/windows/win32/api/lmaccess/nf-lmaccess-netusergetinfo
    const NET_API_STATUS result = NetUserGetInfo(NULL, username, 2, &bufptr);
    if (result != NERR_Success)
        return false;

    const USER_INFO_2* const userinfo2 = reinterpret_cast<USER_INFO_2*>(bufptr);
    Init(userinfo2->usri2_logon_hours);

    NetApiBufferFree(bufptr);

    return true;
}

bool LogonHours::Allowed(EDayOfWeek day, WORD hour) const
{
    if (Sun <= day && day <= Sat && 0 <= hour && hour <= 23)
        return m_data[day][hour];
    return false;
}

bool LogonHours::Allow(EDayOfWeek day, WORD hour, bool allow)
{
    if (Sun <= day && day <= Sat && 0 <= hour && hour <= 23)
        if (m_data[day][hour] != allow)
        {
            m_data[day][hour] = allow;
            if (!allow && m_all)
                m_all = false;
            return true;
        }
    return false;
}

void LogonHours::Get(WeekHours& week, bool utc) const
{
    int pos = utc ? 0 : getSun0();
    for (int d = 0; d < 7; ++d)
    {
        for (WORD h = 0; h < 24; ++h)
        {
            week.array[pos] = m_data[d][h];
            ++pos;
            if (pos >= sizeof(week.array))
                pos = 0;
        }
    }
}

void LogonHours::Set(const WeekHours& week, bool utc)
{
    int pos = utc ? 0 : getSun0();
    for (int d = 0; d < 7; ++d)
    {
        for (WORD h = 0; h < 24; ++h)
        {
            m_data[d][h] = week.array[pos];
            ++pos;
            if (pos >= sizeof(week.array))
                pos = 0;
        }
    }
}

long LogonHours::SecondsLeft(EDayOfWeek day, WORD hour, WORD minute, WORD second) const
{
    if (All())
        return -1;
    if (!Allowed(day, hour))
        return 0;
    long left = (60 - second - 1) + 60 * (60 - minute - 1); // the rest of current hour is allowed
    WORD hour_start = hour + 1;                     // for the current day - start with the next hour
    EDayOfWeek d = day;
    for (int i = 0; i < 7; ++i)                     // we'll check all week days starting from the current one
    {
        for (WORD h = hour_start; h < 24; ++h)      // check hours until end of day
        {
            if (Allowed(day, h))
                left += 3600;
            else
                return left;
        }
        hour_start = 0;                             // for the rest of days
        // next day
        if (d == EDayOfWeek::LAST)
            d = EDayOfWeek::FIRST;
        else
            ++d;
    }
    return left;
}

int LogonHours::getSun0()
{
    TIME_ZONE_INFORMATION tz = { 0 };
    GetTimeZoneInformation(&tz);
    return (tz.Bias < 0) ? (-tz.Bias / 60) : (7 * 24 - tz.Bias / 60);
}
