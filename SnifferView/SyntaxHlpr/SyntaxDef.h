#ifndef SYNTAXDEF_COMLIB_H_H_
#define SYNTAXDEF_COMLIB_H_H_

#define SCI_LABEL_DEFAULT       "Default"
#define SCI_LABEL_TCP_PIPE1     "TcpPipe1"
#define SCI_LABEL_TCP_PIPE2     "TcpPipe2"

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

//tcp stream
#define SCI_PARSER_STAT_TCP_PIPE1   101
#define SCI_PARSER_STAT_TCP_PIPE2   102
#endif //SYNTAXDEF_COMLIB_H_H_
