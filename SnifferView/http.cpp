#include <WinSock2.h>
#include <Windows.h>
#include <map>
#include <mstring.h>
#include "filter.h"
#include "analysis.h"
#include "http.h"

using namespace std;

static HANDLE s_http_lock = CreateMutexA(NULL, FALSE, NULL);
//当前的http状态，因为一个http请求包或者回执包经常会分多个封包进行
//传输，这时候判断一个封包是否是http包只能通过上个包的状态来进行
//判断
static map<mstring, HttpState> s_http_connect;

#define  LOCK_HTTP	WaitForSingleObject(s_http_lock, INFINITE);
#define UNLOCK_HTTP ReleaseMutex(s_http_lock)

//是否是一个http请求头
BOOL IsDataHttpRequstHeader(IN const char *packet, OUT HttpRequest &type)
{
	int mk = 0;
	if (0 == strncmp(packet, HTTP_REQUEST_GET, strlen(HTTP_REQUEST_GET)))
	{
		type = em_http_get;
		mk += strlen(HTTP_REQUEST_GET);
	}
	else if (0 == strncmp(packet, HTTP_REQUEST_POST, strlen(HTTP_REQUEST_POST)))
	{
		type = em_http_post;
		mk += strlen(HTTP_REQUEST_POST);
	}
	else if (0 == strncmp(packet, HTTP_REQUEST_HEAD, strlen(HTTP_REQUEST_HEAD)))
	{
		type = em_http_head;
		mk += strlen(HTTP_REQUEST_HEAD);
	}
	else if (0 == strncmp(packet, HTTP_REQUEST_OPTIONS, strlen(HTTP_REQUEST_OPTIONS)))
	{
		type = em_http_options;
		mk += strlen(HTTP_REQUEST_OPTIONS);
	}
	else if (0 == strncmp(packet, HTTP_REQUEST_PUT, strlen(HTTP_REQUEST_PUT)))
	{
		type = em_http_put;
		mk += strlen(HTTP_REQUEST_PUT);
	}
	else if (0 == strncmp(packet, HTTP_REQUEST_DELETE, strlen(HTTP_REQUEST_DELETE)))
	{
		type = em_http_delete;
		mk += strlen(HTTP_REQUEST_DELETE);
	}
	else if (0 == strncmp(packet, HTTP_REQUEST_TARCE, strlen(HTTP_REQUEST_TARCE)))
	{
		type = em_http_tarce;
		mk += strlen(HTTP_REQUEST_TARCE);
	}
	else
	{
		return FALSE;
	}

	if (packet[mk] != ' ')
	{
		return FALSE;
	}

	const char *p = strstr(packet + mk, HTTP_SECTION_END);
	if (!p)
	{
		return FALSE;
	}
	size_t ms = p - packet;
	//定位到http标记位置
	ms--;
	while(packet[ms] == ' ' && ms > 0)
	{
		ms--;
	}
	if (ms <= 0)
	{
		return FALSE;
	}

	while(packet[ms] != ' ' && ms > 0)
	{
		ms--;
	}
	if (ms <= 0)
	{
		return FALSE;
	}

	ms += 1;
	if (0 != strncmp(packet + ms, HTTP_11_MARK, strlen(HTTP_11_MARK)))
	{
		if (0 != strncmp(packet + ms, HTTP_10_MARK, strlen(HTTP_10_MARK)))
		{
			return FALSE;
		}
	}
	return TRUE;
}

//是否是一个http答复头
BOOL IsDataHttpRespHeader(IN const char *packet, OUT HttpResp &type)
{
	if (0 != strncmp(packet, HTTP_11_MARK, strlen(HTTP_11_MARK)))
	{
		if (0 != strncmp(packet, HTTP_10_MARK, strlen(HTTP_10_MARK)))
		{
			return FALSE;
		}
	}

    //http返回包，解析具体的返回类型
    //LPCSTR ptr = packet + lstrlenA(HTTP_10_MARK);
    //while (*ptr && ' ' == *ptr)
    //{
    //    ptr++;
    //}

	if (strstr(packet + strlen(HTTP_11_MARK), HTTP_SECTION_END))
	{
		type = em_http_unknow;
		return TRUE;
	}
	return FALSE;
}

HttpState::HttpState()
{
	m_http_state = HTTP_STATE_UNINIT;
	m_cur_state = HTTP_STATE_UNINIT;
	m_full_packet = false;
	m_full_count = 0;
	m_pass_count = 0;
	m_full_header = false;
	m_length_type = em_length_uninit;

	//test
	m_src_addr = 0;
	m_src_port = 0;
	m_dst_addr = 0;
	m_dst_port = 0;
}

HttpState::~HttpState()
{
}

void HttpState::reset()
{
	//m_http_state = HTTP_STATE_UNINIT;
	m_full_packet = false;
	m_full_count = 0;
	m_pass_count = 0;
	m_full_header = false;
	m_length_type = em_length_uninit;
	m_packet.clear_with_mem();
	m_header.clear_with_mem();
}

bool HttpState::is_full_header()
{
	int ms = m_packet.find(HTTP_HEADER_END);
	return mstring::npos != ms;
}

void HttpState::update_state()
{
	if (m_length_type == em_length_uninit)
	{
		return;
	}
	int mu = 0;
	mstring ss;
	if (em_var_length == m_length_type)
	{
		int ms = m_packet.find(HTTP_VAR_END);
		if (mstring::npos == ms)
		{
			return;
		}
		else if (mstring::npos != ms)
		{
			mu =  (int)(ms + lstrlenA(HTTP_VAR_END));
			if (m_packet.size() > (size_t)mu)
			{
				//同时处理多个http请求
				ss.append(m_packet.c_str() + mu, m_packet.size() - mu);
				reset();
				push(ss.c_str(), ss.size());
			}
			else
			{
				reset();
			}
		}
	}
	else if (em_fix_length == m_length_type)
	{
		if (m_packet.size() > (size_t)m_full_count)
		{
			//同时处理多个http请求
			ss.append(m_packet.c_str() + m_full_count, m_packet.size() - m_full_count);
			reset();
			push(ss.c_str(), ss.size());
		}
		else if (m_packet.length() == m_full_count)
		{
			reset();
		}
		else if (m_packet.length() < (size_t)m_full_count)
		{
			//http请求尚未完成
		}
	}
	else
	{
		//不可能执行到这里
	}
}

//这个方法的前提是http请求
void HttpState::extract_url()
{
	int b = m_packet.find(" ");
	int e = m_packet.find(HTTP_SECTION_END);
	int c = m_packet.find(HTTP_11_MARK);
	if (c == mstring::npos)
	{
		c = m_packet.find(HTTP_10_MARK);
	}
	if (c == mstring::npos)
	{
		return;
	}
	if (b != mstring::npos && e != mstring::npos &&  e > c && e > b)
	{
		int itm = b;
		while(m_packet.c_str()[itm] == ' ')
		{
			itm++;
		}
		b = itm;
		itm = c - 1;
		if (m_packet.c_str()[itm] != ' ')
		{
			return;
		}
		while(m_packet.c_str()[itm] == ' ')
		{
			itm--;
		}
		c = itm;
		if(b > 0 && c > b)
		{
			m_urls.push_back(mstring(m_packet.c_str() + b, c - b + 1));
		}
	}
}

void HttpState::update_length()
{
	HttpRequest a;
	HttpResp b;
	if (IsDataHttpRequstHeader(m_packet.c_str(), a))
	{
		extract_url();
		m_http_state = a;
		m_cur_state |= a;
	}
	else if (IsDataHttpRespHeader(m_packet.c_str(), b))
	{
		m_http_state = b;
		m_cur_state |= b;
	}
	else
	{
		m_packet.clear();
		//m_org_packet.clear();
		reset();
		return;
	}

	if (!is_full_header())
	{
		return;
	}
	m_full_header = true;
	int ma = 0;
	int mb = 0;
	int ms = m_packet.find(HTTP_HEADER_END);
	if (mstring::npos == ms)
	{
		return;
	}

	m_full_header = true;
	ma = m_packet.rfind(HTTP_LENGTH_MARK, ms);
	mb = m_packet.rfind(HTTP_VAR_MARK, ms);
	int length = 0;
	if (mb != mstring::npos)
	{
		//var
		m_length_type = em_var_length;
	}
	else if (ma != mstring::npos)
	{
		//fix
		length = atoi(m_packet.c_str() + ma + lstrlenA(HTTP_LENGTH_MARK));
		//http头
		length += (ms + lstrlenA(HTTP_HEADER_END));
		m_length_type = em_fix_length;
		m_full_count = length;
	}
	else
	{
		length = (ms + lstrlenA(HTTP_HEADER_END));
		m_length_type = em_fix_length;
		m_full_count = length;
	}
}

void HttpState::set_addr(ULONG src_addr, USHORT src_port, ULONG dst_addr, USHORT dst_port)
{
	if (m_src_addr && m_src_port && m_dst_addr && m_dst_port)
	{
		if (src_addr != m_src_addr || src_port != m_src_port || dst_addr != m_dst_addr || dst_port != m_dst_port)
		{
			int b = 0;
		}
	}
	m_src_addr = src_addr;
	m_src_port = src_port;
	m_dst_addr = dst_addr;
	m_dst_port = dst_port;
}

int HttpState::get_state()
{
	return m_cur_state;
}

void HttpState::get_urls(list<mstring> &urls)
{
	urls = m_urls;
}

void HttpState::add_packet(const char *packet, size_t length)
{
	m_cur_state = HTTP_STATE_UNINIT;
	m_urls.clear();
	push(packet, length);
}

void HttpState::push(const char *buffer, size_t length)
{
	if (buffer[0] == 0x00)
	{
		return;
	}

	if (m_full_packet)
	{
		reset();
	}

	HttpRequest a;
	HttpResp b;
	if((IsDataHttpRequstHeader(buffer, a) || IsDataHttpRespHeader(buffer, b)))
	{
		if (m_packet.size() > 0)
		{
			//OutputDebugStringA("\r\n[k]###############################\r\n");
			//mstring header;
			//int vv = m_packet.find(HTTP_HEADER_END);
			//if (vv == mstring::npos)
			//{
			//	OutputDebugStringA("header error\r\n");
			//	OutputDebugStringA(m_packet.c_str());
			//}
			//vv += lstrlenA(HTTP_HEADER_END);
			//header.append(m_packet.c_str(), vv);
			//OutputDebugStringA("header:\r\n");
			//OutputDebugStringA(header.c_str());

			//IN_ADDR as;
			//as.S_un.S_addr = m_src_addr;
			//IN_ADDR bs;
			//bs.S_un.S_addr = m_dst_addr;
			//mstring ww;
			//USHORT src = n2h_16(m_src_port);
			//USHORT dst = n2h_16(m_dst_port);
			//char xxx[64] = {0x00};
			//char www[64] = {0x00};
			//lstrcpyA(xxx, inet_ntoa(as));
			//lstrcpyA(www, inet_ntoa(bs));
			//ww.format("packet: src:%hs:%u, dst:%hs:%u\r\n", xxx, src, www, dst);
			//OutputDebugStringA(ww.c_str());
			//OutputDebugStringA(packet.c_str());
			reset();
		}
	}
	else if (m_packet.size() == 0)
	{
		reset();
		return;
	}
	m_packet.append(buffer, length);
	update_length();
	update_state();
}

bool HttpState::is_http()
{
	return m_http_state != HTTP_STATE_UNINIT;
}

bool HttpState::is_get()
{
	return m_http_state == em_http_get;
}

bool HttpState::is_post()
{
	return m_http_state == em_http_post;
}

bool HttpState::is_head()
{
	return m_http_state == em_http_head;
}

bool HttpState::is_options()
{
	return m_http_state == em_http_options;
}

bool HttpState::is_put()
{
	return m_http_state == em_http_put;
}

bool HttpState::is_delete()
{
	return m_http_state == em_http_delete;
}

bool HttpState::is_tarce()
{
	return m_http_state == em_http_tarce;
}
bool HttpState::is_resp()
{
	return m_http_state == em_http_unknow;
}

//test 清除http缓存
VOID WINAPI ClearHttpBuffer()
{
	LOCK_HTTP;
	s_http_connect.clear();
	UNLOCK_HTTP;
}

//http观察者接口，负责解析http封吧，真正的http过滤函数直接读取结果即可
BOOL WINAPI HttpWatcher(IN OUT PacketContent &msg)
{
	if (!msg.m_packet_init)
	{
		return FALSE;
	}
	IPHeader *ip = (IPHeader *)msg.m_packet.c_str();
	if (ip->m_ipProtocol != PROTO_TCP)
	{
		return TRUE;
	}

	mstring mark = msg.m_dec_mark;
	TCPHeader *tcp = (TCPHeader *)(msg.m_packet.c_str() + sizeof(IPHeader));
	LOCK_HTTP;
	if (tcp->is_flag_syn() && !tcp->is_flag_ack())
	{
		if (s_http_connect.end() != s_http_connect.find(mark))
		{
			s_http_connect.erase(mark);
			UNLOCK_HTTP;
			return TRUE;
		}
	}

	int mm = sizeof(IPHeader) + tcp->get_tcp_header_length();
	if (msg.m_packet.size() < 0)
	{
		return TRUE;
	}
	bool http = false;
	if (s_http_connect.find(mark) != s_http_connect.end())
	{	
		s_http_connect[mark].set_addr(ip->m_ipSource, tcp->m_sourcePort, ip->m_ipDestination, tcp->m_destinationPort);
		s_http_connect[mark].add_packet(msg.m_packet.c_str() + mm, msg.m_packet.size() - mm);
		http = true;
	}
	else
	{
		HttpRequest a;
		HttpResp b;
		if (IsDataHttpRequstHeader(msg.m_packet.c_str() + mm, a) || IsDataHttpRespHeader(msg.m_packet.c_str() + mm, b))
		{
			s_http_connect[mark].set_addr(ip->m_ipSource, tcp->m_sourcePort, ip->m_ipDestination, tcp->m_destinationPort);
			s_http_connect[mark].add_packet(msg.m_packet.c_str() + mm, msg.m_packet.size() - mm);
			http = true;
		}
	}

	if (http)
	{
		s_http_connect[mark].get_urls(msg.m_urls);
		int state = s_http_connect[mark].get_state();
		if (state != HTTP_STATE_UNINIT)
		{
			msg.m_user_type = em_user_http;
			msg.m_user_content.m_http.m_http_type = state;
		}
	}
	UNLOCK_HTTP;
	return TRUE;
}

//http filter fun
BOOL WINAPI PacketFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	return msg->m_user_type == em_user_http;
}

//http.get filter fun
BOOL WINAPI PacketGetFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	return (msg->m_user_type == em_user_http && (msg->m_user_content.m_http.m_http_type & em_http_get));
}

//http.post filter fun
BOOL WINAPI PacketPostFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	return (msg->m_user_type == em_user_http && (msg->m_user_content.m_http.m_http_type & em_http_post));
}

//http.head
BOOL WINAPI PacketHeadFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	return (msg->m_user_type == em_user_http && (msg->m_user_content.m_http.m_http_type & em_http_head));
}

//http.options
BOOL WINAPI PacketOptionsFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	return (msg->m_user_type == em_user_http && (msg->m_user_content.m_http.m_http_type & em_http_options));
}

//http.put
BOOL WINAPI PacketPutFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	return (msg->m_user_type == em_user_http && (msg->m_user_content.m_http.m_http_type & em_http_put));
}

//http.delete
BOOL WINAPI PacketDeleteFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	return (msg->m_user_type == em_user_http && (msg->m_user_content.m_http.m_http_type & em_http_delete));
}

//http.tarce
BOOL WINAPI PacketTarceFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	return (msg->m_user_type == em_user_http && (msg->m_user_content.m_http.m_http_type & em_http_tarce));
}

//http.resp
BOOL WINAPI PacketRespFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	return (msg->m_user_type == em_user_http && (msg->m_user_content.m_http.m_http_type == em_http_unknow));
}

//http.url contains \".{1,}\"
BOOL WINAPI PacketUrlFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	if (msg->m_user_type == em_user_http)
	{
		list<mstring>::iterator itm;
		mstring url;
		mstring ms;
		for (itm = msg->m_urls.begin() ; itm != msg->m_urls.end() ; itm++)
		{
			url.clear();
			if (g_str_mark)
			{
				url.append(itm->c_str(), itm->size());
				url.makelower();
				ms = param;
				ms.makelower();
				if (url.find(ms.c_str()) != mstring::npos)
				{
					return TRUE;
				}
			}
			else
			{
				if (itm->find(param.c_str()) != mstring::npos)
				{
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}