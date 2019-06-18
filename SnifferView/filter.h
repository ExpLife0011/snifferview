#pragma  once
#include <Windows.h>
#include <list>
#include <vector>
#include <set>
#include <mstring.h>
#include "analysis.h"

using namespace std;

enum FilterSign;

//�۲��߽ӿڣ���������ӿ��Ժ�ÿ����ɶ��ᾭ������ӿ�
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
    em_unsigned_n8,         //�޷���8λ����
    em_unsigned_n16,        //�޷���16λ����
    em_unsigned_n32,        //�޷���32λ����
    em_signed_n8,           //�з���8λ����
    em_signed_n16,          //�з���16λ����
    em_signed_n32,          //�з���32λ����
    em_bytes                //�ַ�������
};

enum FilterEncode
{
    em_big_head,    //��� �����ֽ���
    em_little_head  //С�� �����ֽ���
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

//�̶���ַ������˹���
struct FixedContentsFilter
{
    FilterOffsetType m_offset_type; //���ƫ�ƻ��Ǿ���ƫ��
    unsigned short   m_offest;      //����ƫ��
    FilterEncode     m_encode;      //��������(��С��)
    FilterSign       m_sign;        //���ʽ����
    FilterData       m_data_type;   //��������
    mstring          m_data;        //�����Ҫƥ�������

    bool operator==(const FixedContentsFilter &filter)const;
    FixedContentsFilter();
};

//���̶���ַ���˹���
struct RandContentsFilter
{
    unsigned short m_begin;       //����ƥ�����ʼλ��
    unsigned short m_end;         //����ƥ��Ľ���λ��
    FilterEncode   m_encode;      //��������(��С��)
    FilterSign     m_sign;        //���ʽ����
    FilterData     m_data_type;   //��������
    mstring        m_data;        //�����Ҫƥ�������
    bool operator==(const RandContentsFilter &filter)const;
};

enum StringMatchCM
{
	em_sring_lose_cm,   //���Դ�Сд
	em_string_nolose_cm //�����Դ�Сд
};

enum StringMatchFuzzy
{
    em_string_fuzzy,    //ģ��ƥ��
    em_string_fix       //�ϸ�ƥ��
};

enum FilterRulesStatus
{
    em_status_undefine,
    em_status_normal,
    em_status_confilct
};

//�̶���ַ�ַ���ƥ����˹���
struct FixedStringFilter
{
    FilterOffsetType m_offset_type; //���ƫ�ƻ��Ǿ���ƫ��
    unsigned short   m_offest;      //����ƫ��
    FilterSign m_sign;              //���� ==����!=
    StringMatchCM m_cm;             //�Ƿ���Դ�Сд
    StringMatchFuzzy m_fuzz;        //�Ƿ�ģ��ƥ��
    mstring m_data;                 //�����Ҫƥ�������
    bool operator==(const FixedStringFilter &filter)const;
    FixedStringFilter();
};

//���̶���ַ�ַ���ƥ����˹���
struct RandStringFilter
{
    unsigned short m_begin; //�ַ���ƥ����ʼ��ַ
    unsigned short m_end;   //�ַ���ƥ�������ַ
    FilterSign m_sign;      //���� ==����!=
    StringMatchCM m_cm;     //�Ƿ���Դ�Сд
    StringMatchFuzzy m_fuzz;//�Ƿ�ģ��ƥ��
    mstring m_data;         //�����Ҫƥ�������
    bool operator==(const RandStringFilter &filter)const;
};

//���˺�������
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

//�����е���������������µķ�����˹���
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

//���˹�������
extern BOOL g_net_mark;         //�Ƿ��������ֽ��� Ĭ����
extern BOOL g_str_mark;         //�ַ���ƥ���Ƿ��Сд���� Ĭ����
extern mstring g_show_string;   //��ʾ���ʽ
extern mstring g_filter_string; //���˱��ʽ
extern HexHight g_hex_hight;    //��������

BOOL WINAPI IsPacketPassFilter(IN OUT PacketContent *msg);

BOOL WINAPI IsPacketPassShow(IN OUT PacketContent *msg);

//ע�����۲���
VOID RegistPacketWatcher(PPacketWatcher watcher);
//��������۲���
BOOL TraverseWatchers(IN OUT PacketContent &msg);
//ȡ��xx�۲���
VOID UnRegistWatcher(PPacketWatcher watcher);
//����۲��߻ص�
VOID ClearWatchers();

VOID WINAPI InitFilterEngine();

#define LOCK_RULES   WaitForSingleObject(s_rules_lock, INFINITE)
#define UNLOCK_RULES ReleaseMutex(s_rules_lock)

//��ȡһ��������ı��ʽ�Ľ�����ʽ���ܰ�������
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