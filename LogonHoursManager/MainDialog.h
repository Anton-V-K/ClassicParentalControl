#pragma once
#include <atlcrack.h>           // MSG_WM_INITDIALOG, etc.
#include <atlctrls.h>           // CToolTipCtrl
#include <atlframe.h>           // CUpdateUI
#include <atlwin.h>

#include <list>

#include <Common/WeekHours.h>

#include "WinDataExchangeEx.h"
#include "UIEx.h"

#include "resource.h"

class MainModel;

class CMainDialog
    : public CDialogImpl<CMainDialog>
    , public CUpdateUI<CMainDialog>
    , public CWinDataExchangeEx<CMainDialog>
{
    typedef CMainDialog Self;

private:
    MainModel& m_model;

    CToolTipCtrl m_ctrlToolTip;

    CComboBox   m_comboUserName;
    TCHAR       m_userName[256] = _T("");
    std::vector<std::wstring> m_users;

    /// <summary>
    /// Temporary data while the user is editing it
    /// </summary>
    WeekHours   m_week;
    bool        m_week_modified{ false };

public:
    enum EID
    {
        IDD = IDD_MAIN,

        IDC_HOURS_FIRST = IDC_GROUP_HOURS + 1,
        IDC_HOURS_LAST  = IDC_HOURS_FIRST + 7 * 24 - 1,
        IDC_HOURS_COUNT = IDC_HOURS_LAST - IDC_HOURS_FIRST,
    };

    BEGIN_MSG_MAP(Self)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_RANGE_HANDLER(IDC_HOURS_FIRST, IDC_HOURS_LAST, OnHourChanged)
        COMMAND_HANDLER_EX(IDC_COMBO_USER, CBN_SELENDOK, OnUserChanged)
        COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnClose)
    END_MSG_MAP()

    BEGIN_DDX_MAP(Self)
        DDX_CONTROL_HANDLE(IDC_COMBO_USER, m_comboUserName)
        DDX_COMBO_LIST_SOURCE(IDC_COMBO_USER, m_users)
        DDX_COMBO_TEXT(IDC_COMBO_USER, m_userName)
        DDX_CHECK_ARRAY(IDC_HOURS_FIRST, IDC_HOURS_LAST, m_week.array)
    END_DDX_MAP()

    BEGIN_UPDATE_UI_MAP(Self)
        UPDATE_ELEMENT(IDC_COMBO_USER, UPDUI_CHILDWINDOW)
        UPDATE_ELEMENT(IDOK,            UPDUI_CHILDWINDOW)
        UPDATE_CHILDWINDOWS(IDC_HOURS_FIRST, IDC_HOURS_LAST, 168) // 7 * 24
        // TODO? UPDATE_ELEMENT(IDC_HOURS_FIRST, UPDUI_CHILDWINDOW)
        // TODO? UPDATE_ELEMENT(IDC_HOURS_LAST,  UPDUI_CHILDWINDOW)
    END_UPDATE_UI_MAP()

    CMainDialog(MainModel& model);
    ~CMainDialog();

public:
    LRESULT OnClose(UINT, int, HWND);

    void OnDataExchangeError(UINT, BOOL);

    LRESULT OnInitDialog(HWND, LPARAM);

private:
    LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    LRESULT OnHourChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnUserChanged(UINT, int, HWND);

private:
    void ApplyChanges(bool prev_user);

    void FetchData();

    void UpdateUI(bool saved = false);
};
