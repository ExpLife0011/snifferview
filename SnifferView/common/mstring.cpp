#include <Windows.h>
#include <algorithm>
#include <Shlwapi.h>
#include "mstring.h"

#pragma  comment(lib, "shlwapi.lib")

using namespace std;

mstring::mstring()
{
    initstring();
}

mstring::mstring(const mstring &str)
{
    initstring();
    *this = str;
}

mstring::mstring(const ustring &str)
{
    initstring();
    *this = str;
}

mstring::mstring(const char *buffer)
{
    initstring();
    if (!buffer)
    {
        return;
    }
    *this = buffer;
}

mstring::mstring(const char *buffer, size_t len)
{
    initstring();
    if (!buffer)
    {
        return;
    }
    this->append(buffer, len);
}

mstring::mstring(const string &buffer)
{
    initstring();
    *this = buffer;
}

mstring::mstring(const wstring &buffer)
{
    initstring();
    *this = ustring(buffer);
}

mstring::mstring(const wchar_t *buffer)
{
    initstring();
    if (!buffer)
    {
        return;
    }
    *this = ustring(buffer);
}

mstring::mstring(const wchar_t *buffer, size_t len)
{
    initstring();
    if (!buffer)
    {
        return;
    }
    *this = ustring(buffer, len);
}

mstring::mstring(char c)
{
    initstring();
    *this = c;
}

mstring::mstring(wchar_t w)
{
    initstring();
    *this = w;
}

mstring::~mstring()
{
    if (m_buffer)
    {
        free(m_buffer);
        m_buffer = 0;
        m_size = 0;
    }
}

void mstring::initstring()
{
    m_buffer = 0;
    m_size = 0;
}

//mstring::operator const char *()
//{
//    return this->c_str();
//}

mstring &mstring::operator = (const char *buffer)
{
    if (!buffer)
    {
        return *this;
    }
    this->erase(this->begin(), this->end());
    this->append(buffer);
    return *this;
}

mstring &mstring::operator = (const wchar_t *buffer)
{
    if (!buffer)
    {
        return *this;
    }
    *this = ustring(buffer);
    return *this;
}

mstring &mstring::operator = (char c)
{
    this->erase(this->begin(), this->end());
    this->append(1, c);
    return *this;
}

mstring &mstring::operator = (wchar_t w)
{
    *this = ustring(w);
    return *this;
}

mstring &mstring::operator = (const mstring &m)
{
    this->erase(this->begin(), this->end());
    this->append(m.c_str(), m.size());
    return *this;
}

mstring &mstring::operator = (const ustring &u)
{
    *this = ustring(u).w2a();
    return *this;
}

mstring &mstring::operator = (const string &m)
{
    this->erase(this->begin(), this->end());
    this->append(m.c_str(), m.size());
    return *this;
}

mstring &mstring::operator = (const wstring &u)
{
    *this = ustring(u).w2a();
    return *this;
}

mstring &mstring::operator += (const mstring &m)
{
    this->append(m.c_str(), m.size());
    return *this;
}

mstring &mstring::operator += (const ustring &u)
{
    *this += ustring(u).w2a();
    return *this;
}

mstring &mstring::operator += (char c)
{
    this->append(1, c);
    return *this;
}

mstring &mstring::operator += (wchar_t w)
{
    *this += ustring(w);
    return *this;
}

mstring &mstring::operator += (const char *buffer)
{
    if (!buffer)
    {
        return *this;
    }
    this->append(buffer);
    return *this;
}

mstring &mstring::operator += (const wchar_t *buffer)
{
    if (!buffer)
    {
        return *this;
    }
    *this += ustring(buffer).w2a();
    return *this;
}

mstring &mstring::operator += (const string &m)
{
    *this += mstring(m);
    return *this;
}

mstring &mstring::operator += (const wstring &w)
{
    *this += ustring(w);
    return *this;
}

mstring &mstring::setbuffer(size_t len)
{
    if (!m_buffer)
    {
        return *this;
    }
    erase(begin(), end());
    if (len > 0)
    {
        size_t size = len > m_size ? m_size : len;
        append(m_buffer, size);
    }
    else
    {
        append(m_buffer);
    }
    release();
    return *this;
}

mstring &mstring::format(const char *format, ...)
{
    if(!format || !*format)
    {
        return *this;
    }

    va_list arg;
    char *buffer = 0;
    size_t size = 0;
    buffer = (char *)malloc(sizeof(char) * 512);
    if (!buffer)
    {
        return *this;
    }
    size = 512;
    va_start(arg, format);
    int length = wvnsprintfA(buffer, (int)size, format, arg);
    while(length < 0)
    {
        size *= 2;
        buffer = (char *)realloc(buffer, size);
        if (!buffer)
        {
            break;
        }
        length = wvnsprintfA(buffer, (int)size, format, arg);
    }
    va_end(arg);
    if (buffer)
    {
        this->assign(buffer, length);
        free(buffer);
    }
    return *this;
}

size_t mstring::sfind(const mstring &str, size_t offset, size_t range, bool v)
{
    const char *ptr = c_str();
    const char *sub = str.c_str();
    size_t idx_str = 0;
    size_t idx_this = offset;
    size_t count = 0;
    size_t tmp = 0;
    size_t ret = mstring::npos;
    size_t m = min(range, size() - offset);
    char subi = 0x00;
    char stri = 0x00;
    while (count < m)
    {
        tmp = idx_this;
        idx_str = 0;
        subi = *(sub + idx_str);
        stri = *(ptr + idx_this);
        if (v && subi >= 'A' && subi <= 'Z')
        {
            subi |= 32;
        }

        if (v && stri >= 'A' && stri <= 'Z')
        {
            stri |= 32;
        }

        while (subi == stri)
        {
            idx_str++;
            idx_this++;
            subi = *(sub + idx_str);
            stri = *(ptr + idx_this);
            if (v && subi >= 'A' && subi <= 'Z')
            {
                subi |= 32;
            }

            if (v && stri >= 'A' && stri <= 'Z')
            {
                stri |= 32;
            }

            if (idx_str == str.size())
            {
                ret = tmp;
                goto leave;
            }

            if (m == (idx_this - offset))
            {
                goto leave;
            }
        }
        idx_this = tmp;
        count++;
        idx_this++;
    }
leave:
    return ret;
}

bool mstring::endwith(const char *tail)
{
    if (!tail || !tail[0])
    {
        return false;
    }
    size_t count = lstrlenA(tail);
    if (size() < count)
    {
        return false;
    }
    size_t offset = size() - count;
    return (offset == find_in_range(tail, offset, count));
}

mstring &mstring::path_append(const char *more)
{
    if (empty() || !more || !more[0])
    {
        return *this;
    }
    mstring self(*this);
    self.repsub("/", "\\");
    if (self[self.size() - 1] == '\\')
    {
        self.erase(self.size() - 1, 1);
    }
    mstring add(more);
    add.repsub("/", "\\");
    if (add[0] == '\\')
    {
        add.erase(0, 1);
    }
    bool ret = true;
    size_t flag = 0;
    while (!add.empty())
    {
        if (0 == add.comparei("..\\"))
        {
            if (mstring::npos == (flag = self.rfind('\\')))
            {
                ret = false;
                break;
            }
            self.erase(flag, self.size() - flag);
            add.erase(0, 3);
        }
        else if (0 == add.comparei(".\\"))
        {
            add.erase(0, 2);
        }
        else if (add == "..")
        {
            if (mstring::npos == (flag = self.rfind('\\')))
            {
                ret = false;
                break;
            }
            self.erase(flag, self.size() - flag);
            add.erase(0, 2);
            break;
        }
        else
        {
            break;
        }
    }
    if (!ret || self.empty())
    {
        return *this;
    }
    *this = self;
    if (!add.empty())
    {
        *this += "\\";
        *this += add;
    }
    return *this;
}

//范围查找
//offset:查找的起始位置
//range: 最多查找的字符数
size_t mstring::find_in_range(const char *str, size_t offset, size_t range)
{
    if (!str || !*str)
    {
        return 0;
    }
    return find_in_range(mstring(str), offset, range);
}

size_t mstring::find_in_range(const mstring &str, size_t offset, size_t range)
{
    if (offset >= size())
    {
        return mstring::npos;
    }

    if (str.size() == 0)
    {
        return 0;
    }

    if (-1 == range)
    {
        range = size();
    }
    return sfind(str, offset, range, false);
}

//忽略大小写查找
size_t mstring::find_in_rangei(const char *str, size_t offset, size_t range)
{
    if (!str || !*str)
    {
        return mstring::npos;
    }
    return find_in_rangei(mstring(str), offset, range);
}

size_t mstring::find_in_rangei(const mstring &str, size_t offset, size_t range)
{
    if (offset >= size())
    {
        return mstring::npos;
    }

    if (str.size() == 0)
    {
        return mstring::npos;
    }

    if (-1 == range)
    {
        range = size();
    }
    return sfind(str, offset, range, true);
}

//比较字符串（忽略大小小写）
int mstring::comparei(const char *str, size_t offset)
{
    if (!str || !*str)
    {
        return 1;
    }
    return comparei(mstring(str), offset);
}

//
int mstring::comparei(const mstring &str, size_t offset)
{
    if (offset >= size())
    {
        return -1;
    }
    const char *src = c_str();
    const char *sub = str.c_str();
    char s = 0x00;
    char d = 0x00;
    int ret = 0;
    size_t idex = 0;
    while (idex + offset < size() && idex < str.size())
    {
        s = src[idex + offset];
        d = sub[idex];
        if (s >= 'A' && s <= 'Z')
        {
            s |= 32;
        }

        if (d >= 'A' && d <= 'Z')
        {
            d |= 32;
        }

        if (s > d)
        {
            ret = 1;
            goto leave;
        }

        if (s < d)
        {
            ret = -1;
            goto leave;
        }
        idex++;
    }
    ret = !(idex == str.size());
leave:
    return ret;
}

mstring &mstring::cover(const char *str, size_t offset)
{
    if (!str || !*str)
    {
        return *this;
    }
    return cover(mstring(str), offset);
}

mstring &mstring::cover(const mstring &buffer, size_t offset)
{
    size_t length = offset + buffer.size();
    if (length > size())
    {
        size_t count = length - size();
        char *d = new char[count];
        memset(d, 0x00, count);
        append(d, count);
        delete []d;
    }

    size_t idx = 0;
    size_t idv = 0;
    for (idx = offset ; idx < offset + buffer.size() ; idx++, idv++)
    {
        at(idx) = (buffer.c_str())[idv];
    }
    return *this;
}

//取左边的子串
//count:左边起字符的个数
mstring &mstring::left(size_t count)
{
    if (size() > count)
    {
        erase(count, size() - count);
    }
    return *this;
}

//取右边的子串
//count:右边起字符的个数
mstring &mstring::right(size_t count)
{
    if (size() > count)
    {
        erase(0, size() - count);
    }
    return *this;
}

mstring &mstring::makelower()
{
    transform(this->begin(), this->end(), this->begin(), ::tolower); 
    return *this;
}

mstring &mstring::makeupper()
{
    transform(this->begin(), this->end(), this->begin(), ::toupper); 
    return *this;
}

mstring &mstring::repsub(const char *src, const char *dst)
{
    if (!src || !dst || 0x00 == src[0])
    {
        return *this;
    }
    size_t itm = 0;
    size_t sz = lstrlenA(src);
    size_t dz = lstrlenA(dst);
    while((itm = this->find(src, itm)) != mstring::npos)
    {
        this->erase(itm, sz);
        this->insert(itm, dst);
        itm += dz;
    }
    return *this;
}

char *mstring::alloc(size_t size)
{
    if (m_size >= size)
    {
        return m_buffer;
    }
    m_size = (size + 127) & ~127;
    if (m_buffer)
    {
        if (!(m_buffer = (char *)realloc(m_buffer, sizeof(char) * m_size)))
        {
            m_size = 0;
        }
        else
        {
            memset(m_buffer, 0x00, sizeof(char) * m_size);
        }
    }
    else
    {
        if (!(m_buffer = (char *)malloc(sizeof(char) * m_size)))
        {
            m_size = 0;
        }
        else
        {
            memset(m_buffer, 0x00, sizeof(char) * m_size);
        }
        
    }
    return m_buffer;
}

void mstring::release()
{
    if (m_buffer)
    {
        free(m_buffer);
        m_buffer = 0;
        m_size = 0;
    }
}

void mstring::clear_with_mem()
{
    resize(0);
    reserve();
}

mstring &mstring::delsub(const char *sub)
{
    if (!sub || 0x00 == sub[0])
    {
        return *this;
    }
    int itm = 0;
    int flag = 0;
    size_t length = strlen(sub);
    while((flag = (int)this->find(sub, itm)) != mstring::npos)
    {
        this->erase(flag, length);
        itm = flag;
    }
    return *this;
}

mstring &mstring::delchar(char c)
{
    int itm = 0;
    int flag = 0;
    while((flag = (int)this->find(c, itm)) != mstring::npos)
    {
        this->erase(flag, 1);
        itm = flag;
    }
    return *this;
}

mstring &mstring::trimleft()
{
    if (this->size() <= 0)
    {
        return *this;
    }
    mstring::iterator itm = this->begin();
    while(itm != this->end())
    {
        if (*itm == '\r' || *itm == '\n' || *itm == ' ')
        {
            itm = this->erase(itm);
        }
        else
        {
            break;
        }
    }
    return *this;
}

mstring &mstring::trimright()
{
    if (this->size() <= 0)
    {
        return *this;
    }
    size_t it = this->size() - 1;
    while(it >= 0)
    {
        if (this->at(it) == '\r' || this->at(it) == '\n' || this->at(it) == ' ')
        {
            this->erase(it);
            it--;
        }
        else
        {
            break;
        }
    }
    return *this;
}

mstring &mstring::trim()
{
    trimleft();
    trimright();
    return *this;
}

ustring mstring::a2w()
{
    ustring uf;
    int count = MultiByteToWideChar(CP_ACP, 0, c_str(), (int)size(), NULL, 0);
    if (count > 0)
    {
        wchar_t *buffer = new wchar_t[count];
        if (buffer != 0)
        {
            MultiByteToWideChar(CP_ACP, 0, c_str(), (int)size(), buffer, count);
            uf.append(buffer, count);
            delete []buffer;
        }
    }
    return uf;
}

ustring::ustring()
{
    initstring();
}

ustring::ustring(const mstring &str)
{
    initstring();
    mstring m = str;
    *this = m.a2w();
}

ustring::ustring(const ustring &str)
{
    initstring();
    *this = str;
}

ustring::ustring(const wchar_t *buffer)
{
    initstring();
    if (!buffer)
    {
        return;
    }
    *this = buffer;
}

ustring::ustring(const wchar_t* buffer, size_t len)
{
    initstring();
    if (!buffer)
    {
        return;
    }
    this->append(buffer, len);
}

ustring::ustring(const wstring &buffer)
{
    initstring();
    this->append(buffer.c_str(), buffer.size());
}

ustring::ustring(const char *buffer)
{
    initstring();
    if (!buffer)
    {
        return;
    }
    *this = buffer;
}

ustring::ustring(const char *buffer, size_t len)
{
    initstring();
    if (!buffer)
    {
        return;
    }
    *this = mstring(buffer, len);
}

ustring::ustring(const string &buffer)
{
    initstring();
    *this = mstring(buffer);
}

ustring::ustring(char c)
{
    initstring();
    *this = mstring(c);
}

ustring::ustring(wchar_t w)
{
    initstring();
    this->append(1, w);
}

void ustring::initstring()
{
    m_buffer = 0;
    m_size = 0;
}

ustring::~ustring()
{
    if (m_buffer)
    {
        free(m_buffer);
        m_buffer = 0;
        m_size = 0;
    }
}

ustring &ustring::format(const wchar_t *format, ...)
{
    if (!format || !*format)
    {
        return *this;
    }

    wchar_t *buffer = 0;
    int length;
    int size;
    va_list arg;
    buffer = (wchar_t *)malloc(sizeof(wchar_t) * 512);
    if (!buffer)
    {
        size = 0;
        return *this;
    }
    size = 512;
    va_start(arg, format);
    length = wvnsprintfW(buffer, size, format, arg);
    while(length < 0)
    {
        size *= 2;
        buffer = (wchar_t *)realloc(buffer, size * sizeof(wchar_t));
        if (!buffer)
        {
            break;
        }
        length = wvnsprintfW(buffer, size, format, arg);
    }
    va_end(arg);
    if (buffer)
    {
        this->assign(buffer, length);
        free(buffer);
    }
    return *this;
}

//ustring::operator const wchar_t *()
//{
//    return this->c_str();
//}

ustring &ustring::operator = (const wchar_t *buffer)
{
    if (!buffer)
    {
        return *this;
    }
    this->erase(this->begin(), this->end());
    this->append(buffer);
    return *this;
}

ustring &ustring::operator = (wchar_t w)
{
    this->erase(this->begin(), this->end());
    this->append(1, w);
    return *this;
}

ustring &ustring::operator = (const ustring &u)
{
    this->erase(this->begin(), this->end());
    this->append(u.c_str(), u.size());
    return *this;
}

ustring &ustring::operator = (const mstring &m)
{
    (*this) = mstring(m).a2w();
    return *this;
}

ustring &ustring::operator = (const string &m)
{
    *this = mstring(m).a2w();
    return *this;
}

ustring &ustring::operator = (const wstring &u)
{
    this->erase(this->begin(), this->end());
    this->append(u.c_str(), u.size());
    return *this;
}

ustring &ustring::operator = (const char *buffer)
{
    if (!buffer)
    {
        return *this;
    }
    *this = mstring(buffer);
    return *this;
}

ustring &ustring::operator = (char c)
{
    *this = mstring(c);
    return *this;
}

ustring &ustring::operator += (const ustring &u)
{
    this->append(u.c_str(), u.size());
    return *this;
}

ustring &ustring::operator += (const mstring &m)
{
    *this += mstring(m).a2w();
    return *this;
}

ustring &ustring::operator += (const wchar_t *buffer)
{
    if (!buffer)
    {
        return *this;
    }
    this->append(buffer);
    return *this;
}

ustring &ustring::operator += (wchar_t w)
{
    this->append(1, w);
    return *this;
}

ustring &ustring::operator += (const char *buffer)
{
    if (!buffer)
    {
        return *this;
    }
    this->append(ustring(buffer));
    return *this;
}

ustring &ustring::operator += (char c)
{
    *this += ustring(c);
    return *this;
}

ustring &ustring::operator += (const string &m)
{
    *this += ustring(m);
    return *this;
}

ustring &ustring::operator += (const wstring &u)
{
    *this += ustring(u);
    return *this;
}

size_t ustring::sfind(const ustring &str, size_t offset, size_t range, bool v)
{
    const wchar_t *ptr = c_str();
    const wchar_t *sub = str.c_str();
    size_t idx_str = 0;
    size_t idx_this = offset;
    size_t count = 0;
    size_t tmp = 0;
    size_t ret = mstring::npos;
    size_t m = min(range, size() - offset);
    wchar_t subi = 0;
    wchar_t stri = 0;
    while (count < m)
    {
        tmp = idx_this;
        idx_str = 0;
        subi = *(sub + idx_str);
        stri = *(ptr + idx_this);
        if (v && subi >= L'A' && subi <= L'Z')
        {
            subi |= 32;
        }

        if (v && stri >= L'A' && stri <= L'Z')
        {
            stri |= 32;
        }

        while (subi == stri)
        {
            idx_str++;
            idx_this++;
            subi = *(sub + idx_str);
            stri = *(ptr + idx_this);
            if (v && subi >= L'A' && subi <= L'Z')
            {
                subi |= 32;
            }

            if (v && stri >= L'A' && stri <= L'Z')
            {
                stri |= 32;
            }

            if (idx_str == str.size())
            {
                ret = tmp;
                goto leave;
            }

            if (m == (idx_this - offset))
            {
                goto leave;
            }
        }
        idx_this = tmp;
        count++;
        idx_this++;
    }
leave:
    return ret;
}

size_t ustring::find_in_range(const wchar_t *str, size_t offset, size_t range)
{
    if (!str || !*str)
    {
        return 0;
    }
    return find_in_range(ustring(str), offset, range);
}

size_t ustring::find_in_range(const ustring &str, size_t offset, size_t range)
{
    if (offset >= size())
    {
        return mstring::npos;
    }

    if (str.size() == 0)
    {
        return 0;
    }

    if (-1 == range)
    {
        range = size();
    }
    return sfind(str, offset, range, false);
}

//忽略大小写查找
size_t ustring::find_in_rangei(const wchar_t *str, size_t offset, size_t range)
{
    if (!str || !*str)
    {
        return ustring::npos;
    }
    return find_in_rangei(ustring(str), offset, range);
}

size_t ustring::find_in_rangei(const ustring &str, size_t offset, size_t range)
{
    if (offset >= size())
    {
        return ustring::npos;
    }

    if (str.size() == 0)
    {
        return ustring::npos;
    }

    if (-1 == range)
    {
        range = size();
    }
    return sfind(str, offset, range, true);
}

//比较字符串（忽略大小小写）
int ustring::comparei(const wchar_t *str, size_t offset)
{
    if (!str || !*str)
    {
        return 1;
    }
    return comparei(ustring(str), offset);
}

//
int ustring::comparei(const ustring &str, size_t offset)
{
    if (offset >= size())
    {
        return -1;
    }
    const wchar_t *src = c_str();
    const wchar_t *sub = str.c_str();
    wchar_t s = 0x00;
    wchar_t d = 0x00;
    int ret = 0;
    size_t idex = 0;
    while (idex + offset < size() && idex < str.size())
    {
        s = src[idex + offset];
        d = sub[idex];
        if (s >= L'A' && s <= L'Z')
        {
            s |= 32;
        }

        if (d >= L'A' && d <= L'Z')
        {
            d |= 32;
        }

        if (s > d)
        {
            ret = 1;
            goto leave;
        }

        if (s < d)
        {
            ret = -1;
            goto leave;
        }
        idex++;
    }
    ret = !(idex == str.size());
leave:
    return ret;
}

ustring &ustring::cover(const wchar_t *str, size_t offset)
{
    if (!str || !*str)
    {
        return *this;
    }
    return cover(ustring(str), offset);
}

ustring &ustring::cover(const mstring &buffer, size_t offset)
{
    size_t length = offset + buffer.size();
    if (length > size())
    {
        size_t count = length - size();
        wchar_t *d = new wchar_t[count];
        memset(d, 0x00, count * sizeof(wchar_t));
        append(d, count);
        delete []d;
    }

    size_t idx = 0;
    size_t idv = 0;
    for (idx = offset ; idx < offset + buffer.size() ; idx++, idv++)
    {
        at(idx) = (buffer.c_str())[idv];
    }
    return *this;
}

//取左边的子串
//count:左边起字符的个数
ustring &ustring::left(size_t count)
{
    if (size() > count)
    {
        erase(count, size() - count);
    }
    return *this;
}

//取右边的子串
//count:右边起字符的个数
ustring &ustring::right(size_t count)
{
    if (size() > count)
    {
        erase(0, size() - count);
    }
    return *this;
}

ustring &ustring::makelower()
{
    transform(this->begin(), this->end(), this->begin(), ::tolower); 
    return *this;
}

ustring &ustring::makeupper()
{
    transform(this->begin(), this->end(), this->begin(), ::toupper); 
    return *this;
}

wchar_t *ustring::alloc(size_t size)
{
    if (m_size >= size)
    {
        return m_buffer;
    }
    m_size = (size + 127) & ~127;
    if (m_buffer)
    {
        if (!(m_buffer = (wchar_t *)realloc(m_buffer, sizeof(wchar_t) * m_size)))
        {
            m_size = 0;
        }
        else
        {
            memset(m_buffer, 0x00,  sizeof(wchar_t) * m_size);
        }
    }
    else
    {
        if (!(m_buffer = (wchar_t *)malloc(sizeof(wchar_t) * m_size)))
        {
            m_size = 0;
        }
        else
        {
            memset(m_buffer, 0x00,  sizeof(wchar_t) * m_size);
        }
    }
    return m_buffer;
}

ustring &ustring::repsub(const wchar_t *src, const wchar_t *dst)
{
    if (!src || !dst || 0 == src[0])
    {
        return *this;
    }
    size_t itm = 0;
    size_t sz = lstrlenW(src);
    size_t dz = lstrlenW(dst);
    while((itm = this->find(src, itm)) != ustring::npos)
    {
        this->erase(itm, sz);
        this->insert(itm, dst);
        itm += dz;
    }
    return *this;
}

ustring &ustring::setbuffer(size_t len)
{
    if(!m_buffer)
    {
        return *this;
    }
    erase(begin(), end());
    if (len > 0)
    {
        size_t size = len > m_size ? m_size : len;
        append(m_buffer, size);
    }
    else
    {
        append(m_buffer);
    }
    release();
    return *this;
}

void ustring::release()
{
    if (m_buffer)
    {
        free(m_buffer);
        m_buffer = 0;
        m_size = 0;
    }
}

void ustring::clear_with_mem()
{
    resize(0);
    reserve();
}

ustring &ustring::delsub(const wchar_t *sub)
{
    if (!sub || 0 == sub[0])
    {
        return *this;
    }
    int itm = 0;
    int flag = 0;
    size_t length = wcslen(sub);
    while((flag = (int)this->find(sub, itm)) != ustring::npos)
    {
        this->erase(flag, length);
        itm = flag;
    }
    return *this;
}

ustring &ustring::delchar(wchar_t c)
{
    int itm = 0;
    int flag = 0;
    while((flag = (int)this->find(c, itm)) != ustring::npos)
    {
        this->erase(flag, 1);
        itm = flag;
    }
    return *this;
}

ustring &ustring::trimleft()
{
    if (this->size() <= 0)
    {
        return *this;
    }
    ustring::iterator itm = this->begin();
    while(itm != this->end())
    {
        if (*itm == L'\r' || *itm == L'\n' || *itm == L' ')
        {
            itm = this->erase(itm);
        }
        else
        {
            break;
        }
    }
    return *this;
}

ustring &ustring::trimright()
{
    if (this->size() <= 0)
    {
        return *this;
    }
    size_t it = this->size() - 1;
    while(it >= 0)
    {
        if (this->at(it) == L'\r' || this->at(it) == L'\n' || this->at(it) == L' ')
        {
            this->erase(it);
            it--;
        }
        else
        {
            break;
        }
    }
    return *this;
}

ustring &ustring::trim()
{
    trimleft();
    trimright();
    return *this;
}

mstring ustring::w2a()
{
    mstring ret;
    int count = WideCharToMultiByte(CP_ACP, 0, c_str(), (int)size(), NULL, 0, NULL, NULL);
    if (count > 0)
    {
        char *buffer = new char[count];
        if (buffer != 0)
        {
            WideCharToMultiByte(CP_ACP, 0, c_str(), (int)size(), buffer, count, NULL, NULL);
            ret.append(buffer, count);
            delete []buffer;
        }
    }
    return ret;
}

bool ustring::endwith(const wchar_t *tail)
{
    if (!tail || !tail[0])
    {
        return false;
    }
    return w2a().endwith(mstring(tail).c_str());
}

ustring &ustring::path_append(const wchar_t *more)
{
    if (empty() || !more || !more[0])
    {
        return *this;
    }
    *this = mstring(*this).path_append(mstring(more).c_str());
    return *this;
}