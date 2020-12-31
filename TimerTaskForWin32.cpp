// TimerTaskForWin32.cpp

#include <Windows.h>
#include <CommCtrl.h>
#include <iostream>
#include <time.h>
#include <tlhelp32.h>

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

#define null L""
#define MAX_WCHAR 0x200
#define READTOUTF8 L"r, ccs=utf-8"
#define WRITETOUTF8 L"w, ccs=utf-8"
#define TIMER_TASK 2001
#define TIMER_SHOWWINDOW 2002
#define WM_SHOWTASK 0x0401
#define SECOND 1000                                 // 定时器秒数

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
wchar_t cfgs[][MAXCHAR] = { L"TaskPath=",L"RadioButton=",L"SUNDAY=",L"MONDAY=",L"TUESDAY=",L"WEDNESDAY=",L"THURSDAY=",L"FRIDAY=",L"SATURDAY=",L"STARTTIME=",L"ENDTIME=" };

// 计时器时长(秒)
const unsigned int Second = 1000;

// 系统时间
SYSTEMTIME st;

// 文件选择对话框
OPENFILENAMEW ofn = { 0 };

// 实例句柄
HINSTANCE hIns = NULL;

// 菜单列表
HMENU hMenu = NULL;

// 托盘图标
NOTIFYICONDATAW nid;

BOOL flag = FALSE;

void CfgTimeToSystemTime(HWND hDlg, int nIDDlgItem, wchar_t* wstr);
void CkCheckToCfg(HWND hDlg, int nIDDlgItem, wchar_t*& rewstr, int rewLen = 20);
void SystemTimeToWcs(HWND hDlg, int nIDDlgItem, wchar_t*& wstr, int wcs_Size);
void CALLBACK SetTimerShowWindow(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime);
void CALLBACK TimerProc(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime);
void SetBtnCheck(HWND hDlg, int nIDDlgItem);
void DtnDateTimeChange(HWND hDlg, int nIDDlgItem);
void ParseCfg(HWND hDlg, size_t index, const wchar_t* szCfgContent);
BOOL CfgReadWrite(HWND hDlg, wchar_t const* _FileName, wchar_t const* _Mode);
void InitDialog(HWND hDlg);
INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL IsMutex();

/// <summary>
/// 配置文件时间转SystemTime
/// </summary>
/// <param name="hDlg">窗口句柄</param>
/// <param name="nIDDlgItem">控件ID</param>
/// <param name="wstr">时间字符串</param>
void CfgTimeToSystemTime(HWND hDlg, int nIDDlgItem, wchar_t* wstr)
{
    wchar_t* tmp = wcstok_s(wstr, L":", &wstr);
    st.wHour = _wtoi(tmp);
    tmp = wcstok_s(wstr, L":", &wstr);
    st.wMinute = _wtoi(tmp);
    if (wstr)
    {
        st.wSecond = _wtoi(wstr);
    }
    HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
    SendMessageW(hWnd, DTM_SETSYSTEMTIME, 0, (LPARAM)&st);
}

/// <summary>
/// 多选按钮选择状态保存到Cfg文件
/// </summary>
/// <param name="hDlg">窗口句柄</param>
/// <param name="nIDDlgItem">多选按钮ID</param>
/// <param name="rewstr">Cfg字符串</param>
/// <param name="rewLen">字符串长度(默认为20)</param>
void CkCheckToCfg(HWND hDlg, int nIDDlgItem, wchar_t*& rewstr, int rewLen)
{
    int len = sizeof(int) + 1;
    wchar_t* wstr = new wchar_t[len];
    HWND hWnd = GetDlgItem(hDlg, nIDDlgItem);
    int day = SendMessageW(hWnd, BM_GETCHECK, 0, 0);
    _itow_s(day, wstr, len, 10);
    switch (nIDDlgItem)
    {
    case IDC_CK_SUNDAY:
        wcscpy_s(rewstr, rewLen, L"\nSUNDAY=");
        break;
    case IDC_CK_MONDAY:
        wcscpy_s(rewstr, rewLen, L"\nMONDAY=");
        break;
    case IDC_CK_TUESDAY:
        wcscpy_s(rewstr, rewLen, L"\nTUESDAY=");
        break;
    case IDC_CK_WEDNESDAY:
        wcscpy_s(rewstr, rewLen, L"\nWEDNESDAY=");
        break;
    case IDC_CK_THURSDAY:
        wcscpy_s(rewstr, rewLen, L"\nTHURSDAY=");
        break;
    case IDC_CK_FRIDAY:
        wcscpy_s(rewstr, rewLen, L"\nFRIDAY=");
        break;
    case IDC_CK_SATURDAY:
        wcscpy_s(rewstr, rewLen, L"\nSATURDAY=");
        break;
    default:
        break;
    }
    wcscat_s(rewstr, rewLen, wstr);
}

/// <summary>
/// SystemTime写入配置文件
/// </summary>
/// <param name="hDlg">窗口句柄</param>
/// <param name="nIDDlgItem">控件ID</param>
/// <param name="wstr">时间字符串</param>
/// <param name="wcs_Size">字符串长度</param>
void SystemTimeToWcs(HWND hDlg, int nIDDlgItem, wchar_t*& wstr, int wcs_Size)
{
    SendMessageW(GetDlgItem(hDlg, nIDDlgItem), DTM_GETSYSTEMTIME, 0, (LPARAM)&st);
    wchar_t hour[3] = { 0 };
    _itow_s((int)st.wHour, hour, 3, 10);
    wcscpy_s(wstr, wcs_Size, hour);
    wcscat_s(wstr, wcs_Size, L":");
    wchar_t minute[3] = { 0 };
    _itow_s((int)st.wMinute, minute, 3, 10);
    wcscat_s(wstr, wcs_Size, minute);
    wcscat_s(wstr, wcs_Size, L":");
    wchar_t second[3] = { 0 };
    _itow_s((int)st.wSecond, second, 3, 10);
    wcscat_s(wstr, wcs_Size, second);
}

/// <summary>
/// 用定时器隐藏窗口
/// </summary>
/// <param name="hDlg">窗口句柄</param>
/// <param name="uMsg">消息内容</param>
/// <param name="nIDEvent">定时器ID</param>
/// <param name="dwTime">定时器时长</param>
void CALLBACK SetTimerShowWindow(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
    ShowWindow(hDlg, SW_HIDE);
}

/// <summary>
/// 检测任务是否在可执行时间内
/// 定时器回调函数
/// </summary>
/// <param name="hDlg">窗口句柄</param>
/// <param name="uMsg">消息内容</param>
/// <param name="nIDEvent">定时器ID</param>
/// <param name="dwTime">定时器时长</param>
void CALLBACK TimerProc(HWND hDlg, UINT uMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
    SYSTEMTIME st_start, st_end;
    HWND hWnd = GetDlgItem(hDlg, IDC_DTP_END);
    SendMessageW(hWnd, DTM_GETSYSTEMTIME, GDT_VALID, (LPARAM)&st_start);
    SendMessageW(hWnd, DTM_GETSYSTEMTIME, GDT_VALID, (LPARAM)&st_end);
    time_t _start, _end, _now;
    time(&_now);
    tm _stm = { st_start.wSecond,st_start.wMinute,st_start.wHour,st_start.wDay,st_start.wMonth - 1,st_start.wYear - 1900,st_start.wDayOfWeek,0,0 };
    tm _etm = { st_end.wSecond,st_end.wMinute,st_end.wHour,st_end.wDay,st_end.wMonth - 1,st_end.wYear - 1900,st_end.wDayOfWeek,0,0 };
    _start = mktime(&_stm);
    _end = mktime(&_etm);

    //查找进程
    PROCESSENTRY32W ps;
    HANDLE handle;
    ps.dwSize = sizeof(PROCESSENTRY32W);
    // 遍历进程 
    handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (handle == INVALID_HANDLE_VALUE)
        return;
    if (Process32FirstW(handle, &ps))
    {
        wchar_t* szExeFile = new wchar_t[_MAX_FNAME];
        _wsplitpath_s(szTaskFilePath, NULL, NULL, NULL, NULL, szExeFile, _MAX_FNAME, NULL, NULL);
        wcscat_s(szExeFile, _MAX_FNAME, L".exe");
        //遍历进程快照
        while (Process32NextW(handle, &ps))
        {
            if (0 == wcscmp(ps.szExeFile, szExeFile))
            {
                if (0 >= difftime(_now, _start) || 0 <= difftime(_now, _end))
                {
                    //停止定时器
                    KillTimer(hDlg, TIMER_TASK);
                    //关闭进程快照
                    CloseHandle(handle);
                    //根据PID打开进程
                    handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, 0, ps.th32ProcessID);
                    //结束进程
                    TerminateProcess(handle, EXIT_SUCCESS);
                    SendMessageW(hDlg, WM_COMMAND, IDCANCEL, NULL);
                }
                return;
            }
        }
        ShellExecuteW(hDlg, L"open", szTaskFilePath, NULL, NULL, SW_HIDE);
    }
}

/// <summary>
/// 设置按钮选择状态
/// </summary>
/// <param name="hDlg">窗口句柄</param>
/// <param name="nIDDlgItem">按钮ID</param>
void SetBtnCheck(HWND hDlg, int nIDDlgItem)
{
    switch (nIDDlgItem)
    {
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
    default:
        break;
    }
}

/// <summary>
/// SysDateTimePick32控件的时间变动事件
/// </summary>
/// <param name="hDlg">父窗口句柄</param>
/// <param name="nIDDlgItem">控件ID</param>
void DtnDateTimeChange(HWND hDlg, int nIDDlgItem)
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
        wcscpy_s(str, null);
    }
    else
    {
        wcscpy_s(str, wcstok_s(str, L"=", &tmp));
    }
    int value = _wtoi(str);
    switch (index)
    {
    case 1:
        SetBtnCheck(hDlg, value);
        break;
    case 2:
        CheckDlgButton(hDlg, IDC_CK_SUNDAY, value);
        break;
    case 3:
        CheckDlgButton(hDlg, IDC_CK_MONDAY, value);
        break;
    case 4:
        CheckDlgButton(hDlg, IDC_CK_TUESDAY, value);
        break;
    case 5:
        CheckDlgButton(hDlg, IDC_CK_WEDNESDAY, value);
        break;
    case 6:
        CheckDlgButton(hDlg, IDC_CK_THURSDAY, value);
        break;
    case 7:
        CheckDlgButton(hDlg, IDC_CK_FRIDAY, value);
        break;
    case 8:
        CheckDlgButton(hDlg, IDC_CK_SATURDAY, value);
        break;
    case 9:
        CfgTimeToSystemTime(hDlg, IDC_DTP_START, str);
        break;
    case 10:
        CfgTimeToSystemTime(hDlg, IDC_DTP_END, str);
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
            if (0 == wcscmp(_Mode, READTOUTF8))
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
                unsigned int cfgsLen = sizeof(cfgs) / sizeof(cfgs[0]);
                for (size_t i = 0; i < cfgsLen; i++)
                {
                    ParseCfg(hDlg, i, szCfgContent);
                }
                SetDlgItemTextW(hDlg, IDC_TXT_PATH, szTaskFilePath);

                if (!flag)
                {
                    wchar_t lpszText[MAX_PATH] = L"确定执行:";
                    wcscat_s(lpszText, szTaskFilePath);
                    int uType = MessageBoxW(NULL, lpszText, L"提示", MB_YESNO);
                    if (IDYES == uType)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_BTN_SELECT), FALSE);
                        SendMessageW(hDlg, WM_COMMAND, IDOK, 0);
                    }
                    else
                    {
                        SetDlgItemTextW(hDlg, IDC_TXT_PATH, null);
                        wcscpy_s(szTaskFilePath, null);
                    }
                }
                else
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_BTN_SELECT), FALSE);
                    SendMessageW(hDlg, WM_COMMAND, IDOK, 0);
                }
            }
            if (0 == wcscmp(_Mode, WRITETOUTF8))
            {
                wcscpy_s(szCfgContent, L"TaskPath=");
                wcscat_s(szCfgContent, szTaskFilePath);
                int len = sizeof(int) + 1;
                wchar_t* wstr = new wchar_t[len];
                wcscpy_s(wstr, len, null);
                wcscat_s(szCfgContent, L"\nRadioButton=");
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_RD_EVERYDAY), BM_GETCHECK, 0, 0))
                {
                    _itow_s(IDC_RD_EVERYDAY, wstr, len, 10);
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_RD_WORKINGDAY), BM_GETCHECK, 0, 0))
                {
                    _itow_s(IDC_RD_WORKINGDAY, wstr, len, 10);
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_RD_WEEKEND), BM_GETCHECK, 0, 0))
                {
                    _itow_s(IDC_RD_WEEKEND, wstr, len, 10);
                }
                if (BST_CHECKED == SendMessageW(GetDlgItem(hDlg, IDC_RD_CUST), BM_GETCHECK, 0, 0))
                {
                    _itow_s(IDC_RD_CUST, wstr, len, 10);
                }
                wcscat_s(szCfgContent, wstr);

                CkCheckToCfg(hDlg, IDC_CK_SUNDAY, wstr);
                wcscat_s(szCfgContent, wstr);

                CkCheckToCfg(hDlg, IDC_CK_MONDAY, wstr);
                wcscat_s(szCfgContent, wstr);

                CkCheckToCfg(hDlg, IDC_CK_TUESDAY, wstr);
                wcscat_s(szCfgContent, wstr);

                CkCheckToCfg(hDlg, IDC_CK_WEDNESDAY, wstr);
                wcscat_s(szCfgContent, wstr);

                CkCheckToCfg(hDlg, IDC_CK_THURSDAY, wstr);
                wcscat_s(szCfgContent, wstr);

                CkCheckToCfg(hDlg, IDC_CK_FRIDAY, wstr);
                wcscat_s(szCfgContent, wstr);

                CkCheckToCfg(hDlg, IDC_CK_SATURDAY, wstr);
                wcscat_s(szCfgContent, wstr);

                int stlen = sizeof(SYSTEMTIME);
                wchar_t* wcsTime = new wchar_t[stlen];
                SystemTimeToWcs(hDlg, IDC_DTP_START, wcsTime, stlen);
                wcscat_s(szCfgContent, L"\nSTARTTIME=");
                wcscat_s(szCfgContent, wcsTime);
                SystemTimeToWcs(hDlg, IDC_DTP_END, wcsTime, stlen);
                wcscat_s(szCfgContent, L"\nENDTIME=");
                wcscat_s(szCfgContent, wcsTime);

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
        SetBtnCheck(hDlg, IDC_RD_WORKINGDAY);
        return FALSE;
    }
    catch (const wchar_t* error)
    {
        MessageBoxW(NULL, error, L"提示", MB_OK);
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

    CfgReadWrite(hDlg, szCfgPath, READTOUTF8);
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
            DtnDateTimeChange(hDlg, IDC_DTP_START);
            break;
        case IDC_DTP_END:
            DtnDateTimeChange(hDlg, IDC_DTP_END);
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
            AppendMenuW(hMenu, MF_STRING | MF_ENABLED, IDCANCEL, L"退出(&Q)");
            TrackPopupMenu(hMenu, TPM_LEFTALIGN, point.x, point.y, 0, hDlg, NULL);
        }
        break;
    case WM_COMMAND:
        SetBtnCheck(hDlg, wParam);
        switch (wParam)
        {
        case IDC_BTN_SELECT:
            RtlZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFilter = L"应用程序\0*.exe\0";
            ofn.lpstrInitialDir = L"D:\\Program Files\\";
            ofn.lpstrFile = szTaskFilePath;
            ofn.nMaxFile = sizeof(szTaskFilePath) / sizeof(*szTaskFilePath);
            ofn.lpstrDefExt = L"exe";
            ofn.nFilterIndex = 0;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;// | OFN_EXPLORER;
            if (GetOpenFileNameW(&ofn))
            {
                SetDlgItemTextW(hDlg, IDC_TXT_PATH, szTaskFilePath);
            }
            break;
        case IDOK:
            GetDlgItemTextW(hDlg, IDC_TXT_PATH, szTaskFilePath, MAX_PATH);
            if (0 == wcscmp(szTaskFilePath, null))
            {
                MessageBoxW(NULL, L"请选择要执行的程序！", L"提示", MB_OK);
                break;
            }
            SetTimer(hDlg, TIMER_SHOWWINDOW, 1, SetTimerShowWindow);
            nid.cbSize = sizeof(NOTIFYICONDATAW);
            nid.hWnd = hDlg;
            nid.uID = IDI_ICON;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_SHOWTASK;
            nid.hIcon = LoadIconW(hIns, MAKEINTRESOURCEW(IDI_ICON));
            wcscpy_s(nid.szTip, fileName);
            Shell_NotifyIconW(NIM_ADD, &nid);
            CfgReadWrite(hDlg, szCfgPath, WRITETOUTF8);
            // 设置定时器(每1秒钟触发)
            SetTimer(hDlg, TIMER_TASK, SECOND, TimerProc);
            break;
        case IDCANCEL:
            // 在托盘中删除图标
            Shell_NotifyIconW(NIM_DELETE, &nid);
            DestroyMenu(hMenu);
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
    if (0 == wcscmp(lpCmdLine, L"-no"))
    {
        flag = TRUE;
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
        // 在托盘中删除图标
        Shell_NotifyIconW(NIM_DELETE, &nid);
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
