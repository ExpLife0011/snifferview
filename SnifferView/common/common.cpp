#pragma warning(disable: 4996)
#include <WinSock2.h>
#include <Windows.h>
#include <WtsApi32.h>
#include <AccCtrl.h>
#include <Aclapi.h.>
#include <Userenv.h>
#include <shlwapi.h>
#include <Psapi.h>
#include <set>
#include <list>
#include <stdarg.h>
#include <iphlpapi.h>
#include <CommCtrl.h>
#include <common.h>
#include <TlHelp32.h>
#include <mstring.h>
#include "StrUtil.h"

#pragma comment(lib, "WtsApi32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "Advapi32.lib")

using namespace std;

DWORD n2h_32(IN OUT DWORD v)
{
    return ntohl(v);
}

DWORD h2n_32(IN OUT DWORD v)
{
    return htonl(v);
}

short n2h_16(IN OUT short v)
{
    return ntohs(v);
}

short h2n_16(IN OUT short v)
{
    return htons(v);
}

unsigned short h2n_u16(unsigned short d) {
    const byte *ptr = (byte *)&d;
    unsigned short result;
    byte *p2 = (byte *)&result;
    p2[0] = ptr[1];
    p2[1] = ptr[0];
    return result;
}

unsigned short n2h_u16(unsigned short d) {
    return h2n_u16(d);
}

VOID CenterWindow(HWND hParent, HWND hChild)
{
    if (!hParent)
    {
        hParent = GetDesktopWindow();
    }

    RECT rt = {0};
    GetWindowRect(hParent, &rt);
    RECT crt = {0};
    GetWindowRect(hChild, &crt);
    int iX = 0;
    int iY = 0;
    int icW = crt.right - crt.left;
    int iW = rt.right - rt.left;
    int icH = crt.bottom - crt.top;
    int iH = rt.bottom - rt.top;
    iX = rt.left + (iW - icW) / 2;
    iY = rt.top + (iH - icH) / 2;

    dp(L"x:%d, y:%d, cx:%d, cy:%d", iX, iY, icW, icH);
    MoveWindow(hChild, iX, iY, icW, icH, TRUE);
}

VOID PrintDbgInternal(LPCWSTR wszTarget, LPCSTR szFile, DWORD dwLine, LPCWSTR wszFormat, ...)
{
    WCHAR wszFormat1[1024] = {0};
    WCHAR wszFormat2[1024] = {0};
    lstrcpyW(wszFormat1, L"[%ls][%hs.%d]%ls");
    StrCatW(wszFormat1, L"\n");
    wnsprintfW(wszFormat2, RTL_NUMBER_OF(wszFormat2), wszFormat1, wszTarget, szFile, dwLine, wszFormat);

    WCHAR wszLogInfo[2048];
    va_list vList;
    va_start(vList, wszFormat);
    wvnsprintfW(wszLogInfo, sizeof(wszLogInfo) / sizeof(WCHAR), wszFormat2, vList);
    va_end(vList);
    OutputDebugStringW(wszLogInfo);
}

BOOL WINAPI WriteFileBuffer(HANDLE file, const char *buffer, size_t length)
{
    DWORD write = 0;
    DWORD ms = 0;
    while(write < length)
    {
        ms = 0;
        if (!WriteFile(file, buffer + write, length - write, &ms, NULL))
        {
            return FALSE;
        }
        write += ms;
    }
    return TRUE;
}

//分析一个字符串是否是一个合法的ip格式
BOOL WINAPI CheckIpaddress(const char *buffer, DWORD &addr)
{
    int itm = 0;
    int wd = 0;
    int count = 0;
    int ts;
    unsigned int tmp = 0;
    char ct;
    while(*(buffer + itm) != 0x00)
    {
        ct = buffer[itm];
        if('.' == ct)
        {
            count++;
            if (0 == itm)
            {
                return FALSE;
            }
            else
            {
                if ('.' == buffer[itm - 1])
                {
                    return FALSE;
                }
            }

            if (0x00 == buffer[itm + 1])
            {
                return FALSE;
            }

            if (count > 3)
            {
                return FALSE;
            }
            tmp <<= 8;
            tmp += wd;
            wd = 0;
        }
        else
        {
            if (ct < '0' || ct > '9')
            {
                return FALSE;
            }

            ts = int(ct - '0');
            wd *= 10;
            wd += ts;
            if (wd > 255)
            {
                return FALSE;
            }
        }
        itm++;
    }

    if (3 == count)
    {
        tmp <<= 8;
        tmp += wd;
        addr = tmp;
        return TRUE;
    }
    return FALSE;
}

//检查一个字符串是不是一个合法的8位无符号整型数据
BOOL WINAPI CheckUnsignedN8(IN const char *buffer, OUT UCHAR &bt)
{
    int itm = 0;
    bool is16 = false;
    mstring str(buffer);
    str.makelower();
    if (!buffer || 0x00 == buffer[0])
    {
        return FALSE;
    }

    if (0 == str.find("0x"))
    {
        itm += 2;
        is16 = true;
    }

    int dt = 0;
    int vt = 0;
    for (; buffer[itm] != 0x00 ; itm++)
    {
        char ct = str.at(itm);
        if (is16)
        {
            if (ct < '0' || (ct > '9' && ct < 'a') || ct > 'f')
            {
                return FALSE;
            }

            if (ct >= 'a')
            {
                vt = (int)(ct - 'a' + 10);
            }
            else
            {
                vt = (int)(ct - '0');
            }
            dt = dt * 16 + vt;
            if (dt > 0xff)
            {
                return FALSE;
            }
        }
        else
        {
            if (ct < '0' || ct > '9')
            {
                return FALSE;
            }
            vt = (int)(ct - '0');
            dt = dt * 10 + vt;
            if (dt > 0xff)
            {
                return FALSE;
            }
        }
    }
    bt = (UCHAR)dt;
    return TRUE;
}

//分析一个字符串是不是一个合法的16位无符号数据
BOOL WINAPI CheckUnsignedN16(IN const char *buffer, OUT USHORT &ds)
{
    int itm = 0;
    bool is16 = false;
    mstring str(buffer);
    str.makelower();
    if (!buffer || 0x00 == buffer[0])
    {
        return FALSE;
    }

    if (0 == str.find("0x"))
    {
        itm += 2;
        is16 = true;
    }

    int dt = 0;
    int vt = 0;
    for (; buffer[itm] != 0x00 ; itm++)
    {
        char ct = str.at(itm);
        if (is16)
        {
            if (ct < '0' || (ct > '9' && ct < 'a') || ct > 'f')
            {
                return FALSE;
            }

            if (ct >= 'a')
            {
                vt = (int)(ct - 'a' + 10);
            }
            else
            {
                vt = (int)(ct - '0');
            }
            dt = dt * 16 + vt;
            if (dt > 0xffff)
            {
                return FALSE;
            }
        }
        else
        {
            if (ct < '0' || ct > '9')
            {
                return FALSE;
            }
            vt = (int)(ct - '0');
            dt = dt * 10 + vt;
            if (dt > 0xffff)
            {
                return FALSE;
            }
        }
    }
    ds = (unsigned short)dt;
    return TRUE;
}

//分析一个字符串是不是一个合法的32位无符号数据
BOOL WINAPI CheckUnsignedN32(const char *buffer, ULONG &ds)
{
    int itm = 0;
    bool is16 = false;
    mstring str(buffer);
    str.makelower();
    if (!buffer || 0x00 == buffer[0])
    {
        return FALSE;
    }

    if (0 == str.find("0x"))
    {
        itm += 2;
        is16 = true;
    }

    ULONGLONG dt = 0;
    int vt = 0;
    for (; buffer[itm] != 0x00 ; itm++)
    {
        char ct = str.at(itm);
        if (is16)
        {
            if (ct < '0' || (ct > '9' && ct < 'a') || ct > 'f')
            {
                return FALSE;
            }

            if (ct >= 'a')
            {
                vt = (int)(ct - 'a' + 10);
            }
            else
            {
                vt = (int)(ct - '0');
            }
            dt = dt * 16 + vt;
            if (dt > 0xffffffff)
            {
                return FALSE;
            }
        }
        else
        {
            if (ct < '0' || ct > '9')
            {
                return FALSE;
            }
            vt = (int)(ct - '0');
            dt = dt * 10 + vt;
            if (dt > 0xffffffff)
            {
                return FALSE;
            }
        }
    }
    ds = (unsigned int)dt;
    return TRUE;
}

//获取以指定符号分割的字符串集合
VOID WINAPI GetCutBufferList(IN const char *buffer, IN char ct, OUT list<mstring> &lst)
{
    mstring str(buffer);
    mstring tmp;
    int itm = -1;
    int begin = 0;
    while(mstring::npos != (itm = str.find(ct, itm + 1)))
    {
        if (itm > begin)
        {
            tmp.assign(str, begin, itm - begin);
            lst.push_back(tmp);    
        }
        begin = itm + 1;
    }

    if (str.size() > (size_t)begin)
    {
        tmp.assign(str, begin, str.size() - begin);
        lst.push_back(tmp);
    }
}

//获取网卡列表的详细信息
BOOL WINAPI GetAdapterMsg(OUT vector<AdapterMsg> &nets)
{
    BOOL state = FALSE;
    PIP_ADAPTER_INFO adapterInfo = NULL;
    PIP_ADAPTER_INFO adapter = NULL;
    ULONG length = sizeof(IP_ADAPTER_INFO);
    adapterInfo = (PIP_ADAPTER_INFO)malloc(sizeof(IP_ADAPTER_INFO));
    DWORD ret = GetAdaptersInfo(adapterInfo, &length);
    do 
    {
        if (ERROR_SUCCESS != ret)
        {
            if (ERROR_BUFFER_OVERFLOW == ret)
            {
                adapterInfo = (PIP_ADAPTER_INFO)realloc(adapterInfo, length);
            }
            else
            {
                break;
            }
        }

        if ((GetAdaptersInfo(adapterInfo, &length)) != NO_ERROR)
        {
            break;
        }
        adapter = adapterInfo;
        while (adapter)
        {
            AdapterMsg tmp;
            tmp.m_idex = adapter->ComboIndex;
            tmp.m_name = adapter->AdapterName;
            tmp.m_desc = adapter->Description;
            mstring vt;
            size_t i = 0;
            for (i = 0; i < adapter->AddressLength; i++)
            {
                if (i == (adapter->AddressLength - 1))
                {
                    vt.format("%.2X", (int)adapter->Address[i]);
                }
                else
                {
                    vt.format("%.2X-", (int) adapter->Address[i]);
                }
                tmp.m_mac += vt;
            }
            tmp.m_idex = adapter->Index;
            switch (adapter->Type)
            {
            case MIB_IF_TYPE_OTHER:
                tmp.m_type = "其它";
                break;
            case MIB_IF_TYPE_ETHERNET:
                tmp.m_type = "以太网";
                break;
            case MIB_IF_TYPE_TOKENRING:
                tmp.m_type = "令牌环";
                break;
            case MIB_IF_TYPE_FDDI:
                tmp.m_type = "FDDI";
                break;
            case MIB_IF_TYPE_PPP:
                tmp.m_type = "PPP";
                break;
            case MIB_IF_TYPE_LOOPBACK:
                tmp.m_type = "回路";
                break;
            case MIB_IF_TYPE_SLIP:
                tmp.m_type = "Slip";
                break;
            default:
                tmp.m_type = "未知网卡类型";
                break;
            }
            //ip
            tmp.m_ip = adapter->IpAddressList.IpAddress.String;
            //mask
            tmp.m_mask = adapter->IpAddressList.IpMask.String;
            //网关
            tmp.m_gateway = adapter->GatewayList.IpAddress.String;

            if (adapter->DhcpEnabled)
            {
                tmp.m_dhcp_enable = true;
            }
            else
            {
                tmp.m_dhcp_enable = false;
            }
            adapter = adapter->Next;
            nets.push_back(tmp);
            state = TRUE;
        }
    } while (FALSE);

    if (adapterInfo)
    {
        free(adapterInfo);
    }
    return state;
}

BOOL WINAPI InitEveryMutexACL(IN OUT SECURITY_ATTRIBUTES &sa, IN OUT SECURITY_DESCRIPTOR &sd)
{
    if (!InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION))
    {
        return FALSE;
    }

    if (!SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE))
    {
        return FALSE;
    }

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = &sd;
    return TRUE;
}

//判定当前进程是否具有administrator权限
BOOL WINAPI IsAdminUser()    
{
    char buffer[MAX_PATH];
    size_t length = MAX_PATH;
    GetSystemDirectoryA(buffer, length);
    srand(GetTickCount());
    PathAppendA(buffer, mstring().format("%08x%08x", rand(), rand()).c_str());
    HANDLE file = CreateFileA(buffer, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, NULL, NULL);
    if (file && INVALID_HANDLE_VALUE != file)
    {
        CloseHandle(file);
        DeleteFileA(buffer);
        return TRUE;
    }
    return FALSE;
}

BOOL IsDirectoryExist(const char *dir)   
{  
    if (!dir || 0x00 == dir[0])
    {
        return FALSE;
    }
    BOOL state = FALSE;
    WIN32_FIND_DATA fileinfo;
    mstring ms(dir);
    if((dir[ms.size() - 1] == '//') || (dir[ms.size() - 1] == '/'))   
    {   
        ms.at(ms.size() - 1) = 0x00;
    }   
    HANDLE find  = FindFirstFile(ms.c_str(), &fileinfo); 
    do 
    {
        if(find == INVALID_HANDLE_VALUE)   
        {   
            break;
        }   

        if(fileinfo.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)   
        {   
            state  = TRUE;
            break;
        }   
    } while (FALSE);
    
    if (find && INVALID_HANDLE_VALUE != find)
    {
        FindClose(find);   
    }
    return state;   
}

BOOL GetFileVersion(IN LPCSTR pFile, OUT mstring& ver)   
{   
    LPSTR pBuf = NULL;
    CHAR szVersionBuffer[4096] = {0x00};   
    DWORD dwVerSize = 0;   
    DWORD dwHandle = 0;  
    DWORD dwLength = 4096;
    BOOL bState = FALSE;

    dwVerSize = GetFileVersionInfoSizeA(pFile, &dwHandle);   
    if (dwVerSize > dwLength)
    {
        pBuf = new CHAR[dwVerSize];
        dwLength = dwVerSize;
    }
    else
    {
        pBuf = szVersionBuffer;
    }

    if (GetFileVersionInfoA(pFile, 0, dwLength, pBuf))   
    {   
        VS_FIXEDFILEINFO * pInfo = NULL;   
        unsigned int nInfoLen = 0;   
        if (VerQueryValueA(pBuf, "\\", (void**)&pInfo, &nInfoLen))  
        {  
            wnsprintfA(pBuf, dwLength, "%d.%d.%d.%d", HIWORD(pInfo->dwFileVersionMS), LOWORD(pInfo->dwFileVersionMS), HIWORD(pInfo->dwFileVersionLS), LOWORD(pInfo->dwFileVersionLS));
            ver = szVersionBuffer;
            bState = TRUE;   
        }   
    } 

    if (pBuf != szVersionBuffer)
    {
        delete []pBuf;
    }
    return bState;   
}

/*
*检查某种类型的文件是否已经注册
*ext:    文件扩展名 eg:.txt
*key:    扩展名在注册表里的键值 eg:txtfile
*/
BOOL WINAPI CheckFileRelation(IN const char *ext, IN const char * key)
{
    BOOL ret = FALSE;
    HKEY hkey = NULL;
    char path[_MAX_PATH]; 
    DWORD length = sizeof(path);

    if(ERROR_SUCCESS == RegOpenKeyA(HKEY_CLASSES_ROOT, ext, &hkey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey,NULL,NULL,NULL,(LPBYTE)path,&length))
        {
            ret = (mstring(path) == key);
        }
    }

    if (hkey)
    {
        RegCloseKey(hkey);
    }
    return ret;
}

/*
*为一种特定的文件类型注册关联
*ext:    文件扩展名 eg:.txt
*cmd:    要关联的文件命令行 eg:d:/xxx.exe -f
*key:    扩展名在注册表中的键值 eg:txtfile
*ico:    文件图标，d:/xxx.exe, 0
*dec:    文件类型描述
*/
BOOL WINAPI RegisterFileRelation(IN const char *ext, IN const char *cmd, IN const char *key, IN const char *ico, IN const char *dec)
{
    mstring tmp;
    BOOL ret = FALSE;
    do 
    {
        if (!ext  || !cmd || !key || !ico || !dec)
        {
            break;
        }

        if (ERROR_SUCCESS != SHSetValueA(HKEY_CLASSES_ROOT, ext, "", REG_SZ, key, lstrlenA(key) + 1))
        {
            break;
        }

        if (ERROR_SUCCESS != SHSetValueA(HKEY_CLASSES_ROOT, key, "", REG_SZ, dec, lstrlenA(dec) + 1))
        {
            break;
        }

        tmp = key;
        tmp += "\\DefaultIcon";
        if (ERROR_SUCCESS != SHSetValueA(HKEY_CLASSES_ROOT, tmp.c_str(), "", REG_SZ, ico, lstrlenA(ico) + 1))
        {
            break;
        }

        tmp = key;
        tmp += "\\Shell";
        if (ERROR_SUCCESS != SHSetValueA(HKEY_CLASSES_ROOT, tmp.c_str(), "", REG_SZ, "Open", lstrlenA("Open") + 1))
        {
            break;
        }

        tmp = key;
        tmp += "\\Shell\\Open\\Command";

        mstring vv;
        vv.format("%hs \"%%1\"", cmd);
        if (ERROR_SUCCESS != SHSetValueA(HKEY_CLASSES_ROOT, tmp.c_str(), "", REG_SZ, vv.c_str(), vv.size()))
        {
            break;
        }
        ret = TRUE;
    } while (FALSE);
    return ret;
}

//从PE的资源中释放文件
BOOL ReleaseRes(const char *path, DWORD id, const char *type)  
{
    HRSRC src = NULL;
    HGLOBAL hg = NULL;
    HANDLE file = INVALID_HANDLE_VALUE;
    BOOL state = FALSE;
    do 
    {
        src = FindResourceA(NULL, MAKEINTRESOURCEA(id), type);
        if (!src)
        {
            break;
        }

        hg = LoadResource(NULL, src);
        if (!hg)
        {
            break;
        }

        DWORD size = SizeofResource(NULL, src);  
        file = CreateFileA(
            path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            0,
            NULL
            );
        if (!file || INVALID_HANDLE_VALUE == file)
        {
            break;
        }
        
        if (size <= 0)
        {
            break;
        }

        if(!WriteFileBuffer(file, (const char *)hg, size))
        {
            break;
        }
        state = TRUE;
    } while (FALSE);

    if(hg)
    {
        FreeResource(hg);
    }
    
    if (file && INVALID_HANDLE_VALUE != file)
    {
        CloseHandle(file);
    }
    return TRUE;  
}

BOOL IsWow64System()
{
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    static LPFN_ISWOW64PROCESS s_fnIsWow64Process = NULL;
    if (!s_fnIsWow64Process)
    {
        s_fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
    }
    if (!s_fnIsWow64Process)
    {
        return FALSE;
    }
    BOOL ret = FALSE;
    s_fnIsWow64Process(GetCurrentProcess(),&ret);
    return ret;
}

//根据ListCtrl的列宽总和调整所在对话框的宽度, 列宽中不能有宽度为0的
VOID ResetWidthByColumns(HWND hdlg, HWND list)
{
    int length = 0;
    int itm = 0;
    int xx = 0;
    while((xx = SendMessageA(list, LVM_GETCOLUMNWIDTH, itm, 0)) > 0)
    {
        length += xx;
        itm++;
    }
    RECT rt;
    GetWindowRect(hdlg, &rt);
    RECT rs;
    GetWindowRect(list, &rs);
    int mv = rs.left - rt.left;
    if (rt.right - rt.left < length + 30)
    {
        SetWindowPos(list, 0, 0, 0, length + 30, rs.bottom - rs.top, SWP_NOZORDER | SWP_NOMOVE);
        SetWindowPos(hdlg, 0, 0, 0, length + 30 + mv * 2, rt.bottom - rt.top, SWP_NOZORDER | SWP_NOMOVE);
    }
}

//Dos文件路径转为Nt路径
BOOL DosPathToNtPath(IN const char *src, OUT mstring &dst)
{
    DWORD drivers = GetLogicalDrives();
    int flag = 1;
    int itm = 0;
    char nt[] = "X:";
    char dos[MAX_PATH] = {0x00};
    mstring ms;
    mstring mv(src);
    mstring vv(src);
    mv.makelower();
    for (itm = 0 ; itm < 26 ; itm++)
    {
        if ((flag << itm) & drivers)
        {
            nt[0] = 'A' + itm;
            if (QueryDosDevice(nt, dos, MAX_PATH))
            {
                ms = dos;
                ms.makelower();
                if (0 == mv.find(ms))
                {
                    dst.clear();
                    dst += nt;
                    dst += vv.c_str() + lstrlenA(dos);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

//从进程Pid获取进程的信息
BOOL GetProcessByPid(IN DWORD pid, OUT mstring &path)
{
    BOOL ret = FALSE;
    if (0 == pid)
    {
        path = "[System Idle Process]";
        return TRUE;
    }
    else if (4 == pid)
    {
        path = "System";
        return TRUE;
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    do 
    {
        char ms[MAX_PATH] = {0x00};
        if (process)
        {    
            GetProcessImageFileNameA(process, ms, MAX_PATH);
            int itm = 0;
            if (ms[4] != 0x00)
            {
                if (DosPathToNtPath(ms, path))
                {
                    ret = TRUE;
                    break;
                }
            }
        }
        else
        {
            //可能是没有权限,这里可能需要一个我们的服务进程
            //OutputDebugStringA("get process message error");
        }
    } while (FALSE);

    if (process && INVALID_HANDLE_VALUE != process)
    {
        CloseHandle(process);
    }
    return ret;
}

static BOOL WINAPI FuzzyMatch(IN const char *src, IN const char *dst)
{
    int itm = 0;
    int itn = 0;
    int tmp_a = 0;
    int tmp_b = 0;

    if (0x00 == src[0])
    {
        return TRUE;
    }

    while(src[itm] && dst[itn])
    {
        tmp_a = itm;
        tmp_b = itn;
        while(src[itm] && dst[itn])
        {
            if ('*' == src[itm])
            {
                itm++;
                if (0x00 == src[itm])
                {
                    return TRUE;
                }
                continue;
            }

            if (src[itm] == dst[itn] || '?' == src[itm])
            {
                itm++, itn++;
            }
            else if (src[itm] != dst[itn])
            {
                itn++;
            }
        }

        if (0x00 ==dst[itn])
        {
            while('*' == src[itm])
            {
                itm++;
            }
        }

        if (0x00 == src[itm] && 0x00 == dst[itn])
        {
            return TRUE;
        }
        else
        {
            itm = tmp_a;
            itn = tmp_b + 1;
        }
    }
    return (src[itm] == dst[itn]);
}

//字符串匹配支持*，？的匹配
BOOL WINAPI StringMatch(IN const char *src, IN const char *dst)
{
    bool x = false;
    int itm = 0;
    int itn = 0;
    while(src[itm] && dst[itn])
    {
        if ('*' == src[itm])
        {
            return FuzzyMatch(src + itm + 1, dst + itn);
        }

        if ('?' == src[itm] || src[itm] == dst[itn])
        {
            itm++, itn++;
            continue;
        }
        return FALSE;    
    }
    return (src[itm] == dst[itn]);
}

//获取pe文件属性
BOOL GetFileString(IN const char *path, IN const char *attr, OUT mstring &msg)
{
    struct LanguagePage
    {
        WORD m_language;
        WORD m_page;
    };

    if (!path || !attr || 0x00 == path[0] || 0x00 == attr[0])
    {
        return FALSE;
    }
    char buffer[2096] = {0x00};
    char *ms = buffer;
    DWORD size = 0; 
    DWORD ret = 0; 
    //获取版本信息大小
    size = GetFileVersionInfoSizeA(path,NULL); 
    if (size == 0) 
    { 
        return FALSE;
    }

    if (size > sizeof(buffer))
    {
        ms = new char[size + 1];
    }
    memset(ms, 0, size + 1);
    //获取版本信息
    ret = GetFileVersionInfoA(path, NULL, size, ms);
    LPVOID mv = NULL;
    BOOL res = FALSE;
    UINT len = 0;
    do 
    {
        if(ret == 0) 
        { 
            break; 
        }
        
        len = 0;
        if (0 == VerQueryValueA(ms, "\\VarFileInfo\\Translation", &mv, &len))
        {
            break;
        }
        UINT itm = 0;
        for (itm = 0 ; itm < (len / sizeof(LanguagePage)) ; itm++)
        {
            LanguagePage *uu = ((LanguagePage *)(mv) + itm);
            const char *data = NULL;
            UINT count = 0;
            mstring vv;
            vv.format("\\StringFileInfo\\%04x%04x\\%hs", uu->m_language, uu->m_page, attr);
            if (0 != VerQueryValueA(ms, vv.c_str(), (LPVOID *)(&data), &count))
            {
                if (count > 0)
                {
                    msg = data;
                    res = TRUE;
                    break;
                }
            }
        }
    } while (FALSE);

    if (ms && ms != buffer)
    {
        delete []ms;
    }
    return ret;
}

//由位图转换成cur的接口
static VOID GetMaskBitmaps(HBITMAP bmp, COLORREF mask, HBITMAP &and, HBITMAP &xor)
{
    HDC hdc = GetDC(NULL);
    HDC mdc = CreateCompatibleDC(hdc); 
    HDC ma = CreateCompatibleDC(hdc); 
    HDC mx = CreateCompatibleDC(hdc); 
    BITMAP bm;
    GetObject(bmp, sizeof(BITMAP),&bm);
    and = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);
    xor = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);

    //Select the bitmaps to DC  
    HBITMAP old = (HBITMAP)SelectObject(mdc, bmp);  
    HBITMAP old_and   = (HBITMAP)::SelectObject(ma, and);  
    HBITMAP old_xor   = (HBITMAP)::SelectObject(mx, xor);  

    COLORREF bits;
    int itm = 0;
    int itk = 0;
    for(itm = 0 ; itm < bm.bmWidth ; ++itm)
    {
        for(itk = 0 ; itk < bm.bmHeight ; ++itk)
        {
            bits = GetPixel(mdc, itm, itk);
            if(bits == mask)
            {
                SetPixel(ma, itm, itk, RGB(255,255,255));
                SetPixel(mx, itm, itk, RGB(0,0,0));
            }
            else
            {
                SetPixel(ma, itm, itk,RGB(0,0,0));
                SetPixel(mx, itm, itk, bits);
            }
        }
    }

    SelectObject(mdc,old);
    SelectObject(ma, old_and);  
    SelectObject(mx, old_xor);
    DeleteDC(ma);
    DeleteDC(mx);
    DeleteDC(mdc);
    ReleaseDC(NULL, hdc);
}

//mask是颜色的掩码
HCURSOR CreateCursorFromBitmap(HBITMAP bmp, COLORREF mask)
{
    HCURSOR cur = NULL;
    do
    {
        if(NULL == bmp)
        {
            break;
        }
        HBITMAP and = NULL;
        HBITMAP xor = NULL;
        GetMaskBitmaps(bmp,mask,and,xor);
        if(NULL == and || NULL == xor)
        {
            break;
        }

        ICONINFO iconinfo = {0};
        iconinfo.fIcon        = FALSE;
        iconinfo.xHotspot    = 0;
        iconinfo.yHotspot    = 0;
        iconinfo.hbmMask    = and;
        iconinfo.hbmColor    = xor;
        cur = CreateIconIndirect(&iconinfo);
    }
    while(0);
    return cur;
}

// 注意，此函数的第二个参数很畸形，如果在本函数内声明 PACL* 并且释放的话，则安全描述符失效
// 为了应对此问题，需要调用方提供 PACL* 并在调用完此函数后自行释放内存
static BOOL WINAPI _SecGenerateLowSD(SECURITY_DESCRIPTOR* pSecDesc, PACL* pDacl)
{
    PSID pSidWorld = NULL;
    EXPLICIT_ACCESS ea;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_WORLD_SID_AUTHORITY;
    BOOL bRet = FALSE;

    do
    {
        if (!pSecDesc || !pDacl)
        {
            break;
        }

        if (AllocateAndInitializeSid(&sia, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSidWorld) == 0)
        {
            break;
        }

        ea.grfAccessMode = GRANT_ACCESS;
        ea.grfAccessPermissions = FILE_ALL_ACCESS ;
        ea.grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
        ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        ea.Trustee.pMultipleTrustee = NULL;
        ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea.Trustee.ptstrName = (LPTSTR)pSidWorld;

        if (SetEntriesInAcl(1, &ea, NULL, pDacl) != ERROR_SUCCESS)
        {
            break;
        }

        if (InitializeSecurityDescriptor(pSecDesc, SECURITY_DESCRIPTOR_REVISION) == 0)
        {
            break;
        }

        if (SetSecurityDescriptorDacl(pSecDesc, TRUE, *pDacl, FALSE) == 0)
        {
            break;
        }

        bRet = TRUE;
    } while (FALSE);

    if (NULL != pSidWorld)
    {
        FreeSid(pSidWorld);
        pSidWorld = NULL;
    }
    return bRet;
}

HANDLE CreateLowsdEvent(BOOL bReset, BOOL bInitStat, LPCWSTR wszName)
{
    SECURITY_DESCRIPTOR secDesc;
    PACL pDacl = NULL;
    SECURITY_ATTRIBUTES secAttr;
    secAttr.nLength = sizeof(secAttr);
    secAttr.lpSecurityDescriptor = &secDesc;
    secAttr.bInheritHandle = FALSE;
    if (!_SecGenerateLowSD(&secDesc, &pDacl))
    {
        if (pDacl)
        {
            LocalFree(pDacl);
        }
        return FALSE;
    }

    HANDLE hEvent = CreateEventW(&secAttr, bReset, bInitStat, wszName);
    if (pDacl)
    {
        LocalFree(pDacl);
    }
    return hEvent;
}

static DWORD _GetCurSessionId()
{
    WTS_SESSION_INFOW *pSessions = NULL;
    DWORD dwSessionCount = 0;
    DWORD dwActiveSession = -1;
    WTSEnumerateSessionsW(
        WTS_CURRENT_SERVER_HANDLE,
        0,
        1,
        &pSessions,
        &dwSessionCount
        );
    DWORD dwIdex = 0;
    for (dwIdex = 0 ; dwIdex < dwSessionCount ; dwIdex++)
    {
        if (WTSActive == pSessions[dwIdex].State)
        {
            dwActiveSession = pSessions[dwIdex].SessionId;
            break;
        }
    }
    if (pSessions)
    {
        WTSFreeMemory(pSessions);
    }

    return dwActiveSession;
}

static HANDLE _GetProcessToken(DWORD dwPid)
{
    BOOL bRet = FALSE;
    HANDLE hToken = NULL;
    HANDLE hDup = NULL;
    HANDLE hProcess = NULL;
    do
    {
        if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid)))
        {
            break;
        }

        if (!OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken))
        {
            break;
        }

        if (!DuplicateTokenEx(
            hToken,
            MAXIMUM_ALLOWED,
            NULL,
            SecurityIdentification,
            TokenPrimary,
            &hDup
            ))
        {
            break;
        }
    } while(FALSE);

    if (hToken)
    {
        CloseHandle(hToken);
    }

    if (hProcess)
    {
        CloseHandle(hProcess);
    }
    return hDup;
}

BOOL RunInSession(LPCWSTR wszImage, LPCWSTR wszCmd, DWORD dwSessionId, DWORD dwShell)
{
    dp(L"RunInSession，进程路径:%ls", wszImage);
    if (!wszImage || !*wszImage)
    {
        return FALSE;
    }

    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(wszImage))
    {
        return FALSE;
    }

    BOOL bStat = FALSE;
    HANDLE hThis = NULL;
    HANDLE hDup = NULL;
    LPVOID pEnv = NULL;
    HANDLE hShellToken = NULL;
    DWORD  dwFlag = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;
    do
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hThis))
        {
            break;
        }

        if (!DuplicateTokenEx(hThis, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hDup))
        {
            break;
        }

        if (!dwSessionId)
        {
            dwSessionId = _GetCurSessionId();
        }

        if (-1 == dwSessionId)
        {
            dp(L"未找到活动session");
            break;
        }
        dp(L"在session中启动进程,Session：%d", dwSessionId);
        if (!SetTokenInformation(hDup, TokenSessionId, &dwSessionId, sizeof(DWORD)))
        {
            dp(L"设置进程session属性失败");
            break;
        }

        if (dwShell)
        {
            hShellToken = _GetProcessToken(dwShell);
            CreateEnvironmentBlock(&pEnv, hShellToken, FALSE);
        }
        else
        {
            CreateEnvironmentBlock(&pEnv, hDup, FALSE);
        }

        WCHAR wszParam[1024] = {0};
        if (wszCmd && wszCmd[0])
        {
            wnsprintfW(wszParam, 1024, L"\"%ls\" \"%ls\"", wszImage, wszCmd);
        }
        else
        {
            wnsprintfW(wszParam, 1024, L"\"%ls\"", wszImage);
        }
        STARTUPINFOW si = {sizeof(si)};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(STARTUPINFO);
        si.lpDesktop = L"WinSta0\\Default";
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = TRUE;
        bStat = CreateProcessAsUserW(
            hDup,
            NULL,
            wszParam,
            NULL,
            NULL,
            FALSE,
            dwFlag,
            pEnv,
            NULL,
            &si,
            &pi
            );
        if (pi.hProcess)
        {
            CloseHandle(pi.hProcess);
        }

        if (pi.hThread)
        {
            CloseHandle(pi.hThread);
        }

        if (!bStat)
        {
            dp(L"运行程序失败%d", GetLastError());
        }
    } while (FALSE);

    if (pEnv)
    {
        DestroyEnvironmentBlock(pEnv);
        pEnv = NULL;
    }

    if (hThis && INVALID_HANDLE_VALUE != hThis)
    {
        CloseHandle(hThis);
        hThis = NULL;
    }

    if (hShellToken && INVALID_HANDLE_VALUE != hShellToken)
    {
        CloseHandle(hShellToken);
        hShellToken = NULL;
    }

    if (hDup && INVALID_HANDLE_VALUE != hDup)
    {
        CloseHandle(hDup);
        hDup = NULL;
    }
    return TRUE;
}

BOOL IsSameFileW(LPCWSTR file1, LPCWSTR file2)
{
    BOOL bRet = FALSE;
    HANDLE hFile1 = INVALID_HANDLE_VALUE;
    HANDLE hFile2 = INVALID_HANDLE_VALUE;

    do
    {
        if (!file1 || !file2)
        {
            break;
        }

        // 同一个文件名则视为相同
        if (0 == StrCmpIW(file1, file2))
        {
            bRet = TRUE;
            break;
        }

        hFile1 = CreateFileW(
            file1,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (INVALID_HANDLE_VALUE == hFile1)
        {
            break;
        }

        hFile2 = CreateFileW(
            file2,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if (INVALID_HANDLE_VALUE == hFile2)
        {
            break;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile1, &fileSize))
        {
            break;
        }

        LARGE_INTEGER fileSize2;
        if (!GetFileSizeEx(hFile2, &fileSize2))
        {
            break;
        }

        // 文件大小不同则自然不会相同
        if (fileSize2.QuadPart != fileSize.QuadPart)
        {
            break;
        }

        LONGLONG curReaded = 0;
        char byte1[4096];
        char byte2[4096];
        DWORD readed1;
        DWORD readed2;
        BOOL bSame = TRUE;

        while (curReaded < fileSize.QuadPart)
        {
            if (!ReadFile(hFile1, byte1, 4096, &readed1, NULL))
            {
                bSame = FALSE;
                break;
            }

            if (!ReadFile(hFile2, byte2, 4096, &readed2, NULL))
            {
                bSame = FALSE;
                break;
            }

            if (readed1 != readed2)
            {
                bSame = FALSE;
                break;
            }

            if (memcmp(byte1, byte2, readed1))
            {
                bSame = FALSE;
                break;
            }

            curReaded += readed1;
        }

        bRet = bSame;
    } while (FALSE);

    if (INVALID_HANDLE_VALUE != hFile2)
    {
        CloseHandle(hFile2);
    }

    if (INVALID_HANDLE_VALUE != hFile1)
    {
        CloseHandle(hFile1);
    }

    return bRet;
}

typedef struct _SERVICE_HANDLE_PACK
{
    SC_HANDLE scManager;
    SC_HANDLE serviceHandle;
} SERVICE_HANDLE_PACK, *PSERVICE_HANDLE_PACK;

static void WINAPI _FreeServiceHandlePack(PSERVICE_HANDLE_PACK pHandlePack)
{
    if (pHandlePack)
    {
        if (pHandlePack->serviceHandle)
        {
            CloseServiceHandle(pHandlePack->serviceHandle);
        }

        if (pHandlePack->scManager)
        {
            CloseServiceHandle(pHandlePack->scManager);
        }

        free((void*)pHandlePack);
    }
}

static PSERVICE_HANDLE_PACK WINAPI _GdServGetServiceHandle(LPCWSTR wszServName, DWORD dwDesiredAccess)
{
    BOOL bSucc = FALSE;
    PSERVICE_HANDLE_PACK ret = NULL;

    do
    {
        SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (!scManager)
        {
            break;
        }

        ret = (PSERVICE_HANDLE_PACK)malloc(sizeof(SERVICE_HANDLE_PACK));
        if (!ret)
        {
            CloseServiceHandle(scManager);
            break;
        }

        ret->scManager = scManager;
        if (!wszServName)
        {
            ret->serviceHandle = NULL;
            bSucc = TRUE;
            break;
        }

        ret->serviceHandle = OpenServiceW(scManager, wszServName, dwDesiredAccess);
        if (!ret->serviceHandle)
        {
            break;
        }

        bSucc = TRUE;
    } while (FALSE);

    if (!bSucc)
    {
        _FreeServiceHandlePack(ret);

        ret = NULL;
    }

    return ret;
}

BOOL ServControlW(LPCWSTR servName, DWORD controlCode)
{
    BOOL bRet = FALSE;
    PSERVICE_HANDLE_PACK pHandlePack = NULL;

    do
    {
        pHandlePack = _GdServGetServiceHandle(servName, SERVICE_STOP);
        if (!pHandlePack)
        {
            break;
        }

        SERVICE_STATUS status = {0};
        bRet = ControlService(pHandlePack->serviceHandle, controlCode, &status);
    } while (FALSE);

    _FreeServiceHandlePack(pHandlePack);

    return bRet;
}


BOOL ServStopW(LPCWSTR servName)
{
    return ServControlW(servName, SERVICE_CONTROL_STOP) || (GetLastError() == ERROR_SERVICE_NOT_ACTIVE);
}

BOOL ServStartW(LPCWSTR servName)
{
    BOOL bRet = FALSE;
    PSERVICE_HANDLE_PACK pHandlePack = NULL;

    do
    {
        pHandlePack = _GdServGetServiceHandle(servName, SERVICE_START);
        if (!pHandlePack)
        {
            break;
        }

        bRet = StartService(pHandlePack->serviceHandle, 0, NULL) || (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING);
    } while (FALSE);

    _FreeServiceHandlePack(pHandlePack);

    return bRet;
}

mstring Int32ToIp(unsigned int addr, bool changeOrder) {
    if (addr != 0)
    {
        int dd = 123;
    }

    mstring str;
    for (int i = 0 ; i < 4 ; i++) {
        if (!str.empty())
        {
            str += ".";
        }

        if (changeOrder)
        {
            str += (mstring().format("%d", (int)(addr >> (i * 8)) & 0xff));
        } else {
            str += mstring().format("%d", (int)((addr >> (3 - i) * 8) & 0xff));
        }
    }
    return str;
}

mstring GetPrintStr(const char *szBuffer, int iSize, bool mulitLine)
{
    mstring strOut;
    for (int i = 0 ; i < iSize ;)
    {
        byte letter = szBuffer[i];
        //字符
        if (letter >= 0x20 && letter <= 0x7e)
        {
            strOut += (char)letter;
            i++;
            continue;
        }
        //汉字
        else if (letter >= 0xb0 && letter <= 0xf7)
        {
            if (i < iSize)
            {
                byte next = szBuffer[i + 1];
                if (next >= 0xa1 && next <= 0xfe)
                {
                    strOut += (char)letter;
                    strOut += (char)next;
                    i += 2;
                    continue;
                }
            }
        } else if (mulitLine && (letter == '\r' || letter == '\n' || letter == '\t'))
        {
            strOut += (char)letter;
        } else {
            //不可打印
            strOut += '.';
        }
        i++;
    }
    return strOut;
}

mstring GetWindowStrA(HWND hwnd) {
    if (!IsWindow(hwnd))
    {
        return "";
    }

    char buffer[256];
    buffer[0] = 0;
    int size = GetWindowTextLength(hwnd);
    if (size < 256)
    {
        GetWindowTextA(hwnd, buffer, 256);
        return buffer;
    } else {
        MemoryAlloc<char> alloc;
        char *ptr = alloc.GetMemory(size + 4);
        GetWindowTextA(hwnd, ptr, size + 4);
        return ptr;
    }
}

mstring RegGetStrValueA(HKEY hKey, const mstring &subPath, const mstring &valName) {
    return WtoA(RegGetStrValueW(hKey, AtoW(subPath), AtoW(valName)));
}

ustring RegGetStrValueW(HKEY hKey, const ustring &subPath, const ustring &valName) {
    DWORD dwLength = 0;
    LPVOID pBuf = NULL;
    DWORD dwType = 0;
    SHGetValueW(
        hKey,
        subPath.c_str(),
        valName.c_str(),
        &dwType,
        (LPVOID)pBuf,
        &dwLength
        );
    if (REG_SZ != dwType || !dwLength)
    {
        return L"";
    }

    dwLength += 2;
    pBuf = new BYTE[dwLength];
    memset(pBuf, 0x00, dwLength);
    SHGetValueW(
        hKey,
        subPath.c_str(),
        valName.c_str(),
        NULL,
        pBuf,
        &dwLength
        );
    std::wstring wstrRes = (LPCWSTR)pBuf;
    delete []pBuf;
    return wstrRes;
}

//获取当前用户进程的pid
static DWORD _GetCurrentUserPid() {
    HWND hShellWnd = FindWindowA("Shell_TrayWnd", NULL);

    DWORD pid = 0;
    if (IsWindow(hShellWnd))
    {
        GetWindowThreadProcessId(hShellWnd, &pid);
    } else {
        do
        {
            PROCESSENTRY32 procEntry;
            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnap == INVALID_HANDLE_VALUE)
            {
                break;
            }

            procEntry.dwSize = sizeof(PROCESSENTRY32W);
            if (Process32First(hSnap, &procEntry))
            {
                do
                {
                    if (lstrcmpA(procEntry.szExeFile, "explorer.exe") == 0)
                    {
                        pid = procEntry.th32ProcessID;
                        break;
                    }
                } while (Process32Next(hSnap, &procEntry));
            }
            CloseHandle(hSnap);
        } while (0);
    }
    return pid;
}

//高权限进程启动当前用户权限进程
HANDLE CreateProcWithCurrentUser(const mstring &command, bool show) {
    HANDLE hProcess = NULL;
    HANDLE hPToken = NULL;
    HANDLE hUserTokenDup = NULL;

    do
    {
        DWORD pid = _GetCurrentUserPid();
        if (!pid)
        {
            break;;
        }

        HANDLE hForkProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (hForkProc == NULL)
        {
            break;
        }

        if(!OpenProcessToken(hForkProc, TOKEN_ALL_ACCESS_P,&hPToken))
        {
            break;
        }

        TOKEN_LINKED_TOKEN admin;
        DWORD ret = 0;

        if (GetTokenInformation(hPToken, (TOKEN_INFORMATION_CLASS)TokenLinkedToken, &admin, sizeof(TOKEN_LINKED_TOKEN), &ret))
        {
            hUserTokenDup = admin.LinkedToken;
        } else
        {
            int err = GetLastError();

            TOKEN_PRIVILEGES tp;
            LUID luid;
            if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
            {
                tp.PrivilegeCount =1;
                tp.Privileges[0].Luid =luid;
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            }
            //复制token
            DuplicateTokenEx(hPToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification,TokenPrimary, &hUserTokenDup);
        }

        LPVOID pEnv = NULL;
        DWORD dwCreationFlags = CREATE_PRESERVE_CODE_AUTHZ_LEVEL;

        // hUserTokenDup为当前登陆用户的令牌
        if(CreateEnvironmentBlock(&pEnv, hUserTokenDup, TRUE))
        {
            dwCreationFlags|= CREATE_UNICODE_ENVIRONMENT;
        }
        else
        {
            //环境变量创建失败仍然可以创建进程，
            //但会影响到后面的进程获取环境变量内容
            pEnv = NULL;
        }

        PROCESS_INFORMATION pi = {0};
        STARTUPINFOA si = { sizeof(si) };

        if (show)
        {
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOW;
        } else {
            si.wShowWindow = SW_HIDE;
        }

        if (CreateProcessAsUserA(
            hUserTokenDup,
            NULL,
            (LPSTR)command.c_str(),
            NULL,
            NULL,
            FALSE,
            dwCreationFlags,
            pEnv,
            NULL,
            &si,
            &pi
            ))
        {
            hProcess = pi.hProcess;
            CloseHandle(pi.hThread);
        }
    } while (0);

    if (hPToken)
    {
        CloseHandle(hPToken);
    }

    if (hUserTokenDup)
    {
        CloseHandle(hUserTokenDup);
    }
    return hProcess;
}

mstring GetStdErrorStr(DWORD dwErr)
{
    LPVOID lpMsgBuf = NULL;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |  
        FORMAT_MESSAGE_FROM_SYSTEM |  
        FORMAT_MESSAGE_IGNORE_INSERTS, 
        NULL,
        dwErr,
        MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), //Default language  
        (LPSTR)&lpMsgBuf,  
        0,  
        NULL  
        ); 
    mstring strMsg((LPCSTR)lpMsgBuf);
    if (lpMsgBuf)
    {
        LocalFlags(lpMsgBuf);
    }
    return strMsg;
}

HANDLE ExecProcessW(LPCWSTR cmdLine, DWORD* procId, BOOL bShowWindow)
{
    if (!cmdLine)
    {
        return NULL;
    }

    STARTUPINFOW si = {sizeof(si)};
    if (!bShowWindow)
    {
        si.wShowWindow = SW_HIDE;
        si.dwFlags = STARTF_USESHOWWINDOW;
    }

    PROCESS_INFORMATION pi;

    LPWSTR wszCmdLine = (LPWSTR)malloc((MAX_PATH + lstrlenW(cmdLine)) * sizeof(WCHAR));
    lstrcpyW(wszCmdLine, cmdLine);
    if (CreateProcessW(NULL, wszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hThread);

        if (procId)
        {
            *procId = pi.dwProcessId;
        }

        free((void*)wszCmdLine);
        return pi.hProcess;
    }

    free((void*)wszCmdLine);
    return NULL;
}

HANDLE ExecProcessA(LPCSTR cmdLine, DWORD* procId, BOOL bShowWindow) {
    return ExecProcessW(AtoW(cmdLine).c_str(), procId, bShowWindow);
}

// 需要调用方提供 PACL* 并在调用完此函数后自行释放内存
BOOL GenerateLowSD(SECURITY_DESCRIPTOR* pSecDesc, PACL* pDacl)
{
    PSID pSidWorld = NULL;
    EXPLICIT_ACCESS ea;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_WORLD_SID_AUTHORITY;
    BOOL bRet = FALSE;

    do
    {
        if (!pSecDesc || !pDacl)
        {
            break;
        }

        if (AllocateAndInitializeSid(&sia, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSidWorld) == 0)
        {
            break;
        }

        ea.grfAccessMode = GRANT_ACCESS;
        ea.grfAccessPermissions = FILE_ALL_ACCESS ;
        ea.grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
        ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        ea.Trustee.pMultipleTrustee = NULL;
        ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea.Trustee.ptstrName = (LPTSTR)pSidWorld;

        if (SetEntriesInAcl(1, &ea, NULL, pDacl) != ERROR_SUCCESS)
        {
            break;
        }

        if (InitializeSecurityDescriptor(pSecDesc, SECURITY_DESCRIPTOR_REVISION) == 0)
        {
            break;
        }

        if (SetSecurityDescriptorDacl(pSecDesc, TRUE, *pDacl, FALSE) == 0)
        {
            break;
        }

        bRet = TRUE;
    } while (FALSE);

    if (NULL != pSidWorld)
    {
        FreeSid(pSidWorld);
        pSidWorld = NULL;
    }

    return bRet;
}