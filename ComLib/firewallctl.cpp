#include <windows.h>
#include <netfw.h>
#include <objbase.h>
#include <shlwapi.h>
#include <atlbase.h>

#pragma comment(lib, "shlwapi.lib")

HRESULT WindowsFirewallInitialize(OUT INetFwProfile** fwProfile)
{
    HRESULT hr = S_OK;
    INetFwMgr* fwMgr = NULL;
    INetFwPolicy* fwPolicy = NULL;

    *fwProfile = NULL;

    hr = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&fwMgr);

    if (SUCCEEDED(hr))
    {
        hr = fwMgr->get_LocalPolicy(&fwPolicy);

        if (SUCCEEDED(hr))
        {
            hr = fwPolicy->get_CurrentProfile(fwProfile);

            fwPolicy->Release();
        }

        fwMgr->Release();
    }

    return hr;
}

HRESULT WindowsFirewallInitializeEx(LPCWSTR lpImagePath, LPCWSTR lpfwName)
{
    HRESULT hr = S_OK;
    BOOL bSuccess = FALSE;
    INetFwPolicy2 *pNetFwPolicy2 = NULL;
    INetFwRules *pFwRules = NULL;
    INetFwRule *pFwRule = NULL;
    DWORD dwCurrentProfiles = 0;

    hr = CoCreateInstance(
        __uuidof(NetFwPolicy2),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2),
        (void**)&pNetFwPolicy2
        );

    if (SUCCEEDED(hr))
    {
        hr = pNetFwPolicy2->get_Rules(&pFwRules);

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(
                __uuidof(NetFwRule),
                NULL,
                CLSCTX_INPROC_SERVER,
                __uuidof(INetFwRule),
                (void**)&pFwRule
                );

            if (SUCCEEDED(hr))
            {
                BSTR bstrIterType = SysAllocString(L"All");
                BSTR bstrRuleName = SysAllocString(lpfwName);
                BSTR bstrFileName = SysAllocString(lpImagePath);

                if (bstrIterType != NULL && bstrRuleName != NULL && bstrFileName != NULL)
                {
                    pFwRule->put_Action(NET_FW_ACTION_ALLOW);
                    pFwRule->put_Name(bstrRuleName);
                    pFwRule->put_Enabled(VARIANT_TRUE);
                    pFwRule->put_InterfaceTypes(bstrIterType);
                    pFwRule->put_ApplicationName(bstrFileName);
                    pFwRule->put_Profiles(NET_FW_PROFILE2_DOMAIN | NET_FW_PROFILE2_PRIVATE | NET_FW_PROFILE2_PUBLIC);
                    hr = pFwRules->Add(pFwRule);

                    if (SUCCEEDED(hr))
                    {
                        bSuccess = TRUE;
                    }
                }

                if (bstrIterType != NULL)
                {
                    SysFreeString(bstrIterType);
                }

                if (bstrRuleName != NULL)
                {
                    SysFreeString(bstrRuleName);
                }

                if (bstrFileName != NULL)
                {
                    SysFreeString(bstrFileName);
                }

                if (pFwRule != NULL)
                {
                    pFwRule->Release();
                }
            }

            if (pFwRules != NULL)
            {
                pFwRules->Release();
            }
        }

        if (pNetFwPolicy2 != NULL)
        {
            pNetFwPolicy2->Release();
        }
    }

    return bSuccess;
}

HRESULT WindowsFirewallAppIsEnabled(
    IN INetFwProfile* fwProfile,
    IN const wchar_t* fwProcessImageFileName,
    OUT BOOL* fwAppEnabled
    )
{
    HRESULT hr = S_OK;
    VARIANT_BOOL fwEnabled;
    BSTR fwBstrProcessImageFileName = NULL;
    INetFwAuthorizedApplication* fwApp = NULL;
    INetFwAuthorizedApplications* fwApps = NULL;

    *fwAppEnabled = FALSE;

    hr = fwProfile->get_AuthorizedApplications(&fwApps);

    if (SUCCEEDED(hr))
    {
        fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);

        if (fwBstrProcessImageFileName != NULL)
        {
            hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp);

            if (SUCCEEDED(hr))
            {
                hr = fwApp->get_Enabled(&fwEnabled);

                if (SUCCEEDED(hr) && fwEnabled != VARIANT_FALSE)
                {
                    *fwAppEnabled = TRUE;
                }

                fwApp->Release();
            }
            else
            {
                hr = S_OK;
            }

            SysFreeString(fwBstrProcessImageFileName);
        }

        fwApps->Release();
    }

    return hr;
}

HRESULT WindowsFirewallInsertApp(
    IN INetFwProfile* fwProfile,
    IN const wchar_t* fwProcessImageFileName,
    IN const wchar_t* fwName
    )
{
    HRESULT hr = S_OK;
    BOOL fwAppEnabled;
    BSTR fwBstrName = NULL;
    BSTR fwBstrProcessImageFileName = NULL;
    INetFwAuthorizedApplication* fwApp = NULL;
    INetFwAuthorizedApplications* fwApps = NULL;

    hr = WindowsFirewallAppIsEnabled(fwProfile, fwProcessImageFileName, &fwAppEnabled);

    if (SUCCEEDED(hr) && !fwAppEnabled)
    {
        hr = fwProfile->get_AuthorizedApplications(&fwApps);

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(
                __uuidof(NetFwAuthorizedApplication),
                NULL,
                CLSCTX_INPROC_SERVER,
                __uuidof(INetFwAuthorizedApplication),
                (void**)&fwApp
                );
            if (SUCCEEDED(hr))
            {
                fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);

                if (fwBstrProcessImageFileName != NULL)
                {
                    hr = fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);

                    if (SUCCEEDED(hr))
                    {
                        fwBstrName = SysAllocString(fwName);

                        if (SysStringLen(fwBstrName) != 0)
                        {
                            hr = fwApp->put_Name(fwBstrName);

                            if (SUCCEEDED(hr))
                            {
                                hr = fwApps->Add(fwApp);
                            }

                            SysFreeString(fwBstrName);
                        }
                    }

                    SysFreeString(fwBstrProcessImageFileName);
                }

                fwApp->Release();
            }

            fwApps->Release();
        }
    }

    return hr;
}

HRESULT WindowsFirewallAddAppVista(LPCWSTR lpImagePath, LPCWSTR lpfwName)
{
    HRESULT hr = S_OK;
    BOOL bSuccess = FALSE;
    INetFwPolicy2 *pNetFwPolicy2 = NULL;
    INetFwRules *pFwRules = NULL;
    INetFwRule *pFwRule = NULL;
    DWORD dwCurrentProfiles = 0;

    hr = CoCreateInstance(
        __uuidof(NetFwPolicy2),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(INetFwPolicy2),
        (void**)&pNetFwPolicy2
        );

    if (SUCCEEDED(hr))
    {
        hr = pNetFwPolicy2->get_Rules(&pFwRules);

        if (SUCCEEDED(hr))
        {
            CComBSTR name_in(lpfwName);
            CComBSTR application_name_in(lpImagePath);
            CComPtr<INetFwRule> tmp_rule;

            hr = pFwRules->Item(name_in, &tmp_rule);
            if (SUCCEEDED(hr))
            {
                CComBSTR application_name;
                hr = tmp_rule->get_ApplicationName(&application_name);
                if (SUCCEEDED(hr))
                {
                    application_name.ToLower();
                    application_name_in.ToLower();
                    if (application_name == application_name_in)
                    {
                        if (pFwRules != NULL)
                        {
                            pFwRules->Release();
                        }
                        if (pNetFwPolicy2 != NULL)
                        {
                            pNetFwPolicy2->Release();
                        }

                        return TRUE;
                    }               
                }
            }

            hr = CoCreateInstance(
                __uuidof(NetFwRule),
                NULL,
                CLSCTX_INPROC_SERVER,
                __uuidof(INetFwRule),
                (void**)&pFwRule
                );

            if (SUCCEEDED(hr))
            {
                BSTR bstrIterType = SysAllocString(L"All");
                BSTR bstrRuleName = SysAllocString(lpfwName);
                BSTR bstrFileName = SysAllocString(lpImagePath);

                if (bstrIterType != NULL && bstrRuleName != NULL && bstrFileName != NULL)
                {
                    pFwRule->put_Action(NET_FW_ACTION_ALLOW);
                    pFwRule->put_Name(bstrRuleName);
                    pFwRule->put_Enabled(VARIANT_TRUE);
                    pFwRule->put_InterfaceTypes(bstrIterType);
                    pFwRule->put_ApplicationName(bstrFileName);
                    pFwRule->put_Profiles(NET_FW_PROFILE2_DOMAIN | NET_FW_PROFILE2_PRIVATE | NET_FW_PROFILE2_PUBLIC);
                    hr = pFwRules->Add(pFwRule);

                    if (SUCCEEDED(hr))
                    {
                        bSuccess = TRUE;
                    }
                }

                if (bstrIterType != NULL)
                {
                    SysFreeString(bstrIterType);
                }

                if (bstrRuleName != NULL)
                {
                    SysFreeString(bstrRuleName);
                }

                if (bstrFileName != NULL)
                {
                    SysFreeString(bstrFileName);
                }

                if (pFwRule != NULL)
                {
                    pFwRule->Release();
                }
            }

            if (pFwRules != NULL)
            {
                pFwRules->Release();
            }
        }

        if (pNetFwPolicy2 != NULL)
        {
            pNetFwPolicy2->Release();
        }
    }

    return bSuccess;
}

BOOL WINAPI WindowsFirewallAddAppW(LPCWSTR lpImagePath, LPCWSTR lpfwName)
{
    BOOL bSuccess = FALSE;
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr) && hr != RPC_E_CHANGED_MODE)
    {
        OSVERSIONINFO osi = {sizeof(osi)};
        GetVersionEx(&osi);

        if (osi.dwMajorVersion > 5)
        {
            if (WindowsFirewallAddAppVista(lpImagePath, lpfwName))
            {
                bSuccess = TRUE;
            }
        }
        else
        {
            INetFwProfile* fwProfile = NULL;

            hr = WindowsFirewallInitialize(&fwProfile);

            if (SUCCEEDED(hr))
            {
                hr = WindowsFirewallInsertApp(fwProfile, lpImagePath, lpfwName);

                if (SUCCEEDED(hr))
                {
                    bSuccess = TRUE;
                }

                fwProfile->Release();
            }
        }

        CoUninitialize();
    }

    return bSuccess;
}

BOOL WINAPI WindowsFirewallAddAppA(LPCSTR lpImagePath, LPCSTR lpfwName)
{
    WCHAR szImagePath[MAX_PATH] = {0};
    WCHAR szfwName[MAX_PATH] = {0};

    wnsprintfW(szImagePath, sizeof(szImagePath) / sizeof(WCHAR), L"%S", lpImagePath);
    wnsprintfW(szfwName, sizeof(szfwName) / sizeof(WCHAR), L"%S", lpfwName);

    return WindowsFirewallAddAppW(szImagePath, szfwName);
}