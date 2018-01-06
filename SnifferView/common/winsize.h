/*
 *@filename: winsize.h
 *@author:   lougd
 *@created:  2014-7-15 11:38
 *@version:  1.0.0.1
 *@desc:     SetCtlsCoord:对话框控件大小或者位置自动调整的接口，通过这个接口可以在对话框
 *           大小改变后依据设置的参数自动调整其子控件的大小或者位置
 *           SetWindowRange:对话框大小范围限制接口，这个接口可以限制指定窗体的的大小范围
 *           比如最小宽度，最小高度，最大宽度，最大高度
*/
#pragma  once
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

//这个结构体存放的是窗体子控件的信息和大小调整的系数
//前两个参数填充一个即可，两个都填充的话第一个生效
typedef struct CTL_PARAMS
{
    ULONG m_id;     //对话框子窗体的id
    HWND m_hwnd;    //子窗体的窗口句柄
    FLOAT m_x;      //子控件左上横坐标的调整系数，比如当m_x为0.5的时候，如果窗体大小增加了100像素，窗体将向右移动100 * 0.5像素
    FLOAT m_y;      //子控件左上纵坐标的调整系数，比如当m_x为0.5的时候，如果窗体大小增加了100像素，窗体将向下移动100 * 0.5像素
    FLOAT m_cx;     //子控件宽度的调整系数，比如当m_x为0.5的时候，如果窗体大小增加了100像素，窗体宽度将增加100 * 0.5像素
    FLOAT m_cy;     //子控件高度的调整系数，比如当m_x为0.5的时候，如果窗体大小增加了100像素，窗体高度将增加100 * 0.5像素
}CTL_PARAMS, *PCTL_PARAMS;

//对话框子控件随主窗体调整系数设置
//这个接口最好的调用时机是在WM_INITDIALOG消息中
//hdlg:母窗体句柄
//arry:子窗体系数数组
//count:子窗体数组中元素的个数
//return:TRUR 成功 FALSE 失败
BOOL WINAPI SetCtlsCoord(HWND hdlg, PCTL_PARAMS arry, DWORD count);

//对话框大小范围设置
//这个接口最好的调用时机是在WM_INITDIALOG消息中
//hwnd:要设置的窗体句柄
//min_wide:窗体的最小宽度
//min_high:窗体的最小高度
//max_wide:窗体的最大宽度
//max_hight:窗体的最大高度
//return:TRUE 成功 FALSE 失败
BOOL WINAPI SetWindowRange(HWND hwnd, DWORD min_wide, DWORD min_hight, DWORD max_wide, DWORD max_hight);

#ifdef __cplusplus
}
#endif