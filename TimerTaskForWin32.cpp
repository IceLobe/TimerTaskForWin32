// TimerTaskForWin32.cpp

#include <Windows.h>
#include <CommCtrl.h>
#include <iostream>
#include <time.h>

#include "resource.h"

//适配当前系统窗口样式
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#define MAX_WCHAR 0x200
#define RBTOUTF8 L"r, ccs=utf-8"
#define WBTOUTF8 L"w, ccs=utf-8"
#define TIMER_TASK 2001
#define TIMER_SHOWWINDOW 2002
#define WM_SHOWTASK 0x0401
#define MENU_EXIT 0x100

//#ifdef _DEBUG
//#define IsRelease FALSE
//#else
//#define IsRelease TRUE
//#endif // _DEBUG

// 程序名称
wchar_t fileName[_MAX_FNAME] = { 0 };
// 配置文件路径
wchar_t szCfgPath[MAX_PATH] = { 0 };

// 程序任务所在的完整路径
wchar_t szTaskFilePath[MAX_PATH] = { 0 };

// 配置文件属性前缀
wchar_t cfgs[][MAXCHAR] = { L"TaskPath=",L"RadioButton=",L"SUNDAY=",L"MONDAY=",L"TUESDAY=",L"WEDNESDAY=",L"THURSDAY=",L"FRIDAY=",L"SATURDAY=" };

// 计时器时长(秒)
const unsigned int Second = 1000;

SYSTEMTIME st;

OPENFILENAME ofn = { 0 };

HINSTANCE hIns = NULL;

HMENU hMenu = NULL;

void CALLBACK SetTimerShowWindow(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime);
void CALLBACK TimerProc(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime);
void DtnDatetimechange(HWND hDlg, int nIDDlgItem);
void ParseCfg(HWND hDlg, size_t index, const wchar_t* szCfgContent);
BOOL CfgReadWrite(HWND hDlg, wchar_t const* _FileName, wchar_t const* _Mode);
void InitDialog(HWND hDlg);
INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL IsMutex();

void CALLBACK SetTimerShowWindow(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
    //KillTimer(hDlg, nIDEvent);
    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hDlg;
    nid.uID = IDI_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_SHOWTASK;
    nid.hIcon = LoadIconW(hIns, MAKEINTRESOURCE(IDI_ICON));
    wcscpy_s(nid.szTip, fileName);
    Shell_NotifyIcon(NIM_ADD, &nid);
    ShowWindow(hDlg, SW_HIDE);
}

/// <summary>
/// 定时器回调函数
/// </summary>
/// <param name="hDlg"></param>
/// <param name="uMsg"></param>
/// <param name="nIDEvent"></param>
/// <param name="dwTime"></param>
void CALLBACK TimerProc(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
    SYSTEMTIME st_end;
    HWND hWnd = GetDlgItem(hDlg, IDC_DTP_END);
    SendMessageW(hWnd, DTM_GETSYSTEMTIME, GDT_VALID, (LPARAM)&st_end);
    time_t _end, _now;
    time(&_now);
    tm _etm = { st_end.wSecond,st_end.wMinute,st_end.wHour,st_end.wDay,st_end.wMonth - 1,st_end.wYear - 1900,st_end.wDayOfWeek,0,0 };
    _end = mktime(&_etm);

    hWnd = FindWindowW(L"#32770", L"飞秋(FeiQ)---局域网即时通讯");
    if (hWnd)
    {
        if (0 <= difftime(_now, _end))
        {
            //停止定时器
            KillTimer(hDlg, TIMER_TASK);
            //进程ID
            unsigned long lpProcessId = 0UL;
            //根据窗口句柄得到PID
            GetWindowThreadProcessId(hWnd, &lpProcessId);
            //根据PID打开进程
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, 0, lpProcessId);
            //结束进程
            TerminateProcess(hProcess, 0);
            //销毁窗口
            DestroyWindow(hDlg);
        }
    }
    else
    {
        ShellExecuteW(hDlg, L"open", szTaskFilePath, NULL, NULL, SW_HIDE);
    }
}

/// <summary>
/// SysDateTimePick32控件的时间变动事件
/// </summary>
/// <param name="hDlg">父窗口句柄</param>
/// <param name="nIDDlgItem">控件ID</param>
void DtnDatetimechange(HWND hDlg, int nIDDlgItem)
{
    HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
    SendMessageW(hWnd, DTM_GETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
    if (IDC_DTP_START == nIDDlgItem)
    {
        if (8 == st.wHour && 30 > st.wMinute)
        {
            st.wMinute = 30;
        }
        if (8 > st.wHour)
        {
            st.wHour = 8;
        }
        if (12 < st.wHour)
        {
            st.wHour = 12;
            st.wMinute = 0;
        }
    }
    else
    {
        if (14 == st.wHour && 30 > st.wMinute)
        {
            st.wMinute = 30;
        }
        if (14 > st.wHour)
        {
            st.wHour = 14;
        }
        if (20 <= st.wHour)
        {
            st.wHour = 20;
            st.wMinute = 0;
        }
    }
    st.wSecond = 0;
    st.wMilliseconds = 0;
    // 设置DateTimePicker的时间
    SendMessageW(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
}

/// <summary>
/// 解析配置文件
/// </summary>
/// <param name="hDlg">窗口句柄</param>
/// <param name="index">配置前缀集合下标</param>
/// <param name="szCfgContent">配置文件内容</param>
void ParseCfg(HWND hDlg, size_t index, const wchar_t* szCfgContent)
{
    wchar_t* tmp = { 0 };
    wchar_t str[MAX_WCHAR] = { 0 };
    wcscpy_s(str, szCfgContent);
    wchar_t* temp = wcsstr(str, (wchar_t*)cfgs[index]);
    if (temp)
    {
        wcscpy_s(str, temp);
    }
    else
    {
        return;
    }
    wcscpy_s(str, wcstok_s(str, L"\n", &tmp));
    temp = wcsstr(str, L"=");
    if (temp)
    {
        wcscpy_s(str, temp);
    }
    else
    {
        return;
    }
    if (0 == wcscmp(str, L"="))
    {
        wcscpy_s(str, L"");
    }
    else
    {
        wcscpy_s(str, wcstok_s(str, L"=", &tmp));
    }
    switch (index)
    {
    case 1:
        if (0 == wcscmp(str, L"EVERYDAY"))
        {
            CheckDlgButton(hDlg, IDC_RD_EVERYDAY, TRUE);
        }
        if (0 == wcscmp(str, L"WORKINGDAY"))
        {
            CheckDlgButton(hDlg, IDC_RD_WORKINGDAY, TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_SATURDAY), TRUE);
        }
        if (0 == wcscmp(str, L"WEEKEND"))
        {
            CheckDlgButton(hDlg, IDC_RD_WEEKEND, TRUE);
        }
        if (0 == wcscmp(str, L"CUST"))
        {
            CheckDlgButton(hDlg, IDC_RD_CUST, TRUE);
        }
        break;
    case 2:
        if (0 == wcscmp(str, L"1"))
        {
            CheckDlgButton(hDlg, IDC_CK_SUNDAY, TRUE);
        }
        else
        {
            CheckDlgButton(hDlg, IDC_CK_SUNDAY, FALSE);
        }
        break;
    case 3:
        if (0 == wcscmp(str, L"1"))
        {
            CheckDlgButton(hDlg, IDC_CK_MONDAY, TRUE);
        }
        else
        {
            CheckDlgButton(hDlg, IDC_CK_MONDAY, FALSE);
        }
        break;
    case 4:
        if (0 == wcscmp(str, L"1"))
        {
            CheckDlgButton(hDlg, IDC_CK_TUESDAY, TRUE);
        }
        else
        {
            CheckDlgButton(hDlg, IDC_CK_TUESDAY, FALSE);
        }
        break;
    case 5:
        if (0 == wcscmp(str, L"1"))
        {
            CheckDlgButton(hDlg, IDC_CK_WEDNESDAY, TRUE);
        }
        else
        {
            CheckDlgButton(hDlg, IDC_CK_WEDNESDAY, FALSE);
        }
        break;
    case 6:
        if (0 == wcscmp(str, L"1"))
        {
            CheckDlgButton(hDlg, IDC_CK_THURSDAY, TRUE);
        }
        else
        {
            CheckDlgButton(hDlg, IDC_CK_THURSDAY, FALSE);
        }
        break;
    case 7:
        if (0 == wcscmp(str, L"1"))
        {
            CheckDlgButton(hDlg, IDC_CK_FRIDAY, TRUE);
        }
        else
        {
            CheckDlgButton(hDlg, IDC_CK_FRIDAY, FALSE);
        }
        break;
    case 8:
        if (0 == wcscmp(str, L"1"))
        {
            CheckDlgButton(hDlg, IDC_CK_SATURDAY, TRUE);
        }
        else
        {
            CheckDlgButton(hDlg, IDC_CK_SATURDAY, FALSE);
        }
        break;
    default:
        wcscpy_s(szTaskFilePath, str);
        break;
    }
}

/// <summary>
/// 读写任务配置文件Cfg
/// </summary>
/// <param name="hDlg">父窗口句柄</param>
/// <param name="_FileName">文件路径</param>
/// <param name="_Mode">读写模式</param>
/// <returns>文件不存在返回FALSE</returns>
BOOL CfgReadWrite(HWND hDlg, wchar_t const* _FileName, wchar_t const* _Mode)
{
    try
    {
        FILE* fp;
        _wfopen_s(&fp, _FileName, _Mode);
        if (fp)
        {

            // 配置文件内容
            wchar_t szCfgContent[MAX_WCHAR] = { 0 };

            if (0 == wcscmp(_Mode, RBTOUTF8))
            {
                //获取文件大小
                fseek(fp, 0, SEEK_END);
                unsigned int reslen = ftell(fp) + 1;
                if (reslen <= 1)
                {
                    //DeleteFileW(_FileName);
                    return FALSE;
                }
                //读取文件内容
                fseek(fp, 0, SEEK_SET);
                fread(szCfgContent, sizeof(wchar_t), reslen, fp);
                fflush(fp);
                fclose(fp);

                for (size_t i = 0; i < wcslen(*cfgs); i++)
                {
                    ParseCfg(hDlg, i, szCfgContent);
                }

                wchar_t lpszText[MAXCHAR] = L"确定执行:";
                wcscat_s(lpszText, szTaskFilePath);
                int uType = MessageBoxW(hDlg, lpszText, L"提示", MB_YESNO);
                if (IDYES == uType)
                {
                    SetDlgItemTextW(hDlg, IDC_TXT_PATH, szTaskFilePath);
                    EnableWindow(GetDlgItem(hDlg, IDC_BTN_SELECT), FALSE);
                    SendMessageW(hDlg, WM_COMMAND, IDOK, 0);
                }
                else
                {
                    wcscpy_s(szTaskFilePath, L"");
                }
            }
            if (0 == wcscmp(_Mode, WBTOUTF8))
            {
                wcscpy_s(szCfgContent, L"TaskPath=");
                wcscat_s(szCfgContent, szTaskFilePath);
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_RD_EVERYDAY), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nRadioButton=EVERYDAY");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_RD_WORKINGDAY), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nRadioButton=WORKINGDAY");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_RD_WEEKEND), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nRadioButton=WEEKEND");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_RD_CUST), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nRadioButton=CUST");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_CK_SUNDAY), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nSUNDAY=1");
                }
                else
                {
                    wcscat_s(szCfgContent, L"\nSUNDAY=0");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_CK_MONDAY), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nMONDAY=1");
                }
                else
                {
                    wcscat_s(szCfgContent, L"\nMONDAY=0");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_CK_TUESDAY), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nTUESDAY=1");
                }
                else
                {
                    wcscat_s(szCfgContent, L"\nTUESDAY=0");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_CK_WEDNESDAY), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nWEDNESDAY=1");
                }
                else
                {
                    wcscat_s(szCfgContent, L"\nWEDNESDAY=0");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_CK_THURSDAY), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nTHURSDAY=1");
                }
                else
                {
                    wcscat_s(szCfgContent, L"\nTHURSDAY=0");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_CK_FRIDAY), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nFRIDAY=1");
                }
                else
                {
                    wcscat_s(szCfgContent, L"\nFRIDAY=0");
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_CK_SATURDAY), BM_GETCHECK, 0, 0))
                {
                    wcscat_s(szCfgContent, L"\nSATURDAY=1");
                }
                else
                {
                    wcscat_s(szCfgContent, L"\nSATURDAY=0");
                }
                fseek(fp, 0, SEEK_SET);
                fwrite(szCfgContent, sizeof(wchar_t), wcslen(szCfgContent), fp);
                fflush(fp);
                fclose(fp);
                EnableWindow(GetDlgItem(hDlg, IDC_BTN_SELECT), FALSE);
            }
            _flushall();
            fclose(fp);
            return TRUE;
        }
        CheckDlgButton(hDlg, IDC_RD_WORKINGDAY, TRUE);

        CheckDlgButton(hDlg, IDC_CK_MONDAY, TRUE);
        CheckDlgButton(hDlg, IDC_CK_TUESDAY, TRUE);
        CheckDlgButton(hDlg, IDC_CK_WEDNESDAY, TRUE);
        CheckDlgButton(hDlg, IDC_CK_THURSDAY, TRUE);
        CheckDlgButton(hDlg, IDC_CK_FRIDAY, TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_CK_SATURDAY), TRUE);
        return FALSE;
    }
    catch (const wchar_t* error)
    {
        MessageBoxW(hDlg, error, L"提示", MB_OK);
        return FALSE;
    }
}

/// <summary>
/// 初始化对话窗口
/// </summary>
/// <param name="hDlg"></param>
void InitDialog(HWND hDlg)
{
    HWND hWnd;
    GetLocalTime(&st);
    st.wHour = 8;
    st.wMinute = 30;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    hWnd = GetDlgItem(hDlg, IDC_DTP_START);
    SendMessageW(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);

    st.wHour = 18;
    st.wMinute = 30;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    hWnd = GetDlgItem(hDlg, IDC_DTP_END);
    SendMessageW(hWnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);

    GetModuleFileNameW(NULL, szCfgPath, MAX_PATH);
    wchar_t* tmp = { 0 };
    wcscpy_s(szCfgPath, wcstok_s(szCfgPath, L".", &tmp));
    wcscat_s(szCfgPath, L".cfg");

    CfgReadWrite(hDlg, szCfgPath, RBTOUTF8);
}

/// <summary>
/// 对话窗口回调函数
/// </summary>
/// <param name="hDlg"></param>
/// <param name="message"></param>
/// <param name="wParam"></param>
/// <param name="lParam"></param>
/// <returns></returns>
INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        InitDialog(hDlg);
        break;
    case WM_NOTIFY:
        switch (wParam)
        {
        case IDC_DTP_START:
            DtnDatetimechange(hDlg, IDC_DTP_START);
            break;
        case IDC_DTP_END:
            DtnDatetimechange(hDlg, IDC_DTP_END);
            break;
        default:
            break;
        }
        break;
    case WM_SHOWTASK:
        if (lParam == WM_RBUTTONUP)
        {
            POINT point;
            GetCursorPos(&point);
            hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING | MF_ENABLED, MENU_EXIT, L"退出(&Q)");
            TrackPopupMenu(hMenu, TPM_LEFTALIGN, point.x, point.y, 0, hDlg, NULL);
        }
        break;
    case WM_COMMAND:
        switch (wParam)
        {
        case MENU_EXIT:
            DestroyMenu(hMenu);
            DestroyWindow(hDlg);
            break;
        case IDC_BTN_SELECT:
            ofn = { NULL };
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFilter = L"应用程序(.exe)\0*.exe\0";
            ofn.lpstrInitialDir = L"D:\\Program Files\\";
            ofn.lpstrFile = szTaskFilePath;
            ofn.nMaxFile = sizeof(szTaskFilePath) / sizeof(*szTaskFilePath);
            ofn.nFilterIndex = 0;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
            if (GetOpenFileNameW(&ofn))
            {
                SetDlgItemTextW(hDlg, IDC_TXT_PATH, szTaskFilePath);
            }
            break;
        case IDC_RD_EVERYDAY:
            CheckDlgButton(hDlg, IDC_RD_EVERYDAY, TRUE);

            CheckDlgButton(hDlg, IDC_CK_SUNDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_MONDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_TUESDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_WEDNESDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_THURSDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_FRIDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_SATURDAY, TRUE);

            EnableWindow(GetDlgItem(hDlg, IDC_CK_SUNDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_MONDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_TUESDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_WEDNESDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_THURSDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_FRIDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_SATURDAY), FALSE);
            break;
        case IDC_RD_WORKINGDAY:
            CheckDlgButton(hDlg, IDC_RD_WORKINGDAY, TRUE);

            CheckDlgButton(hDlg, IDC_CK_SUNDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_MONDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_TUESDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_WEDNESDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_THURSDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_FRIDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_SATURDAY, FALSE);

            EnableWindow(GetDlgItem(hDlg, IDC_CK_SUNDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_MONDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_TUESDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_WEDNESDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_THURSDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_FRIDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_SATURDAY), TRUE);
            break;
        case IDC_RD_WEEKEND:
            CheckDlgButton(hDlg, IDC_RD_WEEKEND, TRUE);

            CheckDlgButton(hDlg, IDC_CK_SUNDAY, TRUE);
            CheckDlgButton(hDlg, IDC_CK_MONDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_TUESDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_WEDNESDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_THURSDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_FRIDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_SATURDAY, TRUE);

            EnableWindow(GetDlgItem(hDlg, IDC_CK_SUNDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_MONDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_TUESDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_WEDNESDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_THURSDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_FRIDAY), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_SATURDAY), FALSE);
            break;
        case IDC_RD_CUST:
            CheckDlgButton(hDlg, IDC_RD_CUST, TRUE);

            CheckDlgButton(hDlg, IDC_CK_SUNDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_MONDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_TUESDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_WEDNESDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_THURSDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_FRIDAY, FALSE);
            CheckDlgButton(hDlg, IDC_CK_SATURDAY, FALSE);

            EnableWindow(GetDlgItem(hDlg, IDC_CK_SUNDAY), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_MONDAY), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_TUESDAY), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_WEDNESDAY), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_THURSDAY), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_FRIDAY), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CK_SATURDAY), TRUE);
            break;
        case IDOK:
            GetDlgItemTextW(hDlg, IDC_TXT_PATH, szTaskFilePath, MAX_PATH);
            if (0 == wcscmp(szTaskFilePath, L""))
            {
                MessageBoxW(hDlg, L"请选择要执行的程序！", L"提示", MB_OK);
                break;
            }
            CfgReadWrite(hDlg, szCfgPath, WBTOUTF8);
            SetTimer(hDlg, TIMER_SHOWWINDOW, 1, SetTimerShowWindow);
            // 设置定时器(每1秒钟触发)
            SetTimer(hDlg, TIMER_TASK, 1000, TimerProc);
            break;
        case IDCANCEL:
            DestroyWindow(hDlg);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

/// <summary>
/// 判断程序是否加锁
/// </summary>
/// <returns></returns>
BOOL IsMutex()
{
    // 获得执行文件的路径
    wchar_t szFilePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, szFilePath, MAX_PATH);

    // 分割程序完整路径得到程序名称
    _wsplitpath_s(szFilePath, NULL, NULL, NULL, NULL, fileName, _MAX_FNAME, NULL, NULL);
    // 给进程实例加锁
    HANDLE m_hMutex = CreateMutexW(NULL, FALSE, fileName);
    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        HWND hWnd = FindWindowW(L"TIMER_TASK_WIN32", NULL);
        SetForegroundWindow(hWnd);
        return TRUE;
    }
    return FALSE;
}

/// <summary>
/// 程序入口
/// </summary>
/// <param name="hInstance"></param>
/// <param name="hPrevInstance"></param>
/// <param name="lpCmdLine"></param>
/// <param name="nShowCmd"></param>
/// <returns></returns>
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    //如果程序加锁终止本次执行并激活已存在的程序
    if (IsMutex())
    {
        return 0;
    }

    WNDCLASSW wc;
    // 获取对话框的类信息
    GetClassInfoW(hInstance, L"#32770", &wc);
    wc.hInstance = hInstance;
    // 设置类名
    wc.lpszClassName = L"TIMER_TASK_WIN32";
    // 注册这个类
    RegisterClassW(&wc);

    // 获取对话框句柄
    HWND hWnd = NULL;
    hIns = hInstance;
    // 创建对话框窗口
    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_DLG_TIMERTASK), hWnd, DialogProc, 0L);

    if (!hWnd)
    {
        return FALSE;
    }

    MSG msg;
    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
