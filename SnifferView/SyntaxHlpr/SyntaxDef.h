#ifndef SYNTAXDEF_COMLIB_H_H_
#define SYNTAXDEF_COMLIB_H_H_

#define LABEL_DEFAULT       "Default"
#define LABEL_LOG_CONTENT   "LogContent"
#define LABEL_DBG_CONTENT   "DbgContent"
#define LABEL_CALLSTACK     "CallStack"
#define LABEL_TCP_PIPE1     "TcpPipe1"
#define LABEL_TCP_PIPE2     "TcpPipe2"

/*
20190618
STYLE ���������������ɫ�����ԣ���Ҫע��,����STYLEֵ��256,
�������ֵ�ᵼ�����õ���ɫ������Ч��STYLE_DEFAULT = 32
���ǵķ�Χ��101��ʼ����
*/
#define STYLE_CONTENT    101
#define STYLE_FILTER     102
#define STYLE_KEYWORD    121
#define STYLE_SELECT     124
#define STYLE_ERROR      131
#define STYLE_TCP_PIPE1  132    //tcp����ʽ1
#define STYLE_TCP_PIPE2  133    //tcp����ʽ2

#define NOTE_KEYWORD    SCE_UNIVERSAL_FOUND_STYLE_EXT1      //�ؼ��ָ���
#define NOTE_SELECT     SCE_UNIVERSAL_FOUND_STYLE_EXT2      //ѡ�����
#endif //SYNTAXDEF_COMLIB_H_H_
