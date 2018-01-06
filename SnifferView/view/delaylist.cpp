/*
 *filename:  delaylist.cpp
 *author:    lougd
 *created:   2015-7-27 16:58
 *version:   1.0.0.1
 *desc:      带延迟的ListCtrl框架，这个框架在多线程的情况下会出问题，
 *           是因为SendMessage和互斥时间会产生死锁，用的时候要尽量
 *           在单线程中使用
 *history:
*/
#include <Windows.h>
#include <CommCtrl.h>
#include <map>
#include <set>
#include <vector>
#include <mstring.h>
#include "delaylist.h"

using namespace std;

#define  DELAY_TIMEROUT             100
#define  MSG_DELAY_TIMER            (WM_USER + 8001)
#define  DELAY_TIME_DEFAULT         1000

#define  COLOUR_DEL                 RGB(255, 0, 0)
#define  COLOUR_ADD                 RGB(0, 255, 0)
#define  COLOUR_NORMAL              RGB(255, 255, 255)

class DelayListCtrl;

typedef LRESULT (CALLBACK *PWIN_PROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL (CALLBACK *PDelayProc)(DelayListCtrl *os, PWIN_PROC);

enum ItemState
{
	em_add,
	em_del,
	em_normal
};

struct DelaylistItem
{
	HWND m_list;
	int m_time_count;
	int m_time_total;
	ItemState m_state;
	COLORREF m_col;

	DelaylistItem()
	{
		m_list = NULL;
		m_time_count = 0;
		m_time_total = DELAY_TIME_DEFAULT;
		m_state = em_add;
		m_col = COLOUR_ADD;
	}
};

HANDLE s_list_lock = CreateMutexA(NULL, FALSE, NULL);
#define  LOCK_DELAY     WaitForSingleObject(s_list_lock, INFINITE)
#define  UNLOCK_DELAY   ReleaseMutex(s_list_lock)

map<HWND/*parent*/, set<HWND/*listctrl*/>> s_delay_ms;      //父窗口和ListCtrl控件映射
map<HWND/*listctrl*/, HWND/*parent*/> s_delay_mv;           //ListCtrl和父窗口控件映射
map<HWND/*parent*/, PWIN_PROC> s_delay_mu;                  //父窗口和原始的窗口回调函数映射
map<HWND/*listctrl*/, vector<DelaylistItem>> s_delay_mm;    //ListCtrl控件和其中的数据映射
map<HWND/*listctrl*/, vector<int>> s_delay_idexs;           //标记未被删除的控件的位置，对应s_delay_mm中的位置

LRESULT CALLBACK DelayListProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

VOID RedrawDelayListItem(HWND list, int itm)
{
	SendMessageA(list, LVM_REDRAWITEMS, itm, itm);
}

VOID OnDelayTimer(HWND hdlg, WPARAM wp, LPARAM lp)
{
	LOCK_DELAY;
	DelaylistItem ms;
	if (s_delay_ms.end() != s_delay_ms.find(hdlg))
	{
		set<HWND>::iterator itm;
		for (itm = s_delay_ms[hdlg].begin() ; itm != s_delay_ms[hdlg].end() ; itm++)
		{
			if (s_delay_mm.end() == s_delay_mm.find(*itm))
			{
				continue;
			}
			//update time
			vector<DelaylistItem>::iterator its = s_delay_mm[*itm].begin();
			int itv = 0;
			int itu = 0;
			while (its != s_delay_mm[*itm].end())
			{
				ms = *its;
				if (ms.m_state == em_del)
				{
					its->m_time_count += DELAY_TIMEROUT;
					if (its->m_time_count >= its->m_time_total)
					{
						//delete from listctrl
						SendMessageA(its->m_list, LVM_DELETEITEM, itv, 0);
						its = s_delay_mm[*itm].erase(its);
						//真正删除ListCtrl中的数据后调整s_delay_idex中索引
						for (itu = 0 ; itu < (int)s_delay_idexs[*itm].size() ; itu++)
						{
							if (s_delay_idexs[*itm][itu] >itv)
							{
								s_delay_idexs[*itm][itu]--;
							}
						}
						continue;
					}
				}
				else if (ms.m_state == em_add)
				{
					its->m_time_count += DELAY_TIMEROUT;
					if (its->m_time_count >= its->m_time_total)
					{
						//change state
						its->m_state = em_normal;
						its->m_time_count = 0;
						its->m_time_total = 0;
						its->m_col = COLOUR_NORMAL;
						RedrawDelayListItem(*itm, itv);
					}
				}
				itv++;
				its++;
			}
		}
	}
	UNLOCK_DELAY;
}

LRESULT DelayTableDraw(HWND list, LPARAM lp)
{
	LPNMLVCUSTOMDRAW draw = (LPNMLVCUSTOMDRAW)lp;
	switch(draw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return (CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW);
	case CDDS_ITEMPREPAINT:
		{
			size_t itm = draw->nmcd.dwItemSpec;
			LOCK_DELAY;
			if (s_delay_mm.end() != s_delay_mm.find(list))
			{
				if (s_delay_mm[list].size() > itm)
				{
					draw->clrTextBk = s_delay_mm[list][itm].m_col;
				}
			}
			UNLOCK_DELAY;
		}
		return CDRF_NEWFONT;
	default:
		break;
	}
	return CDRF_DODEFAULT;
}

BOOL OnNotify(HWND hwnd, WPARAM wp, LPARAM lp)
{
	BOOL ret = FALSE;
	LOCK_DELAY;
	LPNMHDR msg = (LPNMHDR)lp;
	do 
	{
		if (s_delay_mv.end() == s_delay_mv.find(msg->hwndFrom))
		{
			break;
		}
		switch(msg->code)
		{
		case NM_RCLICK:
			break;
		case  NM_CUSTOMDRAW:
			{
				SetWindowLongA(hwnd, DWL_MSGRESULT, long(DelayTableDraw(msg->hwndFrom, lp)));
				ret = TRUE;
			}
			break;
		default:
			break;
		}
	} while (FALSE);
	UNLOCK_DELAY;
	return ret;
}

VOID OnDestroy(HWND hwnd, WPARAM wp, LPARAM lp)
{
	LOCK_DELAY;
	//clear window msg
	UNLOCK_DELAY;
}

LRESULT CALLBACK DelayListProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT ret = 0;
	LOCK_DELAY;
	switch(msg)
	{
	case  WM_TIMER:
		{
			OnDelayTimer(hwnd, wp, lp);
		}
		break;
	case WM_NOTIFY:
		{
			if (OnNotify(hwnd, wp, lp))
			{
				ret = TRUE;
				goto end;
			}
		}
		break;
	case  WM_DESTROY:
		{
			OnDestroy(hwnd, wp, lp);
		}
		break;
	}
	ret = CallWindowProc(s_delay_mu[hwnd], hwnd, msg, wp, lp);
end:
	UNLOCK_DELAY;
	return ret;
}

//初始化DelayList，需要在父窗口初始化的时候调用
BOOL InitDelayListctrl(HWND parent, HWND list, BOOL is_dialog)
{
	LOCK_DELAY;
	BOOL ret = FALSE;
	do 
	{
		if (s_delay_mv.end() != s_delay_mv.find(list))
		{
			ret = TRUE;
			break;
		}
		PWIN_PROC vs = (PWIN_PROC)SetWindowLong(parent, DWL_DLGPROC, (LONG)DelayListProc);
		if (!vs)
		{
			break;
		}

		if (s_delay_ms.end() == s_delay_ms.find(parent))
		{
			SetTimer(parent, MSG_DELAY_TIMER, DELAY_TIMEROUT, NULL);
		}

		if (vs)
		{
			s_delay_ms[parent].insert(list);
			s_delay_mv[list] = parent;
			s_delay_mu[parent] = vs;
		}
		ret = TRUE;
	} while (FALSE);
	UNLOCK_DELAY;
	return TRUE;
}

//将数据插入到s_delay_idexs对应的位置中
static VOID InsertItem(HWND list, int itm, LVITEMA &mu)
{
	LOCK_DELAY;
	DelaylistItem ms;
	ms.m_list = list;
	ms.m_col = COLOUR_ADD;
	ms.m_state = em_add;
	ms.m_time_count = 0;
	ms.m_time_total = DELAY_TIME_DEFAULT;
	int idex = 0;
	if (itm > (int)s_delay_idexs[list].size() || itm < 0)
	{
		itm = s_delay_idexs[list].size();
	}

	if (0 == s_delay_idexs[list].size())
	{
		s_delay_mm[list].push_back(ms);
		s_delay_idexs[list].push_back(s_delay_mm[list].size() - 1);
		idex = s_delay_idexs[list][0];
	}
	else
	{
		int mm = 0;
		if (itm > 0)
		{
			mm = s_delay_idexs[list][itm - 1];
		}
		else
		{
			mm = -1;
		}
		s_delay_idexs[list].insert(s_delay_idexs[list].begin() + itm, mm + 1);
		int itu = 0;
		 for (itu = itm + 1 ; itu < (int)s_delay_idexs[list].size() ; itu++)
		 {
			 s_delay_idexs[list][itu]++;
		 }
		s_delay_mm[list].insert(s_delay_mm[list].begin() + (mm + 1), ms);
		idex = mm + 1;
	}
	mu.iItem = idex;
	SendMessageA(list, LVM_INSERTITEM, 0, (LPARAM)&mu);
	UNLOCK_DELAY;
}

BOOL InsertDelaylistItem(HWND list, UINT mask, int itm, int image, const char *text)
{
	BOOL ret = FALSE;
	LOCK_DELAY;
	do 
	{
		if (s_delay_mv.end() == s_delay_mv.find(list))
		{
			break;
		}
		LVITEMA mu = {0};
		if (mask & MASK_IMAGE)
		{
			mu.mask |= LVIF_IMAGE;
		}
		if (mask & MASK_TEXT)
		{
			mu.mask |= LVIF_TEXT;
		}
		mu.iImage = image;
		mu.pszText = (LPSTR)text;
		mu.cchTextMax = lstrlenA(text);
		//按s_delay_idexs中的位置进行插入
		InsertItem(list, itm, mu);
		ret = TRUE;
	} while (FALSE);
	UNLOCK_DELAY;
	return ret;
}

BOOL SetDelaylistText(HWND list, int itm, int sub, const char *text)
{
	LOCK_DELAY;
	BOOL ret = FALSE;
	do
	{
		if (s_delay_idexs.end() == s_delay_idexs.find(list))
		{
			break;
		}
		if (itm >= (int)s_delay_idexs[list].size())
		{
			break;
		}

		int idex = 0;
		idex = s_delay_idexs[list][itm];
		LVITEMA mu = {0};
		mu.mask |= LVIF_TEXT;
		mu.iItem = idex;
		mu.iSubItem = sub;
		mu.pszText = (LPSTR)text;
		mu.cchTextMax = lstrlenA(text);
		SendMessageA(list, LVM_SETITEMTEXT, idex, (LPARAM)&mu);
		ret = TRUE;
	}while(FALSE);
	UNLOCK_DELAY;
	return ret;
}

BOOL GetDelaylistText(HWND list, int itm, int sub, char *buffer, int length)
{
	LOCK_DELAY;
	BOOL ret = FALSE;
	do 
	{
		if (s_delay_idexs.end() == s_delay_idexs.find(list))
		{
			break;
		}
		if (itm >= (int)s_delay_idexs.size())
		{
			break;
		}

		int idex = 0;
		idex = s_delay_idexs[list][itm];
		LVITEMA mu = {0};
		mu.iItem = idex;
		mu.iSubItem = sub;
		mu.pszText = (LPSTR)buffer;
		mu.cchTextMax = length;
		SendMessageA(list, LVM_GETITEMTEXT, idex, (LPARAM)&mu);
		ret = TRUE;
	} while (FALSE);
	UNLOCK_DELAY;
	return ret;
}

//参数必须经过检查，itm不能超过全局变量的下标
static VOID DeleteItem(HWND list, int itm)
{
	int idex = s_delay_idexs[list][itm];
	s_delay_idexs[list].erase(s_delay_idexs[list].begin() + itm);
	s_delay_mm[list][idex].m_state = em_del;
	s_delay_mm[list][idex].m_col = COLOUR_DEL;
	s_delay_mm[list][idex].m_time_count = 0;
	s_delay_mm[list][idex].m_time_total = DELAY_TIME_DEFAULT;
	SendMessageA(list, LVM_REDRAWITEMS, idex, idex);
}

BOOL DeleteDelaylistItem(HWND list, int itm)
{
	BOOL ret = FALSE;
	LOCK_DELAY;
	do 
	{
		if (s_delay_mv.end() == s_delay_mv.find(list))
		{
			break;
		}
		if (itm > (int)(s_delay_idexs[list].size() - 1))
		{
			break;
		}
		DeleteItem(list, itm);
		ret = TRUE;
	} while (FALSE);
	UNLOCK_DELAY;
	return ret;
}

BOOL DeleteDelaylistItems(HWND list, int begin, int end)
{
	if (begin > end)
	{
		return FALSE;
	}
	LOCK_DELAY;
	do 
	{
		if (s_delay_idexs.size() == 0)
		{
			break;
		}

		if (begin >= (int)s_delay_idexs[list].size())
		{
			break;
		}

		if (end >= (int)s_delay_idexs[list].size())
		{
			end = s_delay_idexs[list].size() - 1;
		}

		int itm;
		for (itm = end ; itm >= begin ; itm--)
		{
			DeleteItem(list, itm);
		}
	} while (FALSE);
	UNLOCK_DELAY;
	return TRUE;
}

//通过ListCtrl的Itm获取真实有效的Itm，因为部分数据是无效的但仍缓存到ListCtrl中, -1为无效
int GetDelayValidIdex(HWND list, int idex)
{
	int id = -1;
	LOCK_DELAY;
	if (s_delay_mm.end() != s_delay_mm.find(list) && (int)s_delay_mm[list].size() > idex)
	{
		if (s_delay_mm[list][idex].m_state != em_del)
		{
			vector<int>::iterator itm;
			int mv = 0;
			for (itm = s_delay_idexs[list].begin() ; itm != s_delay_idexs[list].end() ; itm++, mv++)
			{
				if (*itm == idex)
				{
					id = mv;
				}
			}
		}
	}
	UNLOCK_DELAY;
	return id;
}

VOID SetDelaylistTime(HWND list, int add_delay, int del_delay)
{}