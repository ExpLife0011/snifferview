#ifndef COMMON_H_H_
#define COMMON_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <list>
#include <map>
#include <mstring.h>

using namespace std;

#define SENDMSG(hwd, msg) SendMessageA(hwd, msg, 0, 0)
#define  IsChecked(hwd)	(BST_CHECKED == SENDMSG(hwd, BM_GETCHECK))
#define  GetFirstSelect(hwd) (SendMessageA(hwd, LVM_GETNEXTITEM, -1, LVNI_SELECTED))

struct AdapterMsg
{
	UINT m_com_idex;
	UINT m_idex;
	mstring m_name;
	mstring m_desc;
	mstring m_mac;
	mstring m_ip;
	mstring m_mask;
	mstring m_gateway;
	mstring m_type;
	bool m_dhcp_enable;
};

VOID WINAPI CentreWindow(HWND hParent, HWND hChild);

//打印调试信息
VOID WINAPI PrintDbgMessage(LPCSTR pBuffer ...);

DWORD n2h_32(IN OUT DWORD v);

DWORD h2n_32(IN OUT DWORD v);

short n2h_16(IN OUT short v);

short h2n_16(IN OUT short v);

unsigned short h2n_u16(unsigned short d);
unsigned short n2h_u16(unsigned short d);

BOOL WINAPI WriteFileBuffer(HANDLE file, const char *buffer, size_t length);

//分析一个字符串是否是一个合法的ip格式
BOOL WINAPI CheckIpaddress(const char *buffer, DWORD &addr);

//检查一个字符串是不是一个合法的8位无符号整型数据
BOOL WINAPI CheckUnsignedN8(IN const char *buffer, OUT UCHAR &bt);

//分析一个字符串是不是一个合法的16位无符号数据
BOOL WINAPI CheckUnsignedN16(IN const char *buffer, OUT USHORT &ds);

//分析一个字符串是不是一个合法的32位无符号数据
BOOL WINAPI CheckUnsignedN32(const char *buffer, ULONG &ds);

//获取以指定符号分割的字符串集合
VOID WINAPI GetCutBufferList(IN const char *buffer, IN char ct, OUT list<mstring> &lst);

//获取网卡列表的详细信息
BOOL WINAPI GetAdapterMsg(OUT vector<AdapterMsg> &lst);

//获取低权限的描述符
BOOL WINAPI InitEveryMutexACL(IN OUT SECURITY_ATTRIBUTES &sa, IN OUT SECURITY_DESCRIPTOR &sd);

//判定当前进程是否具有administrator权限
BOOL WINAPI IsAdminUser();

BOOL IsDirectoryExist(const char *dir);

//获取pe文件版本信息
BOOL GetFileVersion(IN LPCSTR pFile, OUT mstring& ver);

/*
*检查某种类型的文件是否已经注册
*ext:	文件扩展名 eg:.txt
*key:	扩展名在注册表里的键值 eg:txtfile
*/
BOOL WINAPI CheckFileRelation(IN const char *ext, IN const char * key);

/*
*为一种特定的文件类型注册关联
*ext:	文件扩展名 eg:.txt
*cmd:	要关联的文件命令行 eg:d:/xxx.exe -f
*key:	扩展名在注册表中的键值 eg:txtfile
*ico:	文件图标，d:/xxx.exe, 0
*dec:	文件类型描述
*/
BOOL WINAPI RegisterFileRelation(IN const char *ext, IN const char *cmd, IN const char *key, IN const char *ico, IN const char *dec);

//从PE的资源中释放文件
BOOL ReleaseRes(const char *file, DWORD id, const char *type);

//是否是64位操作系统
BOOL IsWow64System();

//根据ListCtrl的列宽总和调整所在对话框的宽度, 列宽中不能有宽度为0的
VOID ResetWidthByColumns(HWND hdlg, HWND list);

//Dos文件路径转为Nt路径
BOOL DosPathToNtPath(IN const char *src, OUT mstring &dst);

//从进程Pid获取进程的信息
BOOL GetProcessByPid(IN DWORD pid, OUT mstring &path);

//字符串匹配支持*，？的匹配
BOOL WINAPI StringMatch(IN const char *src, IN const char *dst);

//获取pe文件属性
//Comments InternalName ProductName 
//CompanyName LegalCopyright ProductVersion 
//FileDescription LegalTrademarks PrivateBuild 
//FileVersion OriginalFilename SpecialBuild 
BOOL GetFileString(IN const char *path, IN const char *attr, OUT mstring &msg);

//从bmp获取cur句柄，mask是颜色的掩码
HCURSOR CreateCursorFromBitmap(HBITMAP bmp, COLORREF mask);

//创建低权限的事件
HANDLE CreateLowsdEvent(BOOL bReset, BOOL bInitStat, LPCWSTR wszName);

//在当前活动session中以system权限运行一个进程
BOOL RunInSession(LPCWSTR wszImage, LPCWSTR wszCmd, DWORD dwInSession, DWORD dwPid = 0);

//判断两个文件是否相同
BOOL IsSameFileW(LPCWSTR file1, LPCWSTR file2);

//停止指定的服务
BOOL ServStopW(LPCWSTR servName);

//启动指定的服务
BOOL ServStartW(LPCWSTR servName);

//将一个32位无符号整形转为点分十进制ip地址
mstring Int32ToIp(unsigned int addr, bool changeOrder);
#endif