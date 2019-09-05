#include <Windows.h>
#include "dumper.h"

#if defined _M_IX86
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

HMODULE g_hThisModule = NULL;

BOOL WINAPI DllMain(HMODULE hThisModule, DWORD dwReason, void* lpParam)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        g_hThisModule = hThisModule;
        DisableThreadLibraryCalls(hThisModule);
    }
    else if (DLL_PROCESS_DETACH == dwReason)
    {
    }
    return TRUE;
}