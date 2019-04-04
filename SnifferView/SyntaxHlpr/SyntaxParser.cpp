#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <list>
#include "SyntaxParser.h"
#include "SyntaxDef.h"
#include "../common/common.h"

using namespace std;

SyntaxParser::SyntaxParser() :m_pfnRegister(NULL) {
}

SyntaxParser::~SyntaxParser() {
}

SyntaxParser *SyntaxParser::GetInstance() {
    static SyntaxParser *s_ptr = NULL;
    if (s_ptr == NULL)
    {
        s_ptr = new SyntaxParser();
    }
    return s_ptr;
}

bool SyntaxParser::InitParser() {
    RegisterSyntaxProc(SCI_LABEL_DEFAULT, DefaultParser);
    RegisterSyntaxProc(SCI_LABEL_TCP_PIPE1, TcpPipe1);
    RegisterSyntaxProc(SCI_LABEL_TCP_PIPE2, TcpPipe2);
    return true;
}

bool SyntaxParser::RegisterSyntaxProc(const std::string &label, pfnColouriseTextProc pfn) {
    if (!m_pfnRegister)
    {
        m_pfnRegister = (pfnRegisterSyntaxProc)GetProcAddress(GetModuleHandleA("SyntaxView.dll"), "RegisterSyntaxProc");
    }

    m_pfnRegister(label.c_str(), pfn);
    return true;
}

bool SyntaxParser::IsValidChar(char c) {
    if (c == '\r' || c == '\n' || c == ' ')
    {
        return false;
    }
    return true;
}

list<SyntaxParserMsg> SyntaxParser::SplitByLine(int startPos, int length, const string &content) {
    list<SyntaxParserMsg> result;
    size_t lastPos = 0;
    size_t curPos = 0;
    size_t pos1 = 0;

    SyntaxParserMsg tmp;
    while (true) {
        curPos = content.find('\n', lastPos);

        if (curPos == string::npos)
        {
            if (content.size() > lastPos)
            {
                pos1 = startPos + lastPos;
                tmp.m_startPos = startPos + lastPos;
                tmp.m_length = content.size() - lastPos;
                tmp.m_endPos = tmp.m_startPos + tmp.m_length;
                tmp.m_content = content.substr(lastPos);
                result.push_back(tmp);
            }
            break;
        }

        if (curPos >= lastPos)
        {
            pos1 = startPos + lastPos;
            //include last /n
            tmp.m_startPos = startPos + lastPos;
            tmp.m_length = curPos - lastPos + 1;
            tmp.m_endPos = tmp.m_startPos + tmp.m_length;
            tmp.m_content = content.substr(lastPos, curPos - lastPos + 1);
            result.push_back(tmp);
        }
        lastPos = curPos + 1;
    }
    return result;
}

list<SyntaxParserMsg> SyntaxParser::CallStackParserForLine(unsigned int startPos, const string &content)
{
    int curStat = SCI_PARSER_STAT_DEFAULT;
    size_t lastPos = 0;
    SyntaxParserMsg tmp;
    int i = 0;
    char buf[2] = {0};
    list<SyntaxParserMsg> result;

    for (i = 0 ; i < (int)content.size() ; i++)
    {
        char c = content.at(i);
        if (curStat == SCI_PARSER_STAT_DEFAULT)
        {
            if (!IsValidChar(c))
            {
                continue;
            }

            if (i > (int)lastPos)
            {
                tmp.m_stat = SCI_PARSER_STAT_DEFAULT;
                tmp.m_startPos = lastPos + startPos;
                tmp.m_length = i - lastPos;
                tmp.m_endPos = i + startPos;
                tmp.m_content = content.substr(lastPos, i - lastPos);
                result.push_back(tmp);
            }

            lastPos = i;
            if (c == '0' && content.at(i + 1) == 'x')
            {
                curStat = SCI_PARSER_STAT_NUMBER;
            } else {
                curStat = SCI_PARSER_STAT_FUNCTION;
            }
        } else if (curStat == SCI_PARSER_STAT_NUMBER)
        {
            if (IsValidChar(c))
            {
                continue;
            }

            if (i > (int)lastPos)
            {
                tmp.m_stat = SCI_PARSER_STAT_NUMBER;
                tmp.m_startPos = lastPos + startPos;
                tmp.m_length = i - lastPos;
                tmp.m_endPos = i + startPos;
                tmp.m_content = content.substr(lastPos, i - lastPos);
                result.push_back(tmp);
            }

            curStat = SCI_PARSER_STAT_DEFAULT;
            lastPos = i;
        } else if (curStat == SCI_PARSER_STAT_FUNCTION)
        {
            if (IsValidChar(c))
            {
                continue;
            }

            if (i > (int)lastPos)
            {
                tmp.m_stat = SCI_PARSER_STAT_FUNCTION;
                tmp.m_startPos = lastPos + startPos;
                tmp.m_length = i - lastPos;
                tmp.m_endPos = i + startPos;
                tmp.m_content = content.substr(lastPos, i - lastPos);
                result.push_back(tmp);
            }
            curStat = SCI_PARSER_STAT_DEFAULT;
            lastPos = i;
        }
    }

    if (i > (int)lastPos)
    {
        tmp.m_stat = curStat;
        tmp.m_startPos = lastPos + startPos;
        tmp.m_length = i - lastPos;
        tmp.m_endPos = i + startPos;
        tmp.m_content = content.substr(lastPos, i - lastPos);
        result.push_back(tmp);
    }
    return result;
}

void SyntaxParser::CallStackParser(
    int initStyle,
    unsigned int startPos,
    const char *data,
    int length,
    StyleContextBase *sc
    )
{
    string content = data;
    list<SyntaxParserMsg> lines = SplitByLine(startPos, length, content);
    list<SyntaxParserMsg> result;

    for (list<SyntaxParserMsg>::const_iterator it = lines.begin() ; it != lines.end() ; it++)
    {
        list<SyntaxParserMsg> tmp = CallStackParserForLine(startPos, it->m_content);
        result.insert(result.end(), tmp.begin(), tmp.end());
    }

    for (list<SyntaxParserMsg>::const_iterator it = result.begin() ; it != result.end() ; it++)
    {
        sc->SetState(it->m_stat);
        sc->ForwardBytes(it->m_length);
    }
    sc->Complete();
}

//注意:
//Forward是移动字符数
//ForwardBytes是移动字节数
//比如中文一个是一个字符，而是两个字节，此时要用ForwardBytes
void SyntaxParser::DefaultParser(
    int initStyle,
    unsigned int startPos,
    const char *data,
    int length,
    StyleContextBase *sc
    )
{
    sc->SetState(SCI_PARSER_STAT_DEFAULT);
    sc->ForwardBytes(length);
    sc->Complete();
}

void SyntaxParser::TcpPipe1(
    int initStyle,
    unsigned int startPos,
    const char *ptr,
    int length,
    StyleContextBase *sc
    )
{
    sc->SetState(SCI_PARSER_STAT_TCP_PIPE1);
    sc->ForwardBytes(length);
    sc->Complete();
}

void SyntaxParser::TcpPipe2(
    int initStyle,
    unsigned int startPos,
    const char *ptr,
    int length,
    StyleContextBase *sc
    )
{
    sc->SetState(SCI_PARSER_STAT_TCP_PIPE2);
    sc->ForwardBytes(length);
    sc->Complete();
}