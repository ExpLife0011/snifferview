#include "dumper.h"
#include <ShlObj.h>
#include <shlwapi.h>
#include "../ComLib/common.h"
#include "../ComLib/StrUtil.h"
#include "../ComLib/base64.h"
#include "resource.h"

#pragma comment(lib, "shell32.lib")

using namespace std;

CDumperMgr *CDumperMgr::GetInst() {
    static CDumperMgr *sPtr = NULL;

    if (NULL == sPtr)
    {
        sPtr = new CDumperMgr();
    }
    return sPtr;
}

BOOL CDumperMgr::StartRundll(LPCWSTR dllPath, LPCWSTR funName, LPCWSTR param) const {
    if (!dllPath || !*dllPath || !funName || !*funName)
    {
        dp(L"param error");
        return FALSE;
    }

    WCHAR rundll32[MAX_PATH] = {0x00};
    GetSystemDirectoryW(rundll32, MAX_PATH);
    PathAppendW(rundll32, L"rundll32.exe");

    ustring command;
    command.format(L"\"%ls\" \"%ls\",%ls", rundll32, dllPath, funName);
    if (param && *param)
    {
        command += L" ";
        command += param;
    }

    dp(L"command:%ls", command.c_str());
    HANDLE h = ExecProcessW(command.c_str(), NULL, TRUE);

    if (h)
    {
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
        return TRUE;
    }
    return FALSE;
}

ustring CDumperMgr::GetExceptionStr(DWORD code) const {
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            return L"内存访问异常";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            return L"数组访问越界";
        case EXCEPTION_STACK_OVERFLOW:
            return L"内存栈溢出";
        default:
            return L"未知程序错误";
    }
}

LONG CDumperMgr::ExceptionHandler(LPEXCEPTION_POINTERS pExceptionPointers) {
    dp(L"aaaa");
    ustring errDesc;
    errDesc += FormatW(L"操作系统:%hs\r\n\r\n", GetOSVersionExA().c_str());

    FILETIME t1, t2, t3, t4;
    GetProcessTimes(GetCurrentProcess(), &t1, &t2, &t3, &t4);
    FILETIME localFileTime;
    FileTimeToLocalFileTime(&t1, &localFileTime);
    SYSTEMTIME time = {0};
    FileTimeToSystemTime(&localFileTime, &time);
    SYSTEMTIME endTime = {0};
    GetLocalTime(&endTime);
    errDesc += FormatW(
        L"启动时间:%04d-%02d-%02d %02d:%02d:%02d %03d\r\n",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond,
        time.wMilliseconds
        );
    errDesc += FormatW(
        L"异常时间:%04d-%02d-%02d %02d:%02d:%02d %03d\r\n",
        endTime.wYear,
        endTime.wMonth,
        endTime.wDay,
        endTime.wHour,
        endTime.wMinute,
        endTime.wSecond,
        endTime.wMilliseconds
        );

    dp(L"bbbb");
    DWORD code = pExceptionPointers->ExceptionRecord->ExceptionCode;
    errDesc += FormatW(L"异常码:0x%08x(%d), 参考信息:%ls\r\n", code, code, GetInst()->GetExceptionStr(code).c_str());
    errDesc += FormatW(L"异常地址:0x%p\r\n", pExceptionPointers->ExceptionRecord->ExceptionAddress);
    errDesc += FormatW(L"异常描述地址:0x%p(windbg .exr命令查看)\r\n", pExceptionPointers->ExceptionRecord);
    errDesc += FormatW(L"异常上下文地址:0x%p(windbg .cxr命令查看)\r\n", pExceptionPointers->ContextRecord);

    //@@pid:1123
    ustring errDescEncode = AtoW(base64encode(WtoA(errDesc)));
    ustring param = FormatW(L"@@pid:%d@@dir:%ls@@param:%ls", GetCurrentProcessId(), GetInst()->mDumpDir.c_str(), errDescEncode.c_str());

    extern HMODULE g_hThisModule;
    wchar_t dllPath[512];
    GetModuleFileNameW(g_hThisModule, dllPath, sizeof(dllPath));
    dp(L"dllPath:%ls, param:%ls", dllPath, param.c_str());
    GetInst()->StartRundll(dllPath, L"RundllFun", param.c_str());
    return EXCEPTION_EXECUTE_HANDLER;
}

PMiniDumpWriteDump CDumperMgr::GetWriterProc() const {
    PMiniDumpWriteDump pfn = NULL;
    HMODULE m = GetModuleHandleW(L"DBGHELP.dll");
    if (!m)
    {
        m = LoadLibraryW(L"DBGHELP.dll");
    }

    if (!m)
    {
        return NULL;
    }
    return (PMiniDumpWriteDump)GetProcAddress(m, "MiniDumpWriteDump");
}

ustring CDumperMgr::GetDumpFilePath(DWORD pid, const ustring &dir) const {
    ustring procPath;

    mstring tmp;
    if (!GetProcessByPid(pid, tmp))
    {
        return L"";
    }
    procPath = AtoW(tmp);

    ustring procName = PathFindFileNameW(procPath.c_str());
    if (procName.endwith(L".exe") || procName.endwith(L".EXE"))
    {
        procName.erase(procName.size() - 4, 4);
    }

    SYSTEMTIME time = {0};
    GetLocalTime(&time);
    ustring fileName = FormatW(
        L"%ls_%04d-%02d-%02d_%02d_%02d_%02d_pid_%d",
        procName.c_str(),
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond,
        pid
        );

    ustring fullPath(dir);
    fullPath.path_append(fileName.c_str());
    return fullPath;
}

bool CDumperMgr::InitDumper(const wstring &dumpDir) {
    if (mInit == true)
    {
        return false;
    }

    mInit = true;
    mDumpDir = dumpDir;

    SHCreateDirectoryExW(NULL, dumpDir.c_str(), NULL);
    SetUnhandledExceptionFilter(ExceptionHandler);
    return true;
}

CDumperMgr::CDumperMgr(): mInit(false), mPid(0) {
}

CDumperMgr::~CDumperMgr() {
}

void CDumperMgr::ShowErrDlg() {
    struct DumpProcParam {
        DWORD mPid;
        ustring mDumpPath;
        ustring mExceptionDesc;
    };

    class CDlgProc {
    public:
        static void OnInitDlg(HWND hdlg, WPARAM wp, LPARAM lp) {
            extern HMODULE g_hThisModule;
            HWND hFileDesc = GetDlgItem(hdlg, IDC_EDT_PROC_DESC);
            HWND hDumpDesc = GetDlgItem(hdlg, IDC_EDT_DMP_DESC);
            HWND hPicture = GetDlgItem(hdlg, IDC_IMG_PROC);

            DumpProcParam *param = (DumpProcParam *)lp;

            //设置主窗口图标
            SendMessageA(hdlg, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconA(g_hThisModule, MAKEINTRESOURCEA(IDI_MAIN)));

            mstring buff;
            GetProcessByPid(param->mPid, buff);

            mstring fileDesc, fileVerison;
            GetFileString(buff.c_str(), "FileDescription", fileDesc);
            GetFileVersion(buff.c_str(), fileVerison);

            mstring descStr;
            descStr = PathFindFileNameA(buff.c_str());
            mstring low = descStr;
            low.makelower();
            if (low.endwith(".exe"))
            {
                descStr.erase(low.size() - 4, 4);
            }

            if (fileVerison.empty())
            {
                fileVerison = "0.0.0.0";
            }
            descStr += "  版本:", descStr += fileVerison, descStr += "\r\n\r\n";
            descStr += fileDesc, descStr += "\r\n\r\n";
            SetWindowTextA(hFileDesc, descStr.c_str());

            mstring errDesc = WtoA(param->mExceptionDesc);
            errDesc += FormatA("Dump文件:%hs", WtoA(param->mDumpPath).c_str());
            SetWindowTextA(hDumpDesc, errDesc.c_str());

            SHFILEINFOA pe = {0};
            if (0 != SHGetFileInfoA(buff.c_str(), 0, &pe, sizeof(pe), SHGFI_ICON))
            {
                SendMessage(hPicture, STM_SETICON, (WPARAM)pe.hIcon, 0);
            }

            CenterWindow(NULL, hdlg);
            SetForegroundWindow(hdlg);
            SetWindowPos(hdlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetFocus(GetDlgItem(hdlg, IDC_BTN_FIND));
        }

        static INT_PTR CALLBACK DumpDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            switch (uMsg)
            {
            case  WM_INITDIALOG:
                OnInitDlg(hwndDlg, wParam, lParam);
                break;
            case  WM_COMMAND:
                {
                    WORD id = LOWORD(wParam);
                    if (id == IDC_BTN_CANCEL) {
                        SendMessageA(hwndDlg, WM_CLOSE, 0, 0);
                    } else if (id == IDC_BTN_FIND)
                    {
                    }
                }
                break;
            case  WM_CLOSE:
                EndDialog(hwndDlg, 0);
                break;
            }
            return 0;
        }
    };

    extern HMODULE g_hThisModule;
    DumpProcParam param;
    param.mPid = mPid;
    param.mDumpPath = mDumpFullPath;
    param.mExceptionDesc = mExceptionDesc;
    DialogBoxParamW(g_hThisModule, MAKEINTRESOURCEW(IDD_DUMP), NULL, CDlgProc::DumpDlgProc, (LPARAM)&param);
}

void CDumperMgr::RundllFun(HWND hwnd, HINSTANCE hinst, LPSTR command, int show) {
    HANDLE hDbgProcess = NULL;
    HANDLE hDumpFile = NULL;
    bool result = false;

    do
    {
        if (!command || !command[0])
        {
            break;
        }

        dp(L"rundll command:%hs", command);
        mstring param(command);
        if (!param.startwith("@@pid:"))
        {
            break;
        }

        size_t pos1 = 0, pos2 = param.find("@@dir:"), pos3 = param.find("@@param:");
        mstring pidStr, dirStr, paramStr;

        pidStr = param.substr(pos1, pos2 - pos1);
        pidStr.erase(0, lstrlenA("@@pid:"));
        DWORD pid = (DWORD)atoi(pidStr.c_str());
        GetInst()->mPid = pid;

        dirStr = param.substr(pos2, pos3 - pos2);
        dirStr.erase(0, lstrlenA("@@dir:"));

        paramStr = param.substr(pos3, param.size() - pos3);
        paramStr.erase(0, lstrlenA("@@param:"));
        GetInst()->mExceptionDesc = AtoW(base64decode(paramStr));
        GetInst()->mDumpFullPath = AtoW(dirStr);

        dp(L"param1, pid:%d, dir:%ls, param:%ls", pid, GetInst()->mDumpFullPath.c_str(), GetInst()->mExceptionDesc.c_str());

        PMiniDumpWriteDump pfnWriter = GetInst()->GetWriterProc();
        if (!pfnWriter)
        {
            break;
        }

        hDbgProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!hDbgProcess)
        {
            break;
        }

        ustring filePath = GetInst()->GetDumpFilePath(pid, AtoW(dirStr));
        GetInst()->mDumpFullPath = filePath + L".dmp";
        GetInst()->mLogPath = filePath + L".log";

        //wirte log file
        FILE *fp = fopen(WtoA(GetInst()->mLogPath.c_str()).c_str(), "wb+");
        byte bom[] = {0xff, 0xfe};
        fwrite(bom, 1, sizeof(bom), fp);
        fwrite(GetInst()->mExceptionDesc.c_str(), 2, GetInst()->mExceptionDesc.size(), fp);
        fclose(fp);

        hDumpFile = CreateFileW(GetInst()->mDumpFullPath.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (!hDumpFile || INVALID_HANDLE_VALUE == hDumpFile)
        {
            break;
        }

        pfnWriter(hDbgProcess, pid, hDumpFile, MiniDumpNormal, NULL, NULL, NULL);
        result = true;
    } while (FALSE);

    if (hDbgProcess)
    {
        CloseHandle(hDbgProcess);
    }

    if (hDumpFile)
    {
        CloseHandle(hDumpFile);
    }

    if (result == true)
    {
        GetInst()->ShowErrDlg();
    }
}

bool __stdcall DumperInit(const wchar_t *dir) {
    return CDumperMgr::GetInst()->InitDumper(dir);
}