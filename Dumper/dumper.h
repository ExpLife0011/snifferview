#pragma once
#include <Windows.h>
#include <Dbghelp.h>
#include <string>
#include "../ComLib/mstring.h"

typedef BOOL (__stdcall *PMiniDumpWriteDump)(
    HANDLE hProcess,
    DWORD ProcessId,
    HANDLE hFile,
    MINIDUMP_TYPE DumpType,
    PMINIDUMP_EXCEPTION_INFORMATION,
    PMINIDUMP_USER_STREAM_INFORMATION,
    PMINIDUMP_CALLBACK_INFORMATION
    );

class CDumperMgr {
public:
    static CDumperMgr *GetInst();
    bool InitDumper(const std::wstring &dumpDir);

private:
    CDumperMgr();
    virtual ~CDumperMgr();
    PMiniDumpWriteDump GetWriterProc() const;
    std::ustring GetDumpFileName() const;
    void ShowErrDlg();

    BOOL StartRundll(LPCWSTR dllPath, LPCWSTR funName, LPCWSTR param) const;
    std::ustring GetDumpFilePath(DWORD pid, const std::ustring &dir) const;
    static LONG __stdcall ExceptionHandler(LPEXCEPTION_POINTERS pExceptionPointers);
    static void __stdcall RundllFun(HWND hwnd, HINSTANCE hinst, LPSTR command, int show);
    std::ustring GetExceptionStr(DWORD code) const;
private:
    bool mInit;
    std::ustring mDumpDir;

private:
    //rundll32进程参数
    DWORD mPid;
    std::ustring mDumpFullPath;
    std::ustring mExceptionDesc;
};