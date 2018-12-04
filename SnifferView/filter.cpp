/*
 *filename:  filter.cpp
 *author:    lougd
 *created:   2015-3-30 10:23
 *version:   1.0.0.1
 *desc:      过滤规则
 *history:
*/
#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <mstring.h>
#include <common.h>
#include <deelx.h>
#include "filter.h"
#include "analysis.h"
#include "rules.h"
#include "global.h"
#include "view/view.h"

using namespace std;
#pragma  comment(lib, "shlwapi.lib")

//过滤规则配置
BOOL g_net_mark = TRUE;                         //是否是网络字节序 默认是
BOOL g_str_mark = TRUE;                         //字符串匹配是否大小写敏感 默认是
mstring g_show_string = "tcp[chars]==\"GET\"";  //显示表达式
mstring g_filter_string;                        //过滤表达式
HexHight g_hex_hight = em_user_data;            //高亮配置，默认userdata

//观察者列表，当前只应用于http协议的解析
list<PPacketWatcher> s_watchers;

static list<FilterRules> s_filter_policy;       //过滤规则
static list<FilterRules> s_show_policy;         //显示规则

static HANDLE s_rules_lock = CreateMutexA(NULL, FALSE, NULL);
static set<char> s_clear_char;
static BOOL s_init = FALSE;

VOID RegistPacketWatcher(PPacketWatcher watcher)
{
	LOCK_RULES;
	list<PPacketWatcher>::iterator itm;
	for (itm = s_watchers.begin() ; itm != s_watchers.end() ; itm++)
	{
		if (*itm == watcher)
		{
			break;
		}
	}
	if (itm == s_watchers.end())
	{
		s_watchers.push_back(watcher);
	}
	UNLOCK_RULES;
}

BOOL TraverseWatchers(IN OUT PacketContent &packet)
{
	list<PPacketWatcher>::iterator itm;
	LOCK_RULES;
	for (itm = s_watchers.begin() ; itm != s_watchers.end() ; itm++)
	{
		if (!(*itm)(packet))
		{
			UNLOCK_RULES;
			return FALSE;
		}
	}
	UNLOCK_RULES;
	return TRUE;
}

VOID UnRegistWatcher(PPacketWatcher watcher)
{
	LOCK_RULES;
	list<PPacketWatcher>::iterator itm;
	for (itm = s_watchers.begin() ; itm != s_watchers.end() ; itm++)
	{
		if (*itm == watcher)
		{
			s_watchers.erase(itm);
			break;
		}
	}
	UNLOCK_RULES;
}

VOID ClearWatchers()
{
	LOCK_RULES;
	s_watchers.clear();
	UNLOCK_RULES;
}

bool LengthFilter::operator==(const LengthFilter &filter)const
{
	return (
		m_sign == filter.m_sign) &&
		(m_length == filter.m_length
		);
}

FixedContentsFilter::FixedContentsFilter()
{
	m_data_type = em_data_type_undefine;
	m_offset_type = em_offset_abs;
}

bool FixedContentsFilter::operator==(const FixedContentsFilter &filter)const
{
	return (
		m_offset_type == filter.m_offset_type &&
		m_offest == filter.m_offest &&
		m_encode == filter.m_encode &&
		m_sign == filter.m_sign &&
		m_data_type == filter.m_data_type &&
		m_data == filter.m_data
		);
}

bool RandContentsFilter::operator==(const RandContentsFilter &filter)const
{
	return (
		m_begin == filter.m_begin &&
		m_end == filter.m_end &&
		m_encode == filter.m_encode &&
		m_sign == filter.m_sign &&
		m_data_type == filter.m_data_type &&
		m_data == filter.m_data
		);
}

FixedStringFilter::FixedStringFilter()
{
	m_offset_type = em_offset_abs;
}

bool FixedStringFilter::operator==(const FixedStringFilter &filter)const
{
	return (
		m_offset_type == filter.m_offset_type &&
		m_offest == filter.m_offest &&
		m_sign == filter.m_sign &&
		m_cm == filter.m_cm &&
		m_fuzz == filter.m_fuzz &&
		m_data == filter.m_data
		);
}

bool RandStringFilter::operator==(const RandStringFilter &filter)const
{
	return (
		m_begin == filter.m_begin &&
		m_end == filter.m_end &&
		m_sign == filter.m_sign &&
		m_cm == filter.m_cm &&
		m_fuzz == filter.m_fuzz &&
		m_data == filter.m_data
		);
}

FilterRules::FilterRules()
{
	m_status = em_status_undefine;
}

bool FilterRules::is_empty()
{
	return (
		m_status == em_status_undefine &&
		m_fixed_data_filters.size() == 0 &&
		m_fixed_string_filters.size() == 0 &&
		m_rand_data_filters.size() == 0 &&
		m_rand_string_filters.size() == 0 &&
		m_length_filters.size() == 0
		);
}

void FilterRules::clear()
{
	m_status = em_status_undefine;
	m_fixed_data_filters.clear();
	m_fixed_string_filters.clear();
	m_rand_data_filters.clear();
	m_rand_string_filters.clear();
	m_length_filters.clear();
	m_filter_funs.clear();
}

bool FilterRules::operator==(const FilterRules &filter)const
{
	return (
		m_status == filter.m_status &&
		m_fixed_data_filters == filter.m_fixed_data_filters &&
		m_fixed_string_filters == filter.m_fixed_string_filters &&
		m_rand_data_filters == filter.m_rand_data_filters &&
		m_rand_string_filters == filter.m_rand_string_filters &&
		m_length_filters == filter.m_length_filters &&
		m_filter_funs == filter.m_filter_funs
		);
}

FunFilter::FunFilter()
{
}

FunFilter::~FunFilter()
{
}

//表达式中是否存在括号，刨除双引号中的数据
BOOL WINAPI HaveBracket(const char *buffer)
{
	BOOL b = FALSE;
	BOOL flag = FALSE;
	char ct = 0x00;
	int itm = 0;
	while(0x00 != (ct = buffer[itm]))
	{
		if ('"' == ct)
		{
			flag = !flag;
		}

		if (flag)
		{
			itm++;
			continue;
		}

		if ('(' == ct)
		{
			return TRUE;
		}
		itm++;
	}
	return FALSE;
}

//表达式中的字符串转为小写，刨除引号中的字符串
VOID WINAPI MakeBufferLower(IN OUT mstring &buffer)
{
	size_t itm = 0;
	bool flag = false;
	char ct;
	for (itm = 0 ; itm != buffer.size() ; itm++)
	{
		ct = buffer.at(itm);
		if ('"' == ct)
		{
			flag = !flag;
		}
		if (flag)
		{
			continue;
		}
		if (ct >= 'A' && ct <= 'Z')
		{
			buffer.at(itm) = (ct | 32);
		}
	}
}

//检查两条规则是否有冲突
BOOL WINAPI CheckFixedContConfilct(FixedContentsFilter rulea, FixedContentsFilter ruleb)
{
	//先检查最简单的情况，其他情况暂不考虑
	if (rulea.m_data_type != ruleb.m_data_type || rulea.m_offest != ruleb.m_offest || rulea.m_encode != ruleb.m_encode)
	{
		return FALSE;
	}

	if (rulea.m_sign == em_equal && ruleb.m_sign == em_equal)
	{
		if (rulea.m_data != ruleb.m_data)
		{
			return TRUE;
		}
	}
	return FALSE;
}

//两个过滤规则进行and运算
BOOL WINAPI FilterRulesMerger(IN FilterRules rulea, IN FilterRules ruleb, OUT FilterRules &rulec)
{
	BOOL state = FALSE;
	list<list<FixedContentsFilter>>::iterator itm;
	list<list<FixedContentsFilter>>::iterator itk;
	for (itm = rulea.m_fixed_data_filters.begin() ; itm != rulea.m_fixed_data_filters.end() ; itm++)
	{
		itk = ruleb.m_fixed_data_filters.begin() ;
		while(itk != ruleb.m_fixed_data_filters.end())
		{
			if (*itm == *itk)
			{
				itk = ruleb.m_fixed_data_filters.erase(itk);
			}
			else if (1 == itm->size() && 1 == itk->size())
			{
				if (CheckFixedContConfilct(itm->front(), itk->front()))
				{
					rulec.clear();
					rulec.m_status = em_status_confilct;
					return FALSE;
				}
				itk++;
			}
			else
			{
				itk++;
			}
		}
	}
	rulec.m_fixed_data_filters.insert(rulec.m_fixed_data_filters.end(), rulea.m_fixed_data_filters.begin(), rulea.m_fixed_data_filters.end());
	rulec.m_fixed_data_filters.insert(rulec.m_fixed_data_filters.end(), ruleb.m_fixed_data_filters.begin(), ruleb.m_fixed_data_filters.end());
	
	list<list<RandContentsFilter>>::iterator itv;
	list<list<RandContentsFilter>>::iterator its;
	for (itv = rulea.m_rand_data_filters.begin() ; itv != rulea.m_rand_data_filters.end() ; itv++)
	{
		its = ruleb.m_rand_data_filters.begin();
		while(its != ruleb.m_rand_data_filters.end())
		{
			if (*itv == *its)
			{
				its = ruleb.m_rand_data_filters.erase(its);
			}
			else
			{
				its++;
			}
		}
	}
	rulec.m_rand_data_filters.insert(rulec.m_rand_data_filters.end(), rulea.m_rand_data_filters.begin(), rulea.m_rand_data_filters.end());
	rulec.m_rand_data_filters.insert(rulec.m_rand_data_filters.end(), ruleb.m_rand_data_filters.begin(), ruleb.m_rand_data_filters.end());
	
	list<list<FixedStringFilter>>::iterator itu;
	list<list<FixedStringFilter>>::iterator itn;
	for (itu = rulea.m_fixed_string_filters.begin() ; itu != rulea.m_fixed_string_filters.end() ; itu++)
	{
		itn = ruleb.m_fixed_string_filters.begin();
		while(itn != ruleb.m_fixed_string_filters.end())
		{
			if (*itu == *itn)
			{
				itn = ruleb.m_fixed_string_filters.erase(itn);
			}
			else
			{
				itn++;
			}
		}
	}
	rulec.m_fixed_string_filters.insert(rulec.m_fixed_string_filters.end(), rulea.m_fixed_string_filters.begin(), rulea.m_fixed_string_filters.end());
	rulec.m_fixed_string_filters.insert(rulec.m_fixed_string_filters.end(), ruleb.m_fixed_string_filters.begin(), ruleb.m_fixed_string_filters.end());

	list<list<RandStringFilter>>::iterator itw;
	list<list<RandStringFilter>>::iterator itz;
	for (itw = rulea.m_rand_string_filters.begin() ; itw != rulea.m_rand_string_filters.end() ; itw++)
	{
		itz = ruleb.m_rand_string_filters.begin();
		while(itz != ruleb.m_rand_string_filters.end())
		{
			if (*itw == *itz)
			{
				itz = ruleb.m_rand_string_filters.erase(itz);
			}
			else
			{
				itz++;
			}
		}
	}
	rulec.m_rand_string_filters.insert(rulec.m_rand_string_filters.end(), rulea.m_rand_string_filters.begin(), rulea.m_rand_string_filters.end());
	rulec.m_rand_string_filters.insert(rulec.m_rand_string_filters.end(), ruleb.m_rand_string_filters.begin(), ruleb.m_rand_string_filters.end());
	
	list<list<LengthFilter>>::iterator itx;
	list<list<LengthFilter>>::iterator itq;
	for (itx = rulea.m_length_filters.begin() ; itx != rulea.m_length_filters.end() ; itx++)
	{
		itq = ruleb.m_length_filters.begin();
		while(itq != ruleb.m_length_filters.end())
		{
			if (*itx == *itq)
			{
				itq = ruleb.m_length_filters.erase(itq);
			}
			else
			{
				itq++;
			}
		}
	}
	rulec.m_length_filters.insert(rulec.m_length_filters.end(), rulea.m_length_filters.begin(), rulea.m_length_filters.end());
	rulec.m_length_filters.insert(rulec.m_length_filters.end(), ruleb.m_length_filters.begin(), ruleb.m_length_filters.end());

	list<FunFilter>::iterator itc;
	list<FunFilter>::iterator itd;
	for (itc = rulea.m_filter_funs.begin() ; itc != rulea.m_filter_funs.end() ; itc++)
	{
		itd = ruleb.m_filter_funs.begin();
		while(itd != ruleb.m_filter_funs.end())
		{
			if (*itc == *itd)
			{
				itd = ruleb.m_filter_funs.erase(itd);
			}
			else
			{
				itd++;
			}
		}
	}
	rulec.m_filter_funs.insert(rulec.m_filter_funs.end(), rulea.m_filter_funs.begin(), rulea.m_filter_funs.end());
	rulec.m_filter_funs.insert(rulec.m_filter_funs.end(), ruleb.m_filter_funs.begin(), ruleb.m_filter_funs.end());
	rulec.m_status = em_status_normal;
	state = TRUE;
	return state;
}

BOOL WINAPI IsTrue(UINT a, FilterSign sign, UINT b)
{
	switch(sign)
	{
	case  em_equal:
		return a == b;
	case  em_greate:
		return a > b;
	case  em_less:
		return a < b;
	case  em_eq_greate:
		return a >= b;
	case  em_eq_less:
		return a <= b;
	case  em_not_equal:
		return a != b;
	case  em_bit_and:
		return a & b;
	}
	return FALSE;
}

BOOL WINAPI IsStringTrue(const mstring &a, FilterSign sign, const mstring &c)
{
	switch(sign)
	{
	case  em_equal:
		return a == c;
	case  em_greate:
		return a > c;
	case  em_less:
		return a < c;
	case  em_eq_greate:
		return a >= c;
	case  em_eq_less:
		return a <= c;
	case  em_not_equal:
		return a != c;
	}
	return FALSE;
}

BOOL WINAPI IsPacketPassFixData(IN list<list<FixedContentsFilter>> &filters, IN mstring &packet)
{
	list<list<FixedContentsFilter>>::iterator itm;
	list<FixedContentsFilter>::iterator itk;
	unsigned int n32;
	unsigned short n16;
	unsigned char n8;
	unsigned int tk;
	BOOL mv;
	unsigned short offset;
	TCPHeader *tcp = NULL;
	for (itm = filters.begin() ; itm != filters.end() ; itm++)
	{
		mv = FALSE;
		for (itk = itm->begin() ; itk != itm->end() && !mv; itk++)
		{
			offset = 0;
			if (itk->m_offset_type == em_offset_abs)
			{
				offset = itk->m_offest;
			}
			else if (itk->m_offset_type == em_offset_tcp)
			{
				tcp = (TCPHeader *)(packet.c_str() + sizeof(IPHeader));
				offset = (itk->m_offest + (sizeof(IPHeader) + tcp->get_tcp_header_length()));
			}
			else
			{
				dp(L"FixData offset error");
				continue;
			}

			n8 = 0;
			tk = 0;
			switch(itk->m_data_type)
			{
			case  em_signed_n8:
			case  em_unsigned_n8:
				{
					if (packet.size() < offset + sizeof(n8))
					{
						return FALSE;
					}
					memcpy(&n8, packet.c_str() + offset, sizeof(n8));
					memcpy(&tk, itk->m_data.c_str(), sizeof(n8));
					mv = IsTrue(n8, itk->m_sign, tk);
				}
				break;
			case  em_signed_n16:
			case  em_unsigned_n16:
				{
					if (packet.size() < offset+ sizeof(n16))
					{
						return FALSE;
					}
					memcpy(&n16, packet.c_str() + offset, sizeof(n16));
					if (itk->m_encode == em_big_head)
					{
						n16 = n2h_16(n16);
					}
					memcpy(&tk, itk->m_data.c_str(), sizeof(n16));
					mv = IsTrue(n16, itk->m_sign, tk);
				}
				break;
			case  em_signed_n32:
			case  em_unsigned_n32:
				{
					if (packet.size() < offset + sizeof(n32))
					{
						return FALSE;
					}
					memcpy(&n32, packet.c_str() + offset, sizeof(n32));
					if (itk->m_encode == em_big_head)
					{
						n32 = n2h_32(n32);
					}
					memcpy(&tk, itk->m_data.c_str(), sizeof(n32));
					mv = IsTrue(n32, itk->m_sign, tk);
				}
				break;
			case  em_bytes:
				{
					if (packet.size() <= offset)
					{
						return FALSE;
					}

					mstring vvm;
					if(packet.size() >= offset + itk->m_data.size())
					{
						vvm.append(packet.c_str() + offset, itk->m_data.size());
					}
					else
					{
						vvm.append(packet.c_str() + offset, packet.size() - offset);
					}

					switch(itk->m_sign)
					{
					case em_equal:
						mv = (vvm == itk->m_data);
						break;
					case  em_eq_greate:
						mv = (vvm >= itk->m_data);
						break;
					case  em_eq_less:
						mv = (vvm <= itk->m_data);
						break;
					case  em_greate:
						mv = (vvm  > itk->m_data);
						break;
					case  em_less:
						mv  = (vvm < itk->m_data);
						break;
					case  em_not_equal:
						mv = (vvm != itk->m_data);
						break;
					default:
						break;
					}
				}
				break;
			default:
				{
					dp(L"filter error aaa");
					return TRUE;
				}		
			}
		}
		if (!mv)
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL WINAPI IsPacketPassFixString(IN list<list<FixedStringFilter>> &filters, IN mstring &packet)
{
	list<list<FixedStringFilter>>::iterator itm;
	list<FixedStringFilter>::iterator itk;
	BOOL mv;
	mstring ms;
	mstring sv;
	unsigned short offset = 0;
	TCPHeader *tcp = NULL;
	for (itm = filters.begin() ; itm != filters.end() ; itm++)
	{
		mv = FALSE;
		for (itk = itm->begin() ; itk != itm->end() && !mv; itk++)
		{
			offset = 0;
			if (itk->m_offset_type == em_offset_abs)
			{
				offset = itk->m_offest;
			}
			else if(itk->m_offset_type == em_offset_tcp)
			{
				tcp = (TCPHeader *)(packet.c_str() + sizeof(IPHeader));
				offset = (itk->m_offest + sizeof(IPHeader) + tcp->get_tcp_header_length());
			}
			else
			{
				dp(L"FixString offset error");
				continue;
			}

			if (packet.size() <= offset)
			{
				break;
			}

			ms.clear();
			sv.clear();
			if (packet.size() >= offset+ itk->m_data.size())
			{
				ms.append(packet.c_str() + offset, itk->m_data.size());
			}
			else
			{
				ms.append(packet.c_str()+ offset, packet.size() - offset);
			}

			if (em_sring_lose_cm == itk->m_cm)
			{
				ms.makelower();
				sv = itk->m_data;
				sv.makelower();
				mv = IsStringTrue(ms, itk->m_sign, sv);
			}
			else
			{
				mv = IsStringTrue(ms, itk->m_sign, itk->m_data);
			}
		}

		if (!mv)
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL WINAPI IsPassLength(IN list<list<LengthFilter>> &filter, IN mstring &packet)
{
	list<list<LengthFilter>>::iterator itm;
	list<LengthFilter>::iterator itk;
	BOOL mv;
	for (itm = filter.begin() ; itm != filter.end() ; itm++)
	{
		mv = FALSE;
		for (itk = itm->begin() ; itk != itm->end() && !mv; itk++)
		{
			mv = IsTrue(packet.size(), itk->m_sign, itk->m_length);
		}

		if (!mv)
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL WINAPI IsPassRandString(IN list<list<RandStringFilter>> &filter, IN mstring &packet)
{
	list<list<RandStringFilter>>::iterator itm;
	list<RandStringFilter>::iterator itk;
	BOOL mv;
	mstring low;
	mstring vv;
	int mark = 0;
	for (itm = filter.begin() ; itm != filter.end() ; itm++)
	{
		mv = FALSE;
		for (itk = itm->begin() ; itk != itm->end() && !mv ; itk++)
		{
			if (packet.size() <= itk->m_begin)
			{
				continue;
			}

			if (itk->m_cm == em_sring_lose_cm)
			{
				low = packet; 
				low.makelower();
				mark = low.find(itk->m_data, itk->m_begin);
			}
			else
			{
				mark = packet.find(itk->m_data, itk->m_begin);
			}

			if (mstring::npos == mark)
			{
				continue;
			}
			mv = ((mark + itk->m_data.size()) <= itk->m_end);
		}
		if (!mv)
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL WINAPI IsPassFilterFun(IN list<FunFilter> &filter, IN OUT PacketContent *msg)
{
	list<FunFilter>::iterator itm;
	for (itm = filter.begin() ; itm != filter.end() ; itm++)
	{
		if (!itm->m_filter_fun(msg, itm->m_sign, itm->m_params))
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL WINAPI IsPacketPassSingleFilter(IN FilterRules &rules, IN OUT PacketContent *msg)
{
	if (!IsPacketPassFixData(rules.m_fixed_data_filters, msg->m_packet))
	{
		return FALSE;
	}

	if (!IsPacketPassFixString(rules.m_fixed_string_filters, msg->m_packet))
	{
		return FALSE;
	}

	if (!IsPassRandString(rules.m_rand_string_filters,  msg->m_packet))
	{
		return FALSE;
	}

	if (!IsPassLength(rules.m_length_filters, msg->m_packet))
	{
		return FALSE;
	}

	if (!IsPassFilterFun(rules.m_filter_funs, msg))
	{
		return FALSE;
	}
	return TRUE;
}

static BOOL IsPacketPass(IN list<FilterRules> &rules, IN OUT PacketContent *msg)
{
	LOCK_RULES;
	if (0 == rules.size())
	{
		UNLOCK_RULES;
		return TRUE;
	}
	//check filter
	list<FilterRules>::iterator itm;
	for (itm = rules.begin() ; itm != rules.end() ; itm++)
	{
		if (IsPacketPassSingleFilter(*itm, msg))
		{
			UNLOCK_RULES;
			return TRUE;
		}
	}
	UNLOCK_RULES;
	return FALSE;
}

BOOL WINAPI IsPacketPassFilter(IN OUT PacketContent *msg)
{
	return IsPacketPass(s_filter_policy, msg);
}

BOOL WINAPI IsPacketPassShow(IN OUT PacketContent *msg)
{
	return IsPacketPass(s_show_policy, msg);
}

//获取一个处理过的buffer的表达式的结果
BOOL WINAPI GetBufferResult(const char *buffer, FilterRules &res)
{
	PRegMatchFun fun = GetRulesHandle(buffer);
	if (fun)
	{
		return fun(buffer, res);
	}
	return FALSE;
}

//清除字符串中多余的空格符但是刨除双引号中的字符串
VOID WINAPI ClearBufferSpace(IN OUT mstring &buffer)
{
	BOOL flag = FALSE;
	mstring tmp;
	int itm = 0;
	bool del = false;
	tmp.trimleft();
	tmp.trimright();
	char ct;
	for (itm = 0 ; itm != buffer.size() ; itm++)
	{
		ct = buffer.c_str()[itm];
		if('"' == ct)
		{
			flag = !flag;
		}

		if (!flag && ' ' == ct && !del)
		{
			del = true;
			tmp += ct;
			continue;
		}

		if (!flag)
		{
			if (' ' == ct && del)
			{
				continue;
			}
			else
			{
				del = false;
			}

			if ((s_clear_char.end() != s_clear_char.find(ct))&& tmp.size() > 0 && ' ' == tmp.c_str()[tmp.size() - 1])
			{
				tmp.erase(tmp.size() - 1, 1);
			}
			
			if (s_clear_char.end() != s_clear_char.find(ct))
			{
				del = true;
			}
		}
		tmp += buffer.c_str()[itm];
	}
	buffer = tmp;
}

//置换非引号中的 && 和 || 字符，换为and和or
VOID WINAPI ReplaceBuffer(IN OUT mstring &buffer)
{
	BOOL flag = FALSE;
	const char *st = NULL;
	int itm = 0;
	st = buffer.c_str();
	char ct = 0x00;
	mstring tmp;
	while(0x00 != (ct = st[itm]))
	{
		if ('"' == ct)
		{
			flag = !flag;
		}
		
		if (!flag)
		{
			if ('&' == st[itm] && '&' == st[itm + 1])
			{
				tmp.append(" and ");
				itm += 2;
				continue;
			}

			if ('|' == st[itm] && '|' == st[itm + 1])
			{
				tmp.append(" or ");
				itm += 2;
				continue;
			}
		}
		tmp += ct;
		itm++;
	}
	buffer = tmp;
}

//抽取各个过滤条件，去除掉第一层括号
BOOL WINAPI FilterStringBuild(IN const char *buffer, OUT vector<mstring> &policys)
{
	int itm = 0;
	char ct = 0x00;
	list<int> lst;
	bool is_chars = false;
	int beg = 0;
	int flag = 0;
	bool is_ks = false;
	string filter = buffer;
	while(0x00 != *(filter.c_str() + itm))
	{
		ct = (filter.c_str() + itm)[0];
		if ('"' == ct)
		{
			is_chars = !is_chars;
		}

		if(is_chars)
		{
			itm++;
			continue;
		}

		if ('(' == ct)
		{
			if (!is_ks)
			{
				if (itm > beg)
				{
					policys.push_back(string(filter.c_str() + beg, itm - beg));		
				}
				beg = itm + 1;
			}
			is_ks = true;
			lst.push_back(itm);
			itm++;
			continue;
		}

		if (')' == ct)
		{
			if (0 == lst.size())
			{
				policys.clear();
				 return FALSE;
			}
			lst.pop_back();
			if (0 == lst.size())
			{
				is_ks = false;
				if (itm > beg)
				{
					policys.push_back(string(filter.c_str() + beg, itm - beg));	
				}
				beg = itm + 1;
			}
			itm++;
			continue;
		}

		if (!is_chars && !is_ks)
		{
			if(itm > 1 && (' ' == filter.c_str()[itm - 1] || ')' == filter.c_str()[itm - 1]))
			{
				if (0 == filter.compare(itm, 3, "and")&& (' '== filter.c_str()[itm + 3] || '(' == filter.c_str()[itm + 3]))
				{
					if (itm > beg)
					{
						policys.push_back(string(filter.c_str(), beg, itm - beg - 1));
					}
					policys.push_back("and");
					if(' '== filter.c_str()[itm + 3])
					{
						itm += 4;
					}
					else
					{
						itm += 3;
					}
					beg = itm;
					continue;
				}

				if (0 == filter.compare(itm, 2, "or") && (' '== filter.c_str()[itm + 2] || '(' == filter.c_str()[itm + 2]))
				{
					if (itm > beg)
					{
						policys.push_back(string(filter, beg, itm - beg - 1));
					}

					policys.push_back("or");
					if(' '== filter[itm + 2])
					{
						itm += 3;
					}
					else
					{
						itm += 2;
					}
					beg = itm;
					continue;
				}
			}
		}
		itm++;
	}

	if (is_chars || lst.size() != 0)
	{
		policys.clear();
		return FALSE;
	}

	if (itm > beg)
	{
		policys.push_back(string(filter.c_str() + beg, itm - beg));
	}
	return TRUE;
}

//从and 和 or的角度检查表达式列表否合法
BOOL WINAPI CheckConditionExpression(IN vector<mstring> &policys)
{
	size_t itm = 0;
	if (0 == policys.size())
	{
		return FALSE;
	}

	for (itm = 0 ; itm < policys.size() ; itm++)
	{
		if (itm % 2 != 0)
		{
			if (policys[itm] != "and" && policys[itm] != "or")
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

//计算一个没有括号的表达式的结果
BOOL WINAPI GetSimpleExpressionResult(IN const char *ext, OUT list<FilterRules> &res)
{
	BOOL ret = FALSE;
	vector<mstring> policys;
	if (HaveBracket(ext))
	{
		throw(FilterException(FILTER_EXCEPTION_SYNTAX_EREROR));
		return FALSE;
	}

	if (!FilterStringBuild(ext, policys) || 0 == policys.size())
	{
		throw(FilterException(FILTER_EXCEPTION_SYNTAX_EREROR));
		return FALSE;
	}

	if (!CheckConditionExpression(policys))
	{
		throw(FilterException(FILTER_EXCEPTION_SYNTAX_EREROR));
		return FALSE;
	}

	FilterRules mv;
	FilterRules tmp;
	list<FilterRules> mvs;
	vector<mstring>::iterator itm;
	int ms = 0;
	BOOL sv = FALSE;
	list<FilterRules>::iterator itv;
	for(itm = policys.begin() ; itm != policys.end() ; itm++, ms++)
	{
		if (ms % 2 != 0)
		{
			if (*itm == "and")
			{
				sv = FALSE;
			}
			else
			{
				sv = TRUE;
			}
		}
		else
		{
			mv.clear();
			if (!GetBufferResult(itm->c_str(), mv))
			{
				throw(FilterException(FILTER_EXCEPTION_SYNTAX_EREROR));
				return FALSE;
			}

			if (mv.is_empty())
			{
				continue;
			}

			if (0 == ms)
			{
				mvs.push_back(mv);
			}

			if (ms > 0)
			{
				if (sv)
				{	
					mvs.push_back(mv);
				}
				else
				{
					for (itv = mvs.begin() ; itv != mvs.end() ; itv++)
					{
						tmp.clear();
						if (!FilterRulesMerger(*itv, mv, tmp))
						{
							return FALSE;
						}
						*itv = tmp;
					}
				}
			}
		}
	}
	res = mvs;
	return TRUE;
}

//将过滤规则进行合并
VOID WINAPI FiltersCombination(IN list<FilterRules> a, IN list<FilterRules> b, OUT list<FilterRules> &c, IN BOOL and)
{
	list<FilterRules>::iterator itm;
	list<FilterRules>::iterator itv;
	list<FilterRules>::iterator itu;
	FilterRules vt;
	c.clear();
	if (and)
	{
		for (itm = a.begin() ; itm != a.end() ; itm++)
		{
			for (itv = b.begin() ; itv != b.end() ; itv++)
			{
				vt.clear();
				if (FilterRulesMerger(*itm, *itv, vt))
				{
					for (itu = c.begin() ; itu != c.end() ; itu++)
					{
						if (*itu == vt)
						{
							break;
						}
					}

					if (itu == c.end())
					{
						c.push_back(vt);
					}
				}
			}
		}
	}
	else
	{
		c.insert(c.end(), a.begin(), a.end());
		for(itm = b.begin() ; itm != b.end() ; itm++)
		{
			for (itv = c.begin() ; itv != c.end() ; itv++)
			{
				if (*itv == *itm)
				{
					break;
				}
			}

			if (itv == c.end())
			{
				c.push_back(*itm);
			}
		}
	}
}

VOID WINAPI InitFilterEngine()
{
	s_init = TRUE;
	RegistFIlterRules();
	//这些字符前后的空格将会被清理
	s_clear_char.insert('(');
	s_clear_char.insert(')');
	s_clear_char.insert('=');
	s_clear_char.insert('>');
	s_clear_char.insert('<');
	s_clear_char.insert('!');
	s_clear_char.insert('&');
	s_clear_char.insert(':');
	s_clear_char.insert('[');
	s_clear_char.insert(']');
	s_clear_char.insert('|');
}

//获取一个处理过的表达式的结果表达式可能包含括号
BOOL WINAPI RulesCompile(IN const char *ext, OUT list<FilterRules> &res)
{
	BOOL ret = FALSE;
	BOOL flag = FALSE;
	vector<mstring> policys;
	mstring sm(ext);
	if (!s_init)
	{
		InitFilterEngine();
	}
	ClearBufferSpace(sm);
	ReplaceBuffer(sm);
	MakeBufferLower(sm);
	try
	{
		if (!FilterStringBuild(sm.c_str(), policys) || 0 == policys.size())
		{
			throw(FilterException(FILTER_EXCEPTION_SYNTAX_EREROR));
		}

		if (!CheckConditionExpression(policys))
		{
			throw(FilterException(FILTER_EXCEPTION_SYNTAX_EREROR));
		}

		vector<mstring>::iterator itm;
		list<FilterRules> rst;
		list<FilterRules> mmv;
		list<FilterRules>::iterator itv;
		list<FilterRules>::iterator itu;
		BOOL sv = FALSE;
		int ms = 0;
		for(itm = policys.begin() ; itm != policys.end() ; itm++, ms++)
		{
			if (ms % 2 != 0)
			{
				if (*itm == "and")
				{
					sv = TRUE;
				}
				else
				{
					sv = FALSE;
				}
			}
			else
			{
				mmv.clear();
				if (HaveBracket(itm->c_str()))
				{
					if (!RulesCompile(itm->c_str(), mmv))
					{
						throw(FilterException(FILTER_EXCEPTION_SYNTAX_EREROR));
					}
				}
				else
				{
					if (!GetSimpleExpressionResult(itm->c_str(), mmv))
					{
						throw(FilterException(FILTER_EXCEPTION_SYNTAX_EREROR));
						return FALSE;
					}
				}

				if (0 == ms)
				{
					rst = mmv;
				}
				else
				{
					list<FilterRules> vm;
					FiltersCombination(mmv, rst, vm, sv);
					rst = vm;
				}
			}		
		}
		res = rst;
	}
	catch (FilterException& e)
	{
		dp(L"error, msg:%hs", e.get_error_msg());
		return FALSE;
	}
	return TRUE;
}

BOOL ChangeFilterRules(IN const list<FilterRules> &filter)
{
	LOCK_RULES;
	if (filter != s_filter_policy)
	{
		s_filter_policy = filter;
		UNLOCK_RULES;
		return TRUE;
	}
	UNLOCK_RULES;
	return FALSE;
}

BOOL ChangeShowRules(IN const list<FilterRules> &filter)
{
	LOCK_RULES;
	if (filter != s_show_policy)
	{
		s_show_policy = filter;
		UNLOCK_RULES;
		return TRUE;
	}
	UNLOCK_RULES;
	return FALSE;
}

VOID ClearFilterRules()
{
	LOCK_RULES;
	s_filter_policy.clear();
	UNLOCK_RULES;
}

VOID ClearShowRules()
{
	LOCK_RULES;
	s_show_policy.clear();
	UNLOCK_RULES;
}

BOOL IsFilterRulesDif(const list<FilterRules> &rules)
{
	BOOL ret;
	LOCK_RULES;
	ret = (rules != s_filter_policy);
	UNLOCK_RULES;
	return ret;
}

BOOL IsShowRulesDif(const list<FilterRules> &rules)
{
	BOOL ret;
	LOCK_RULES;
	ret = (rules != s_show_policy);
	UNLOCK_RULES;
	return ret;
}

VOID InitFilterConfig()
{
	LOCK_RULES;
	if(g_show_string.size() > 0)
	{
		RulesCompile(g_show_string.c_str(), s_show_policy);
	}

	if (g_filter_string.size() > 0)
	{
		RulesCompile(g_filter_string.c_str(), s_filter_policy);
	}
	UNLOCK_RULES;
}