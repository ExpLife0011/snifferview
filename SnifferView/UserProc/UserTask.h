#pragma once
#include <Windows.h>
#include "../common/mstring.h"

#define TASK_OPEN_DUMP    "OpenDump"
#define TASK_SAVE_DUMP    "SaveDump"

class CUserTaskMgr {
public:
    static CUserTaskMgr *GetInst();
    void StartService(DWORD parent);
    std::mstring SendTask(const std::mstring &task, const std::mstring &param) const;

private:
    CUserTaskMgr();
    virtual ~CUserTaskMgr();
    std::mstring RunTaskInService(const std::mstring &taskType, const std::mstring &taskParam) const;
    void OnTask() const;
    static DWORD __stdcall ParentCheckThread(LPVOID param);

private:
    bool mStart;
    HANDLE mTaskThread;
    HANDLE mTaskNotify;
    DWORD mParentPid;
    bool mExitService;
};