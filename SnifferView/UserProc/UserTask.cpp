#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <list>
#include <string>
#include "UserTask.h"
#include "../common/common.h"
#include "../common/StrUtil.h"

#define EVENT_TASK_NOTIFY   "Global\\1a2c1501-6707-47be-a509-9e13ddab9e15_TaskNotify"
#define PATH_TASK_CACHE     "software\\snifferview\\UserTask"
#define PATH_TASK_RESULT    "software\\snifferview\\TaskResult"

using namespace std;

CUserTaskMgr *CUserTaskMgr::GetInst() {
    static CUserTaskMgr *sPtr = NULL;

    if (NULL == sPtr)
    {
        sPtr = new CUserTaskMgr();
    }
    return sPtr;
}

UINT_PTR CUserTaskMgr::OFNHookProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_INITDIALOG)
    {
        dp(L"init fileDlg");
        HWND parent = GetParent(hdlg);
        SetForegroundWindow(parent);
        CenterWindow(NULL, parent);

        SetWindowPos(parent, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    return 0;
}

mstring CUserTaskMgr::ShowOpenFileDlg(const mstring &defDlg) const {
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    char filename[MAX_PATH] = {0};
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter ="Packet File(*.vex)\0*.vex\0\0";
    ofn.lpstrDefExt = "vex";
    ofn.lpstrTitle = "打开封包数据文件";
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
    ofn.FlagsEx = OFN_EX_NOPLACESBAR;
    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.lpfnHook = OFNHookProc;
    ofn.hwndOwner = NULL;
    mstring file;
    if (GetOpenFileNameA(&ofn))  
    {
        dp(L"success 1, fileName:%hs", filename);
        return filename;
    } else {
        return RESULT_NULL;
    }
}

mstring CUserTaskMgr::ShowSaveFileDlg(const mstring &defName, const mstring &defDlg) const {
    OPENFILENAMEA ofn = { sizeof(ofn) };
    char filename[MAX_PATH] = {0};
    strcpy_s(filename, sizeof(filename), defName.c_str());
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter ="Packet File(*.vex)\0*.vex\0\0";  
    ofn.lpstrDefExt = "vex";
    ofn.lpstrTitle = "保存封包数据文件";
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;
    //if (defDlg.size() && IsDirectoryExist(defDlg.c_str()))
    //{
    //    ofn.lpstrInitialDir = defDlg.c_str();
    //}
    ofn.FlagsEx = OFN_EX_NOPLACESBAR;
    ofn.lpfnHook = OFNHookProc;
    ofn.hwndOwner = NULL;

    dp(L"test3");
    if (GetSaveFileNameA(&ofn))  
    {
        dp(L"success 2, fileName:%hs", filename);
        return filename;
    } else {
        dp(L"err2:%hs", GetStdErrorStr().c_str());
    }
    return RESULT_NULL;
}

mstring CUserTaskMgr::RunTaskInService(const mstring &taskType, const mstring &taskParam) const {
    mstring result;
    if (taskType == TASK_OPEN_DUMP)
    {
        return ShowOpenFileDlg(taskParam);
    } else if (taskType == TASK_SAVE_DUMP)
    {
        dp(L"save dump");
        SYSTEMTIME time;
        GetLocalTime(&time);
        mstring name;
        name.format("%04d%02d%02d%02d%02d%02d%03d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

        return ShowSaveFileDlg(name, taskParam);
    }
    return result;
}

void CUserTaskMgr::OnTask() const {
    HKEY hKey = NULL;
    if (ERROR_SUCCESS != RegOpenKeyA(HKEY_LOCAL_MACHINE, PATH_TASK_CACHE, &hKey))
    {
        return;
    }

    DWORD dwIdex = 0;
    char name[MAX_PATH] = {0x00};
    DWORD dwNameLen = MAX_PATH;
    DWORD dwStatus = 0;
    list<string> lst;
    while (TRUE)
    {
        dwNameLen = MAX_PATH;
        name[0] = 0;
        dwStatus = SHEnumKeyExA(hKey, dwIdex++, name, &dwNameLen);
        if (name[0])
        {
            lst.push_back(name);
        }

        if (ERROR_SUCCESS != dwStatus)
        {
            break;
        }

        if (lstrlenA("aabb-ccdd") == lstrlenA(name))
        {
            mstring task = RegGetStrValueA(hKey, name, "taskType");
            mstring param = RegGetStrValueA(hKey, name, "taskParam");
            mstring notify = RegGetStrValueA(hKey, name, "taskNotify");

            mstring result = RunTaskInService(task, param);
            dp(L"result:%hs", result.c_str());
            SHSetValueA(HKEY_LOCAL_MACHINE, PATH_TASK_RESULT, notify.c_str(), REG_SZ, result.c_str(), result.size());
            HANDLE complete = OpenEventA(EVENT_MODIFY_STATE, FALSE, notify.c_str());

            if (complete)
            {
                SetEvent(complete);
                CloseHandle(complete);
            }
        }
    }

    list<string>::iterator itm;
    for (itm = lst.begin() ; itm != lst.end() ; itm++)
    {
        SHDeleteKeyA(hKey, itm->c_str());
    }
    RegCloseKey(hKey);
}

void CUserTaskMgr::StartService(DWORD parentPid) {
    mParentPid = parentPid;
    mTaskNotify = CreateEventA(NULL, FALSE, FALSE, EVENT_TASK_NOTIFY);
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, mParentPid);

    if (!mTaskNotify || !process)
    {
        return;
    }

    HANDLE arry[] = {mTaskNotify, process};
    while (true) {
        DWORD ret = WaitForMultipleObjects(2, arry, FALSE, INFINITE);

        if (WAIT_OBJECT_0 == ret)
        {
            OnTask();
        } else if ((WAIT_OBJECT_0 + 1) == ret)
        {
            break;
        } else {
            break;
        }
    }

    CloseHandle(mTaskNotify);
    CloseHandle(process);
    mTaskNotify = NULL, process = NULL;
}

mstring CUserTaskMgr::SendTask(const mstring &task, const mstring &param) const {
    HANDLE notify = OpenEventA(EVENT_MODIFY_STATE, FALSE, EVENT_TASK_NOTIFY);

    if (!notify)
    {
        return "";
    }

    static DWORD sMagic = 0xf1;
    srand(sMagic++ + GetTickCount());
    mstring magic = FormatA(
        "%02x%02x-%02x%02x",
        rand() % 0xff,
        rand() % 0xff,
        rand() % 0xff,
        rand() % 0xff
        );

    mstring nameNotify = FormatA("Global\\%hs", magic.c_str());
    HANDLE complete = CreateLowsdEvent(FALSE, FALSE, AtoW(nameNotify).c_str());

    mstring path = FormatA("%hs\\%hs", PATH_TASK_CACHE, magic.c_str());
    SHSetValueA(HKEY_LOCAL_MACHINE, path.c_str(), "taskType", REG_SZ, task.c_str(), task.size());
    SHSetValueA(HKEY_LOCAL_MACHINE, path.c_str(), "taskParam", REG_SZ, param.c_str(), param.size());
    SHSetValueA(HKEY_LOCAL_MACHINE, path.c_str(), "taskNotify", REG_SZ, nameNotify.c_str(), nameNotify.size());
    SetEvent(notify);
    CloseHandle(notify);

    WaitForSingleObject(complete, INFINITE);
    CloseHandle(complete);
    string result = RegGetStrValueA(HKEY_LOCAL_MACHINE, PATH_TASK_RESULT, nameNotify.c_str());
    SHDeleteValueA(HKEY_LOCAL_MACHINE, PATH_TASK_RESULT, nameNotify.c_str());
    return result;
}

CUserTaskMgr::CUserTaskMgr() {
    mTaskThread = NULL;
    mTaskNotify = NULL;
    mParentPid = 0;
    mExitService = false;
}

CUserTaskMgr::~CUserTaskMgr() {
}

DWORD CUserTaskMgr::ParentCheckThread(LPVOID param) {
    CUserTaskMgr *pThis = (CUserTaskMgr *)param;
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pThis->mParentPid);

    WaitForSingleObject(process, INFINITE);
    pThis->mExitService = true;
    SetEvent(pThis->mTaskNotify);
    return 0;
}