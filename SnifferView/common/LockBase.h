#ifndef LOCKBASE_VDEBUG_H_H_
#define LOCKBASE_VDEBUG_H_H_
#include <Windows.h>

class CLockable
{
public:
    virtual void Lock() const = 0;
    virtual void Unlock() const = 0;
};

class CCriticalSectionLockable : public CLockable
{
public:
    CCriticalSectionLockable()
    {
        InitializeCriticalSection(&m_cs);
    }

    virtual ~CCriticalSectionLockable()
    {
        DeleteCriticalSection(&m_cs);
    }

    virtual void Lock() const
    {
        EnterCriticalSection(&m_cs);
    }

    virtual void Unlock() const
    {
        LeaveCriticalSection(&m_cs);
    }

private:
    mutable CRITICAL_SECTION m_cs;
};

class CScopedLocker
{
public:
    CScopedLocker(const CLockable *pLockable)
        : m_pLockable(pLockable)
        , m_pcs(NULL)
    {
        m_pLockable->Lock();
    }

    CScopedLocker(LPCRITICAL_SECTION lpCs)
        : m_pLockable(NULL)
        , m_pcs(lpCs)
    {
        EnterCriticalSection(m_pcs);
    }

    virtual ~CScopedLocker()
    {
        if (m_pLockable)
        {
            m_pLockable->Unlock();
        }
        else if (m_pcs)
        {
            LeaveCriticalSection(m_pcs);
        }
    }

private:
    CScopedLocker(const CScopedLocker &);
    CScopedLocker &operator=(const CScopedLocker &);

private:
    const CLockable *m_pLockable;
    LPCRITICAL_SECTION m_pcs;
};

#endif
