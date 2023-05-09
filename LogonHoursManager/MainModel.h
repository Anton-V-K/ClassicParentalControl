#pragma once

#include <Common/LogonHours.h>

class MainModel
{
public:
    MainModel();
    ~MainModel();

    const LogonHours&   getHours() const;
    LogonHours&         getHours();

    const wchar_t* getUser() const;
    std::vector<std::wstring> getUsers() const;

    bool isElevated() const;

    void selectUser(const wchar_t* user);

protected:
    void fetchUsers();

private:
    struct Data;
    Data* const m_;
};
