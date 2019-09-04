#include <WinSock2.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <list>
#include <set>
#include <vector>
#include "../../ComLib/common.h"
#include <algorithm>
#include "../../ComLib/winsize.h"
#include "netstat.h"
#include "delaylist.h"
#include "../resource.h"
#include "../global.h"
#include "../process.h"

#define  NETSTATE_TIMEOUT               100
#define  HIGH_CONTINUE                  1000

//msg
#define  MSG_CHECK_TIMER                (WM_USER + 5001)
#define  MSG_ACTIVE_WINDOW              (WM_USER + 5005)
#define  MSG_ON_FILTER_RET              (WM_USER + 5007)

#define MSG_FILTER_PROMPT               ("�ڴ���������������������������÷ֺŽ��зָ�")

#define  LOCK_NS        WaitForSingleObject(s_netstat_lock, INFINITE)
#define  UNLOCK_NS      ReleaseMutex(s_netstat_lock)

static HWND s_netstat_view = NULL;
static HWND s_netstat_list = NULL;
static HWND s_statusbar = NULL;
static HWND s_edit = NULL;

//�������ͺ͹������
static mstring s_netstat_str = "";
//������������֧��
static set<mstring> s_netstat_filters;

static PWIN_PROC s_edit_proc = NULL;

//image
static HIMAGELIST s_image_list = NULL;
static map<mstring, int> s_images_data;					//����·����Ӧico�����Ļ���
static map<int, ProcessInfo> s_pe_data;					//����Pid��Ӧ����ȫ·���Ļ���
static set<HICON> s_icos;								//ico���

static HANDLE s_leave_event = CreateEventA(NULL, FALSE, FALSE, NULL);
static HANDLE s_netstat_lock = CreateMutexA(NULL, FALSE, NULL);

static vector<int> s_cur_filter;						//���˺�����������б�,���ݼ�Ϊs_cur_netstates�е��±�
static vector<NetstatInfo> s_cur_netstates;				//���������б�������ListCtrl��չʾ
static set<NetstatInfo> s_org_tcp;						//ԭʼ��tcp��·�����б��ѽ����������ڽ��бȶ�
static set<NetstatInfo> s_org_udp;						//ԭʼ��udp���������б�, �ѽ����������ڽ��бȶ�

static BOOL IsNetstatFilter(NetstatInfo &itm)
{
	if (s_netstat_filters.size() == 0)
	{
		return TRUE;
	}

	mstring mu;
	mstring mk;
	set<mstring>::iterator its;
	bool pass = false;
	for (its = s_netstat_filters.begin() ; its != s_netstat_filters.end() ; its++)
	{
        if (mstring::npos != itm.m_index.find_in_rangei(*its))
        {
            return TRUE;
        }
    }
    return FALSE;
}

//�������ź����������бȽϣ���ȡ�����ٵ����ݺ�����������
VOID NetstatsCompare(IN set<NetstatInfo> &org, IN set<NetstatInfo> &dst, OUT set<NetstatInfo> &add, OUT set<NetstatInfo> &del)
{
	set<NetstatInfo>::iterator itm = org.begin();
	set<NetstatInfo>::iterator its = dst.begin();
	while(itm != org.end() && its != dst.end())
	{
		if ((*itm) == (*its))
		{
			itm++;
			its++;
		}
		else if (*itm > *its)
		{
			add.insert(*its);
			its++;
		}
		else
		{
			del.insert(*itm);
			itm++;
		}
	}

	while(itm != org.end())
	{
		del.insert(*itm++);
	}

	while(its != dst.end())
	{
		add.insert(*its++);
	}
	return;
}

static VOID InsertNetstatInfo(int idex, NetstatInfo info)
{
	mstring ms;
	InsertDelaylistItem(s_netstat_list, MASK_TEXT | MASK_IMAGE, idex, info.m_process.m_image_idex, info.m_process.m_name.c_str());
	ms.format("%d", info.m_Pid);
	SetDelaylistText(s_netstat_list, idex, 1, ms.c_str());
	if (info.m_type == em_netstat_tcp)
	{
		ms.format("Tcp");
	}
	else if (info.m_type == em_netstat_udp)
	{
		ms.format("Udp");
	}
	SetDelaylistText(s_netstat_list, idex, 2, ms.c_str());

	if (info.m_type == em_netstat_tcp)
	{
		if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_CLOSED)
		{
			ms.format("Closed");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_LISTEN)
		{
			ms.format("Listen");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_SYN_SENT)
		{
			ms.format("Syn Sent");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_SYN_RCVD)
		{
			ms.format("Syn Received");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_ESTAB)
		{
			ms.format("Established");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_FIN_WAIT1 || info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_FIN_WAIT2)
		{
			ms.format("Fin Wait");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_CLOSE_WAIT)
		{
			ms.format("Close Wait");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_CLOSING)
		{
			ms.format("Closing");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_LAST_ACK)
		{
			ms.format("Last Ack");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_TIME_WAIT)
		{
			ms.format("Time Wait");
		}
		else if (info.m_state.m_tcp_state.dwState == MIB_TCP_STATE_DELETE_TCB)
		{
			ms.format("Delete Tcp");
		}
		else
		{
			ms.format("Tcp Unknow");
		}
		SetDelaylistText(s_netstat_list, idex, 3, ms.c_str());
		//src addr
		DWORD addr = (info.m_state.m_tcp_state.dwLocalAddr);
		IN_ADDR as;
		as.S_un.S_addr = addr;
		ms = inet_ntoa(as);
		SetDelaylistText(s_netstat_list, idex, 4, ms.c_str());
		//src port
		USHORT port = n2h_16((USHORT)(info.m_state.m_tcp_state.dwLocalPort));
		ms.format("%u", port);
		SetDelaylistText(s_netstat_list, idex, 5, ms.c_str());
		//dst addr
		addr = (info.m_state.m_tcp_state.dwRemoteAddr);
		as.S_un.S_addr = addr;
		ms = inet_ntoa(as);
		SetDelaylistText(s_netstat_list, idex, 6, ms.c_str());
		//dst port
		port = n2h_16((USHORT)(info.m_state.m_tcp_state.dwRemotePort));
		ms.format("%u", port);
		SetDelaylistText(s_netstat_list, idex, 7, ms.c_str());
	}
	else if (info.m_type == em_netstat_udp)
	{
		ms = "*";
		SetDelaylistText(s_netstat_list, idex, 3, ms.c_str());
		//src addr
		DWORD addr = (info.m_state.m_udp_state.dwLocalAddr);
		IN_ADDR as;
		as.S_un.S_addr = addr;
		ms = inet_ntoa(as);
		SetDelaylistText(s_netstat_list, idex, 4, ms.c_str());
		//src port
		USHORT port = n2h_16((USHORT)(info.m_state.m_udp_state.dwLocalPort));
		ms.format("%u", port);
		SetDelaylistText(s_netstat_list, idex, 5, ms.c_str());
		//dst addr
		ms = "*";
		SetDelaylistText(s_netstat_list, idex, 6, ms.c_str());
		//dst port;
		SetDelaylistText(s_netstat_list, idex, 7, ms.c_str());
	}
}

//�����ݲ��뵽��ʾ�б�,�������б��е�����
int InsertIntoNetstat(vector<int> &vos, int idex)
{
	vector<int>::iterator itm;
	int itk = 0;
	int itu = 0;;
	for (itm = vos.begin() ; itm != vos.end() ; itm++, itk++)
	{
		if (*itm > idex)
		{
			vos.insert(vos.begin() + itk, idex);
			itu = itk;
			return itu;
		}
	}

	if (itm == vos.end())
	{
		vos.push_back(idex);
		return (vos.size() - 1);
	}
	return 0;
}

//�����ӵ��������Ӽӵ�չʾ�б���
VOID NetstatusAdd(IN set<NetstatInfo> &add)
{
	set<NetstatInfo>::iterator itm;
	vector<NetstatInfo>::reverse_iterator itk;
	vector<int>::reverse_iterator itu;
	int idex = 0;
	bool raw = false;
	for (itm = add.begin() ; itm != add.end() ; itm++)
	{
        itm->toString();
		idex = s_cur_netstates.size();
		raw = false;
		for (itk = s_cur_netstates.rbegin() ; itk != s_cur_netstates.rend() ; itk++, idex--)
		{
			if ((*itk).m_Pid == (*itm).m_Pid)
			{
				raw = true;
				s_cur_netstates.insert(s_cur_netstates.begin() + idex, *itm);
				break;
			}
		}

		if (!raw)
		{
			s_cur_netstates.push_back(*itm);
			idex = s_cur_netstates.size() - 1;
		}

		//����s_cur_filter����Ӧ�����
		for (itu = s_cur_filter.rbegin() ; itu != s_cur_filter.rend() ; itu++)
		{
			if(*itu >= idex)
			{
				*itu += 1;
			}
			else
			{
				break;
			}
		}

		if (IsNetstatFilter(*itm))
		{
			int itu = InsertIntoNetstat(s_cur_filter, idex);
			//�ػ�ListCtrl����������
			InsertNetstatInfo(itu, *itm);
		}
	}
	return;
}

int DeleteNetstat(vector<int> &vos, int idex)
{
	int itu = 0;
	vector<int>::iterator itm;
	for (itm = vos.begin() ; itm != vos.end() ; itm++, itu++)
	{
		if (*itm == idex)
		{
			vos.erase(itm);
			return itu;
		}
	}
	return -1;
}

//�����ٵ������������б��н��б��
VOID NetstatusDel(IN set<NetstatInfo> &del)
{
	set<NetstatInfo>::iterator itm;
	vector<NetstatInfo>::iterator itk;
	int idex = s_cur_netstates.size();
	set<int>::iterator itu;
	bool raw = false;
	for (itm = del.begin() ; itm != del.end() ; itm++)
	{
		raw = false;
		idex = 0;
		for (itk = s_cur_netstates.begin() ; itk != s_cur_netstates.end() ; itk++, idex++)
		{
			if ((*itk) == (*itm))
			{
				int its = DeleteNetstat(s_cur_filter, idex);
				if (its >= 0)
				{
					//����s_cur_filter����Ӧ�����
					int itu = 0;
					for (itu = its ; itu < (int)s_cur_filter.size() ; itu++)
					{
						s_cur_filter[itu]--;
					}
					DeleteDelaylistItem(s_netstat_list, its);
				}
				else
				{
					//����s_cur_filter����Ӧ�����
					int itu = 0;
					for (itu = 0 ; itu < (int)s_cur_filter.size() ; itu++)
					{
						if (s_cur_filter[itu] > idex)
						{
							s_cur_filter[itu]--;
						}
					}
				}
				s_cur_netstates.erase(itk);
				break;
			}
		}
	}
	return;
}

//��ȡtcp��������״̬�б�
BOOL GetTcpConnectTable(OUT mstring &buffer)
{
	static char *s_buffer = NULL;
	static  DWORD s_length = 0;
	DWORD ret = 0;
	DWORD size = s_length;
	while(ERROR_INSUFFICIENT_BUFFER == (ret = GetExtendedTcpTable(s_buffer, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0)))
	{
		if (s_length < size)
		{
			if(!s_buffer)
			{
				s_buffer = (char *)malloc(size + 2);
				s_length = size + 2;
			}
			else
			{
				s_buffer = (char *)realloc(s_buffer, size + 2);
				s_length = size + 2;
			}
		}

		if (!s_buffer)
		{
			return FALSE;
		}
		size = s_length;
	}

	if (ret != NO_ERROR)
	{
		return FALSE;
	}
	buffer.assign(s_buffer, size);
	return TRUE;
}

//��ȡudp��������״̬�б�
BOOL GetUdpConnectTable(OUT mstring &buffer)
{
	static char *s_buffer = NULL;
	static  DWORD s_length = 0;
	DWORD ret = 0;
	DWORD size = s_length;
	while(ERROR_INSUFFICIENT_BUFFER == (ret = GetExtendedUdpTable(s_buffer, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0)))
	{
		if (s_length < size)
		{
			if(!s_buffer)
			{
				s_buffer = (char *)malloc(size + 2);
				s_length = size + 2;
			}
			else
			{
				s_buffer = (char *)realloc(s_buffer, size + 2);
				s_length = size + 2;
			}
		}

		if (!s_buffer)
		{
			return FALSE;
		}
		size = s_length;
	}

	if (ret != NO_ERROR)
	{
		return FALSE;
	}
	buffer.assign(s_buffer, size);
	return TRUE;
}

int GetFileImageIdex(IN const char *path)
{
	SHFILEINFO info = {0};
	int index = 0;
	if (s_images_data.end() != s_images_data.find(path))
	{
		return s_images_data[path];
	}

	if (0 != SHGetFileInfoA(path, 0, &info, sizeof(&info), SHGFI_ICON))
	{
		index = ImageList_AddIcon(s_image_list, info.hIcon);
		s_images_data[path] = index;
		s_icos.insert(info.hIcon);
	}
	return index;
}

BOOL GetProcessMsgByPid(IN DWORD pid, OUT ProcessInfo &msg)
{
	BOOL ret = FALSE;
	if (s_pe_data.end() != s_pe_data.find(pid))
	{
		msg = s_pe_data[pid];
		ret = TRUE;
	}
	else
	{
		if (GetProcessByPid(pid, msg.m_path))
		{
			//System | System Idle Process
			msg.m_pid = pid;
			if (pid == 0 || pid == 4)
			{
				msg.m_name = msg.m_path;
				msg.m_image_idex = 0;
			}
			else
			{
				mstring ms = msg.m_path;
				ms.repsub("/", "\\");
				int itm = ms.rfind("\\");
				msg.m_name.assign(ms, itm + 1, ms.size() - itm);
				msg.m_image_idex = GetFileImageIdex(msg.m_path.c_str());
			}
			s_pe_data[pid] = msg;
			ret = TRUE;
		}
	}
	return ret;
}

VOID RefushNetstat()
{
	PMIB_TCPTABLE_OWNER_PID tcp = NULL;
	PMIB_UDPTABLE_OWNER_PID udp = NULL;
	set<NetstatInfo> lst;
	set<NetstatInfo> add;
	set<NetstatInfo> del;
	NetstatInfo mm;
	NetstatInfo vv;
	mstring tmp;
	unsigned int itm = 0;
	s_pe_data.clear();	//���pe���ݵĻ���
	if (GetTcpConnectTable(tmp))
	{
		lst.clear();
		tcp = (PMIB_TCPTABLE_OWNER_PID)tmp.c_str();
		for (itm = 0 ; itm < tcp->dwNumEntries ; itm++)
		{
			vv.m_state.m_tcp_state = tcp->table[itm];
			vv.m_Pid = vv.m_state.m_tcp_state.dwOwningPid;
			vv.m_type = em_netstat_tcp;
			if (GetProcessMsgByPid(vv.m_Pid, vv.m_process))
			{
				lst.insert(vv);
			}
		}
		NetstatsCompare(s_org_tcp, lst, add, del);
		s_org_tcp = lst;
		NetstatusAdd(add);
		NetstatusDel(del);
	}
	add.clear();
	del.clear();
	tmp.clear();
	if (GetUdpConnectTable(tmp))
	{
		lst.clear();
		udp = (PMIB_UDPTABLE_OWNER_PID)tmp.c_str();
		for (itm = 0 ; itm < udp->dwNumEntries ; itm++)
		{
			vv.m_state.m_udp_state = udp->table[itm];
			vv.m_Pid = vv.m_state.m_udp_state.dwOwningPid;
			vv.m_type = em_netstat_udp;
			if (GetProcessMsgByPid(vv.m_Pid, vv.m_process))
			{
				lst.insert(vv);
			}
		}
		NetstatsCompare(s_org_udp, lst, add, del);
		s_org_udp = lst;
		NetstatusAdd(add);
		NetstatusDel(del);
	}
}

static VOID WINAPI InitListView()
{
	ListView_SetExtendedListViewStyle(s_netstat_list, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	LVCOLUMNA col;
	memset(&col, 0x00, sizeof(col));
	col.mask = LVCF_TEXT | LVCF_WIDTH;

	col.cx = 120;
	col.pszText = (LPSTR)"����";
	SendMessageA(s_netstat_list, LVM_INSERTCOLUMNA, 0, (LPARAM)&col);

	col.cx = 50;
	col.pszText = (LPSTR)"Pid";
	SendMessageA(s_netstat_list, LVM_INSERTCOLUMNA, 1, (LPARAM)&col);

	col.cx = 50;
	col.pszText = (LPSTR)"Э��";
	SendMessageA(s_netstat_list, LVM_INSERTCOLUMNA, 2, (LPARAM)&col);

	col.cx = 75;
	col.pszText = (LPSTR)"״̬";
	SendMessageA(s_netstat_list, LVM_INSERTCOLUMNA, 3, (LPARAM)&col);

	col.cx = 110;
	col.pszText = (LPSTR)"���ص�ַ";
	SendMessageA(s_netstat_list, LVM_INSERTCOLUMNA, 4, (LPARAM)&col);

	col.cx = 60;
	col.pszText = (LPSTR)"���ض˿�";
	SendMessageA(s_netstat_list, LVM_INSERTCOLUMNA, 5, (LPARAM)&col);

	col.cx = 110;
	col.pszText = (LPSTR)"Զ�˵�ַ";
	SendMessageA(s_netstat_list, LVM_INSERTCOLUMNA, 6, (LPARAM)&col);

	col.cx = 60;
	col.pszText = (LPSTR)"Զ�˶˿�";
	SendMessageA(s_netstat_list, LVM_INSERTCOLUMNA, 7, (LPARAM)&col);

	ResetWidthByColumns(s_netstat_view, s_netstat_list);
}

HWND WINAPI CreateNetstatusBar(HWND hdlg)
{
	HWND status = CreateStatusWindowA(WS_CHILD | WS_VISIBLE, NULL, hdlg, 100051);
	int wide[3] = {0};
	int length = 0;
	wide[0] = 380;
	wide[1] = wide[0] + 320; 
	wide[2]= wide[1] + 320;
	SendMessage(status, SB_SETPARTS, sizeof(wide) / sizeof(int), (LPARAM)(LPINT)wide); 
	return status;
}

VOID SetStatusMsg(int idex, const char *msg)
{
	SendMessageA(s_statusbar, SB_SETTEXT, idex, (LPARAM)msg);
}

VOID UpdateNetMsg()
{
	mstring dbg;
	dbg.format("  ��������״̬ tcp��%d udp��%d  ����ɸѡ�����ģ�%d", s_org_tcp.size(), s_org_udp.size(), s_cur_filter.size());
	SetStatusMsg(0, dbg.c_str());
}

LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (WM_KEYDOWN == msg)
	{
		if (0x0d == wp)
		{
			SendMessageA(s_netstat_view, MSG_ON_FILTER_RET, 0, 0);
		}
	}
	return CallWindowProc(s_edit_proc, hwnd, msg, wp, lp);
}

VOID AnalysisFilterStr()
{
	int itm = 0;
	int flag = 0;
	s_netstat_filters.clear();
	s_netstat_str.repsub("��", ";");
	while(mstring::npos != (itm = s_netstat_str.find(";", flag)))
	{
		if (itm > flag)
		{
			s_netstat_filters.insert(mstring(s_netstat_str.c_str() + flag, itm - flag));
		}
		flag = itm + 1;
	}

	if (flag < (int)s_netstat_str.size())
	{
		s_netstat_filters.insert(mstring(s_netstat_str.c_str() + flag, s_netstat_str.size() - flag));
	}
}

VOID OnFilterRet(HWND hdlg, WPARAM wp, LPARAM lp)
{
	SetStatusMsg(1, "");

    int length = GetWindowTextLengthA(s_edit);
    char *buffer = new char[length + 1];
    buffer[length] = 0x00;
    GetWindowTextA(s_edit, buffer, length + 1);

    s_netstat_str = buffer;
    delete []buffer;
    AnalysisFilterStr();

	set<int> vvs;
	vector<int>::iterator itk = s_cur_filter.begin();
	int bb = 0;
	while (itk != s_cur_filter.end())
	{
		if (!IsNetstatFilter(s_cur_netstates[*itk]))
		{
			//��չʾ�б���ɾ������
			itk = s_cur_filter.erase(itk);
			DeleteDelaylistItem(s_netstat_list, bb);
			continue;
		}
		vvs.insert(*itk);
		bb++;
		itk++;
	}

	vector<NetstatInfo>::iterator itm = s_cur_netstates.begin();
	mstring mu;
	mstring mk;
	int idex = 0;
	while(itm != s_cur_netstates.end())
	{
		if (IsNetstatFilter(*itm))
		{
			if (vvs.end() == vvs.find(idex))
			{
				//��������
				int uu = InsertIntoNetstat(s_cur_filter, idex);
				InsertNetstatInfo(uu, *itm);
			}
		}
		idex++;
		itm++;
	}
}

VOID OnInitDialog(HWND hdlg, WPARAM wp, LPARAM lp)
{
	s_netstat_view = hdlg;
	s_netstat_list = GetDlgItem(hdlg, IDC_ST_LIST);
    s_edit = GetDlgItem(s_netstat_view, IDC_NETSTAT_EDT_SEARCH);
	CenterWindow(GetParent(hdlg), hdlg);
	InitListView();
	s_statusbar = CreateNetstatusBar(hdlg);
	SendMessageA(hdlg, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconA(g_m, MAKEINTRESOURCEA(IDI_MAIN)));
	
	CTL_PARAMS arry[] =
	{
		{0, s_edit, 0, 0, 1.0f, 0},
        {IDC_NETSTAT_BTN_SEARCH, 0, 1.0f, 0, 0, 0},
		{0, s_netstat_list, 0, 0, 1.0f, 1.0f},
		{0, s_statusbar, 0, 1, 1.0f, 0.0f}
	};
	SetCtlsCoord(hdlg, arry, sizeof(arry) / sizeof(CTL_PARAMS));

	RECT rt;
	GetWindowRect(hdlg, &rt);
	SetWindowRange(hdlg, rt.right - rt.left, rt.bottom - rt.top, 0, 0);
	
	//init image list
	s_image_list = ImageList_Create(15, 16, ILC_COLOR32 | ILC_MASK, 5, 1);
	SendMessageA(s_netstat_list, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)s_image_list);
	HICON ico = LoadIconA(NULL, MAKEINTRESOURCEA(IDI_APPLICATION));
	ImageList_AddIcon(s_image_list, ico);
	s_icos.insert(ico);

	s_edit_proc = (PWIN_PROC)SetWindowLong(s_edit, GWL_WNDPROC, (long)EditProc);
	InitDelayListctrl(hdlg, s_netstat_list);
	RefushNetstat();
	s_netstat_str.clear();
	OnFilterRet(hdlg, 0, 0);
	UpdateNetMsg();
	SetTimer(hdlg, MSG_CHECK_TIMER, 1000, NULL);
}

VOID OnTimer(HWND hdlg, WPARAM wp, LPARAM lp)
{
	if (wp == MSG_CHECK_TIMER)
	{
		RefushNetstat();
		UpdateNetMsg();
	}
}

VOID OnActiveWindow(HWND hdlg, WPARAM wp, LPARAM lp)
{
	if(IsIconic(hdlg))
	{
		SendMessageA(hdlg, WM_SYSCOMMAND, SC_RESTORE, 0);
	}

	if (!IsZoomed(hdlg))
	{
		CenterWindow(GetParent(hdlg), s_netstat_view);
		RECT rect;
		GetWindowRect(s_netstat_view, &rect);
		if (rect.left < 0 || rect.top < 0)
		{
			SetWindowPos(hdlg, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}
	}
	SetWindowPos(hdlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetWindowPos(hdlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

VOID OnNetViewClose(HWND hdlg, WPARAM wp, LPARAM lp)
{
	KillTimer(hdlg, MSG_CHECK_TIMER);
	EndDialog(hdlg, 0);
	DestroyWindow(hdlg);
	s_netstat_view = NULL;
	ImageList_Destroy(s_image_list);
	s_image_list = NULL;
	s_images_data.clear();
	s_pe_data.clear();
	s_cur_filter.clear();	
	s_cur_netstates.clear();
	s_org_tcp.clear();
	s_org_udp.clear();
	set<HICON>::iterator itm;
	for (itm = s_icos.begin() ; itm != s_icos.end() ; itm++)
	{
		DestroyIcon(*itm);
	}
	s_icos.clear();
}

static VOID OnNotify(HWND hdlg, WPARAM wp, LPARAM lp)
{
	LPNMHDR msg = (LPNMHDR)lp;
	if (!msg || s_netstat_list != msg->hwndFrom)
	{
		return;
	}

	switch(msg->code)
	{
	case  NM_DBLCLK:
		{
			LPNMITEMACTIVATE mu = (LPNMITEMACTIVATE)lp;
			int idex = GetDelayValidIdex(s_netstat_list, mu->iItem);
			ProcessInfo info;
			if (s_cur_filter.size() > (size_t)idex)
			{
				info = s_cur_netstates[s_cur_filter[idex]].m_process;
				if (info.m_pid != 0 && info.m_pid != 4)
				{
					ShowProcessStat(hdlg, s_cur_netstates[s_cur_filter[idex]].m_process);
				}
			}
		}
		break;
	}
	return;
}

VOID OnExecCmd(HWND hdlg, WPARAM wp, LPARAM lp)
{
	NetstatCmd *cmd = (NetstatCmd *)lp;
	if (cmd)
	{
		s_netstat_str = cmd->m_cmd;
		AnalysisFilterStr();
		SetWindowTextA(s_edit, s_netstat_str.c_str());
		OnFilterRet(hdlg, wp, lp);
		UpdateNetMsg();
	}
}

static void _OnCommand(HWND hdlg, WPARAM wp, LPARAM lp) {
    WORD id = LOWORD(wp);
    if (id == IDC_NETSTAT_BTN_SEARCH)
    {
        SendMessageA(s_netstat_view, MSG_ON_FILTER_RET, 0, 0);
    }
}

INT_PTR CALLBACK NetStateProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
		case WM_INITDIALOG:
			{
				OnInitDialog(hdlg, wp, lp);
			}
			break;
		case  WM_TIMER:
			{
				OnTimer(hdlg, wp, lp);
			}
			break;
        case WM_COMMAND:
            _OnCommand(hdlg, wp, lp);
            break;
		case  WM_NOTIFY:
			{
				OnNotify(hdlg, wp, lp);
			}
			break;
		case  MSG_ACTIVE_WINDOW:
			{
				OnActiveWindow(hdlg, wp, lp);
			}
			break;
		case  MSG_ON_FILTER_RET:
			{
				OnFilterRet(hdlg, wp, lp);
				UpdateNetMsg();
			}
			break;
		case MSG_EXEC_CMD:
			{
				OnExecCmd(hdlg, wp, lp);
			}
			break;
		case  WM_CLOSE:
			{
				OnNetViewClose(hdlg, wp, lp);
			}
			break;
	}
	return 0;
}

HWND RunNetstatView(HWND parent, LPVOID param)
{
	HWND hwnd = NULL;
	if (!s_netstat_view)
	{
		hwnd = CreateDialogParamA(g_m, MAKEINTRESOURCEA(IDD_NETSTATE), parent, NetStateProc, (LPARAM)param);
	}
	else
	{
		SendMessageA(s_netstat_view, MSG_ACTIVE_WINDOW, (WPARAM)parent, 0);
		hwnd = s_netstat_view;
	}
	return hwnd;
}