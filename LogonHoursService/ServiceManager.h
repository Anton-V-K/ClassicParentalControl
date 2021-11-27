#if !defined(ServiceManager_h_)
#define ServiceManager_h_
//////////////////////////////////////////////////////////////////////
// Author :- Nish 
//////////////////////////////////////////////////////////////////////

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct SERVICEINFO
{
public:
    bool    bAutoStart;
    LPCTSTR lpBinaryPathName;
    LPCTSTR lpDisplayName;
    LPCTSTR lpServiceName;

    SERVICEINFO();
};

class CServiceManager
{
public:
    CServiceManager();
    ~CServiceManager();

    bool Create(const SERVICEINFO& info);
    bool Delete(LPCTSTR serviceName);

    bool Continue(LPCTSTR serviceName);
    bool Pause(LPCTSTR serviceName);
    bool Start(LPCTSTR serviceName);
    bool Stop(LPCTSTR serviceName);

    bool SetDescription(LPCTSTR serviceName, LPCTSTR description);

private:
    SC_HANDLE m_scm;
};


#endif
