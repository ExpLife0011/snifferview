/*
 *	filename:  delaylist.h
 *	author:	    lougd
 *	created:    2015-7-24 15:18
 *	version:    1.0.0.1
 *	desc:			封装延时的listctrl控件
 *  history:
*/
#pragma once
#include <Windows.h>

#define  MASK_IMAGE         1 << 0
#define  MASK_TEXT          1 << 1

//初始化DelayList，需要在父窗口初始化的时候调用
BOOL InitDelayListctrl(HWND parent, HWND list, BOOL is_dialog = TRUE);

//向DelayList中插入数据
BOOL InsertDelaylistItem(HWND list, UINT mask, int itm, int image, const char *text);

//从DelayList中删除单条数据
BOOL DeleteDelaylistItem(HWND list, int itm);

//从DelayList中删除多条数据
BOOL DeleteDelaylistItems(HWND list, int begin, int end);

//设置DelayList字符串
BOOL SetDelaylistText(HWND list, int itm, int sub, const char *text);

//获取DelayList字符串
BOOL GetDelaylistText(HWND list, int itm, int sub, char *buffer, int length);

//通过ListCtrl的Itm获取真实有效的Itm，因为部分数据是无效的但仍缓存到ListCtrl中, -1为无效
int GetDelayValidIdex(HWND list, int idex);