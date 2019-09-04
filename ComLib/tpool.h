//thread pool lougd 2018 11-26
#ifndef TPOOL_DPMSG_H_H_
#define TPOOL_DPMSG_H_H_
#include <Windows.h>
#include <list>
#include <vector>
#include <string>
#include "LockBase.h"

using namespace std;

class ThreadRunable {
public:
    virtual ~ThreadRunable() {}
    virtual void run() = 0;
};

class ThreadPool : public CCriticalSectionLockable {
public:
    ThreadPool(int initCount = 1, int maxCount = 4);
    bool exec(ThreadRunable *runable);
    virtual ~ThreadPool();
    string toString();

private:
    bool CreateNewThread();

private:
    HANDLE m_hWorkNotify;
    HANDLE m_hExitNotify;
    int m_initCount;
    int m_curCount;
    int m_freeCount;
    int m_maxCount;
    list<ThreadRunable *> m_runable;
    vector<HANDLE> m_threadSet;
    static DWORD WINAPI ThreadProc(LPVOID pParam);
};
#endif //TPOOL_DPMSG_H_H_