#pragma once
#include <Windows.h>
#include "../common/mstring.h"

#define TASK_OPEN_DUMP    "OpenDump"
#define TASK_SAVE_DUMP    "SaveDump"

class CUserTaskMgr {
public:
    static CUserTaskMgr *GetInst();
    void StartService();
    std::mstring SendTask(const std::mstring &task, const std::mstring &param) const;

private:
    CUserTaskMgr();
    virtual ~CUserTaskMgr();
    std::mstring RunTaskInService(const std::mstring &taskType, const std::mstring &taskParam) const;
    void OnTask() const;

private:
    bool mStart;
    HANDLE mTaskThread;
    HANDLE mTaskNotify;
};