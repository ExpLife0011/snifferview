/*
 *  filename:  rules.cpp
 *  author:    lougd
 *  created:    2015-4-22 14:58
 *  version:    1.0.0.1
 *  desc:       过滤规则的具体实现
 *  history:
*/
#include <WinSock2.h>
#include "../ComLib/deelx.h"
#include <map>
#include "rules.h"
#include "http.h"

using namespace std;

#define  FILTER_RULES_IP                    ("ip")
#define  FILTER_RULES_IP_ADDR               ("ip\\.(src|dst|addr)(>=|<=|==|!=|>|<|&)((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)")
#define  FILTER_RULES_IP_LENGTH             ("ip\\.length(>=|<=|==|!=|>|<|&)(0x[a-f0-9]{1,4}|[0-9]{1,6})")

#define  FILTER_RULES_TCP                   ("tcp")
#define  FILTER_RULES_TCP_FLAG              ("tcp\\.flag\\.(syn|ack|fin|rst|psh|urg)")
#define  FILTER_RULES_TCP_PORT              ("tcp\\.(src|dst|port)(>=|<=|==|!=|>|<|&)(0x[a-f0-9]{1,4}|[0-9]{1,6})")
#define  FILTER_RULES_TCP_LENGTH            ("tcp\\.length(>=|<=|==|!=|>|<|&)(0x[a-f0-9]{1,4}|[0-9]{1,6})")
#define  FILTER_RULES_TCP_CONTENT           ("tcp\\[((0x[0-9a-f]{1,4}|[0-9]{1,6}):){0,1}(n8|n16|n32|byte|bytes|char|chars)\\](>=|<=|>|<|==|!=|&)(0x[0-9a-f]{1,8}|[0-9]{1,16}|(((0x[0-9a-f]{1,8}|[0-9]{1,16}):){0,}(0x[0-9a-f]{1,8}|[0-9]){1,16})|'.'|\".{1,}\"")
#define  FILTER_RULES_TCP_CONTAINS          ("tcp contains \".{1,}\"")

#define  FILTER_RULES_UDP                   ("udp")
#define  FILTER_RULES_UDP_PORT              ("udp\\.(src|dst|port)(>=|<=|==|!=|>|<|&)(0x[a-f0-9]{1,4}|[0-9]{1,6})")
#define  FILTER_RULES_UDP_LENGTH            ("udp\\.length(>=|<=|==|!=|>|<|&)(0x[a-f0-9]{1,4}|[0-9]{1,6})")
#define  FILTER_RULES_UDP_CONTENT           ("udp\\[((0x[0-9a-f]{1,4}|[0-9]{1,6}):){0,1}(n8|n16|n32|byte|bytes|char|chars)\\](>=|<=|>|<|==|!=|&)(0x[0-9a-f]{1,8}|[0-9]{1,16}|(((0x[0-9a-f]{1,8}|[0-9]{1,16}):){0,}(0x[0-9a-f]{1,8}|[0-9]){1,16})|'.'|\".{1,}\"")
#define  FILTER_RULES_UDP_CONTAINS          ("udp contains \".{1,}\"")

#define  FILTER_RULES_ICMP                  ("icmp")

#define  FILTER_RULES_HTTP                  ("http")
#define  FILTER_RULES_HTTP_GET              ("http\\.get")
#define  FILTER_RULES_HTTP_POST             ("http\\.post")
#define  FILTER_RULES_HTTP_HEAD             ("http\\.head")
#define  FILTER_RULES_HTTP_OPTIONS          ("http\\.options")
#define  FILTER_RULES_HTTP_PUT              ("http\\.put")
#define  FILTER_RULES_HTTP_DELETE           ("http\\.delete")
#define  FILTER_RULES_HTTP_TARCE            ("http\\.tarce")
#define  FILTER_RULES_HTTP_RESP             ("http\\.resp")
#define  FILTER_RULES_HTTP_URLS             ("http\\.url contains \".{1,}\"")

#define  ISSIGN(letter) (letter == '>' || letter == '<' || letter == '=' || letter == '!' || letter == '&')
#define  OFFSET(class_name, member) (size_t)&(((class_name *)NULL)->member)

static map<mstring, PRegMatchFun> s_rules_analysis;

static map<mstring, FilterSign> s_string_sign;

static map<mstring, FilterData> s_string_data_type;

//ip
BOOL WINAPI FilterRulesIp(IN const char *rule, OUT FilterRules &result)
{
	result.clear();
	result.m_status = em_status_normal;
	return TRUE;
}

//ip\.(src|dst|addr)(>=|<=|==|!=|>|<|&)((2[0-4]\d|25[0-5]|[01]?\d\d?)\.){3}(2[0-4]\d|25[0-5]|[01]?\d\d?)
BOOL WINAPI FilterRulesIpaddr(IN const char *rule, OUT FilterRules &result)
{
	mstring tmp(rule);
	size_t mark;
	FixedContentsFilter src;
	FixedContentsFilter dst;
	FilterSign sign;
	char tm[5] = {0x00};
	list<FixedContentsFilter> lst;
	if (0 == tmp.find("ip.src"))
	{
		mark = lstrlenA("ip.src");
		src.m_data_type = em_unsigned_n32;
		src.m_encode = em_big_head;
		src.m_offest = 12;
	}
	else if (0 == tmp.find("ip.dst"))
	{
		mark = lstrlenA("ip.dst");
		dst.m_data_type = em_unsigned_n32;
		dst.m_encode = em_big_head;
		dst.m_offest = 16;
	}
	else if (0 == tmp.find("ip.addr"))
	{
		mark = lstrlenA("ip.addr");
		src.m_data_type = em_unsigned_n32;
		src.m_encode = em_big_head;
		src.m_offest = 12;
		dst.m_data_type = em_unsigned_n32;
		dst.m_encode = em_big_head;
		dst.m_offest = 16;
	}
	else
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	if (ISSIGN(*(rule + mark)))
	{
		tm[0] = rule[mark];
		if (ISSIGN(*(rule + mark + 1)))
		{
			tm[1] = rule[mark + 1];
		}
	}

	mark += lstrlen(tm);
	mstring data = (tmp.c_str() + mark);
	if (s_string_sign.end() == s_string_sign.find(tm))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	DWORD ds;
	if (!CheckIpaddress(data.c_str(), ds))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}	
	sign = s_string_sign[tm];
	if (src.m_data_type != em_data_type_undefine)
	{
		src.m_sign = sign;
		src.m_data.append((const char *)&ds, sizeof(ds));
		lst.push_back(src);
	}

	if (dst.m_data_type != em_data_type_undefine)
	{
		dst.m_sign = sign;
		dst.m_data.append((const char *)&ds, sizeof(ds));
		lst.push_back(dst);
	}

	if (lst.size() > 0)
	{
		result.m_status = em_status_normal;
		result.m_fixed_data_filters.push_back(lst);
	}
	return (lst.size() > 0);
}

//ip.length(>=|<=|==|!=|>|<)(0x[a-f0-9]{1,4}|[0-9]{1,6})
BOOL WINAPI FilterRulesIpLength(IN const char *rule, OUT FilterRules &result)
{
	size_t mark = lstrlenA("ip.length");
	size_t vt = 0;
	char ms[5] = {0x00};
	if (ISSIGN((rule + mark)[0]))
	{
		ms[0] = (rule + mark)[0];
		vt += 1;
		if (ISSIGN((rule + mark)[1]))
		{
			ms[1] = (rule + mark)[1];
			vt += 1;
		}
	}	

	if (s_string_sign.end() == s_string_sign.find(ms))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	mark += vt;
	mstring data(rule + mark);
	unsigned short n16;
	if (!CheckUnsignedN16(data.c_str(), n16))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	list<LengthFilter> lst;
	LengthFilter vm;
	vm.m_length = n16 + sizeof(IPHeader);
	vm.m_sign = s_string_sign[ms];
	lst.push_back(vm);
	result.m_length_filters.push_back(lst);
	result.m_status = em_status_normal;
	return TRUE;
}

//tcp
BOOL WINAPI FilterRulesTcp(IN const char *rule, OUT FilterRules &result)
{
	result.clear();
	list<FixedContentsFilter> lst;
	FixedContentsFilter filter;
	filter.m_offest = OFFSET(IPHeader, m_ipProtocol);
	filter.m_data_type = em_unsigned_n8;
	filter.m_encode = em_big_head;
	filter.m_sign = em_equal;
	UCHAR pt = IPPROTO_TCP;
	filter.m_data.append((const char *)(&pt), sizeof(pt));
	lst.push_back(filter);
	result.m_status = em_status_normal;
	result.m_fixed_data_filters.push_back(lst);
	return TRUE;
}

//tcp\\.flag\\.(syn|ack|fin|rst|psh|urg|ace|cwr)
BOOL WINAPI FilterRulesTcpFlag(IN const char *rule, OUT FilterRules &result)
{
	result.clear();
	FilterRulesTcp("tcp", result);
	mstring tmp = rule + lstrlenA("tcp.flag.");
	FixedContentsFilter filter;
	list<FixedContentsFilter> lst;
	filter.m_data_type = em_unsigned_n8;
	filter.m_encode = em_big_head;
	filter.m_offest = sizeof(IPHeader) + OFFSET(TCPHeader, m_flags);
	filter.m_sign = em_bit_and;
	UCHAR u;
	if (tmp == "syn")
	{
		u = 0x02;
	}
	else if (tmp == "ack")
	{
		u = 0x10;
	}
	else if (tmp == "fin")
	{
		u = 0x01;
	}
	else if (tmp == "rst")
	{
		u = 0x04;
	}
	else if (tmp == "psh")
	{
		u = 0x08;
	}
	else if (tmp == "urg")
	{
		u = 0x20;
	}
	else if (tmp == "ace")
	{
		u = 0x40;
	}
	else if (tmp == "cwr")
	{
		u = 0x80;
	}
	else
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	filter.m_data.append((const char *)&u, sizeof(u));
	lst.push_back(filter);
	result.m_fixed_data_filters.push_back(lst);
	return TRUE;
}

//tcp\.(src|dst|port)(>=|<=|==|!=|>|<)(0x[a-f0-9]{1,4}|[0-9]{1,6})
BOOL WINAPI FilterRulesTcpPort(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcp("tcp", result);
	size_t mark = 0;
	mstring tmp(rule);
	FixedContentsFilter src;
	FixedContentsFilter dst;
	list<FixedContentsFilter> lst_port;
	if (0 == tmp.find("tcp.port"))
	{
		mark = lstrlenA("tcp.port");
		src.m_data_type = em_unsigned_n16;
		src.m_encode = em_big_head;
		src.m_offest = sizeof(IPHeader) + OFFSET(TCPHeader, m_sourcePort);
		dst.m_data_type = em_unsigned_n16;
		dst.m_encode = em_big_head;
		dst.m_offest = sizeof(IPHeader) + OFFSET(TCPHeader, m_destinationPort);
	}
	else if (0 == tmp.find("tcp.src"))
	{
		mark = lstrlenA("tcp.src");
		src.m_data_type = em_unsigned_n16;
		src.m_encode = em_big_head;
		src.m_offest = sizeof(IPHeader) + OFFSET(TCPHeader, m_sourcePort);
	}
	else if (0 == tmp.find("tcp.dst"))
	{
		mark = lstrlenA("tcp.dst");
		dst.m_data_type = em_unsigned_n16;
		dst.m_encode = em_big_head;
		dst.m_offest = sizeof(IPHeader) + OFFSET(TCPHeader, m_destinationPort);
	}
	else
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	char mv[5] = {0x00};
	size_t sv = 0;
	if (ISSIGN(rule[mark]))
	{
		mv[0] = rule[mark];
		sv++;
		if (ISSIGN(rule[mark + 1]))
		{
			mv[1] = rule[mark + 1];
			sv++;
		}
	}

	if (s_string_sign.end() == s_string_sign.find(mv))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	mark += sv;
	mstring data(tmp.c_str() + mark);
	USHORT port;
	if (!CheckUnsignedN16(data.c_str(), port))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	if (src.m_data_type != em_data_type_undefine)
	{
		src.m_sign = s_string_sign[mv];
		src.m_data.append((const char *)&port, sizeof(port));
		lst_port.push_back(src);
	}

	if (dst.m_data_type != em_data_type_undefine)
	{
		dst.m_sign = s_string_sign[mv];
		dst.m_data.append((const char *)&port, sizeof(port));
		lst_port.push_back(dst);
	}
	result.m_status = em_status_normal;
	result.m_fixed_data_filters.push_back(lst_port);
	return TRUE;
}

static BOOL WINAPI FilterRulesTcpLengthFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param)
{
	unsigned short length = *((unsigned short *)param.c_str());
	TCPHeader *tcp = (TCPHeader *)(msg->m_packet.c_str() + sizeof(IPHeader));
	length += (sizeof(IPHeader) + tcp->get_tcp_header_length());
	return IsTrue(msg->m_packet.size(), sign, length);
}

//tcp.length(>=|<=|==|!=|>|<)(0x[a-f0-9]{1,4}|[0-9]{1,6})，不包括tcp头
BOOL WINAPI FilterRulesTcpLength(IN const char *rule, OUT FilterRules &result)
{
	result.clear();
	FilterRulesTcp("tcp", result);
	size_t mark = lstrlenA("tcp.length");
	char mv[5] = {0x00};
	size_t sv = 0;
	if (ISSIGN(rule[mark]))
	{
		mv[0] = rule[mark];
		sv++;
		if (ISSIGN(rule[mark + 1]))
		{
			mv[1] = rule[mark + 1];
			sv++;
		}
	}
	if (s_string_sign.end() == s_string_sign.find(mv))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	mark += sv;
	unsigned short length;
	if (!CheckUnsignedN16(rule + mark, length))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	//tcp包头是变长的
	FunFilter vv;
	vv.m_filter_fun = FilterRulesTcpLengthFun;
	vv.m_params.append((const char *)(&length), sizeof(length));
	vv.m_sign = s_string_sign[mv];
	result.m_filter_funs.push_back(vv);	
	return TRUE;
}

//tcp\\[((0x[0-9a-f]{1,4}|[0-9]{1,6}):){0,1}(n8|n16|n32|byte|bytes|char|chars)\\](>=|<=|>|<|==|!=)(0x[0-9a-f]{1,8}|[0-9]{1,16}|'.'|\".{0,}\"
BOOL WINAPI FilterRulesTcpContent(IN const char *rule, OUT FilterRules &result)
{
	result.clear();
	FilterRulesTcp("tcp", result);
	mstring tmp = rule;
	size_t es = tmp.find('[');
	size_t ev = tmp.find(']');
	int m = tmp.find(':');
	unsigned short offset;
	mstring ms;
	if (m == mstring::npos || m > (int)ev)
	{
		//offset = sizeof(IPHeader) + sizeof(TCPHeader);
		offset = 0;
		ms.assign(tmp, es + 1, ev - es - 1);
	}
	else
	{
		mstring vm(rule + lstrlenA("tcp["), m - lstrlenA("tcp["));
		if (!CheckUnsignedN16(vm.c_str(), offset))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		//offset += sizeof(IPHeader);
		//offset += sizeof(TCPHeader);
		ms.assign(tmp, m + 1, ev - m - 1);
	}

	size_t mark = ev + 1;
	size_t sv = 0;
	char mv[5] = {0x00};
	if (ISSIGN(rule[mark]))
	{
		mv[0] = rule[mark];
		sv++;
		if (ISSIGN(rule[mark + 1]))
		{
			mv[1] = rule[mark + 1];
			sv++;
		}
	}

	if (s_string_sign.end() == s_string_sign.find(mv))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	mark += sv;
	if (s_string_data_type.end() == s_string_data_type.find(ms) && ms != "chars")
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	list<FixedContentsFilter> lst;
	FixedContentsFilter filter;
	list<FixedStringFilter> lsm;
	FixedStringFilter vm;
	FilterData vt = s_string_data_type[mv];
	mstring dv = rule + mark;
	filter.m_offset_type = em_offset_tcp;
	filter.m_offest = offset;
	vm.m_offset_type = em_offset_tcp;
	vm.m_offest = offset;
	FilterSign sign = s_string_sign[mv];
	filter.m_sign = sign;
	vm.m_sign = sign;
	bool ma = false;
	bool mb = false;
	if (g_net_mark)
	{
		filter.m_encode = em_big_head;
	}
	else
	{
		filter.m_encode = em_little_head;
	}

	if (ms == "n8")
	{
		UCHAR u8;
		if (!CheckUnsignedN8(dv.c_str(), u8))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n8;
		filter.m_data.append((const char *)&u8, sizeof(u8));
		lst.push_back(filter);
	}
	else if (ms == "n16")
	{
		USHORT u16;
		if (!CheckUnsignedN16(dv.c_str(), u16))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n16;
		filter.m_data.append((const char *)&u16, sizeof(u16));
		lst.push_back(filter);
	}
	else if (ms == "n32")
	{
		ULONG u32;
		if (!CheckUnsignedN32(dv.c_str(), u32))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n32;
		filter.m_data.append((const char *)&u32, sizeof(u32));
		lst.push_back(filter);
	}
	else if (ms == "char")
	{
		if (dv.size() > 1)
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n8;
		filter.m_data += dv.c_str()[0];
		lst.push_back(filter);
	}
	else if (ms == "chars")
	{
		if (sign == em_bit_and)
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}

		dv.repsub("\\r", "\r");
		dv.repsub("\\n", "\n");
		if (dv.size() <= 2 || dv.c_str()[0] != '\"' || dv.c_str()[dv.size() - 1] != '\"')
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}

		if (g_str_mark)
		{
			vm.m_cm = em_string_nolose_cm;
		}
		else
		{
			vm.m_cm = em_sring_lose_cm;
		}
		vm.m_fuzz = em_string_fix;
		vm.m_data.assign(dv, 1, dv.size() - 2);
		lsm.push_back(vm);
	}
	else if (ms == "byte")
	{
		UCHAR u8;
		if (!CheckUnsignedN8(dv.c_str(), u8))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n8;
		filter.m_data.append((const char *)&u8, sizeof(u8));
		lst.push_back(filter);
	}
	else if(ms == "bytes")
	{
		if (sign == em_bit_and)
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
	
		list<mstring> vc;
		GetCutBufferList(dv.c_str(), ':', vc);
		if (vc.size() == 0)
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}

		list<mstring>::iterator itu;
		for (itu = vc.begin() ; itu != vc.end() ; itu++)
		{
			UCHAR u8;
			if (!CheckUnsignedN8(itu->c_str(), u8))
			{
				throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
				return FALSE;
			}
			filter.m_data_type = em_bytes;
			filter.m_data.append((const char *)&u8, sizeof(u8));
			lst.push_back(filter);
		}
	}
	else
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	if (lst.size() > 0)
	{
		result.m_fixed_data_filters.push_back(lst);
	}
	
	if (lsm.size() > 0)
	{
		result.m_fixed_string_filters.push_back(lsm);
	}
	return TRUE;
}

//tcp contains \".{1,}\"
BOOL WINAPI FilterRulesTcpContains(IN const char *rule, OUT FilterRules &result)
{
	result.clear();
	FilterRulesTcp("tcp", result);
	mstring vs = rule;
	int begin = vs.find('"');
	if (mstring::npos == begin)
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	int end = vs.size() - 1;
	size_t count = end - begin - 1;
	if (0 == count)
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	mstring vv(rule + begin + 1, count);
	vv.repsub("\\r", "\r");
	vv.repsub("\\n", "\n");
	list<RandStringFilter> lst;
	RandStringFilter rs;
	rs.m_begin = sizeof(IPHeader) + sizeof(TCPHeader);
	rs.m_end = 0xffff;
	g_str_mark ? rs.m_cm = em_string_nolose_cm : rs.m_cm = em_sring_lose_cm;
	rs.m_fuzz = em_string_fix;
	rs.m_sign = em_equal;
	rs.m_data = vv;
	if (rs.m_cm == em_sring_lose_cm)
	{
		rs.m_data.makelower();
	}
	lst.push_back(rs);
	result.m_rand_string_filters.push_back(lst);
	return TRUE;
}

//udp
BOOL WINAPI FilterRulesUdp(IN const char *rule, OUT FilterRules &result)
{
	list<FixedContentsFilter> lst;
	FixedContentsFilter filter;
	filter.m_offest = OFFSET(IPHeader, m_ipProtocol);
	filter.m_data_type = em_unsigned_n8;
	filter.m_encode = em_big_head;
	filter.m_sign = em_equal;
	UCHAR pt = IPPROTO_UDP;
	filter.m_data.append((const char *)(&pt), sizeof(pt));
	lst.push_back(filter);
	result.m_status = em_status_normal;
	result.m_fixed_data_filters.push_back(lst);
	return TRUE;
}

//udp\.(src|dst|port)(>=|<=|==|!=|>|<|&)(0x[a-f0-9]{1,4}|[0-9]{1,6})
BOOL WINAPI FilterRulesUdpPort(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesUdp("udp", result);
	size_t mark = 0;
	mstring tmp(rule);
	FixedContentsFilter src;
	FixedContentsFilter dst;
	list<FixedContentsFilter> lst_port;
	if (0 == tmp.find("udp.port"))
	{
		mark = lstrlenA("udp.port");
		src.m_data_type = em_unsigned_n16;
		src.m_encode = em_big_head;
		src.m_offest = sizeof(IPHeader) + OFFSET(UDPHeader, m_sourcePort);
		dst.m_data_type = em_unsigned_n16;
		dst.m_encode = em_big_head;
		dst.m_offest = sizeof(IPHeader) + OFFSET(UDPHeader, m_destinationPort);
	}
	else if (0 == tmp.find("udp.src"))
	{
		mark = lstrlenA("udp.src");
		src.m_data_type = em_unsigned_n16;
		src.m_encode = em_big_head;
		src.m_offest = sizeof(IPHeader) + OFFSET(UDPHeader, m_sourcePort);
	}
	else if (0 == tmp.find("udp.dst"))
	{
		mark = lstrlenA("udp.dst");
		dst.m_data_type = em_unsigned_n16;
		dst.m_encode = em_big_head;
		dst.m_offest = sizeof(IPHeader) + OFFSET(UDPHeader, m_destinationPort);
	}
	else
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	char mv[5] = {0x00};
	size_t sv = 0;
	if (ISSIGN(rule[mark]))
	{
		mv[0] = rule[mark];
		sv++;
		if (ISSIGN(rule[mark + 1]))
		{
			mv[1] = rule[mark + 1];
			sv++;
		}
	}

	if (s_string_sign.end() == s_string_sign.find(mv))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	mark += sv;
	mstring data(tmp.c_str() + mark);
	USHORT port;
	if (!CheckUnsignedN16(data.c_str(), port))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	if (src.m_data_type != em_data_type_undefine)
	{
		src.m_sign = s_string_sign[mv];
		src.m_data.append((const char *)&port, sizeof(port));
		lst_port.push_back(src);
	}

	if (dst.m_data_type != em_data_type_undefine)
	{
		dst.m_sign = s_string_sign[mv];
		dst.m_data.append((const char *)&port, sizeof(port));
		lst_port.push_back(dst);
	}
	result.m_fixed_data_filters.push_back(lst_port);
	return TRUE;
}

//udp.length(>=|<=|==|!=|>|<|&)(0x[a-f0-9]{1,4}|[0-9]{1,6})
BOOL WINAPI FilterRulesUdpLength(IN const char *rule, OUT FilterRules &result)
{
	result.clear();
	FilterRulesUdp("udp", result);
	size_t mark = lstrlenA("udp.length");
	char mv[5] = {0x00};
	size_t sv = 0;
	if (ISSIGN(rule[mark]))
	{
		mv[0] = rule[mark];
		sv++;
		if (ISSIGN(rule[mark + 1]))
		{
			mv[1] = rule[mark + 1];
			sv++;
		}
	}
	if (s_string_sign.end() == s_string_sign.find(mv))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	mark += sv;
	unsigned short length;
	if (!CheckUnsignedN16(rule + mark, length))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	LengthFilter filter;
	list<LengthFilter> lst;
	filter.m_length = length + sizeof(IPHeader) + sizeof(UDPHeader);
	filter.m_sign = s_string_sign[mv];
	lst.push_back(filter);
	result.m_length_filters.push_back(lst);
	return TRUE;
}

//udp\\[((0x[0-9a-f]{1,4}|[0-9]{1,6}):){0,1}(n8|n16|n32|byte|bytes|char|chars)\\](>=|<=|>|<|==|!=|&)(0x[0-9a-f]{1,8}|[0-9]{1,16}|(((0x[0-9a-f]{1,8}|[0-9]{1,16}):){0,}(0x[0-9a-f]{1,8}|[0-9]){1,16})|'.'|\".{1,}\"
BOOL WINAPI FilterRulesUdpContent(IN const char *rule, OUT FilterRules &result)
{
	result.clear();
	FilterRulesUdp("udp", result);
	mstring tmp = rule;
	size_t es = tmp.find('[');
	size_t ev = tmp.find(']');
	int m = tmp.find(':');
	unsigned short offset;
	mstring ms;
	if (m == mstring::npos || m > (int)ev)
	{
		offset = sizeof(IPHeader) + sizeof(UDPHeader);
		ms.assign(tmp, es + 1, ev - es - 1);
	}
	else
	{
		mstring vm(rule + lstrlenA("udp["), m - lstrlenA("udp["));
		if (!CheckUnsignedN16(vm.c_str(), offset))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		offset += sizeof(IPHeader);
		offset += sizeof(UDPHeader);
		ms.assign(tmp, m + 1, ev - m - 1);
	}

	size_t mark = ev + 1;
	size_t sv = 0;
	char mv[5] = {0x00};
	if (ISSIGN(rule[mark]))
	{
		mv[0] = rule[mark];
		sv++;
		if (ISSIGN(rule[mark + 1]))
		{
			mv[1] = rule[mark + 1];
			sv++;
		}
	}

	if (s_string_sign.end() == s_string_sign.find(mv))
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	mark += sv;
	if (s_string_data_type.end() == s_string_data_type.find(ms) && ms != "chars")
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	list<FixedContentsFilter> lst;
	FixedContentsFilter filter;
	list<FixedStringFilter> lsm;
	FixedStringFilter vm;
	FilterData vt = s_string_data_type[mv];
	mstring dv = rule + mark;
	filter.m_offest = offset;
	vm.m_offest = offset;
	FilterSign sign = s_string_sign[mv];
	filter.m_sign = sign;
	vm.m_sign = sign;
	bool ma = false;
	bool mb = false;
	if (g_net_mark)
	{
		filter.m_encode = em_big_head;
	}
	else
	{
		filter.m_encode = em_little_head;
	}

	if (ms == "n8")
	{
		UCHAR u8;
		if (!CheckUnsignedN8(dv.c_str(), u8))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n8;
		filter.m_data.append((const char *)&u8, sizeof(u8));
		lst.push_back(filter);
	}
	else if (ms == "n16")
	{
		USHORT u16;
		if (!CheckUnsignedN16(dv.c_str(), u16))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n16;
		filter.m_data.append((const char *)&u16, sizeof(u16));
		lst.push_back(filter);
	}
	else if (ms == "n32")
	{
		ULONG u32;
		if (!CheckUnsignedN32(dv.c_str(), u32))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n32;
		filter.m_data.append((const char *)&u32, sizeof(u32));
		lst.push_back(filter);
	}
	else if (ms == "char")
	{
		if (dv.size() > 1)
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n8;
		filter.m_data += dv.c_str()[0];
		lst.push_back(filter);
	}
	else if (ms == "chars")
	{
		if (sign == em_bit_and)
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}

		dv.repsub("\\r", "\r");
		dv.repsub("\\n", "\n");
		if (dv.size() <= 2 || dv.c_str()[0] != '\"' || dv.c_str()[dv.size() - 1] != '\"')
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}

		if (g_str_mark)
		{
			vm.m_cm = em_string_nolose_cm;
		}
		else
		{
			vm.m_cm = em_sring_lose_cm;
		}
		vm.m_fuzz = em_string_fix;
		vm.m_data.assign(dv, 1, dv.size() - 2);
		lsm.push_back(vm);
	}
	else if (ms == "byte")
	{
		UCHAR u8;
		if (!CheckUnsignedN8(dv.c_str(), u8))
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}
		filter.m_data_type = em_unsigned_n8;
		filter.m_data.append((const char *)&u8, sizeof(u8));
		lst.push_back(filter);
	}
	else if(ms == "bytes")
	{
		if (sign == em_bit_and)
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}

		list<mstring> vc;
		GetCutBufferList(dv.c_str(), ':', vc);
		if (vc.size() == 0)
		{
			throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
			return FALSE;
		}

		list<mstring>::iterator itu;
		for (itu = vc.begin() ; itu != vc.end() ; itu++)
		{
			UCHAR u8;
			if (!CheckUnsignedN8(itu->c_str(), u8))
			{
				throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
				return FALSE;
			}
			filter.m_data_type = em_bytes;
			filter.m_data.append((const char *)&u8, sizeof(u8));
			lst.push_back(filter);
		}
	}
	else
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	if (lst.size() > 0)
	{
		result.m_fixed_data_filters.push_back(lst);
	}

	if (lsm.size() > 0)
	{
		result.m_fixed_string_filters.push_back(lsm);
	}
	return TRUE;
}

BOOL WINAPI FilterRulesUdpContains(IN const char *rule, OUT FilterRules &result)
{
	result.clear();
	FilterRulesUdp("udp", result);
	mstring vs = rule;
	int begin = vs.find('"');
	if (mstring::npos == begin)
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}
	int end = vs.size() - 1;
	size_t count = end - begin - 1;
	if (0 == count)
	{
		throw FilterException(FILTER_EXCEPTION_SYNTAX_EREROR);
		return FALSE;
	}

	mstring vv(rule + begin + 1, count);
	vv.repsub("\\r", "\r");
	vv.repsub("\\n", "\n");
	list<RandStringFilter> lst;
	RandStringFilter rs;
	rs.m_begin = sizeof(IPHeader) + sizeof(UDPHeader);
	rs.m_end = 0xffff;
	g_str_mark ? rs.m_cm = em_string_nolose_cm : rs.m_cm = em_sring_lose_cm;
	rs.m_fuzz = em_string_fix;
	rs.m_sign = em_equal;
	rs.m_data = vv;
	lst.push_back(rs);
	result.m_rand_string_filters.push_back(lst);
	return TRUE;
}

BOOL WINAPI FilterRulesIcmp(IN const char *rule, OUT FilterRules &result)
{
    list<FixedContentsFilter> lst;
    FixedContentsFilter filter;
    filter.m_offest = OFFSET(IPHeader, m_ipProtocol);
    filter.m_data_type = em_unsigned_n8;
    filter.m_encode = em_big_head;
    filter.m_sign = em_equal;
    UCHAR pt = IPPROTO_ICMP;
    filter.m_data.append((const char *)(&pt), sizeof(pt));
    lst.push_back(filter);
    result.m_status = em_status_normal;
    result.m_fixed_data_filters.push_back(lst);
    return TRUE;
}

//http
BOOL WINAPI FilterRulesHttp(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	FunFilter http;
	http.m_filter_fun = PacketFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

//http.get
BOOL WINAPI FilterRulesHttpGet(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	FunFilter http;
	http.m_filter_fun = PacketGetFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

//http.post
BOOL WINAPI FilterRulesHttpPost(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	FunFilter http;
	http.m_filter_fun = PacketPostFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

//http.head
BOOL WINAPI FilterRulesHttpHead(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	FunFilter http;
	http.m_filter_fun = PacketHeadFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

//http.options
BOOL WINAPI FilterRulesHttpOptions(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	FunFilter http;
	http.m_filter_fun = PacketOptionsFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

//http.put
BOOL WINAPI FilterRulesHttpPut(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	FunFilter http;
	http.m_filter_fun = PacketPutFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

//http.delete
BOOL WINAPI FilterRulesHttpDelete(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	FunFilter http;
	http.m_filter_fun = PacketDeleteFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

//http.tarce
BOOL WINAPI FilterRulesHttpTarce(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	FunFilter http;
	http.m_filter_fun = PacketTarceFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

//http.resp
BOOL WINAPI FilterRulesHttpResp(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	FunFilter http;
	http.m_filter_fun = PacketRespFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

//http.url contains "xxx"
BOOL WINAPI FilterRulesHttpUrl(IN const char *rule, OUT FilterRules &result)
{
	FilterRulesTcpLength("tcp.length>0", result);
	mstring ms(rule);
	int b = ms.find("\"");
	int e = ms.find("\"", b + 1);
	if (mstring::npos == b || mstring::npos == e || (e == b + 1))
	{
		return FALSE;
	}
	FunFilter http;
	http.m_params.assign(ms, b + 1, e - b - 1);
	http.m_filter_fun = PacketUrlFilterFun;
	list<FunFilter>::iterator itm;
	for (itm = result.m_filter_funs.begin() ; itm != result.m_filter_funs.end() ; itm++)
	{
		if (*itm == http)
		{
			return TRUE;
		}
	}
	result.m_filter_funs.push_back(http);
	return TRUE;
}

VOID WINAPI RegistFIlterRules()
{
	s_rules_analysis[FILTER_RULES_IP] = FilterRulesIp;
	s_rules_analysis[FILTER_RULES_IP_ADDR] = FilterRulesIpaddr;
	s_rules_analysis[FILTER_RULES_IP_LENGTH] = FilterRulesIpLength;

	s_rules_analysis[FILTER_RULES_TCP] = FilterRulesTcp;
	s_rules_analysis[FILTER_RULES_TCP_FLAG] = FilterRulesTcpFlag;
	s_rules_analysis[FILTER_RULES_TCP_PORT] = FilterRulesTcpPort;
	s_rules_analysis[FILTER_RULES_TCP_LENGTH] = FilterRulesTcpLength;
	s_rules_analysis[FILTER_RULES_TCP_CONTENT] = FilterRulesTcpContent;
	s_rules_analysis[FILTER_RULES_TCP_CONTAINS] = FilterRulesTcpContains;

	s_rules_analysis[FILTER_RULES_UDP] = FilterRulesUdp;
	s_rules_analysis[FILTER_RULES_UDP_PORT] = FilterRulesUdpPort;
	s_rules_analysis[FILTER_RULES_UDP_LENGTH] = FilterRulesUdpLength;
	s_rules_analysis[FILTER_RULES_UDP_CONTENT] = FilterRulesUdpContent;
	s_rules_analysis[FILTER_RULES_UDP_CONTAINS] = FilterRulesUdpContains;

    s_rules_analysis[FILTER_RULES_ICMP] = FilterRulesIcmp;

	//http
	s_rules_analysis[FILTER_RULES_HTTP] = FilterRulesHttp;
	s_rules_analysis[FILTER_RULES_HTTP_GET] = FilterRulesHttpGet;
	s_rules_analysis[FILTER_RULES_HTTP_POST] = FilterRulesHttpPost;
	s_rules_analysis[FILTER_RULES_HTTP_HEAD] = FilterRulesHttpHead;
	s_rules_analysis[FILTER_RULES_HTTP_OPTIONS] = FilterRulesHttpOptions;
	s_rules_analysis[FILTER_RULES_HTTP_PUT] = FilterRulesHttpPut;
	s_rules_analysis[FILTER_RULES_HTTP_DELETE] = FilterRulesHttpDelete;
	s_rules_analysis[FILTER_RULES_HTTP_TARCE] = FilterRulesHttpTarce;
	s_rules_analysis[FILTER_RULES_HTTP_RESP] = FilterRulesHttpResp;
	s_rules_analysis[FILTER_RULES_HTTP_URLS] = FilterRulesHttpUrl;

	//表达式符号
	s_string_sign[">="] = em_eq_greate;
	s_string_sign["<="] = em_eq_less;
	s_string_sign["=="] = em_equal;
	s_string_sign[">"] = em_greate;
	s_string_sign["<"] = em_less;
	s_string_sign["!="] = em_not_equal;
	s_string_sign["&"] = em_bit_and;

	//数据类型
	s_string_data_type["n8"] = em_unsigned_n8;
	s_string_data_type["n16"] = em_unsigned_n16;
	s_string_data_type["n32"] = em_unsigned_n32;
	s_string_data_type["byte"] = em_unsigned_n8;
	s_string_data_type["bytes"] = em_bytes;
	s_string_data_type["char"] = em_unsigned_n8;
}

PRegMatchFun WINAPI GetRulesHandle(const char *filter)
{
	CRegexpT<char> regular;
	MatchResult result;
	map<mstring, PRegMatchFun>::iterator itm;
	PRegMatchFun fun = NULL;
	for (itm = s_rules_analysis.begin() ; itm != s_rules_analysis.end() ; itm++)
	{
		regular.Compile(itm->first.c_str());
		result = regular.MatchExact(filter);
		if (result.IsMatched())
		{
			fun = itm->second;
			break;
		}
	}
	return fun;
}