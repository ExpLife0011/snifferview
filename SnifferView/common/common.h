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

//��ӡ������Ϣ
VOID WINAPI PrintDbgMessage(LPCSTR pBuffer ...);

DWORD n2h_32(IN OUT DWORD v);

DWORD h2n_32(IN OUT DWORD v);

short n2h_16(IN OUT short v);

short h2n_16(IN OUT short v);

unsigned short h2n_u16(unsigned short d);
unsigned short n2h_u16(unsigned short d);

BOOL WINAPI WriteFileBuffer(HANDLE file, const char *buffer, size_t length);

//����һ���ַ����Ƿ���һ���Ϸ���ip��ʽ
BOOL WINAPI CheckIpaddress(const char *buffer, DWORD &addr);

//���һ���ַ����ǲ���һ���Ϸ���8λ�޷�����������
BOOL WINAPI CheckUnsignedN8(IN const char *buffer, OUT UCHAR &bt);

//����һ���ַ����ǲ���һ���Ϸ���16λ�޷�������
BOOL WINAPI CheckUnsignedN16(IN const char *buffer, OUT USHORT &ds);

//����һ���ַ����ǲ���һ���Ϸ���32λ�޷�������
BOOL WINAPI CheckUnsignedN32(const char *buffer, ULONG &ds);

//��ȡ��ָ�����ŷָ���ַ�������
VOID WINAPI GetCutBufferList(IN const char *buffer, IN char ct, OUT list<mstring> &lst);

//��ȡ�����б����ϸ��Ϣ
BOOL WINAPI GetAdapterMsg(OUT vector<AdapterMsg> &lst);

//��ȡ��Ȩ�޵�������
BOOL WINAPI InitEveryMutexACL(IN OUT SECURITY_ATTRIBUTES &sa, IN OUT SECURITY_DESCRIPTOR &sd);

//�ж���ǰ�����Ƿ����administratorȨ��
BOOL WINAPI IsAdminUser();

BOOL IsDirectoryExist(const char *dir);

//��ȡpe�ļ��汾��Ϣ
BOOL GetFileVersion(IN LPCSTR pFile, OUT mstring& ver);

/*
*���ĳ�����͵��ļ��Ƿ��Ѿ�ע��
*ext:	�ļ���չ�� eg:.txt
*key:	��չ����ע�����ļ�ֵ eg:txtfile
*/
BOOL WINAPI CheckFileRelation(IN const char *ext, IN const char * key);

/*
*Ϊһ���ض����ļ�����ע�����
*ext:	�ļ���չ�� eg:.txt
*cmd:	Ҫ�������ļ������� eg:d:/xxx.exe -f
*key:	��չ����ע����еļ�ֵ eg:txtfile
*ico:	�ļ�ͼ�꣬d:/xxx.exe, 0
*dec:	�ļ���������
*/
BOOL WINAPI RegisterFileRelation(IN const char *ext, IN const char *cmd, IN const char *key, IN const char *ico, IN const char *dec);

//��PE����Դ���ͷ��ļ�
BOOL ReleaseRes(const char *file, DWORD id, const char *type);

//�Ƿ���64λ����ϵͳ
BOOL IsWow64System();

//����ListCtrl���п��ܺ͵������ڶԻ���Ŀ��, �п��в����п��Ϊ0��
VOID ResetWidthByColumns(HWND hdlg, HWND list);

//Dos�ļ�·��תΪNt·��
BOOL DosPathToNtPath(IN const char *src, OUT mstring &dst);

//�ӽ���Pid��ȡ���̵���Ϣ
BOOL GetProcessByPid(IN DWORD pid, OUT mstring &path);

//�ַ���ƥ��֧��*������ƥ��
BOOL WINAPI StringMatch(IN const char *src, IN const char *dst);

//��ȡpe�ļ�����
//Comments InternalName ProductName 
//CompanyName LegalCopyright ProductVersion 
//FileDescription LegalTrademarks PrivateBuild 
//FileVersion OriginalFilename SpecialBuild 
BOOL GetFileString(IN const char *path, IN const char *attr, OUT mstring &msg);

//��bmp��ȡcur�����mask����ɫ������
HCURSOR CreateCursorFromBitmap(HBITMAP bmp, COLORREF mask);

//������Ȩ�޵��¼�
HANDLE CreateLowsdEvent(BOOL bReset, BOOL bInitStat, LPCWSTR wszName);

//�ڵ�ǰ�session����systemȨ������һ������
BOOL RunInSession(LPCWSTR wszImage, LPCWSTR wszCmd, DWORD dwInSession, DWORD dwPid = 0);

//�ж������ļ��Ƿ���ͬ
BOOL IsSameFileW(LPCWSTR file1, LPCWSTR file2);

//ָֹͣ���ķ���
BOOL ServStopW(LPCWSTR servName);

//����ָ���ķ���
BOOL ServStartW(LPCWSTR servName);

//��һ��32λ�޷�������תΪ���ʮ����ip��ַ
mstring Int32ToIp(unsigned int addr, bool changeOrder);
#endif