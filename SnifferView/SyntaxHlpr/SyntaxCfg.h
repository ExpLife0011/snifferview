#ifndef SYNTAXCFG_H_H_
#define SYNTAXCFG_H_H_
#include <Windows.h>
#include <string>
#include "SyntaxView.h"
#include "SyntaxDef.h"

#define NULL_COLOUR 0xffffffff

/*语法高亮配置 开始*/
#define COLOUR_DEFAULT          GetSyntaxCfg(SCI_PARSER_STAT_DEFAULT)       //默认配置
#define COLOUR_KEYWORD          GetSyntaxCfg(SCI_PARSER_STAT_KEYWORD)       //关键字
#define COLOUR_NUM              GetSyntaxCfg(SCI_PARSER_STAT_NUMBER)        //数字
#define COLOUR_HIGHT            GetSyntaxCfg(SCI_PARSER_STAT_HIGHT)         //高亮
/*语法高亮配置 结束*/

struct SyntaxColourDesc
{
    std::mstring m_strDesc;
    DWORD m_dwTextColour;
    DWORD m_dwBackColour;

    BOOL m_bBold;
    BOOL m_bItalic;

    SyntaxColourDesc(
        DWORD dwTextColour = NULL_COLOUR,
        DWORD dwBackColour = NULL_COLOUR,
        BOOL bBold = FALSE,
        BOOL bItalic = FALSE
        )
    {
        m_dwTextColour = dwTextColour;
        m_dwBackColour = dwBackColour;
        m_bBold = bBold;
        m_bItalic = bItalic;
    }

    bool IsValid()
    {
        return (m_dwBackColour || m_dwTextColour);
    }
};

SyntaxColourDesc *GetSyntaxCfg(int type);
#endif