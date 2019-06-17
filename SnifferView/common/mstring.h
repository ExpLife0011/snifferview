/*
 *@filename: mstring.h
 *@author:   lougd
 *@created:  2014-7-15 11:38
 *@version:  1.0.0.1
 *@desc:     ��װ��C++��׼���е�string���wstring�࣬�����һЩ���õķ���
 *           �����ַ�����ʽ���ӿڣ���󻯽ӿڣ���С���ӿڣ���ֵ��������������
 *           ����ӿڣ��ַ�������ӿڣ��滻�Ӵ��ӿڣ�ɾ���Ӵ��ӿڣ���׼base64
 *           �������ӿڣ�˽��base64�������ӿڣ�gbk,unicode,utf8ת���ӿ�
 *           ������ڴ�ӿڵȵȡ�
 *@warning:  ����string��������������������⺯������mstring�������ڶ�̬��ʱ��
 *           ���������mstring���������������ܻᵼ��mstring�ڲ�������ڴ��޷�
 *           ���ͷŵ������Բ�Ҫ����ʹ�á�
*/
#pragma  once
#include <string>

namespace std
{
    class ustring;
    class mstring;

    typedef string utf8_string;
    typedef mstring utf8_mstring;
 
    //�ַ���������multibyte�汾
    class mstring : public string
    {
    public:
        //Ĭ�Ϲ���
        mstring();

        //��������
        mstring(const mstring &str);

        //��������
        //mstring(const ustring &str);

        //���죬���Ե����м���/0���ַ���
        mstring(const char* buffer, size_t len);

        //���죬�Զ���������ת�������Ե����м���0x0000���ַ���
        //mstring(const wchar_t *buffer, size_t len);

        //��������
        mstring(const string &buffer);

        //��������
        //mstring(const wstring &buffer);

        //���죬��������Ϊconst char *
        mstring(const char* buffer);

        //���죬�Զ���������ת��
        //mstring(const wchar_t *buffer);

        //
        mstring(char c);

        //
        //mstring(wchar_t w); 

        //����������һЩ������
        virtual ~mstring();

    public:
        //�ַ�����ʽ�����÷�����C�������е�printf
        mstring &format(const char *format, ...);

        //������ֵ������
        //operator const char *();

        //��ֵ������
        mstring &operator = (const char *buffer);

        //����Ϊunicode�ĸ�ֵ�����������Զ���unicodeתΪgbk
        //mstring &operator = (const wchar_t *buffer);

        //��ֵ������������Ϊchar����
        mstring &operator = (char c);

        //��ֵ������������Ϊwchar����
        //mstring &operator = (wchar_t w);

        //��ֵ������������Ϊmstring����
        mstring &operator = (const mstring &m);

        //��ֵ������������Ϊustring���ͣ��Ե���������ת��
        //mstring &operator = (const ustring &u);

        //��ֵ������
        mstring &operator = (const string &m);

        //��ֵ������
        //mstring &operator = (const wstring &u);

        //+=������
        mstring &operator += (const char *buffer);

        //+=�����������������Զ�����ת��
        //mstring &operator += (const wchar_t *buffer);

        //+=������
        mstring &operator += (char c);

        //+=����������������wchar���Զ���������ת��
        //mstring &operator += (wchar_t w);

        mstring &operator += (const mstring &m);

        //+=������������Ϊustring���ͣ��Զ�����ת��
        //mstring &operator += (const ustring &u);

        //+=������
        mstring &operator += (const string &m);

        //+=������
        //mstring &operator += (const wstring &u);

        //��Χ����
        //offset:���ҵ���ʼλ��
        //range: �����ҵ��ַ���
        size_t find_in_range(const char *str, size_t offset = 0, size_t range = -1);

        size_t find_in_range(const mstring &str, size_t offset = 0, size_t range = -1);

        //���Դ�Сд����
        size_t find_in_rangei(const char *str, size_t offset = 0, size_t range = -1) const;
        size_t find_in_rangei(const mstring &str, size_t offset = 0, size_t range = -1) const;

        //�Ƚ��ַ��������Դ�ССд��
        int comparei(const char *str, size_t offset = 0) const;
        //
        int comparei(const mstring &str, size_t offset = 0) const;

        //����д����
        //buffer��Ҫ����д��Ļ�����
        //offset:д���ƫ�ƣ��������ĳ��Ȳ��������ڲ����з���;
        mstring &cover(const mstring &buffer, size_t offset = 0);

        mstring &cover(const char *str, size_t offset = 0);

        //ȡ��ߵ��Ӵ�
        //count:������ַ��ĸ���
        mstring &left(size_t count);

        //ȡ�ұߵ��Ӵ�
        //count:�ұ����ַ��ĸ���
        mstring &right(size_t count);

        //��С���ַ���
        mstring &makelower();

        //����ַ���
        mstring &makeupper();

        //�滻�ַ����е��ִ���֧�ֶ���ִ�
        mstring &repsub(const char *src, const char *dst);

        //����ָ�����ȵĻ�������������ʹ��
        char *alloc(size_t size);

        //���û���̬����Ļ����������Լ�
        mstring &setbuffer(size_t len = 0);

        //�ͷ��û�����Ļ�����
        void release();

        //������ݲ��ͷ��ڴ棬��Ϊstring������ڴ����ģ����ֻ�������ģ�����ͨ������ӿ����ͷŵ��ڴ�
        void clear_with_mem();

        //ɾ���ִ���֧�ֶ��
        mstring &delsub(const char *sub);

        //ɾ��ָ�����ַ�
        mstring &delchar(char c);

        //�����ַ�����ߣ�����/r,/n,�ո�
        mstring &trimleft();

        //�����ַ����ұ�
        mstring &trimright();

        //ͬʱ�����ַ���������
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

    //�ַ���������widechar�汾
    class ustring : public wstring
    {
    public:
        //Ĭ�Ϲ���
        ustring();

        //���죬�Զ��������͵�ת��
        //ustring(const mstring &str);

        //��������
        ustring(const ustring &str);

        //���죬�Զ���������ת��
        //ustring(const char* buffer);

        //����
        ustring(const wchar_t *buffer);

        //���죬�ַ����п�����'/0'
        //ustring(const char* buffer, size_t len);

        //���죬�ַ����п�����0x00
        ustring(const wchar_t *buffer, size_t len);

        //�������죬�Զ���������ת��
        //ustring(const string &buffer);

        //��������
        ustring(const wstring &buffer);

        //���죬����Ϊ�ַ����Զ��������͵�ת��
        //ustring(char c);

        //
        ustring(wchar_t w); 

        //����������һЩ������
        virtual ~ustring();

    public:
        //�ַ�����ʽ���ӿڣ��÷�������C�������е�printf
        ustring &format(const wchar_t *format, ...);

        //������ֵ������
        //operator const wchar_t *();

        //��ֵ���������Զ���������ת��
        //ustring &operator = (const char *buffer);

        //��ֵ������
        ustring &operator = (const wchar_t *buffer);

        //��ֵ���������Զ���������ת��
        //ustring &operator = (char c);

        //��ֵ������
        ustring &operator = (wchar_t w);

        //��ֵ������
        ustring &operator = (const ustring &u);

        //��ֵ���������Զ���������ת��
        //ustring &operator = (const mstring &m);

        //��ֵ������
        //ustring &operator = (const string &m);

        //��ֵ������
        ustring &operator = (const wstring &u);

        //+=������
        ustring &operator += (const ustring &u);

        //+=���������Զ���������ת��
        //ustring &operator += (const mstring &m);

        //+=���������Զ���������ת��
        //ustring &operator += (const char *buffer);

        //+=������
        ustring &operator += (const wchar_t *buffer);

        //+=���������Զ���������ת��
        //ustring &operator += (char c);

        //+=������
        ustring &operator += (wchar_t w);

        //+=���������Զ��������͵�ת��
        //ustring &operator += (const string &m);

        //+=������
        ustring &operator += (const wstring &u);

        //��Χ����
        //offset:���ҵ���ʼλ��
        //range: �����ҵ��ַ���
        size_t find_in_range(const wchar_t *str, size_t offset = 0, size_t range = -1);

        size_t find_in_range(const ustring &str, size_t offset = 0, size_t range = -1);

        //���Դ�Сд����
        size_t find_in_rangei(const wchar_t *str, size_t offset = 0, size_t range = -1);

        size_t find_in_rangei(const ustring &str, size_t offset = 0, size_t range = -1);

        //�Ƚ��ַ��������Դ�ССд��
        int comparei(const wchar_t *str, size_t offset = 0);

        //
        int comparei(const ustring &str, size_t offset = 0);

        //�ִ��滻�ӿڣ�֧�ֶ���ֶ�
        ustring &repsub(const wchar_t *src, const wchar_t *dst);

        ustring &cover(const wchar_t *str, size_t offset = 0);

        ustring &cover(const ustring &buffer, size_t offset = 0);

        //ȡ��ߵ��Ӵ�
        //count:������ַ��ĸ���
        ustring &left(size_t count);

        //ȡ�ұߵ��Ӵ�
        //count:�ұ����ַ��ĸ���
        ustring &right(size_t count);

        //�ַ�����С���ӿ�
        ustring &makelower();

        //�ַ�����󻯽ӿ�
        ustring &makeupper();

        //��̬�����ڴ�Ľӿ�
        wchar_t *alloc(size_t size);

        //����̬�����ڴ�д��Ľӿ�
        ustring &setbuffer(size_t size = 0);

        //�ͷŶ�̬������ڴ�
        void release();

        //������ݲ��ͷ��ڴ棬��Ϊstring������ڴ����ģ����ֻ�������ģ�����ͨ������ӿ����ͷŵ��ڴ�
        void clear_with_mem();

        //ɾ���ִ��ӿڣ�֧�ֶ���ֶ�
        ustring &delsub(const wchar_t *sub);

        //ɾ���ַ��ӿڣ�֧�ֶ���ַ�
        ustring &delchar(wchar_t c);

        //�����ַ������ӿ�
        ustring &trimleft();

        //�����ַ����Ҳ�ӿ�
        ustring &trimright();

        //ͬʱ�����ַ���������
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