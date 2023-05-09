#include "stdafx.h"
#include "MainModel.h"

struct MainModel::Data
{
    //       username   => logon hours
    std::map<std::wstring, LogonHours> hours;

    std::wstring user;    // active user
};

MainModel::MainModel()
    : m_(new Data)
{
    fetchUsers();
    selectUser(nullptr);
}

MainModel::~MainModel()
{
    delete m_;
}

const LogonHours& MainModel::getHours() const
{
    return m_->hours[m_->user];
}

LogonHours& MainModel::getHours()
{
    return m_->hours[m_->user];
}

const wchar_t* MainModel::getUser() const
{
    return m_->user.c_str();
}

std::vector<std::wstring> MainModel::getUsers() const
{
    std::vector<std::wstring> users;
    for (const auto& it : m_->hours)
    {
        users.push_back(it.first);
    }
    return users;
}

bool MainModel::isElevated() const
{
    static bool init_ok{ false };
    static TOKEN_ELEVATION_TYPE type;
    if (!init_ok)
    {
        init_ok = true;
        HANDLE hProcess;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hProcess))
        {
            DWORD size;
            if (GetTokenInformation(hProcess, TokenElevationType, &type, sizeof(type), &size))
            {
                if (size == sizeof(type))
                {
                    LOG_DEBUG(__func__) << "GetTokenInformation: TOKEN_ELEVATION_TYPE = " << int(type);
                }
            }
            else
                LOG_ERROR(__func__) << "GetTokenInformation(..., TokenElevationType, ...) failed with error " << GetLastError();
            CloseHandle(hProcess);
        }
        else
            LOG_ERROR(__func__) << "OpenProcessToken(GetCurrentProcess(), TOKEN_READ, ...) failed with error " << GetLastError();
    }

    return type == TokenElevationTypeFull;
}

void MainModel::selectUser(const wchar_t* user)
{
    if (user)
    {
        m_->user = user;
    }
    else
    {
        TCHAR username[256];
        DWORD len = _countof(username);
        if (GetUserName(username, &len) == 0)
            LOG_ERROR(__func__) << "GetUserName() failed with error " << GetLastError();
        else
            m_->user = username;
    }

    if (!m_->user.empty())
        m_->hours[m_->user].InitFrom(m_->user.c_str());
}

void MainModel::fetchUsers()
{
    LPBYTE buffer;
    DWORD entriesread, totalentries;
    if (NetUserEnum(NULL, 1, FILTER_NORMAL_ACCOUNT, &buffer, MAX_PREFERRED_LENGTH, &entriesread, &totalentries, NULL) != NERR_Success)
        return;
    const auto* const userinfo = reinterpret_cast<const USER_INFO_1*>(buffer);
    for (DWORD i = 0; i < entriesread; ++i)
    {
        if ((userinfo[i].usri1_flags & USER_PRIV_MASK) == USER_PRIV_USER)
            m_->hours[userinfo[i].usri1_name] = LogonHours();
    }
    NetApiBufferFree(buffer);
}
