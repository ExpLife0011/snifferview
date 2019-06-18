#pragma  once
#include <Windows.h>
#include <list>
#include <vector>
#include <set>
#include <mstring.h>
#include "analysis.h"

using namespace std;

enum FilterSign;

//观察者接口，设置这个接口以后，每个封吧都会经过这个接口
typedef BOOL (WINAPI *PPacketWatcher)(PacketContent &msg);

typedef BOOL (WINAPI *PPacketFilterFun)(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);

#define FILTER_EXCEPTION_SYNTAX_EREROR  ("syntax error")

enum FilterSign
{
    em_equal,       //==
    em_not_equal,   //!=
    em_eq_greate,   //>=
    em_eq_less,     //<=
    em_greate,      //>
    em_less,        //<
    em_bit_and      //&
};

enum FilterData
{
    em_data_type_undefine,
    em_unsigned_n8,         //无符号8位整形
    em_unsigned_n16,        //无符号16位整形
    em_unsigned_n32,        //无符号32位整形
    em_signed_n8,           //有符号8位整形
    em_signed_n16,          //有符号16位整形
    em_signed_n32,          //有符号32位整形
    em_bytes                //字符串类型
};

enum FilterEncode
{
    em_big_head,    //大端 网络字节序
    em_little_head  //小端 主机字节序
};

struct LengthFilter
{
    FilterSign m_sign;
    unsigned m_length;
    bool operator==(const LengthFilter &filter)const;
};

enum FilterOffsetType
{
    em_offset_abs,
    em_offset_tcp
};

//固定地址封包过滤规则
struct FixedContentsFilter
{
    FilterOffsetType m_offset_type; //相对偏移还是绝对偏移
    unsigned short   m_offest;      //数据偏移
    FilterEncode     m_encode;      //编码类型(大小端)
    FilterSign       m_sign;        //表达式符号
    FilterData       m_data_type;   //数据类型
    mstring          m_data;        //具体的要匹配的数据

    bool operator==(const FixedContentsFilter &filter)const;
    FixedContentsFilter();
};

//不固定地址过滤规则
struct RandContentsFilter
{
    unsigned short m_begin;       //数据匹配的起始位置
    unsigned short m_end;         //数据匹配的结束位置
    FilterEncode   m_encode;      //编码类型(大小端)
    FilterSign     m_sign;        //表达式符号
    FilterData     m_data_type;   //数据类型
    mstring        m_data;        //具体的要匹配的数据
    bool operator==(const RandContentsFilter &filter)const;
};

enum StringMatchCM
{
	em_sring_lose_cm,   //忽略大小写
	em_string_nolose_cm //不忽略大小写
};

enum StringMatchFuzzy
{
    em_string_fuzzy,    //模糊匹配
    em_string_fix       //严格匹配
};

enum FilterRulesStatus
{
    em_status_undefine,
    em_status_normal,
    em_status_confilct
};

//固定地址字符串匹配过滤规则
struct FixedStringFilter
{
    FilterOffsetType m_offset_type; //相对偏移还是绝对偏移
    unsigned short   m_offest;      //数据偏移
    FilterSign m_sign;              //符号 ==或者!=
    StringMatchCM m_cm;             //是否忽略大小写
    StringMatchFuzzy m_fuzz;        //是否模糊匹配
    mstring m_data;                 //具体的要匹配的数据
    bool operator==(const FixedStringFilter &filter)const;
    FixedStringFilter();
};

//不固定地址字符串匹配过滤规则
struct RandStringFilter
{
    unsigned short m_begin; //字符串匹配起始地址
    unsigned short m_end;   //字符串匹配结束地址
    FilterSign m_sign;      //符号 ==或者!=
    StringMatchCM m_cm;     //是否忽略大小写
    StringMatchFuzzy m_fuzz;//是否模糊匹配
    mstring m_data;         //具体的要匹配的数据
    bool operator==(const RandStringFilter &filter)const;
};

//过滤函数规则
struct FunFilter
{
    PPacketFilterFun m_filter_fun;
    mstring m_params;
    FilterSign m_sign;

    bool operator==(const FunFilter &filter)const
    {
        return ((m_filter_fun == filter.m_filter_fun) && (m_params == filter.m_params) && (m_sign == filter.m_sign));
    }

    FunFilter();
    virtual ~FunFilter();
};

class FilterException
{
public:
    FilterException(const char *msg)
    {
        m_error_msg = msg;
    }
    virtual ~FilterException()
    {}

    const char *get_error_msg()
    {
        return m_error_msg.c_str();
    }
protected:
    mstring m_error_msg;
};

//将所有的网络封包抽象成如下的封包过滤规则
struct FilterRules
{
    FilterRulesStatus m_status;

    list<list<FixedContentsFilter>> m_fixed_data_filters;

    list<list<RandContentsFilter>> m_rand_data_filters;

    list<list<FixedStringFilter>> m_fixed_string_filters;

    list<list<RandStringFilter>> m_rand_string_filters;

    list<list<LengthFilter>> m_length_filters;

    list<FunFilter> m_filter_funs;

    FilterRules();

    bool is_empty();

    void clear();

    bool operator==(const FilterRules &filter)const;
};

enum HexHight
{
    em_ip_header,
    em_tcp_udp_icmp_header,
    em_user_data
};

//过滤规则配置
extern BOOL g_net_mark;         //是否是网络字节序 默认是
extern BOOL g_str_mark;         //字符串匹配是否大小写敏感 默认是
extern mstring g_show_string;   //显示表达式
extern mstring g_filter_string; //过滤表达式
extern HexHight g_hex_hight;    //高亮规则

BOOL WINAPI IsPacketPassFilter(IN OUT PacketContent *msg);

BOOL WINAPI IsPacketPassShow(IN OUT PacketContent *msg);

//注册封包观察者
VOID RegistPacketWatcher(PPacketWatcher watcher);
//遍历封包观察者
BOOL TraverseWatchers(IN OUT PacketContent &msg);
//取消xx观察者
VOID UnRegistWatcher(PPacketWatcher watcher);
//清除观察者回调
VOID ClearWatchers();

VOID WINAPI InitFilterEngine();

#define LOCK_RULES   WaitForSingleObject(s_rules_lock, INFINITE)
#define UNLOCK_RULES ReleaseMutex(s_rules_lock)

//获取一个处理过的表达式的结果表达式可能包含括号
BOOL WINAPI RulesCompile(IN const char *ext, OUT list<FilterRules> &res);

VOID SaveFilterConfig();
VOID GetFilterConfig();
void ResetFilterConfig();

BOOL ChangeFilterRules(IN const list<FilterRules> &filter);

VOID ClearFilterRules();

BOOL ChangeShowRules(IN const list<FilterRules> &filter);

VOID ClearShowRules();

BOOL IsFilterRulesDif(const list<FilterRules> &rules);

BOOL IsShowRulesDif(const list<FilterRules> &rules);

VOID InitFilterConfig();

BOOL WINAPI IsTrue(UINT a, FilterSign sign, UINT b);