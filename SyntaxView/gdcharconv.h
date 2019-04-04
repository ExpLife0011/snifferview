#ifndef _GDCHARCONV_H
#define _GDCHARCONV_H

#ifndef __cplusplus
#  error gdcharconv requires C++ compilation (use a .cpp suffix)
#endif

#pragma warning(disable: 4786)
#include <string>

// 四种类型，四种简写如下：
// A: string   mbcs字符串
// W: wstring  unicode字符串
// T: tstring  定义了_UNICODE时是unicode字符串，其余情况下是mbcs字符串
// U: strutf8  utf-8字符串
// tstring和strutf8都是std命名空间中的类型
// 
// 十六种变换调用如下（括号内的为别名）：
// AtoA                string  -> string 
// WtoW                wstring -> wstring
// TtoT                tstring -> tstring
// UtoU                strutf8 -> strutf8
// AtoU (ToUtf8A    )  string  -> strutf8
// UtoA (ToCommonA  )  strutf8 -> string 
// WtoU (ToUtf8W    )  wstring -> strutf8
// UtoW (ToCommonW  )  strutf8 -> wstring
// AtoW (ToWideChar )  string  -> wstring
// WtoA (ToMultiByte)  wstring -> string 
// AtoT                string  -> tstring
// TtoA                tstring -> string 
// WtoT                wstring -> tstring
// TtoW                tstring -> wstring
// UtoT (ToCommon   )  strutf8 -> tstring
// TtoU (ToUtf8     )  tstring -> strutf8

_STD_BEGIN
//strutf8类型定义，同string同类型
typedef string strutf8;
//tstring类型定义
#ifdef _UNICODE
typedef wstring tstring;
#else
typedef string tstring;
#endif
_STD_END

// 
template <class _Self> _Self _StoS(const _Self &_self)
{
    return _self;
}

// string -> string
#define AtoA _StoS<std::string>

// wstring -> wstring
#define WtoW _StoS<std::wstring>

// tstring -> tstring
#define TtoT _StoS<std::tstring>

// strutf8 -> strutf8
#define UtoU _StoS<std::strutf8>

//string -> strutf8
std::strutf8 ToUtf8A(const std::string &str);
#define AtoU ToUtf8A

//strutf8 -> string
std::string ToCommonA(const std::strutf8 &str);
#define UtoA ToCommonA

//wstring -> strutf8
std::strutf8 ToUtf8W(const std::wstring &str);
#define WtoU ToUtf8W

//strutf8 -> wstring
std::wstring ToCommonW(const std::strutf8 &str);
#define UtoW ToCommonW

//string -> wstring 
std::wstring ToWideChar(const std::string &str);
#define AtoW ToWideChar

//wstring -> string
std::string ToMultiByte(const std::wstring &str);
#define WtoA ToMultiByte

// string -> tstring
#ifdef _UNICODE
#  define AtoT AtoW
#else
#  define AtoT AtoA
#endif

// tstring -> string
#ifdef _UNICODE
#  define TtoA WtoA
#else
#  define TtoA AtoA
#endif

// wstring -> tstring
#ifdef _UNICODE
#  define WtoT WtoW
#else
#  define WtoT WtoA
#endif

// tstring -> wstring
#ifdef _UNICODE
#  define TtoW WtoW
#else
#  define TtoW AtoW
#endif

// strutf8 - > tstring
#ifdef _UNICODE
#  define UtoT UtoW
#else
#  define UtoT UtoA
#endif
#define ToCommon UtoT

// tstring - > strutf8
#ifdef _UNICODE
#  define TtoU WtoU
#else
#  define TtoU AtoU
#endif
#define ToUtf8 TtoU

// 格式化显示快捷函数，注意：返回结果最多不超过2048个字符，超过的会被自动截断
std::string fmt(const char *format, ...);
std::wstring fmt(const wchar_t *format, ...);

std::string toLower(const std::string &str);
std::wstring toLower(const std::wstring &str);

std::string toUpper(const std::string &str);
std::wstring toUpper(const std::wstring &str);

#endif