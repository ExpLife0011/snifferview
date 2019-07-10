#pragma once
#include <Windows.h>
#include "../common/mstring.h"

#define TASK_OPEN_DUMP    "OpenDump"
#define TASK_SAVE_DUMP    "SaveDump"
#define RESULT_NULL       "nullResult"

class CUserTaskMgr {
public:
    static CUserTaskMgr *GetInst();
    void ClearCache() const;
    void StartService(DWORD parent);
    std::mstring SendTask(const std::mstring &task, const std::mstring &param) const;

private:
    CUserTaskMgr();
    virtual ~CUserTaskMgr();
    std::mstring RunTaskInService(const std::mstring &taskType, const std::mstring &taskParam) const;
    void OnTask() const;

    static UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp);
    static DWORD __stdcall ParentCheckThread(LPVOID param);

    HKEY CreateLowSecurityKey(HKEY hkey, const std::mstring &subKey) const;
    std::mstring ShowOpenFileDlg(const std::mstring &defDlg) const;
    std::mstring ShowSaveFileDlg(const std::mstring &defName, const std::mstring &defDlg) const;

private:
    bool mStart;
    HANDLE mTaskThread;
    HANDLE mTaskNotify;
    DWORD mParentPid;
    bool mExitService;
};