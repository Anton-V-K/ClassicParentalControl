#pragma once

#include "WeekHours.h"

enum EDayOfWeek
{
    // NOTE: the order can't be changed - it matches the logon hours bit-array
    Sun, Mon, Tue, Wed, Thu, Fri, Sat,
    FIRST = Sun,
    LAST = Sat,
};
EDayOfWeek& operator ++(EDayOfWeek& day);

class LogonHours
{
public:
    LogonHours();
    LogonHours(PBYTE usri2_logon_hours);

    bool ApplyTo(const char* username) const;
    bool ApplyTo(const wchar_t* username) const;

    void Init(PBYTE usri2_logon_hours);

    bool InitFrom(const char* username);
    bool InitFrom(const wchar_t* username);

    bool All() const { return m_all; }
    bool Allowed(EDayOfWeek day, WORD hour) const;

    bool Allow(EDayOfWeek day, WORD hour, bool allow);

    void Get(WeekHours& week, bool utc = true) const;
    void Set(const WeekHours& week, bool utc = true);

    // @return seconds left (for the given time) until the allowed logon period expires, or -1 if no restrictions are set
    // @note 0 (zero) is returned if logon isn't allowed during given time
    long SecondsLeft(EDayOfWeek day, WORD hour, WORD minute, WORD second) const;

protected:
    /// <summary>
    /// Calculates cell index which corresponds to Sun 0:00 UTC in local time zone.
    /// </summary>
    /// <returns>Zero-based index of cell in WeekHours.array</returns>
    static int getSun0();

private:
    bool m_all{ true };             // true if no logon restrictions
    bool m_data[7][24];
};
