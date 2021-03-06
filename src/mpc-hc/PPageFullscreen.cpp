/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "MainFrm.h"
#include "mplayerc.h"
#include "PPageFullscreen.h"
#include "WinAPIUtils.h"

#include "Monitors.h"
#include "MultiMonitor.h"

#define SANE_TIMEOUT_FOR_SHOW_CONTROLS_ON_MOUSE_MOVE 200

// CPPagePlayer dialog

IMPLEMENT_DYNAMIC(CPPageFullscreen, CPPageBase)
CPPageFullscreen::CPPageFullscreen()
    : CPPageBase(CPPageFullscreen::IDD, CPPageFullscreen::IDD)
    , m_launchfullscreen(FALSE)
    , m_fSetFullscreenRes(FALSE)
    , m_fSetDefault(FALSE)
    , m_bHideFullscreenControls(FALSE)
    , m_uHideFullscreenControlsDelay(0)
    , m_bHideFullscreenDockedPanels(FALSE)
    , m_fExitFullScreenAtTheEnd(FALSE)
    , m_fRestoreResAfterExit(TRUE)
    , m_uAutoChangeFullscrResDelay(0)
    , m_iMonitorType(0)
    , m_list(0)
    , m_iSel(-1)
{
    memset(m_iSeldm, -1, sizeof(m_iSeldm));
}

CPPageFullscreen::~CPPageFullscreen()
{
}

void CPPageFullscreen::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_CHECK1, m_launchfullscreen);
    DDX_Check(pDX, IDC_CHECK2, m_fSetFullscreenRes);
    DDX_Check(pDX, IDC_CHECK3, m_fSetDefault);
    DDX_CBIndex(pDX, IDC_COMBO1, m_iMonitorType);
    DDX_Control(pDX, IDC_COMBO1, m_iMonitorTypeCtrl);
    DDX_Control(pDX, IDC_LIST1, m_list);
    DDX_Check(pDX, IDC_CHECK4, m_bHideFullscreenControls);
    DDX_Control(pDX, IDC_COMBO2, m_hidePolicy);
    DDX_Text(pDX, IDC_EDIT1, m_uHideFullscreenControlsDelay);
    DDX_Check(pDX, IDC_CHECK6, m_bHideFullscreenDockedPanels);
    DDX_Check(pDX, IDC_CHECK5, m_fExitFullScreenAtTheEnd);
    DDX_Check(pDX, IDC_RESTORERESCHECK, m_fRestoreResAfterExit);
    DDX_Text(pDX, IDC_EDIT2, m_uAutoChangeFullscrResDelay);
    DDX_Control(pDX, IDC_SPIN1, m_delaySpinner);
}

BEGIN_MESSAGE_MAP(CPPageFullscreen, CPPageBase)
    ON_CBN_SELCHANGE(IDC_COMBO1, OnUpdateFullScrCombo)
    ON_NOTIFY(LVN_DOLABELEDIT, IDC_LIST1, OnDolabeleditList)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnLvnItemchangedList1)
    ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST1, OnBeginlabeleditList)
    ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST1, OnEndlabeleditList)
    ON_NOTIFY(NM_CLICK, IDC_LIST1, OnNMClickList1)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST1, OnCustomdrawList)
    ON_CLBN_CHKCHANGE(IDC_LIST1, OnCheckChangeList)
    ON_UPDATE_COMMAND_UI(IDC_LIST1, OnUpdateAutoChangeFullscrRes)
    ON_UPDATE_COMMAND_UI(IDC_CHECK3, OnUpdateAutoChangeFullscrRes)
    ON_CBN_SELCHANGE(IDC_COMBO2, OnHideControlsPolicyChange)
    ON_UPDATE_COMMAND_UI(IDC_COMBO2, OnUpdateHideControls)
    ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateHideDelay)
    ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateHideDelay)
    ON_UPDATE_COMMAND_UI(IDC_CHECK6, OnUpdateHideControls)
    ON_UPDATE_COMMAND_UI(IDC_RESTORERESCHECK, OnUpdateAutoChangeFullscrRes)
    ON_BN_CLICKED(IDC_BUTTON2, OnRemove)
    ON_UPDATE_COMMAND_UI(IDC_BUTTON2, OnUpdateRemove)
    ON_BN_CLICKED(IDC_BUTTON1, OnAdd)
    ON_UPDATE_COMMAND_UI(IDC_BUTTON1, OnUpdateAutoChangeFullscrRes)
    ON_BN_CLICKED(IDC_BUTTON3, OnMoveUp)
    ON_BN_CLICKED(IDC_BUTTON4, OnMoveDown)
    ON_UPDATE_COMMAND_UI(IDC_BUTTON3, OnUpdateUp)
    ON_UPDATE_COMMAND_UI(IDC_BUTTON4, OnUpdateDown)
    ON_UPDATE_COMMAND_UI(IDC_EDIT2, OnUpdateAutoChangeFullscrRes)
    ON_UPDATE_COMMAND_UI(IDC_SPIN1, OnUpdateAutoChangeFullscrRes)
    ON_UPDATE_COMMAND_UI(IDC_STATIC2, OnUpdateAutoChangeFullscrRes)
END_MESSAGE_MAP()

// CPPagePlayer message handlers

BOOL CPPageFullscreen::OnInitDialog()
{
    __super::OnInitDialog();

    SetHandCursor(m_hWnd, IDC_COMBO1);

    const CAppSettings& s = AfxGetAppSettings();

    m_launchfullscreen = s.fLaunchfullscreen;
    m_AutoChangeFullscrRes = s.AutoChangeFullscrRes;
    m_uAutoChangeFullscrResDelay = s.uAutoChangeFullscrResDelay;
    m_fSetDefault = s.AutoChangeFullscrRes.bApplyDefault;
    m_f_hmonitor = s.strFullScreenMonitor;
    m_fExitFullScreenAtTheEnd = s.fExitFullScreenAtTheEnd;
    m_fRestoreResAfterExit = s.fRestoreResAfterExit;

    CString str;
    m_iMonitorType = 0;
    CMonitor monitor;
    CMonitors monitors;

    CString strCurMon;

    monitor = monitors.GetNearestMonitor(AfxGetApp()->m_pMainWnd);
    monitor.GetName(strCurMon);

    m_iMonitorTypeCtrl.AddString(ResStr(IDS_FULLSCREENMONITOR_CURRENT));
    m_MonitorDisplayNames.Add(_T("Current"));
    if (m_f_hmonitor == _T("Current")) {
        m_iMonitorType = m_iMonitorTypeCtrl.GetCount() - 1;
    }

    for (int i = 0; i < monitors.GetCount(); i++) {
        monitor = monitors.GetMonitor(i);
        monitor.GetName(str);

        if (monitor.IsMonitor()) {
            DISPLAY_DEVICE displayDevice;
            ZeroMemory(&displayDevice, sizeof(displayDevice));
            displayDevice.cb = sizeof(displayDevice);
            VERIFY(EnumDisplayDevices(str, 0, &displayDevice, 0) == (i != monitors.GetCount() - 1));
            if (str == strCurMon) {
                m_iMonitorTypeCtrl.AddString(str + _T(" - [") + ResStr(IDS_FULLSCREENMONITOR_CURRENT) + _T("] - ") + displayDevice.DeviceString);
            } else {
                m_iMonitorTypeCtrl.AddString(str + _T(" - ") + displayDevice.DeviceString);
            }
            m_MonitorDisplayNames.Add(str);

            if (m_f_hmonitor == str && m_iMonitorType == 0) {
                m_iMonitorType = m_iMonitorTypeCtrl.GetCount() - 1;
            }
        }
    }

    if (m_iMonitorTypeCtrl.GetCount() > 2) {
        GetDlgItem(IDC_COMBO1)->EnableWindow(TRUE);
    } else {
        m_iMonitorType = 0;
        GetDlgItem(IDC_COMBO1)->EnableWindow(FALSE);
    }

    m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER
                            | LVS_EX_GRIDLINES | LVS_EX_BORDERSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_CHECKBOXES | LVS_EX_FLATSB);
    m_list.InsertColumn(COL_Z, ResStr(IDS_PPAGE_FS_CLN_ON_OFF), LVCFMT_LEFT, 60);
    m_list.InsertColumn(COL_VFR_F, ResStr(IDS_PPAGE_FS_CLN_FROM_FPS), LVCFMT_RIGHT, 60);
    m_list.InsertColumn(COL_VFR_T, ResStr(IDS_PPAGE_FS_CLN_TO_FPS), LVCFMT_RIGHT, 60);
    m_list.InsertColumn(COL_SRR, ResStr(IDS_PPAGE_FS_CLN_DISPLAY_MODE), LVCFMT_LEFT, 135);


    m_bHideFullscreenControls = s.bHideFullscreenControls;
    m_uHideFullscreenControlsDelay = s.uHideFullscreenControlsDelay;
    m_bHideFullscreenDockedPanels = s.bHideFullscreenDockedPanels;

    auto addHidePolicy = [&](LPCTSTR lpszName, CAppSettings::HideFullscreenControlsPolicy ePolicy) {
        int n = m_hidePolicy.InsertString(-1, lpszName);
        if (n >= 0) {
            VERIFY(m_hidePolicy.SetItemData(n, static_cast<DWORD_PTR>(ePolicy)) != CB_ERR);
            if (ePolicy == s.eHideFullscreenControlsPolicy) {
                VERIFY(m_hidePolicy.SetCurSel(n) == n);
            }
        } else {
            ASSERT(FALSE);
        }
    };
    auto loadString = [&](UINT nID) {
        CString ret;
        VERIFY(ret.LoadString(nID));
        return ret;
    };
    addHidePolicy(loadString(IDS_PPAGEFULLSCREEN_SHOWNEVER), CAppSettings::HideFullscreenControlsPolicy::SHOW_NEVER);
    addHidePolicy(loadString(IDS_PPAGEFULLSCREEN_SHOWMOVED), CAppSettings::HideFullscreenControlsPolicy::SHOW_WHEN_CURSOR_MOVED);
    addHidePolicy(loadString(IDS_PPAGEFULLSCREEN_SHOHHOVERED), CAppSettings::HideFullscreenControlsPolicy::SHOW_WHEN_HOVERED);

    m_delaySpinner.SetRange(0, 9);

    ModesUpdate();
    UpdateData(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CPPageFullscreen::OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    *pResult = CDRF_DODEFAULT;

    if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage) {
        *pResult = CDRF_NOTIFYITEMDRAW;
    } else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage) {
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
    } else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage) {
        COLORREF crText;
        if (m_list.GetCheck((int)pLVCD->nmcd.dwItemSpec) == 0) {
            crText = RGB(128, 128, 128);
        } else {
            crText = RGB(0, 0, 0);
        }
        pLVCD->clrText = crText;
        *pResult = CDRF_DODEFAULT;
    }
}

BOOL CPPageFullscreen::OnApply()
{
    UpdateData();

    CAppSettings& s = AfxGetAppSettings();
    m_AutoChangeFullscrRes.bEnabled = !!m_fSetFullscreenRes;

    for (int i = 0; i < MAX_FPS_COUNT; i++) {
        int n = m_iSeldm[i];
        if (n >= 0 && (size_t)n < m_dms.GetCount() && i < m_list.GetItemCount()) {
            m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes = m_dms[n];
            m_AutoChangeFullscrRes.dmFullscreenRes[i].fChecked = !!m_list.GetCheck(i);
            m_AutoChangeFullscrRes.dmFullscreenRes[i].fIsData = true;

            if (i == 0) {
                m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_from = 0;
                m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_to = 0;
            } else {
                m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_from = wcstod(m_list.GetItemText(i, COL_VFR_F), nullptr);
                m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_to = wcstod(m_list.GetItemText(i, COL_VFR_T), nullptr);
            }
        } else {
            m_AutoChangeFullscrRes.dmFullscreenRes[i].fIsData = false;
            m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_from = 0;
            m_AutoChangeFullscrRes.dmFullscreenRes[i].vfr_to = 0;
            m_AutoChangeFullscrRes.dmFullscreenRes[i].fChecked = 0;
            m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.bpp = 0;
            m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.dmDisplayFlags = 0;
            m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.freq = 0;
            m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.fValid = 0;
            m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes.size = 0;
        }
    }

    m_AutoChangeFullscrRes.bApplyDefault = !!m_fSetDefault;
    s.AutoChangeFullscrRes = m_AutoChangeFullscrRes;
    s.uAutoChangeFullscrResDelay = m_uAutoChangeFullscrResDelay;

    s.fLaunchfullscreen = !!m_launchfullscreen;
    s.strFullScreenMonitor = m_f_hmonitor;
    s.fExitFullScreenAtTheEnd = !!m_fExitFullScreenAtTheEnd;
    s.fRestoreResAfterExit = !!m_fRestoreResAfterExit;

    s.bHideFullscreenControls = !!m_bHideFullscreenControls;
    {
        int n = m_hidePolicy.GetCurSel();
        if (n != CB_ERR) {
            s.eHideFullscreenControlsPolicy =
                static_cast<CAppSettings::HideFullscreenControlsPolicy>(m_hidePolicy.GetItemData(n));
        } else {
            ASSERT(FALSE);
        }
    }
    if (s.eHideFullscreenControlsPolicy == CAppSettings::HideFullscreenControlsPolicy::SHOW_WHEN_CURSOR_MOVED &&
            m_uHideFullscreenControlsDelay > 0 && m_uHideFullscreenControlsDelay < SANE_TIMEOUT_FOR_SHOW_CONTROLS_ON_MOUSE_MOVE) {
        m_uHideFullscreenControlsDelay = SANE_TIMEOUT_FOR_SHOW_CONTROLS_ON_MOUSE_MOVE;
        UpdateData(FALSE);
    }
    s.uHideFullscreenControlsDelay = m_uHideFullscreenControlsDelay;
    s.bHideFullscreenDockedPanels = !!m_bHideFullscreenDockedPanels;

    // There is no main frame when the option dialog is displayed stand-alone
    if (CMainFrame* pMainFrame = AfxGetMainFrame()) {
        pMainFrame->UpdateControlState(CMainFrame::UPDATE_CONTROLS_VISIBILITY);
    }

    return __super::OnApply();
}

void CPPageFullscreen::OnNMClickList1(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;
    if (lpnmlv->iItem >= 0 && lpnmlv->iSubItem == COL_SRR) {
    }
    *pResult = 0;
}

void CPPageFullscreen::OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    if (pNMLV->iItem >= 0 && pNMLV->iSubItem == COL_SRR) {
    }
    *pResult = 0;
}

void CPPageFullscreen::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
    LV_ITEM* pItem = &pDispInfo->item;
    *pResult = FALSE;
    if (pItem->iItem < 0) {
        return;
    }
    *pResult = TRUE;
}

void CPPageFullscreen::OnDolabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
    LV_ITEM* pItem = &pDispInfo->item;
    *pResult = FALSE;
    if (pItem->iItem < 0) {
        return;
    }
    CAtlList<CString> sl1;
    CMonitors monitors;

    switch (pItem->iSubItem) {
        case COL_SRR:
            sl1.RemoveAll();
            for (int i = 0; (size_t)i < sl.GetCount(); i++) {
                sl1.AddTail(sl[i]);
                if (m_list.GetItemText(pItem->iItem, COL_SRR) == sl[i]) {
                    m_iSel = i;
                }
            }
            m_list.ShowInPlaceComboBox(pItem->iItem, pItem->iSubItem, sl1, m_iSel);
            break;
        case COL_VFR_F:
        case COL_VFR_T:
            if (pItem->iItem != 0) {
                m_list.ShowInPlaceFloatEdit(pItem->iItem, pItem->iSubItem);
                //CEdit* pFloatEdit = (CEdit*)m_list.GetDlgItem(IDC_EDIT1);
            }
            break;
    }
    m_list.RedrawWindow();
    *pResult = TRUE;
}

void CPPageFullscreen::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
    LV_ITEM* pItem = &pDispInfo->item;
    *pResult = FALSE;
    if (!m_list.m_fInPlaceDirty) {
        return;
    }
    if (pItem->iItem < 0) {
        return;
    }
    switch (pItem->iSubItem) {
        case COL_SRR:
            if (pItem->lParam >= 0) {
                m_iSeldm[pItem->iItem] = m_iSel = (int)pItem->lParam;
                m_list.SetItemText(pItem->iItem, pItem->iSubItem, pItem->pszText);
            }
            break;
        case COL_VFR_F:
        case COL_VFR_T:
            if (pItem->pszText) {
                CString str = pItem->pszText;
                int dotpos = str.Find('.');
                if (dotpos >= 0 && str.GetLength() - dotpos > 4) {
                    str.Truncate(dotpos + 4);
                }
                double f = min(max(_tstof(str), 1.0), 125.999);
                str.Format(_T("%.3f"), f);
                m_list.SetItemText(pItem->iItem, pItem->iSubItem, str);
            }
            break;
    }

    *pResult = TRUE;

    if (*pResult) {
        SetModified();
    }
}

void CPPageFullscreen::OnCheckChangeList()
{
    SetModified();
}

void CPPageFullscreen::OnUpdateFullScrCombo()
{
    CMonitors monitors;
    m_f_hmonitor = m_MonitorDisplayNames[m_iMonitorTypeCtrl.GetCurSel()];
    if (AfxGetAppSettings().strFullScreenMonitor != m_f_hmonitor) {
        m_AutoChangeFullscrRes.bEnabled = false;
    }

    ModesUpdate();
    SetModified();
}

void CPPageFullscreen::OnUpdateHideControls(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK4));
}

void CPPageFullscreen::OnHideControlsPolicyChange()
{
    UpdateData(TRUE);
    if (m_hidePolicy.GetItemData(m_hidePolicy.GetCurSel()) ==
            static_cast<int>(CAppSettings::HideFullscreenControlsPolicy::SHOW_WHEN_CURSOR_MOVED) &&
            m_uHideFullscreenControlsDelay > 0 && m_uHideFullscreenControlsDelay < SANE_TIMEOUT_FOR_SHOW_CONTROLS_ON_MOUSE_MOVE) {
        m_uHideFullscreenControlsDelay = SANE_TIMEOUT_FOR_SHOW_CONTROLS_ON_MOUSE_MOVE;
        UpdateData(FALSE);
    }
}

void CPPageFullscreen::OnUpdateHideDelay(CCmdUI* pCmdUI)
{
    int n = m_hidePolicy.GetCurSel();
    pCmdUI->Enable(IsDlgButtonChecked(IDC_CHECK4) && n != CB_ERR && n != 0);
}

void CPPageFullscreen::ModesUpdate()
{
    CMonitors monitors;

    m_fSetFullscreenRes = m_AutoChangeFullscrRes.bEnabled;
    CString sl2[MAX_FPS_COUNT];
    dispmode dm, dmtoset[MAX_FPS_COUNT];

    int i0 = 0;

    CString strModes;
    CString strCur;
    if (!GetCurDispModeString(strCur)) {
        return;
    }

    int iNoData = 0;
    for (int i = 0; i < MAX_FPS_COUNT; i++) {
        dmtoset[i] = m_AutoChangeFullscrRes.dmFullscreenRes[i].dmFSRes;
        if (m_AutoChangeFullscrRes.dmFullscreenRes[i].fIsData == true) {
            iNoData++;
        }
    }

    if (!m_AutoChangeFullscrRes.bEnabled
            || m_AutoChangeFullscrRes.dmFullscreenRes[0].dmFSRes.freq < 0
            || m_AutoChangeFullscrRes.dmFullscreenRes[0].fIsData == false) {
        if (!CMainFrame::GetCurDispMode(m_f_hmonitor, dmtoset[0])) {
            return;
        }

        for (int i = 1; i < MAX_FPS_COUNT; i++) {
            dmtoset[i] = dmtoset[0];
        }
    }
    m_list.DeleteAllItems();
    m_dms.RemoveAll();
    sl.RemoveAll();
    for (int i = 1; i < MAX_FPS_COUNT; i++) {
        sl2[i] = _T("");
    }
    memset(m_iSeldm, -1, sizeof(m_iSeldm));
    m_iSel = -1;

    for (int i = 0, m = 0;  ; i++) {
        if (!CMainFrame::GetDispMode(m_f_hmonitor, i, dm)) {
            break;
        }
        if (dm.bpp != 32 || dm.size.cx < 640) {
            continue; // skip low resolution and non 32bpp mode
        }

        int j = 0;
        while (j < m) {
            if (dm.bpp                == m_dms[j].bpp &&
                    dm.dmDisplayFlags == m_dms[j].dmDisplayFlags &&
                    dm.freq           == m_dms[j].freq &&
                    dm.fValid         == m_dms[j].fValid &&
                    dm.size           == m_dms[j].size) {
                break;
            }
            j++;
        }
        if (j < m) {
            continue;
        }
        m_dms.Add(dm);
        m++;
    }

    // sort display modes
    for (unsigned int j, i = 1; i < m_dms.GetCount(); i++) {
        dm = m_dms[i];
        j = i - 1;
        while (j != -1 && m_dms[j].size.cx >= dm.size.cx &&
                m_dms[j].size.cy >= dm.size.cy &&
                m_dms[j].freq > dm.freq) {
            m_dms[j + 1] = m_dms[j];
            j--;
        }
        m_dms[j + 1] = dm;
    }

    for (int i = 0; (size_t) i < m_dms.GetCount(); i++) {
        strModes.Format(_T("[ %d ]  @ %dx%d "), m_dms[i].freq, m_dms[i].size.cx, m_dms[i].size.cy);
        if (m_dms[i].dmDisplayFlags == DM_INTERLACED) {
            strModes += _T("i");
        } else {
            strModes += _T("p");
        }

        sl.Add(strModes);
        for (int n = 0; n < MAX_FPS_COUNT; n++) {
            if (m_iSeldm[n] < 0
                    && dmtoset[n].fValid
                    && m_dms[i].size            == dmtoset[n].size
                    && m_dms[i].bpp             == dmtoset[n].bpp
                    && m_dms[i].freq            == dmtoset[n].freq
                    && m_dms[i].dmDisplayFlags  == dmtoset[n].dmDisplayFlags) {
                m_iSeldm[n] = i;
                sl2[n] = sl[i];
                if (strCur == strModes) {
                    i0 = i;
                }
            }
        }
    }

    for (int n = 0; n < MAX_FPS_COUNT; n++) {
        if (m_AutoChangeFullscrRes.dmFullscreenRes[n].fIsData == true) {
            m_list.InsertItem(n, _T(""));
            CString ss = sl2[n];
            m_list.SetItemText(n, COL_SRR, ss);
            m_list.SetCheck(n, m_AutoChangeFullscrRes.dmFullscreenRes[n].fChecked);
            if (n == 0) {
                m_list.SetItemText(n, COL_Z, ResStr(IDS_PPAGE_FS_DEFAULT));
                m_list.SetItemText(n, COL_VFR_F, ResStr(IDS_PPAGE_FS_OTHER));
                m_list.SetItemText(n, COL_VFR_T, ResStr(IDS_PPAGE_FS_OTHER));
            } else {
                n > 9 ? ss.Format(_T("%d"), n) : ss.Format(_T("0%d"), n);
                m_list.SetItemText(n, COL_Z, ss);

                ss.Format(_T("%.3f"), m_AutoChangeFullscrRes.dmFullscreenRes[n].vfr_from);
                m_list.SetItemText(n, COL_VFR_F, ss);

                ss.Format(_T("%.3f"), m_AutoChangeFullscrRes.dmFullscreenRes[n].vfr_to);
                m_list.SetItemText(n, COL_VFR_T, ss);
            }
        }
    }
    if (m_list.GetItemCount() < 1 || iNoData == 0) {
        strModes.Format(_T("[ %d ]  @ %dx%d "), dmtoset[0].freq, dmtoset[0].size.cx, dmtoset[0].size.cy);
        (dmtoset[0].dmDisplayFlags == DM_INTERLACED) ? strModes += _T("i") : strModes += _T("p");

        int idx = 0;
        m_list.InsertItem(idx, ResStr(IDS_PPAGE_FS_DEFAULT));
        m_list.SetItemText(idx, COL_VFR_F, ResStr(IDS_PPAGE_FS_OTHER));
        m_list.SetItemText(idx, COL_VFR_T, ResStr(IDS_PPAGE_FS_OTHER));
        m_list.SetItemText(idx, COL_SRR, strModes);
        m_iSeldm[idx] = i0;
        m_list.SetCheck(idx, 1);
        idx++;
        m_list.InsertItem(idx, _T("01"));
        m_list.SetItemText(idx, COL_VFR_F, _T("23.500"));
        m_list.SetItemText(idx, COL_VFR_T, _T("23.981"));
        m_list.SetItemText(idx, COL_SRR, strModes);
        m_iSeldm[idx] = i0;
        m_list.SetCheck(idx, 1);
        idx++;
        m_list.InsertItem(idx, _T("02"));
        m_list.SetItemText(idx, COL_VFR_F, _T("23.982"));
        m_list.SetItemText(idx, COL_VFR_T, _T("24.499"));
        m_list.SetItemText(idx, COL_SRR, strModes);
        m_iSeldm[idx] = i0;
        m_list.SetCheck(idx, 1);
        idx++;
        m_list.InsertItem(idx, _T("03"));
        m_list.SetItemText(idx, COL_VFR_F, _T("24.500"));
        m_list.SetItemText(idx, COL_VFR_T, _T("25.499"));
        m_list.SetItemText(idx, COL_SRR, strModes);
        m_iSeldm[idx] = i0;
        m_list.SetCheck(idx, 1);
        idx++;
        m_list.InsertItem(idx, _T("04"));
        m_list.SetItemText(idx, COL_VFR_F, _T("29.500"));
        m_list.SetItemText(idx, COL_VFR_T, _T("29.981"));
        m_list.SetItemText(idx, COL_SRR, strModes);
        m_iSeldm[idx] = i0;
        m_list.SetCheck(idx, 1);
        idx++;
        m_list.InsertItem(idx, _T("05"));
        m_list.SetItemText(idx, COL_VFR_F, _T("29.982"));
        m_list.SetItemText(idx, COL_VFR_T, _T("30.499"));
        m_list.SetItemText(idx, COL_SRR, strModes);
        m_iSeldm[idx] = i0;
        m_list.SetCheck(idx, 1);
        idx++;
        m_list.InsertItem(idx, _T("06"));
        m_list.SetItemText(idx, COL_VFR_F, _T("49.500"));
        m_list.SetItemText(idx, COL_VFR_T, _T("50.499"));
        m_list.SetItemText(idx, COL_SRR, strModes);
        m_iSeldm[idx] = i0;
        m_list.SetCheck(idx, 1);
        idx++;
        m_list.InsertItem(idx, _T("07"));
        m_list.SetItemText(idx, COL_VFR_F, _T("59.500"));
        m_list.SetItemText(idx, COL_VFR_T, _T("59.945"));
        m_list.SetItemText(idx, COL_SRR, strModes);
        m_iSeldm[idx] = i0;
        m_list.SetCheck(idx, 1);
        idx++;
        m_list.InsertItem(idx, _T("08"));
        m_list.SetItemText(idx, COL_VFR_F, _T("59.946"));
        m_list.SetItemText(idx, COL_VFR_T, _T("60.499"));
        m_list.SetItemText(idx, COL_SRR, strModes);
        m_iSeldm[idx] = i0;
        m_list.SetCheck(idx, 1);
    }
    //ReindexListSubItem();
}

void CPPageFullscreen::OnRemove()
{
    if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
        int nItem = m_list.GetNextSelectedItem(pos);
        if (nItem <= 0 || nItem >= m_list.GetItemCount()) {
            return;
        }
        m_list.DeleteItem(nItem);
        nItem = min(nItem, m_list.GetItemCount() - 1);
        m_list.SetSelectionMark(nItem);
        m_list.SetFocus();
        m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ReindexList();
        ReindexListSubItem();

        SetModified();
    }
}

void CPPageFullscreen::OnUpdateRemove(CCmdUI* pCmdUI)
{
    POSITION pos = m_list.GetFirstSelectedItemPosition();
    int i = m_list.GetNextSelectedItem(pos);
    pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2) && (i > 0 || pos != nullptr));
}

void CPPageFullscreen::OnAdd()
{
    POSITION pos = m_list.GetFirstSelectedItemPosition();
    int i = m_list.GetNextSelectedItem(pos) + 1;
    if (i <= 0) {
        i = m_list.GetItemCount();
    }
    if (m_list.GetItemCount() <= MAX_FPS_COUNT) {
        CString str, strCur;
        (i < 10) ? str.Format(_T("0%d"), i) : str.Format(_T("%d"), i);
        m_list.InsertItem(i, str);
        m_list.SetItemText(i, COL_VFR_F, _T("1.000"));
        m_list.SetItemText(i, COL_VFR_T, _T("1.000"));
        GetCurDispModeString(strCur);
        m_list.SetItemText(i, COL_SRR, strCur);
        m_list.SetCheck(i, 0);
        m_list.SetFocus();
        m_list.SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ReindexList();
        ReindexListSubItem();

        SetModified();
    }
}

void CPPageFullscreen::OnMoveUp()
{
    if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
        int nItem = m_list.GetNextSelectedItem(pos);
        if (nItem <= 0) {
            return;
        }

        DWORD_PTR data = m_list.GetItemData(nItem);
        int nCheckCur = m_list.GetCheck(nItem);
        CString strN = m_list.GetItemText(nItem, 0);
        CString strF = m_list.GetItemText(nItem, 1);
        CString strT = m_list.GetItemText(nItem, 2);
        CString strDM = m_list.GetItemText(nItem, 3);
        m_list.DeleteItem(nItem);

        nItem--;
        m_list.InsertItem(nItem, strN);
        m_list.SetItemData(nItem, data);
        m_list.SetItemText(nItem, 1, strF);
        m_list.SetItemText(nItem, 2, strT);
        m_list.SetItemText(nItem, 3, strDM);
        m_list.SetCheck(nItem, nCheckCur);
        m_list.SetFocus();
        m_list.SetSelectionMark(nItem);
        m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ReindexList();
        ReindexListSubItem();

        SetModified();
    }
}

void CPPageFullscreen::OnMoveDown()
{
    if (POSITION pos = m_list.GetFirstSelectedItemPosition()) {
        int nItem = m_list.GetNextSelectedItem(pos);
        if (nItem < 0 || nItem >= m_list.GetItemCount() - 1) {
            return;
        }

        DWORD_PTR data = m_list.GetItemData(nItem);
        int nCheckCur = m_list.GetCheck(nItem);
        CString strN = m_list.GetItemText(nItem, 0);
        CString strF = m_list.GetItemText(nItem, 1);
        CString strT = m_list.GetItemText(nItem, 2);
        CString strDM = m_list.GetItemText(nItem, 3);
        m_list.DeleteItem(nItem);

        nItem++;

        m_list.InsertItem(nItem, strN);
        m_list.SetItemData(nItem, data);
        m_list.SetItemText(nItem, 1, strF);
        m_list.SetItemText(nItem, 2, strT);
        m_list.SetItemText(nItem, 3, strDM);
        m_list.SetCheck(nItem, nCheckCur);
        m_list.SetFocus();
        m_list.SetSelectionMark(nItem);
        m_list.SetItemState(nItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ReindexList();
        ReindexListSubItem();

        SetModified();
    }
}

void CPPageFullscreen::OnUpdateUp(CCmdUI* pCmdUI)
{
    POSITION pos = m_list.GetFirstSelectedItemPosition();
    int i = m_list.GetNextSelectedItem(pos);
    pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2) && (i > 1 || pos != nullptr));
}

void CPPageFullscreen::OnUpdateDown(CCmdUI* pCmdUI)
{
    POSITION pos = m_list.GetFirstSelectedItemPosition();
    int i = m_list.GetNextSelectedItem(pos);
    pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2) && (i > 0 && i < m_list.GetItemCount() - 1));

}

void CPPageFullscreen::OnUpdateAutoChangeFullscrRes(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!!IsDlgButtonChecked(IDC_CHECK2));
}

void CPPageFullscreen::ReindexList()
{
    if (m_list.GetItemCount() > 1) {
        CString str;
        for (int i = 1; i < m_list.GetItemCount(); i++) {
            (i < 10) ? str.Format(_T("0%d"), i) : str.Format(_T("%d"), i);
            m_list.SetItemText(i, 0, str);
        }
    }
}

void CPPageFullscreen::ReindexListSubItem()
{
    for (int i = 0; (size_t) i < sl.GetCount(); i++) {
        for (int n = 0; n < m_list.GetItemCount(); n++) {
            if (m_list.GetItemText(n, COL_SRR) == sl[i]) {
                m_iSeldm[n] = i;
            }
        }
    }
}

bool CPPageFullscreen::GetCurDispModeString(CString& strCur)
{
    dispmode dmod;
    bool ret = CMainFrame::GetCurDispMode(m_f_hmonitor, dmod);
    if (ret) {
        strCur.Format(_T("[ %d ]  @ %dx%d "), dmod.freq, dmod.size.cx, dmod.size.cy);
        strCur += (dmod.dmDisplayFlags == DM_INTERLACED) ? _T("i") : _T("p");
    }

    return ret;
}
