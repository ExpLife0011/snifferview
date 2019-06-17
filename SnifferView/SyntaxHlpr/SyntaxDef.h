#ifndef SYNTAXDEF_COMLIB_H_H_
#define SYNTAXDEF_COMLIB_H_H_

#define LABEL_DEFAULT       "Default"
#define LABEL_LOG_CONTENT   "LogContent"
#define LABEL_DBG_CONTENT   "DbgContent"
#define LABEL_CALLSTACK     "CallStack"
#define LABEL_TCP_PIPE1     "TcpPipe1"
#define LABEL_TCP_PIPE2     "TcpPipe2"

#define SCI_PARSER_STAT_DEFAULT     1       //for default style
#define SCI_PARSER_STAT_NUMBER      2       //for number style
#define SCI_PARSER_STAT_FUNCTION    3       //for function msg
#define SCI_PARSER_STAT_ADDR        4
#define SCI_PARSER_STAT_REGISTER    5
#define SCI_PARSER_STAT_ERROR       6
#define SCI_PARSER_STAT_MESSAGE     7
#define SCI_PARSER_STAT_HEX         8
#define SCI_PARSER_STAT_DATA        9
#define SCI_PARSER_STAT_BYTE        10
#define SCI_PARSER_STAT_INST        11
#define SCI_PARSER_STAT_CALL        12
#define SCI_PARSER_STAT_JMP         13
#define SCI_PARSER_STAT_PROC        14
#define SCI_PARSER_STAT_MODULE      15
#define SCI_PARSER_STAT_PARAM       16
#define SCI_PARSER_STAT_KEYWORD     17
#define SCI_PARSER_STAT_HIGHT       18

//log content stat
#define STAT_CONTENT    101
#define STAT_FILTER     102
#define STAT_KEYWORD    121
#define STAT_SELECT     124
#define STAT_ERROR      131

#define STAT_TCP_PIPE1  301
#define STAT_TCP_PIPE2  302

#define NOTE_KEYWORD    SCE_UNIVERSAL_FOUND_STYLE_EXT1      //关键字高亮
#define NOTE_SELECT     SCE_UNIVERSAL_FOUND_STYLE_EXT2      //选择高亮
#endif //SYNTAXDEF_COMLIB_H_H_
