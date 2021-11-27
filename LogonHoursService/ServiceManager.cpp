//////////////////////////////////////////////////////////////////////
// Author :- Nish (c) https://www.codeproject.com/Articles/2312/CServiceManager
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ServiceManager.h"

//////////////////////////////////////////////////////////////////////
// SERVICEINFO
//////////////////////////////////////////////////////////////////////

SERVICEINFO::SERVICEINFO()
{
    bAutoStart = false;
    lpBinaryPathName = NULL;
    lpServiceName = NULL;
    lpDisplayName = NULL;
}

CServiceManager::CServiceManager()
{
    m_scm = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
}

CServiceManager::~CServiceManager()
{
    CloseServiceHandle(m_scm);
}

bool CServiceManager::Create(const SERVICEINFO& info)
{
    bool ok = false;
    if (info.lpServiceName &&
        info.lpDisplayName &&
        info.lpBinaryPathName)
    {
        const SC_HANDLE hService = CreateService(m_scm, info.lpServiceName, info.lpDisplayName,
            SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
            info.bAutoStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            info.lpBinaryPathName,
            0, 0, 0, 0, 0);
        if (hService)
        {
            ok = true;
            CloseServiceHandle(hService);
        }
    }
    return ok;
}

bool CServiceManager::Delete(LPCTSTR serviceName)
{
    bool ok = false;
    if (serviceName)
    {
        const SC_HANDLE hService = OpenService(m_scm, serviceName, SERVICE_ALL_ACCESS);
        if (hService)
        {
            if (DeleteService(hService))
            {
                ok = true;
            }
            CloseServiceHandle(hService);
        }
    }
    return ok;

}

bool CServiceManager::Start(LPCTSTR serviceName)
{
    bool ok = false;
    if (serviceName)
    {
        const SC_HANDLE hService = OpenService(m_scm, serviceName, SERVICE_ALL_ACCESS);
        if (hService)
        {
            if (StartService(hService, 0, NULL))
            {
                ok = true;
            }
            CloseServiceHandle(hService);
        }
    }
    return ok;
}

bool CServiceManager::Stop(LPCTSTR serviceName)
{
    bool ok = false;
    if (serviceName)
    {
        const SC_HANDLE hService = OpenService(m_scm, serviceName, SERVICE_ALL_ACCESS);
        if (hService)
        {
            SERVICE_STATUS m_SERVICE_STATUS;
            if (ControlService(hService, SERVICE_CONTROL_STOP, &m_SERVICE_STATUS))
            {
                ok = true;
            }
            CloseServiceHandle(hService);
        }
    }
    return ok;

}

bool CServiceManager::Pause(LPCTSTR serviceName)
{
    bool ok = false;
    if (serviceName)
    {
        const SC_HANDLE hService = OpenService(m_scm, serviceName, SERVICE_ALL_ACCESS);
        if (hService)
        {
            SERVICE_STATUS m_SERVICE_STATUS;

            if (ControlService(hService,
                SERVICE_CONTROL_PAUSE,
                &m_SERVICE_STATUS))
            {
                ok = true;
            }
            CloseServiceHandle(hService);
        }
    }
    return ok;

}

bool CServiceManager::Continue(LPCTSTR serviceName)
{
    bool ok = false;
    if (serviceName)
    {
        const SC_HANDLE hService = OpenService(m_scm, serviceName, SERVICE_ALL_ACCESS);
        if (hService)
        {
            SERVICE_STATUS m_SERVICE_STATUS;
            if (ControlService(hService, SERVICE_CONTROL_CONTINUE, &m_SERVICE_STATUS))
            {
                ok = true;
            }
            CloseServiceHandle(hService);
        }
    }
    return ok;
}

bool CServiceManager::SetDescription(LPCTSTR serviceName, LPCTSTR description)
{
    bool ok = false;
    if (serviceName)
    {
        const SC_HANDLE hService = OpenService(m_scm, serviceName, SERVICE_CHANGE_CONFIG);
        if (hService)
        {
            SERVICE_DESCRIPTION desc;
            desc.lpDescription = _tcsdup(description);
            if (ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &desc))
            {
                ok = true;
            }
            free(desc.lpDescription);
            CloseServiceHandle(hService);
        }
    }
    return ok;
}
