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
	em_length_uninit	,						//长度类型没有初始化
	em_fix_length,							//对应content-length字段
	em_var_length							//对应transfer-encoding字段
};

struct HttpState
{
	int m_http_state;							//http类型 get, post等等
	int m_cur_state;							//当前封包的状态，取值范围和m_http_state相同

	bool m_full_header;					//封包头是否已被接收
	mstring m_header;						//临时存储http封包头,因为要根据完整的封包头来获取http封包的长度
	mstring m_url;							//get请求的url

	bool m_full_packet;						//是否是完整的http包
	int m_full_count;							//完整http包的长度
	int m_pass_count;						//已接收的封包长度

	mstring m_packet;						//已经获取的http封包，调试用
	HttpLengthType m_length_type;	//长度类型，定长或者变长

	//网络地址，调试用
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
	//这个方法的前提是http请求
	void extract_url();
};

struct HttpPacket
{
	DWORD m_http_type;		//http封包类型，get，post，回复包等等
};

#define  HTTP_SECTION_END			("\r\n")											//http段结束标识
#define  HTTP_HEADER_END			("\r\n\r\n")										//http头结束标识
#define  HTTP_VAR_END					("\r\n0\r\n\r\n")								//变长http封包结束标识
#define  HTTP_LENGTH_MARK		("\r\nContent-Length:")						//http定长长度标识
#define  HTTP_VAR_MARK				("\r\nTransfer-Encoding:")				//http变长标识
#define  HTTP_11_MARK					("HTTP/1.1")									//http1.1版本标识							
#define  HTTP_10_MARK					("HTTP/1.0")									//http1.0版本标识

#define HTTP_REQUEST_GET			("GET")					//http get
#define HTTP_REQUEST_POST			("POST")				//http post
#define HTTP_REQUEST_HEAD		("HEAD")				//http head
#define HTTP_REQUEST_OPTIONS	("OPTIONS")			//http options
#define HTTP_REQUEST_PUT			("PUT")					//http put
#define HTTP_REQUEST_DELETE		("DELETE")				//http delete
#define HTTP_REQUEST_TARCE		("TARCE")				//http tarce

struct PacketContent;

//test 清除http缓存
VOID WINAPI ClearHttpBuffer();

//http观察者接口，负责解析http封吧，真正的http过滤函数直接读取结果即可
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