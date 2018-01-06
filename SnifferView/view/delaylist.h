/*
 *	filename:  delaylist.h
 *	author:	    lougd
 *	created:    2015-7-24 15:18
 *	version:    1.0.0.1
 *	desc:			��װ��ʱ��listctrl�ؼ�
 *  history:
*/
#pragma once
#include <Windows.h>

#define  MASK_IMAGE         1 << 0
#define  MASK_TEXT          1 << 1

//��ʼ��DelayList����Ҫ�ڸ����ڳ�ʼ����ʱ�����
BOOL InitDelayListctrl(HWND parent, HWND list, BOOL is_dialog = TRUE);

//��DelayList�в�������
BOOL InsertDelaylistItem(HWND list, UINT mask, int itm, int image, const char *text);

//��DelayList��ɾ����������
BOOL DeleteDelaylistItem(HWND list, int itm);

//��DelayList��ɾ����������
BOOL DeleteDelaylistItems(HWND list, int begin, int end);

//����DelayList�ַ���
BOOL SetDelaylistText(HWND list, int itm, int sub, const char *text);

//��ȡDelayList�ַ���
BOOL GetDelaylistText(HWND list, int itm, int sub, char *buffer, int length);

//ͨ��ListCtrl��Itm��ȡ��ʵ��Ч��Itm����Ϊ������������Ч�ĵ��Ի��浽ListCtrl��, -1Ϊ��Ч
int GetDelayValidIdex(HWND list, int idex);