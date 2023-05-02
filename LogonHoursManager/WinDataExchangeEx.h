#pragma once

#include <atlddx.h>
#include <windowsx.h>

template <class T>
class CWinDataExchangeEx : public CWinDataExchange<T>
{
    typedef CWinDataExchange<T> Base_;

public:
    BOOL DDX_Combo_Text(UINT nID, LPTSTR lpstrText, int cbSize, BOOL bSave, BOOL bValidate = FALSE, int nLength = 0)
    {
        T* const pT = static_cast<T*>(this);
        BOOL bSuccess = TRUE;

        HWND hWndCtrl = pT->GetDlgItem(nID);
        if (bSave)
        {
            int nRetLen = ::GetWindowText(hWndCtrl, lpstrText, cbSize / sizeof(TCHAR));
            if (nRetLen < ::GetWindowTextLength(hWndCtrl))
                bSuccess = FALSE;
        }
        else
        {
            ATLASSERT(!bValidate || (lstrlen(lpstrText) <= nLength));
            // bSuccess = pT->SetDlgItemText(nID, lpstrText); // doesn't work for combo boxes and similar controls
            // SetWindowText() won't help either
            bSuccess = ComboBox_SelectString(hWndCtrl, -1, lpstrText) != CB_ERR;
        }

        if (!bSuccess)
        {
            pT->OnDataExchangeError(nID, bSave);
        }
        else if (bSave && bValidate)   // validation
        {
            ATLASSERT(nLength > 0);
            if (lstrlen(lpstrText) > nLength)
            {
                typename Base_::_XData data = { Base_::ddxDataText };
                data.textData.nLength = lstrlen(lpstrText);
                data.textData.nMaxLength = nLength;
                pT->OnDataValidateError(nID, bSave, data);
                bSuccess = FALSE;
            }
        }
        return bSuccess;
    }

    template <class Container>
    BOOL DDX_Combo_List(UINT nID, Container& list, BOOL bSave, BOOL bValidate = FALSE, int nLength = 0)
    {
        T* const pT = static_cast<T*>(this);
        BOOL bSuccess = TRUE;

        HWND hWndCtrl = pT->GetDlgItem(nID);
        if (bSave)
        {
            const int count = ComboBox_GetCount(hWndCtrl);
            list.clear();
            for (int i = 0; i < count; ++i)
            {
                TCHAR str[256] = { 0 };
                ComboBox_GetLBText(hWndCtrl, i, str);
                list.push_back(str);
            }
        }
        else
        {
            ComboBox_ResetContent(hWndCtrl);
            for (const auto& str : list)
            {
                ComboBox_AddString(hWndCtrl, str.c_str());
            }
        }

        if (!bSuccess)
        {
            pT->OnDataExchangeError(nID, bSave);
        }
        else if (bSave && bValidate)   // validation
        {
/*TODO?
            ATLASSERT(nLength > 0);
            if (strText.GetLength() > nLength)
            {
                _XData data = { ddxDataText };
                data.textData.nLength = strText.GetLength();
                data.textData.nMaxLength = nLength;
                pT->OnDataValidateError(nID, bSave, data);
                bSuccess = FALSE;
            }
*/
        }
        return bSuccess;
    }
};

#define DDX_CHECK_ARRAY(nFirstID, nLastID, arr) \
        static_assert(_countof(arr) == nLastID - nFirstID, "The elements range should match the array's size"); \
        if (nFirstID <= nCtlID && nCtlID <= nLastID) \
            DDX_Check(nCtlID, arr[nCtlID - nFirstID], bSaveAndValidate); \
        if (nCtlID == (UINT)-1) \
            for (UINT id = nFirstID; id <= nLastID; ++id) \
                DDX_Check(id, arr[id - nFirstID], bSaveAndValidate);

#define DDX_COMBO_LIST_SOURCE(nID, var) \
		if((nCtlID == (UINT)-1) || (nCtlID == nID)) \
		{ \
            if (!bSaveAndValidate) \
			if(!DDX_Combo_List(nID, var, bSaveAndValidate)) \
				return FALSE; \
		}

#define DDX_COMBO_TEXT(nID, var) \
		if((nCtlID == (UINT)-1) || (nCtlID == nID)) \
		{ \
			if(!DDX_Combo_Text(nID, var, sizeof(var), bSaveAndValidate)) \
				return FALSE; \
		}
