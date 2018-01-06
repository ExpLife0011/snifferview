/*
 *@filename: winsize.h
 *@author:   lougd
 *@created:  2014-7-15 11:38
 *@version:  1.0.0.1
 *@desc:     SetCtlsCoord:�Ի���ؼ���С����λ���Զ������Ľӿڣ�ͨ������ӿڿ����ڶԻ���
 *           ��С�ı���������õĲ����Զ��������ӿؼ��Ĵ�С����λ��
 *           SetWindowRange:�Ի����С��Χ���ƽӿڣ�����ӿڿ�������ָ������ĵĴ�С��Χ
 *           ������С��ȣ���С�߶ȣ�����ȣ����߶�
*/
#pragma  once
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

//����ṹ���ŵ��Ǵ����ӿؼ�����Ϣ�ʹ�С������ϵ��
//ǰ�����������һ�����ɣ����������Ļ���һ����Ч
typedef struct CTL_PARAMS
{
    ULONG m_id;     //�Ի����Ӵ����id
    HWND m_hwnd;    //�Ӵ���Ĵ��ھ��
    FLOAT m_x;      //�ӿؼ����Ϻ�����ĵ���ϵ�������統m_xΪ0.5��ʱ����������С������100���أ����彫�����ƶ�100 * 0.5����
    FLOAT m_y;      //�ӿؼ�����������ĵ���ϵ�������統m_xΪ0.5��ʱ����������С������100���أ����彫�����ƶ�100 * 0.5����
    FLOAT m_cx;     //�ӿؼ���ȵĵ���ϵ�������統m_xΪ0.5��ʱ����������С������100���أ������Ƚ�����100 * 0.5����
    FLOAT m_cy;     //�ӿؼ��߶ȵĵ���ϵ�������統m_xΪ0.5��ʱ����������С������100���أ�����߶Ƚ�����100 * 0.5����
}CTL_PARAMS, *PCTL_PARAMS;

//�Ի����ӿؼ������������ϵ������
//����ӿ���õĵ���ʱ������WM_INITDIALOG��Ϣ��
//hdlg:ĸ������
//arry:�Ӵ���ϵ������
//count:�Ӵ���������Ԫ�صĸ���
//return:TRUR �ɹ� FALSE ʧ��
BOOL WINAPI SetCtlsCoord(HWND hdlg, PCTL_PARAMS arry, DWORD count);

//�Ի����С��Χ����
//����ӿ���õĵ���ʱ������WM_INITDIALOG��Ϣ��
//hwnd:Ҫ���õĴ�����
//min_wide:�������С���
//min_high:�������С�߶�
//max_wide:����������
//max_hight:��������߶�
//return:TRUE �ɹ� FALSE ʧ��
BOOL WINAPI SetWindowRange(HWND hwnd, DWORD min_wide, DWORD min_hight, DWORD max_wide, DWORD max_hight);

#ifdef __cplusplus
}
#endif