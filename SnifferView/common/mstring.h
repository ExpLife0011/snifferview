/*
 *@filename: mstring.h
 *@author:   lougd
 *@created:  2014-7-15 11:38
 *@version:  1.0.0.1
 *@desc:     封装了C++标准库中的string类和wstring类，添加了一些常用的方法
 *           包括字符串格式化接口，最大化接口，最小化接口，左值操作符，缓冲区
 *           分配接口，字符串清理接口，替换子串接口，删除子串接口，标准base64
 *           编码解码接口，私有base64编码解码接口，gbk,unicode,utf8转换接口
 *           ，清空内存接口等等。
 *@warning:  由于string对象的析构函数不是虚拟函数，当mstring对象用于多态的时候
 *           将不会调用mstring的析构函数，可能会导致mstring内部申请的内存无法
 *           被释放掉，所以不要这样使用。
*/
#pragma  once
#include <string>

namespace std
{
    class ustring;
    class mstring;

    typedef string utf8_string;
    typedef mstring utf8_mstring;
 
    //字符串解析类multibyte版本
    class mstring : public string
    {
    public:
        //默认构造
        mstring();

        //拷贝构造
        mstring(const mstring &str);

        //拷贝构造
        //mstring(const ustring &str);

        //构造，可以导入中间有/0的字符串
        mstring(const char* buffer, size_t len);

        //构造，自动进行类型转换，可以导入中间有0x0000的字符串
        //mstring(const wchar_t *buffer, size_t len);

        //拷贝构造
        mstring(const string &buffer);

        //拷贝构造
        //mstring(const wstring &buffer);

        //构造，参数类型为const char *
        mstring(const char* buffer);

        //构造，自动进行类型转换
        //mstring(const wchar_t *buffer);

        //
        mstring(char c);

        //
        //mstring(wchar_t w); 

        //析构，做了一些清理工作
        virtual ~mstring();

    public:
        //字符串格式化，用法类似C函数库中的printf
        mstring &format(const char *format, ...);

        //重载左值操作符
        //operator const char *();

        //赋值操作符
        mstring &operator = (const char *buffer);

        //参数为unicode的赋值操作符，会自动将unicode转为gbk
        //mstring &operator = (const wchar_t *buffer);

        //赋值操作符，参数为char类型
        mstring &operator = (char c);

        //赋值操作符，参数为wchar类型
        //mstring &operator = (wchar_t w);

        //赋值操作符，参数为mstring类型
        mstring &operator = (const mstring &m);

        //赋值操作符，参数为ustring类型，自当进行类型转换
        //mstring &operator = (const ustring &u);

        //赋值操作符
        mstring &operator = (const string &m);

        //赋值操作符
        //mstring &operator = (const wstring &u);

        //+=操作符
        mstring &operator += (const char *buffer);

        //+=操作符，参数类型自动进行转换
        //mstring &operator += (const wchar_t *buffer);

        //+=操作符
        mstring &operator += (char c);

        //+=操作符，参数类型wchar，自动进行类型转换
        //mstring &operator += (wchar_t w);

        mstring &operator += (const mstring &m);

        //+=操作符，参数为ustring类型，自动进行转换
        //mstring &operator += (const ustring &u);

        //+=操作符
        mstring &operator += (const string &m);

        //+=操作符
        //mstring &operator += (const wstring &u);

        //范围查找
        //offset:查找的起始位置
        //range: 最多查找的字符数
        size_t find_in_range(const char *str, size_t offset = 0, size_t range = -1);

        size_t find_in_range(const mstring &str, size_t offset = 0, size_t range = -1);

        //忽略大小写查找
        size_t find_in_rangei(const char *str, size_t offset = 0, size_t range = -1) const;
        size_t find_in_rangei(const mstring &str, size_t offset = 0, size_t range = -1) const;

        //比较字符串（忽略大小小写）
        int comparei(const char *str, size_t offset = 0) const;
        //
        int comparei(const mstring &str, size_t offset = 0) const;

        //覆盖写数据
        //buffer：要覆盖写入的缓冲区
        //offset:写入的偏移，如果自身的长度不够会在内部进行分配;
        mstring &cover(const mstring &buffer, size_t offset = 0);

        mstring &cover(const char *str, size_t offset = 0);

        //取左边的子串
        //count:左边起字符的个数
        mstring &left(size_t count);

        //取右边的子串
        //count:右边起字符的个数
        mstring &right(size_t count);

        //最小化字符串
        mstring &makelower();

        //最大化字符串
        mstring &makeupper();

        //替换字符串中的字串，支持多个字串
        mstring &repsub(const char *src, const char *dst);

        //分配指定长度的缓冲区给调用者使用
        char *alloc(size_t size);

        //将用户动态分配的缓冲区赋给自己
        mstring &setbuffer(size_t len = 0);

        //释放用户申请的缓冲区
        void release();

        //清空数据并释放内存，因为string对象的内存管理模型是只增不减的，可以通过这个接口来释放掉内存
        void clear_with_mem();

        //删除字串，支持多个
        mstring &delsub(const char *sub);

        //删除指定的字符
        mstring &delchar(char c);

        //清理字符串左边，包括/r,/n,空格
        mstring &trimleft();

        //清理字符串右边
        mstring &trimright();

        //同时清理字符串的两边
        mstring &trim();

        //mstring &a2u();

        //ustring a2w();

        //mstring &u2a();

        //ustring u2w();

        bool startwith(const char *start);
        bool endwith(const char *tail);

        mstring &path_append(const char *more);
        //path_find_dir
        //path_find_name
        //path_find_ext

        bool isnumber();

    protected:
        size_t sfind(const mstring &str, size_t offset, size_t range, bool v) const;

        void initstring();

    protected:
        char *m_buffer;
        size_t m_size;
    };

    //字符串解析类widechar版本
    class ustring : public wstring
    {
    public:
        //默认构造
        ustring();

        //构造，自动进行类型的转换
        //ustring(const mstring &str);

        //拷贝构造
        ustring(const ustring &str);

        //构造，自动进行类型转换
        //ustring(const char* buffer);

        //构造
        ustring(const wchar_t *buffer);

        //构造，字符串中可以有'/0'
        //ustring(const char* buffer, size_t len);

        //构造，字符串中可以有0x00
        ustring(const wchar_t *buffer, size_t len);

        //拷贝构造，自动进行类型转换
        //ustring(const string &buffer);

        //拷贝构造
        ustring(const wstring &buffer);

        //构造，参数为字符，自动进行类型的转换
        //ustring(char c);

        //
        ustring(wchar_t w); 

        //析构，做了一些清理工作
        virtual ~ustring();

    public:
        //字符串格式化接口，用法类似于C函数库中的printf
        ustring &format(const wchar_t *format, ...);

        //重载左值操作符
        //operator const wchar_t *();

        //赋值操作符，自动进行类型转换
        //ustring &operator = (const char *buffer);

        //赋值操作符
        ustring &operator = (const wchar_t *buffer);

        //赋值操作符，自动进行类型转换
        //ustring &operator = (char c);

        //赋值操作符
        ustring &operator = (wchar_t w);

        //赋值操作符
        ustring &operator = (const ustring &u);

        //赋值操作符，自动进行类型转换
        //ustring &operator = (const mstring &m);

        //赋值操作符
        //ustring &operator = (const string &m);

        //赋值操作符
        ustring &operator = (const wstring &u);

        //+=操作符
        ustring &operator += (const ustring &u);

        //+=操作符，自动进行类型转换
        //ustring &operator += (const mstring &m);

        //+=操作符，自动进行类型转换
        //ustring &operator += (const char *buffer);

        //+=操作符
        ustring &operator += (const wchar_t *buffer);

        //+=操作符，自动进行类型转换
        //ustring &operator += (char c);

        //+=操作符
        ustring &operator += (wchar_t w);

        //+=操作符，自动进行类型的转换
        //ustring &operator += (const string &m);

        //+=操作符
        ustring &operator += (const wstring &u);

        //范围查找
        //offset:查找的起始位置
        //range: 最多查找的字符数
        size_t find_in_range(const wchar_t *str, size_t offset = 0, size_t range = -1);

        size_t find_in_range(const ustring &str, size_t offset = 0, size_t range = -1);

        //忽略大小写查找
        size_t find_in_rangei(const wchar_t *str, size_t offset = 0, size_t range = -1);

        size_t find_in_rangei(const ustring &str, size_t offset = 0, size_t range = -1);

        //比较字符串（忽略大小小写）
        int comparei(const wchar_t *str, size_t offset = 0);

        //
        int comparei(const ustring &str, size_t offset = 0);

        //字串替换接口，支持多个字段
        ustring &repsub(const wchar_t *src, const wchar_t *dst);

        ustring &cover(const wchar_t *str, size_t offset = 0);

        ustring &cover(const ustring &buffer, size_t offset = 0);

        //取左边的子串
        //count:左边起字符的个数
        ustring &left(size_t count);

        //取右边的子串
        //count:右边起字符的个数
        ustring &right(size_t count);

        //字符串最小化接口
        ustring &makelower();

        //字符串最大化接口
        ustring &makeupper();

        //动态分配内存的接口
        wchar_t *alloc(size_t size);

        //将动态分配内存写入的接口
        ustring &setbuffer(size_t size = 0);

        //释放动态分配的内存
        void release();

        //清空数据并释放内存，因为string对象的内存管理模型是只增不减的，可以通过这个接口来释放掉内存
        void clear_with_mem();

        //删除字串接口，支持多个字段
        ustring &delsub(const wchar_t *sub);

        //删除字符接口，支持多个字符
        ustring &delchar(wchar_t c);

        //清理字符串左侧接口
        ustring &trimleft();

        //清理字符串右侧接口
        ustring &trimright();

        //同时清理字符串的两边
        ustring &trim();

        //mstring w2u();

        //mstring w2a();

        bool startwith(const wchar_t *start);

        bool endwith(const wchar_t *tail);

        ustring &path_append(const wchar_t *more);

        bool isnumber();

    protected:
        size_t sfind(const ustring &str, size_t offset, size_t range, bool v);

        void initstring();

    protected:
        wchar_t *m_buffer;
        size_t m_size;
    };
}