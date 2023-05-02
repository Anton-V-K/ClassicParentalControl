#include "stdafx.h"
#include "MainDialog.h"

#include "MainModel.h"

CMainDialog::CMainDialog(MainModel& model)
    :m_model(model)
{
}

CMainDialog::~CMainDialog()
{
}

LRESULT CMainDialog::OnClose(UINT, int, HWND)
{
    EndDialog(0);
    return 0;
}

void CMainDialog::OnDataExchangeError(UINT, BOOL)
{
}

LRESULT CMainDialog::OnInitDialog(HWND, LPARAM)
{
    HICON hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_APP));
    SetIcon(hIcon);

    m_ctrlToolTip.Create(m_hWnd); // , rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, WS_EX_TOPMOST);
    ATLASSERT(m_ctrlToolTip.IsWindow());

    const auto& wndGroup = GetDlgItem(IDC_GROUP_HOURS);
    RECT rcGroup;
    wndGroup.GetWindowRect(&rcGroup);
    // MapDialogRect(&rcGroup);

    const auto units = GetDialogBaseUnits();
    const auto xunits = LOWORD(units);
    const auto yunits = HIWORD(units);

    UINT xshift = 30; // width of left-most column with day names
    const UINT yshift = 4;
    UINT width = (rcGroup.right - rcGroup.left - xshift - 10) / 24; // width of each column with a checkbox
    UINT height = (rcGroup.bottom - rcGroup.top - 12 - yshift) / 8; // height of each row (applies to header as well)
    // starting ID for checkboxes: Sun 0:00-1:00, Sun 1:00-2:00, ..., Sun 23:00-24:00, Mon 0:00-1:00, etc.
    UINT id = IDC_HOURS_FIRST;
    // Columns labels
    for (WORD hour = 0; hour < 24; ++hour)
    {
        UINT x = rcGroup.left + xshift + hour * width;
        UINT y = rcGroup.top + yshift - height * 3 / 4;
        //if (hour % 2 == 0)
        {
            wchar_t header[12];
            swprintf_s(header, L"%02d", hour);
            const auto hwndLabel = CreateWindow(_T("Static"), header, WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_LEFT,
                x, y, width, height, m_hWnd, HMENU(IDC_STATIC), NULL, NULL);
            SendMessage(hwndLabel, WM_SETFONT, WPARAM(GetFont()), 0);
        }
    }
    static const wchar_t* namesShort[]
        = { L"SUN",     L"MON",     L"TUE",     L"WED",         L"THU",         L"FRI",     L"SAT" };
    static const wchar_t* namesLong[]
        = { L"Sunday",  L"Monday",  L"Tuesday", L"Wednesday",   L"Thursday",    L"Friday",  L"Saturday" };
    // Row labels and check boxes
    for (WORD day = 0; day < 7; ++day)
    {
        const UINT x = rcGroup.left;
        const UINT y = rcGroup.top + yshift + height * day + height / 5;

        const auto hwndLabel = CreateWindow(_T("Static"), namesShort[day], WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_LEFT,
            x, y, xshift, height, m_hWnd, HMENU(IDC_STATIC), NULL, NULL);
        SendMessage(hwndLabel, WM_SETFONT, WPARAM(GetFont()), 0);

        // TODO? CToolInfo ti(TTF_SUBCLASS, hwndLabel, 0, NULL, LPTSTR(namesLong[day]));
        // TODO? ATLASSERT(m_ctrlToolTip.AddTool(ti));

        for (WORD hour = 0; hour < 24; ++hour)
        {
            const UINT x = rcGroup.left + xshift + hour * width;
            const auto hwndCheckBox = CreateWindow(_T("Button"), NULL, WS_CHILD | WS_VISIBLE | WS_EX_NOPARENTNOTIFY | BS_CHECKBOX | BS_PUSHLIKE,
                x, y, width, height, m_hWnd, HMENU(id), NULL, NULL);
#if 1
            wchar_t tttext[42];
            swprintf_s(tttext, L"%s %02d:00-%02d:00", namesLong[day], hour, hour + 1);
            CToolInfo ti(TTF_SUBCLASS, hwndCheckBox, 0, NULL, tttext);
            ATLASSERT(m_ctrlToolTip.AddTool(ti));
            // m_ctrlToolTip.AddTool(hwndCheckBox, tttext); // , & rcTool, TTF_IDISHWND);
#else
            RECT rcTool;
            rcTool.left = x;
            rcTool.top = y;
            rcTool.right = x + width;
            rcTool.bottom = y + height;
            TOOLINFOW toolinfo = { 0 };
            toolinfo.cbSize = sizeof(toolinfo); // TTTOOLINFO_V1_SIZE;
            toolinfo.hwnd = m_hWnd;
            toolinfo.rect = rcTool;
            // toolinfo.uFlags = TTF_IDISHWND;
            toolinfo.uId = id;
            toolinfo.lpszText = LPTSTR(L"Это поле для ввода текста");
            ::SendMessageW(m_ctrlToolTip, TTM_ADDTOOL, 0, LPARAM(&toolinfo));
#endif

            ++id;
        }
    }

    m_ctrlToolTip.Activate(TRUE);

    FetchData();

    DoDataExchange(DDX_LOAD);

    UIAddChildWindowContainer(m_hWnd);
    UpdateUI(true);

    return 0;
}
/*DEL?
LRESULT CMainDialog::OnHourChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (wNotifyCode == BN_CLICKED)
    {
        // UpdateData();
        // DisplayGUID();
        if (IDC_HOURS_FIRST <= wID && wID <= IDC_HOURS_LAST)
        {
            const auto span = wID - IDC_HOURS_FIRST;
            const auto day = span / 24;
            const auto hour = span % 24;
            const auto check = Button_GetCheck(hWndCtl);
            m_model.getHours().Allow(EDayOfWeek(day), hour, check == BST_CHECKED);
            // TODO m_model.getHours().ApplyTo(m_model.getUser());
        }
    }
    return 0;
}
*/
LRESULT CMainDialog::OnUserChanged(UINT, int, HWND)
{
    UpdateUI();
    m_model.selectUser(m_userName);
    FetchData();
    DoDataExchange(DDX_LOAD);
    return 0;
}

void CMainDialog::FetchData()
{
    wcscpy_s(m_userName, m_model.getUser());
    m_users = m_model.getUsers();
    m_model.getHours().Get(m_week, false);
}

void CMainDialog::UpdateUI(bool saved)
{
    const auto isValid = saved || DoDataExchange(DDX_SAVE);

    for (UINT id = IDC_HOURS_FIRST; id <= IDC_HOURS_LAST; ++id)
        UIEnable(id, isValid && m_userName[0] != 0);

    if (isValid)
    {

/*DEL
        UIEnable(IDC_EXP, TRUE);
        UIEnable(IDC_LN, m_number > 0);
        UIEnable(IDC_SQRT, m_number >= 0);
        UIEnable(IDC_FACT, m_number <= 100 && m_number > 0 && ((double)(unsigned long)m_number == m_number));
*/
    }
    else
    {
/*DEL
        UIEnable(IDC_EXP, FALSE);
        UIEnable(IDC_LN, FALSE);
        UIEnable(IDC_SQRT, FALSE);
        UIEnable(IDC_FACT, FALSE);
*/
    }

    UIUpdateChildWindows();
}

