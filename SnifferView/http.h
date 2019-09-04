#pragma  once
#include <Windows.h>
#include "../ComLib/mstring.h"
//#include "analysis.h"

#define  HTTP_STATE_UNINIT	0

enum HttpRequest
{
	em_http_get =		1 << 0,
	em_http_post =		1 << 1,
	em_http_head =		1 << 2,
	em_http_options =   1 << 3,
	em_http_put =		1 << 4,
	em_http_delete =	1 << 5,
	em_http_tarce =		1 << 6
};

enum HttpResp
{
    em_http_100      =  0x0000ffff,
    em_http_101,

	em_http_200,
    em_http_201,
    em_http_202,
    em_http_203,
    em_http_204,
    em_http_205,
    em_http_206,

    em_http_300,
    em_http_301,
    em_http_302,
    em_http_303,
    em_http_304,
    em_http_305,
    em_http_306,
    em_http_307,

	em_http_400,
    em_http_401,
    em_http_402,
    em_http_403,
    em_http_404,
	em_http_unknow = 1 << 9
};

enum HttpLengthType
{
	em_length_uninit	,						//��������û�г�ʼ��
	em_fix_length,							//��Ӧcontent-length�ֶ�
	em_var_length							//��Ӧtransfer-encoding�ֶ�
};

struct HttpState
{
	int m_http_state;							//http���� get, post�ȵ�
	int m_cur_state;							//��ǰ�����״̬��ȡֵ��Χ��m_http_state��ͬ

	bool m_full_header;					//���ͷ�Ƿ��ѱ�����
	mstring m_header;						//��ʱ�洢http���ͷ,��ΪҪ���������ķ��ͷ����ȡhttp����ĳ���
	mstring m_url;							//get�����url

	bool m_full_packet;						//�Ƿ���������http��
	int m_full_count;							//����http���ĳ���
	int m_pass_count;						//�ѽ��յķ������

	mstring m_packet;						//�Ѿ���ȡ��http�����������
	HttpLengthType m_length_type;	//�������ͣ��������߱䳤

	//�����ַ��������
	ULONG m_src_addr;
	USHORT m_src_port;
	ULONG m_dst_addr;
	USHORT m_dst_port;
	list<mstring> m_urls;
public:
	HttpState();
	virtual ~HttpState();
	int get_state();
	void get_urls(list<mstring> &urls);
	void add_packet(const char *buffer, size_t length);
	void push(const char *buffer, size_t length);
	bool is_http();
	bool is_get();
	bool is_post();
	bool is_head();
	bool is_options();
	bool is_put();
	bool is_delete();
	bool is_tarce();
	bool is_resp();
	void set_addr(ULONG src_addr, USHORT src_port, ULONG dst_addr, USHORT dst_port);

protected:
	void reset();
	bool is_full_header();
	void update_state();
	void update_length();
	//���������ǰ����http����
	void extract_url();
};

struct HttpPacket
{
	DWORD m_http_type;		//http������ͣ�get��post���ظ����ȵ�
};

#define  HTTP_SECTION_END			("\r\n")											//http�ν�����ʶ
#define  HTTP_HEADER_END			("\r\n\r\n")										//httpͷ������ʶ
#define  HTTP_VAR_END					("\r\n0\r\n\r\n")								//�䳤http���������ʶ
#define  HTTP_LENGTH_MARK		("\r\nContent-Length:")						//http�������ȱ�ʶ
#define  HTTP_VAR_MARK				("\r\nTransfer-Encoding:")				//http�䳤��ʶ
#define  HTTP_11_MARK					("HTTP/1.1")									//http1.1�汾��ʶ							
#define  HTTP_10_MARK					("HTTP/1.0")									//http1.0�汾��ʶ

#define HTTP_REQUEST_GET			("GET")					//http get
#define HTTP_REQUEST_POST			("POST")				//http post
#define HTTP_REQUEST_HEAD		("HEAD")				//http head
#define HTTP_REQUEST_OPTIONS	("OPTIONS")			//http options
#define HTTP_REQUEST_PUT			("PUT")					//http put
#define HTTP_REQUEST_DELETE		("DELETE")				//http delete
#define HTTP_REQUEST_TARCE		("TARCE")				//http tarce

struct PacketContent;

//test ���http����
VOID WINAPI ClearHttpBuffer();

//http�۲��߽ӿڣ��������http��ɣ�������http���˺���ֱ�Ӷ�ȡ�������
BOOL WINAPI HttpWatcher(IN OUT PacketContent &msg);

//http filter fun
BOOL WINAPI PacketFilterFun(IN OUT PacketContent *msg, IN enum FilterSign sign, IN mstring &param);

//http.get filter fun
BOOL WINAPI PacketGetFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);

//http.post filter fun
BOOL WINAPI PacketPostFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);

//http.head
BOOL WINAPI PacketHeadFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);

//http.options
BOOL WINAPI PacketOptionsFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);

//http.put
BOOL WINAPI PacketPutFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);

//http.delete
BOOL WINAPI PacketDeleteFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);

//http.tarce
BOOL WINAPI PacketTarceFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);

//http.resp
BOOL WINAPI PacketRespFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);

//http.url contains \".{1,}\"
BOOL WINAPI PacketUrlFilterFun(IN OUT PacketContent *msg, IN FilterSign sign, IN mstring &param);