#include <Windows.h>
#include <stdio.h>
#include "../../ComLib/mstring.h"
#include "../global.h"
#include "../resource.h"
#include "view.h"

HWND g_parent = NULL;
HWND g_view = NULL;

const char g_tabHexCharacters[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

// windows-class-name
#define HEXEDITBASECTRL_CLASSNAME					("HexViewClass")

#define CONTROL_BORDER_SPACE							5
#define  ADR_DATA_SPACE										5
#define MAX_HIGHLIGHT_POLYPOINTS					8
#define DATA_ASCII_SPACE										5

#define UM_SETSCROLRANGE								(WM_USER + 0x5000)
#define NOSECTION_VAL										0xffffffff

#define NORMALIZE_SELECTION(beg, end) if(beg>end){UINT tmp = end; end=beg; beg=tmp; }

struct PAINTINGDETAILS
{
	UINT nFullVisibleLines;
	UINT nLastLineHeight;
	UINT nVisibleLines;
	UINT nLineHeight;
	UINT nCharacterWidth;
	UINT nBytesPerRow;
	UINT nHexPos;
	UINT nHexLen;
	UINT nAsciiPos;
	UINT nAsciiLen;
	UINT nAddressPos;
	UINT nAddressLen;
	RECT cPaintingRect;
};

enum PointAddrType
{
	em_undefine = 0,
	em_addr,
	em_hex,
	em_ascii
};

PointAddrType g_addr_type = em_undefine;

HFONT g_font = NULL;
HFONT g_old_font = NULL;

UINT g_ScrollPostionX = 0;
UINT g_ScrollPostionY = 0;

UINT g_ScrollRangeX = 0;
UINT g_ScrollRangeY = 0;

UINT g_length;
PAINTINGDETAILS g_paint_details;

mstring g_data;
UINT g_addr_size = 0;

//colour
COLORREF g_addr_back_colour = 0;
COLORREF g_addr_text_colour = 0;
COLORREF g_border_colour = 0;
COLORREF g_back_colour = 0;

COLORREF g_hex_back_colour = 0;
COLORREF g_hex_text_colour = 0;

COLORREF g_ascii_back_colour = 0;
COLORREF g_ascii_text_colour = 0;

COLORREF g_hight_light_back_colour = 0;
COLORREF g_hight_light_frame_colour = 0;
COLORREF g_hight_light_text_colour = 0;

COLORREF g_select_focus_bk_colour = GetSysColor(COLOR_HIGHLIGHT);
COLORREF g_select_nofocus_bk_colour = 0;
COLORREF g_select_focus_text_colour = GetSysColor(COLOR_HIGHLIGHTTEXT);
COLORREF g_select_nofocus_text_colour = 0;

UINT g_highlightedbegin = NOSECTION_VAL;
UINT g_highlightedend = NOSECTION_VAL;

UINT g_selection_begin = NOSECTION_VAL;
UINT g_selection_end = NOSECTION_VAL;

UINT g_selecting_begin = NOSECTION_VAL;
UINT g_selecting_end = NOSECTION_VAL;

UINT g_current_address = NOSECTION_VAL;

VOID WINAPI HexViewInit(HWND hwnd)
{
	//HWND parent = GetParent(hwnd);
	//if (IsWindow(parent))
	//{
	//	g_font = (HFONT)SendMessage(parent, WM_GETFONT, 0, 0);
	//}
	//else
	//{
	//	g_font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	//}
	g_font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	g_border_colour = GetSysColor(COLOR_WINDOW);
	g_addr_back_colour = GetSysColor(COLOR_WINDOW);
	g_addr_text_colour = RGB(0, 0, 0);
	g_hex_back_colour = GetSysColor(COLOR_WINDOW);
	g_ascii_back_colour = GetSysColor(COLOR_WINDOW);
	g_hex_text_colour = RGB(0, 0, 255);
	g_ascii_text_colour = RGB(0, 0, 255);
	g_back_colour = GetSysColor(COLOR_SCROLLBAR);

	g_select_focus_bk_colour = GetSysColor(COLOR_HIGHLIGHT);
	g_select_focus_text_colour = GetSysColor(COLOR_HIGHLIGHTTEXT);

	g_hight_light_back_colour = RGB(0xc1, 0xff, 0xc1);//BFEFFF
	g_hight_light_frame_colour = RGB(0, 0, 0);
	g_hight_light_text_colour = g_hex_text_colour;
	g_addr_size = 4;
	return;
}

VOID WINAPI CalculatePaintingDetails(HWND hwnd)
{
	int width = 0;
	HDC dc = GetDC(hwnd);
	HFONT old = (HFONT)SelectObject(dc, g_font);
	GetCharWidth32A(dc, '0', '0', &width);
	g_paint_details.nCharacterWidth = width;

	SIZE sz;
	GetTextExtentPoint32A(dc, "0", 1, &sz);
	g_paint_details.nLineHeight = sz.cy;
	SelectObject(dc, old);
	ReleaseDC(hwnd, dc);
	GetClientRect(hwnd, &g_paint_details.cPaintingRect);
	InflateRect(&g_paint_details.cPaintingRect, -CONTROL_BORDER_SPACE, -CONTROL_BORDER_SPACE);
	if (g_paint_details.cPaintingRect.right  < g_paint_details.cPaintingRect.left)
	{
		g_paint_details.cPaintingRect.right = g_paint_details.cPaintingRect.left;
	}

	if (g_paint_details.cPaintingRect.bottom < g_paint_details.cPaintingRect.top)
	{
		g_paint_details.cPaintingRect.bottom = g_paint_details.cPaintingRect.top;
	}

	int hight = g_paint_details.cPaintingRect.bottom - g_paint_details.cPaintingRect.top;
	width = g_paint_details.cPaintingRect.right - g_paint_details.cPaintingRect.left;
	g_paint_details.nVisibleLines = hight / g_paint_details.nLineHeight;
	g_paint_details.nLastLineHeight = hight % g_paint_details.nLineHeight;
	if (g_paint_details.nLastLineHeight > 0)
	{
		g_paint_details.nFullVisibleLines = g_paint_details.nVisibleLines;
		g_paint_details.nVisibleLines++;
	}
	else
	{
		g_paint_details.nFullVisibleLines = g_paint_details.nVisibleLines;
		g_paint_details.nLastLineHeight = g_paint_details.nLineHeight;
	}
	g_paint_details.nAddressPos = 0;
	g_paint_details.nAddressLen = ADR_DATA_SPACE + g_paint_details.nCharacterWidth * g_addr_size;

	//计算我们可以显示多少位数据
	int freespace = width - g_paint_details.nAddressLen;
	freespace -= DATA_ASCII_SPACE;
	if (freespace < 0)
	{
		g_paint_details.nBytesPerRow = 1;
	}
	else
	{
		// 2(HEXDATA) + 1(SPACE) + 1(ASCII) = 4
		g_paint_details.nBytesPerRow = freespace / (4 * g_paint_details.nCharacterWidth);
		if ((freespace % (4 * g_paint_details.nCharacterWidth)) >= (3 * g_paint_details.nCharacterWidth))
		{
			g_paint_details.nBytesPerRow++;
		}
		//g_paint_details.nBytesPerRow = g
	}

	if (g_paint_details.nBytesPerRow == 0)
	{
		g_paint_details.nBytesPerRow = 1;
	}

	//position & size of the hex-data
	g_paint_details.nHexPos = g_paint_details.nAddressPos + g_paint_details.nAddressLen;
	g_paint_details.nHexLen = (g_paint_details.nBytesPerRow * 2 + g_paint_details.nBytesPerRow - 1) * g_paint_details.nCharacterWidth;

	width = g_paint_details.nHexPos + g_paint_details.nHexLen;
	g_paint_details.nHexLen += DATA_ASCII_SPACE;

	//position & size of the ascii-data
	g_paint_details.nAsciiPos = g_paint_details.nHexPos + g_paint_details.nHexLen;
	g_paint_details.nAsciiLen = g_paint_details.nBytesPerRow * g_paint_details.nCharacterWidth;
	width = g_paint_details.nAsciiPos + g_paint_details.nAsciiLen;

	//calculate scrollrange
	//Y bar
	UINT totallines;
	totallines = (g_length + g_paint_details.nBytesPerRow - 1) / g_paint_details.nBytesPerRow;
	if (totallines > g_paint_details.nFullVisibleLines)
	{
		g_ScrollRangeY = totallines - g_paint_details.nFullVisibleLines;
	}
	else
	{
		g_ScrollRangeY = 0;
	}

	if (g_ScrollPostionY > g_ScrollRangeY)
	{
		g_ScrollPostionY = g_ScrollRangeY;
	}

	UINT cx = g_paint_details.cPaintingRect.right - g_paint_details.cPaintingRect.left;
	//X bar
	if (width > (int)cx)
	{
		g_ScrollRangeX = width - cx;
	}
	else
	{
		g_ScrollRangeX = 0;
	}
	
	if (g_ScrollPostionX > g_ScrollRangeX)
	{
		g_ScrollPostionX = g_ScrollRangeX;
	}
	PostMessage(hwnd, UM_SETSCROLRANGE, 0 ,0);
}

VOID WINAPI PrintAddress(HDC mdc)
{
	//if (g_length < 1 || NULL == g_data)
	//{
	//	return;
	//}

	mstring format;
	format.format("%%0%uX", g_addr_size);
	HBRUSH brush = CreateSolidBrush(g_addr_back_colour);

	RECT rt = g_paint_details.cPaintingRect;
	rt.left += g_paint_details.nAddressPos - g_ScrollPostionX;
	rt.right = rt.left + g_paint_details.nAddressLen - DATA_ASCII_SPACE;

	FillRect(mdc, &rt, brush);
	rt.bottom = rt.top + g_paint_details.nLineHeight;

	//start & end address
	int addr =g_ScrollPostionY * g_paint_details.nBytesPerRow;
	int end = addr + g_paint_details.nVisibleLines * g_paint_details.nBytesPerRow;

	if (end >= (int)g_length)
	{
		end = g_length - 1;
	}

	if (end < addr)
	{
		end = addr;
	}

	SetBkMode(mdc, OPAQUE);
	SetTextColor(mdc, g_addr_text_colour);
	SetBkColor(mdc, g_addr_back_colour);
	g_old_font = (HFONT)SelectObject(mdc, g_font);
	mstring tmp;
	for (; addr <= end; addr += g_paint_details.nBytesPerRow)
	{
		tmp.format(format.c_str(), addr);
		DrawTextA(mdc, tmp.c_str(), tmp.size(), &rt, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
		OffsetRect(&rt, 0, g_paint_details.nLineHeight);
	}
	SelectObject(mdc, g_old_font);
	DeleteObject((HGDIOBJ)brush);
}

UINT WINAPI CreateHexHighlightingPolygons(RECT hex_rect, UINT begin, UINT end, POINT *points)
{
	int start = begin - g_ScrollPostionY * g_paint_details.nBytesPerRow;
	int last = 1 + end - g_ScrollPostionY * g_paint_details.nBytesPerRow;
	int start_row = start / (int)g_paint_details.nBytesPerRow;
	int start_column = start % (int)g_paint_details.nBytesPerRow;
	int last_row = last / (int)g_paint_details.nBytesPerRow;
	int last_column = last % (int)g_paint_details.nBytesPerRow;
	int count = 0;

	if (last <= 0 || start >= int(g_paint_details.nVisibleLines * g_paint_details.nBytesPerRow))
	{
		return 0;
	}

	if (last_row == start_row + 1 && 0 == last_column)
	{
		last_row = start_row;
		last_column = g_paint_details.nBytesPerRow;
	}

	//first two or one point(s)
	if (start_row < -1)
	{
		//we don't see the beginning(or any parts of it)
		points[count].x = (short)hex_rect.left - 2;
		points[count].y = (short)hex_rect.top - 1;
		count++;
	}
	else
	{
		if (start_column < 0 && 0 == start_row)
		{
			start_column += g_paint_details.nBytesPerRow;
			start_row = -1;
		}

		if (start_column < 0)
		{
			start_column = 0;
		}
		points[count].x = (short)hex_rect.left - 2 + start_column * 3 * g_paint_details.nCharacterWidth;
		points[count].y = (short)hex_rect.top + (1 + start_row) * g_paint_details.nLineHeight;
		++count;
		points[count].x = points[count - 1].x;
		points[count].y = points[count - 1].y - g_paint_details.nLineHeight;
		++count;
	}

	if (start_row == last_row)
	{
		//a simple one (two more points and we are finished)
		points[count].x = (short)hex_rect.left + 2 + (last_column * 3 - 1) * g_paint_details.nCharacterWidth;
		points[count].y = (short)hex_rect.top + last_row * g_paint_details.nLineHeight;
		++count;
		points[count].x = points[count - 1].x;
		points[count].y = points[count -1].y + g_paint_details.nLineHeight;
		++count;
	}
	else
	{
		//last _row > start_row
		points[count].x = (short)hex_rect.right + 1;
		points[count].y = (count > 1) ? points[1].y : points[0].y;
		++count;
		points[count].x = hex_rect.right + 1;
		points[count].y = hex_rect.top + last_row * g_paint_details.nLineHeight;
		++count;
		if (last_column > 0)
		{
			points[count].x = (short)hex_rect.left + 2 + (last_column * 3 - 1) * g_paint_details.nCharacterWidth;
			points[count].y = points[count - 1].y;
			++count;
			points[count].x = points[count - 1].x;
			points[count].y = points[count - 1].y + g_paint_details.nLineHeight;
			++count;
		}
		points[count].x = (short)hex_rect.left - 2;
		points[count].y = points[count - 1].y;
		++count;
		points[count].x = (short)hex_rect.left - 2;
		points[count].y = points[0].y;
		++count;
	}
	return count;
}

UINT WINAPI CreateAsciiHighlightingPolygons(RECT hex_rect, UINT begin, UINT end, POINT *points)
{
	int start = begin - g_ScrollPostionY * g_paint_details.nBytesPerRow;
	int last = 1 + end - g_ScrollPostionY * g_paint_details.nBytesPerRow;
	int start_row = start / (int)g_paint_details.nBytesPerRow;
	int start_column = start % (int)g_paint_details.nBytesPerRow;
	int last_row = last / (int)g_paint_details.nBytesPerRow;
	int last_column = last % (int)g_paint_details.nBytesPerRow;
	int count = 0;

	if (last <= 0 || start >= int(g_paint_details.nVisibleLines * g_paint_details.nBytesPerRow))
	{
		return 0;
	}

	if (last_row == start_row + 1 && 0 == last_column)
	{
		last_row = start_row;
		last_column = g_paint_details.nBytesPerRow;
	}

	//first two or one point(s)
	if (start_row < -1)
	{
		//we don't see the beginning(or any parts of it)
		points[count].x = (short)hex_rect.left - 2;
		points[count].y = (short)hex_rect.top - 1;
		count++;
	}
	else
	{
		if (start_column < 0 && 0 == start_row)
		{
			start_column += g_paint_details.nBytesPerRow;
			start_row = -1;
		}

		if (start_column < 0)
		{
			start_column = 0;
		}
		points[count].x = (short)hex_rect.left - 2 + start_column * 1 * g_paint_details.nCharacterWidth;
		points[count].y = (short)hex_rect.top + (1 + start_row) * g_paint_details.nLineHeight;
		++count;
		points[count].x = points[count - 1].x;
		points[count].y = points[count - 1].y - g_paint_details.nLineHeight;
		++count;
	}

	if (start_row == last_row)
	{
		//a simple one (two more points and we are finished)
		points[count].x = (short)hex_rect.left + 2 + (last_column) * g_paint_details.nCharacterWidth;
		points[count].y = (short)hex_rect.top + last_row * g_paint_details.nLineHeight;
		++count;
		points[count].x = points[count - 1].x;
		points[count].y = points[count -1].y + g_paint_details.nLineHeight;
		++count;
	}
	else
	{
		//last _row > start_row
		points[count].x = (short)hex_rect.right + 1;
		points[count].y = (count > 1) ? points[1].y : points[0].y;
		++count;
		points[count].x = hex_rect.right + 1;
		points[count].y = hex_rect.top + last_row * g_paint_details.nLineHeight;
		++count;
		if (last_column > 0)
		{
			points[count].x = (short)hex_rect.left + 2 + (last_column * 1) * g_paint_details.nCharacterWidth;
			points[count].y = points[count - 1].y;
			++count;
			points[count].x = points[count - 1].x;
			points[count].y = points[count - 1].y + g_paint_details.nLineHeight;
			++count;
		}
		points[count].x = (short)hex_rect.left - 2;
		points[count].y = points[count - 1].y;
		++count;
		points[count].x = (short)hex_rect.left - 2;
		points[count].y = points[0].y;
		++count;
	}
	return count;
}

VOID WINAPI PrintHexData(HDC mdc)
{
	//if (g_length < 1 || !g_data)
	//{
	//	return;
	//}
	const char *hight_begin = NULL;
	const char *hight_end = NULL;
	const char *select_begin = NULL;
	const char *select_end = NULL;

	RECT hex_rect = g_paint_details.cPaintingRect;
	HBRUSH brush = CreateSolidBrush(g_hex_back_colour);
	hex_rect.left += g_paint_details.nHexPos - g_ScrollPostionX;
	hex_rect.right = hex_rect.left + g_paint_details.nHexLen - DATA_ASCII_SPACE;
	FillRect(mdc, &hex_rect, brush);
	DeleteObject(brush);
	hex_rect.bottom = hex_rect.top + g_paint_details.nLineHeight;
	
	BOOL is_seq = FALSE;
	UINT select_count = 0;
	POINT points[MAX_HIGHLIGHT_POLYPOINTS];
	
	//highlighting section(only background and frame)
	if (g_highlightedbegin != NOSECTION_VAL && g_highlightedend != NOSECTION_VAL)
	{
		select_count = CreateHexHighlightingPolygons(hex_rect, g_highlightedbegin, g_highlightedend, points);

		int start_tmp = g_highlightedbegin - g_ScrollPostionY * g_paint_details.nBytesPerRow;
		int last_tmp = 1 + g_highlightedend - g_ScrollPostionY * g_paint_details.nBytesPerRow;
		int start_row = start_tmp / g_paint_details.nBytesPerRow;
		int last_row = last_tmp / g_paint_details.nBytesPerRow;
		//如果是8个点的话判定这八个点是不是分离的
		if (8 == select_count)
		{
			if (points[0].x > points[4].x && last_row == start_row + 1)
			{
				is_seq = TRUE;
			}
		}
		HBRUSH bt = CreateSolidBrush(g_hight_light_back_colour);
		HPEN pt = CreatePen(PS_SOLID, 1, g_hight_light_frame_colour);
		HBRUSH old_brush = (HBRUSH)SelectObject(mdc, (HGDIOBJ)bt);
		HBRUSH old_pen = (HBRUSH)SelectObject(mdc, (HGDIOBJ)pt);

		if (is_seq)
		{
			Polygon(mdc, points, 4);
			Polygon(mdc, points + 4, 4);
		}
		else
		{
			Polygon(mdc, points, select_count);
		}
		hight_begin = g_data.c_str() + g_highlightedbegin;
		hight_end = g_data.c_str() + g_highlightedend;

		SelectObject(mdc, (HGDIOBJ)old_brush);
		SelectObject(mdc, (HGDIOBJ)old_pen);
		DeleteObject(bt);
		DeleteObject(pt);
	}
	else
	{
		hight_begin = g_data.c_str() + g_highlightedbegin;
		hight_end = g_data.c_str() + g_highlightedend;
	}
	//selection pointers
	if (g_selection_begin != NOSECTION_VAL && g_selection_end != NOSECTION_VAL)
	{
		select_begin = g_data.c_str() + g_selection_begin;
		select_end = g_data.c_str() + g_selection_end;
	}
	else
	{
		select_begin = select_end = NULL;
	}

	//UpdateHexSelect(g_selection_begin, g_selection_end, g_highlightedbegin, g_highlightedend);
    SendUserSelectMessage(g_highlightedbegin, g_highlightedend, g_selection_begin, g_selection_end);

	//start & end-address(& pointers)
	UINT start = g_ScrollPostionY * g_paint_details.nBytesPerRow;
	UINT end = start + g_paint_details.nVisibleLines * g_paint_details.nBytesPerRow;
	if (end >= g_length)
	{
		end = g_length - 1;
	}

	if (end < start)
	{
		end = start;
	}

	const UCHAR *ptr = (const UCHAR *)(g_data.c_str() + start);
	const UCHAR *end_data = (const UCHAR *)(g_data.c_str() + end);
	//paint
	COLORREF cl = GetBkColor(mdc);
	SetBkMode(mdc, TRANSPARENT);
	cl = GetBkColor(mdc);
	const UCHAR *line_end = NULL;
	char *select_buffer_begin = NULL;
	char *select_buffer_end = NULL;
	char *high_buffer_begin = NULL;
	char *high_buffer_end = NULL;
	char *buf_ptr = NULL;
	char buffer[1024] = {0x00};
	while(ptr < end_data + 1)
	{
		memset(buffer, ' ', 1024);
		buffer[1023] = 0x00;
		line_end = ptr + g_paint_details.nBytesPerRow;
		if (line_end > end_data)
		{
			line_end = end_data + 1;
		}
		select_buffer_begin = NULL;
		select_buffer_end = NULL;
		high_buffer_begin = NULL;
		high_buffer_end = NULL;
		if (ptr >= (const UCHAR *)select_begin && ptr <= (const UCHAR *)select_end)
		{
			select_buffer_begin = buffer;
		}

		if (ptr >= (const UCHAR *)hight_begin && ptr <= (const UCHAR *)hight_end)
		{
			high_buffer_begin = buffer;
		}

		for (buf_ptr = buffer ; ptr < line_end ; ptr++)
		{
			if (ptr == (const UCHAR *)select_begin)
			{
				select_buffer_begin = buf_ptr;
			}

			if (ptr == (const UCHAR *)select_end)
			{
				if (select_buffer_begin == NULL)
				{
					select_buffer_begin = buffer;
				}
				select_buffer_end = buf_ptr + 2;
			}

			if (ptr == (const UCHAR *)hight_begin)
			{
				high_buffer_begin = buf_ptr;
			}

			if (ptr == (const UCHAR *)hight_end)
			{
				if (high_buffer_begin == NULL)
				{
					high_buffer_begin = buffer;
				}
				high_buffer_end = buf_ptr + 2;
			}
			*buf_ptr++ = g_tabHexCharacters[*ptr >> 4];
			*buf_ptr++ = g_tabHexCharacters[*ptr & 0xf];
			buf_ptr++;
		}
		*--buf_ptr = 0x00;
		//set end-pointers
		if (high_buffer_end == NULL)
		{
			high_buffer_end = buf_ptr;
		}
		
		if (select_buffer_end == NULL)
		{
			select_buffer_end = buf_ptr;
		}
		
		SetTextColor(mdc, g_hex_text_colour);
		COLORREF cl = GetBkColor(mdc);
		DrawTextA(mdc, buffer, lstrlenA(buffer), &hex_rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

		//highlighted section now
		if (high_buffer_begin != NULL)
		{
			RECT rs = hex_rect;
			rs.left += (high_buffer_begin - buffer) * g_paint_details.nCharacterWidth;
			*high_buffer_end = 0x00;
			SetTextColor(mdc, g_hight_light_text_colour);
			SetBkColor(mdc, g_hight_light_back_colour);
			DrawTextA(mdc, high_buffer_begin, high_buffer_end - high_buffer_begin, &rs,  DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
			*high_buffer_end = ' ';
		}
		//selection
		if (select_buffer_begin != NULL)
		{
			bool has_focus = (GetFocus() == g_view);
			if (has_focus)
			{
				RECT rt = hex_rect;
				rt.left += (select_buffer_begin - buffer) * g_paint_details.nCharacterWidth;
				rt.right -= (buffer - 1 + g_paint_details.nBytesPerRow * 3 - select_buffer_end) * g_paint_details.nCharacterWidth;
				RECT select_rect = rt;
				select_rect.left -= 0;
				select_rect.top -= -1;
				select_rect.right += 1;
				select_rect.bottom += 0;
				HBRUSH bm = CreateSolidBrush(g_select_focus_bk_colour);
				FillRect(mdc, &select_rect, bm);
				*select_buffer_end = 0x00;
				SetTextColor(mdc, g_select_focus_text_colour);
				SetBkColor(mdc, g_select_focus_bk_colour);
				DrawTextA(mdc, select_buffer_begin, lstrlenA(select_buffer_begin), &select_rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
				*select_buffer_end = ' ';
				DeleteObject(bm);
			}
		}
		OffsetRect(&hex_rect, 0, g_paint_details.nLineHeight);
	}
	
	if (g_highlightedbegin != NOSECTION_VAL && g_highlightedend != NOSECTION_VAL)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, g_hight_light_frame_colour);
		HPEN old = (HPEN)SelectObject(mdc, pen);
		if (is_seq)
		{
			Polyline(mdc, points, 4);
			Polyline(mdc, points + 4, 4);
		}
		else
		{
			Polyline(mdc, points, select_count);
		}
		SelectObject(mdc, old);
		DeleteObject(pen);
	}
	DeleteObject((HGDIOBJ)brush);
}

VOID WINAPI PrintAsciiData(HDC mdc)
{
	//if (g_length < 1 || g_data == NULL)
	//{
	//	return;
	//}
	char buffer[1024] = {0x00};
	HBRUSH brush = CreateSolidBrush(g_ascii_back_colour);
	RECT ascii_rect = g_paint_details.cPaintingRect;
	ascii_rect.left += g_paint_details.nAsciiPos - g_ScrollPostionX;
	ascii_rect.right = ascii_rect.left + g_paint_details.nAsciiLen;
	FillRect(mdc, &ascii_rect, brush);
	ascii_rect.bottom = ascii_rect.top + g_paint_details.nLineHeight;

	const char *hight_begin = NULL;
	const char *hight_end = NULL;
	POINT points[MAX_HIGHLIGHT_POLYPOINTS];
	BOOL is_seq = FALSE;
	UINT count = CreateAsciiHighlightingPolygons(ascii_rect, g_highlightedbegin, g_highlightedend, points);

	int start_tmp = g_highlightedbegin - g_ScrollPostionY * g_paint_details.nBytesPerRow;
	int last_tmp = 1 + g_highlightedend - g_ScrollPostionY * g_paint_details.nBytesPerRow;
	int start_row = start_tmp / g_paint_details.nBytesPerRow;
	int last_row = last_tmp / g_paint_details.nBytesPerRow;

	//如果是8个点的话判定这八个点是不是分离的
	if (8 == count)
	{
		if (points[0].x > points[4].x && last_row == start_row + 1)
		{
			is_seq = TRUE;
		}
	}
	HBRUSH bt = CreateSolidBrush(g_hight_light_back_colour);
	HPEN pt = CreatePen(PS_SOLID, 1, g_hight_light_frame_colour);
	HBRUSH old_brush = (HBRUSH)SelectObject(mdc, (HGDIOBJ)bt);
	HBRUSH old_pen = (HBRUSH)SelectObject(mdc, (HGDIOBJ)pt);
	
	if (is_seq)
	{
		Polygon(mdc, points, 4);
		Polygon(mdc, points + 4, 4);
	}
	else
	{
		Polygon(mdc, points, count);
	}
	hight_begin = g_data.c_str() + g_highlightedbegin;
	hight_end = g_data.c_str() + g_highlightedend;

	SelectObject(mdc, (HGDIOBJ)old_brush);
	SelectObject(mdc, (HGDIOBJ)old_pen);
	DeleteObject(bt);
	DeleteObject(pt);

	//start & end-address
	int start = g_ScrollPostionY * g_paint_details.nBytesPerRow;
	int end = start + g_paint_details.nVisibleLines * g_paint_details.nBytesPerRow;
	if (end >= (int)g_length)
	{
		end = (int)g_length - 1;
	}

	if (end < start)
	{
		end = start;
	}

	bool selection = false;
	int select_begin = NULL;
	int select_end = NULL;
	//selection pointers
	if (g_selection_begin != NOSECTION_VAL && g_selection_end != NOSECTION_VAL)
	{
		selection = true;
		select_begin = g_selection_begin;
		select_end = g_selection_end;
	}
	else
	{
		select_begin = select_end = 0;
	}

	const unsigned char *ptr = (const unsigned char *)(g_data.c_str() + start);
	SetBkMode(mdc, OPAQUE);
	SetTextColor(mdc, g_ascii_text_colour);
	SetBkColor(mdc, g_ascii_back_colour);

	char *select_buffer_begin = NULL;
	char *select_buffer_end = NULL;
	const unsigned char *end_ptr = NULL;
	char *buf_ptr = NULL;
	SetBkMode(mdc, TRANSPARENT);
	for (; start <= end ; start += g_paint_details.nBytesPerRow)
	{
		select_buffer_begin = NULL;
		select_buffer_end = NULL;
		if (selection)
		{
			if (select_begin >= start && select_begin < (start + (int)g_paint_details.nBytesPerRow))
			{
				select_buffer_begin = buffer + (select_begin - start);
			}
			else if (select_begin < start && select_end >= start)
			{
				select_buffer_begin = buffer;
			}

			if (select_end < (start + (int)g_paint_details.nBytesPerRow))
			{
				select_buffer_end = buffer + (select_end - start);
			}
			else
			{
				select_buffer_end = (buffer + g_paint_details.nBytesPerRow - 1);
			}
		}
		
		end_ptr = ptr + g_paint_details.nBytesPerRow;
		if (end_ptr > (const unsigned char *)(g_data.c_str() + end))
		{
			end_ptr = (const unsigned char *)(g_data.c_str() + end + 1);
		}

		for (buf_ptr = buffer ; ptr < end_ptr ; ++ptr, ++buf_ptr)
		{
			*buf_ptr = isprint(*ptr) ? (char)*ptr : '.';
		}
		*buf_ptr = 0x00;
		SetTextColor(mdc, g_ascii_text_colour);
		DrawTextA(mdc, buffer, lstrlenA(buffer), &ascii_rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
		
		if (selection && NULL != select_buffer_begin && NULL != select_buffer_end)
		{
			bool has_focus = (GetFocus() == g_view);
			if (has_focus)
			{
				RECT rt = ascii_rect;
				rt.left += (select_buffer_begin - buffer) * g_paint_details.nCharacterWidth;
				rt.right -= (buffer - 1 + g_paint_details.nBytesPerRow - select_buffer_end) * g_paint_details.nCharacterWidth;
				RECT select_rect = rt;
				select_rect.left -= 0;
				select_rect.top -= -1;
				select_rect.right += 1;
				select_rect.bottom += 0;
				HBRUSH bm = CreateSolidBrush(g_select_focus_bk_colour);
				FillRect(mdc, &select_rect, bm);
				//*select_buffer_end = 0x00;
				COLORREF old_text = GetTextColor(mdc);
				COLORREF old_back = GetBkColor(mdc);
				SetTextColor(mdc, g_select_focus_text_colour);
				SetBkColor(mdc, g_select_focus_bk_colour);
				DrawTextA(mdc, select_buffer_begin, lstrlenA(select_buffer_begin), &select_rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
				SetTextColor(mdc, old_text);
				SetBkColor(mdc, old_back);
				DeleteObject(bm);
			}
		}
		OffsetRect(&ascii_rect, 0, g_paint_details.nLineHeight);
	}

	if (g_highlightedbegin != NOSECTION_VAL && g_highlightedend != NOSECTION_VAL)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, g_hight_light_frame_colour);
		HPEN old = (HPEN)SelectObject(mdc, pen);
		if (is_seq)
		{
			Polyline(mdc, points, 4);
			Polyline(mdc, points + 4, 4);
		}
		else
		{
			Polyline(mdc, points, count);
		}
		SelectObject(mdc, old);
		DeleteObject(pen);
	}
	DeleteObject(brush);
}

VOID WINAPI OnPaint(HWND hwnd, HDC dc, WPARAM wp, LPARAM lp)
{
	
	HDC mdc = CreateCompatibleDC(dc);
	RECT client;
	GetClientRect(hwnd, &client);
	HBITMAP mbmp = CreateCompatibleBitmap(dc, client.right - client.left, client.bottom - client.top);
	HBITMAP old_bmp = (HBITMAP)SelectObject(mdc, (HGDIOBJ)mbmp);
	HFONT old = (HFONT)SelectObject(mdc, g_font);
	CalculatePaintingDetails(hwnd);
	
	HBRUSH brush = CreateSolidBrush(g_back_colour);
	FillRect(mdc, &client, brush);

	HPEN pn = CreatePen(PS_SOLID, 2 , RGB(130, 135, 144));
	HBRUSH fm = (HBRUSH)GetStockObject(NULL_BRUSH);
	HPEN old_pen = (HPEN)SelectObject(mdc, pn);
	HBRUSH old_bs = (HBRUSH)SelectObject(mdc, fm);
	Rectangle(mdc, client.left, client.top, client.right, client.bottom);

	DeleteObject((HGDIOBJ)brush);
	SelectObject(mdc, old_pen);
	SelectObject(mdc, old_bs);

	DeleteObject(pn);
	DeleteObject(fm);
	
	RECT rt = g_paint_details.cPaintingRect;
	rt.left -= 2;
	rt.right += 2;
	HRGN rgn = CreateRectRgnIndirect(&rt);
	SelectClipRgn(mdc, rgn);
	PrintAddress(mdc);
	PrintHexData(mdc);
	PrintAsciiData(mdc);
	SelectObject(mdc, old);
	BitBlt(dc, 0, 0, client.right, client.bottom, mdc, 0, 0, SRCCOPY);
	SelectObject(mdc, (HGDIOBJ)old_bmp);
	DeleteObject(mbmp);
	ReleaseDC(hwnd, dc);
	DeleteDC(mdc);
	DeleteObject(rgn);
	return;
}

PointAddrType WINAPI GetPointArea(POINT pt)
{
	pt.x += g_ScrollPostionX;
	if (pt.x < (LONG)(g_paint_details.nAddressPos + g_paint_details.nAddressLen + CONTROL_BORDER_SPACE))
	{
		return em_addr;
	}
	
	if (pt.x >= (LONG)(g_paint_details.nAddressPos + g_paint_details.nAddressLen + CONTROL_BORDER_SPACE))
	{
		if (pt.x < (LONG)(g_paint_details.nHexPos + g_paint_details.nHexLen + CONTROL_BORDER_SPACE))
		{
			return em_hex;
		}
	}
	return em_ascii;
}

VOID WINAPI GetAddressFromPoint(POINT pt, UINT &address, IN PointAddrType type = em_hex)
{
	if (g_data.empty() || NULL == g_length)
	{
		return;
	}

	PointAddrType ts = GetPointArea(pt);
	pt.x += g_ScrollPostionX;
	pt.y -= g_paint_details.cPaintingRect.top;
	pt.x += (g_paint_details.nCharacterWidth >> 1) - CONTROL_BORDER_SPACE;
	if (pt.y < 0)
	{
		pt.y = 0;
	}
	else if (pt.y > (int)(g_paint_details.nVisibleLines * g_paint_details.nLineHeight))
	{
		pt.y = g_paint_details.nVisibleLines * g_paint_details.nLineHeight;
	}

	UINT row = 0;
	UINT char_column = 0;
	UINT column = 0;
	if (em_hex == type)
	{
		if (pt.x < (int)g_paint_details.nHexPos)
		{
			pt.x = g_paint_details.nHexPos;
		}
		else if (pt.x > (int)(g_paint_details.nHexPos + g_paint_details.nHexLen - DATA_ASCII_SPACE))
		{
			pt.x = g_paint_details.nHexPos + g_paint_details.nHexLen - DATA_ASCII_SPACE;
		}
		pt.x -= g_paint_details.nHexPos;
		row = pt.y / g_paint_details.nLineHeight;
		char_column = pt.x / g_paint_details.nCharacterWidth;
		column = char_column / 3;
	}
	else if (em_ascii == type)
	{
		if (pt.x < (int)g_paint_details.nAsciiPos)
		{
			pt.x = g_paint_details.nAsciiPos;
		}
		else if (pt.x > (int)(g_paint_details.nAsciiPos + g_paint_details.nAsciiLen - CONTROL_BORDER_SPACE))
		{
			pt.x = g_paint_details.nAsciiPos + g_paint_details.nAsciiLen - CONTROL_BORDER_SPACE;
		}
		pt.x -= g_paint_details.nAsciiPos;
		row = pt.y / g_paint_details.nLineHeight;
		char_column = pt.x / g_paint_details.nCharacterWidth;
		column = char_column;
	}
	else
	{
		return;
	}
	address = column + (row + g_ScrollPostionY) * g_paint_details.nBytesPerRow;
	if (address >= g_length)
	{
		address = g_length - 1;
	}
}

VOID WINAPI OnLButtonDown(DWORD wp, DWORD lp)
{
	SetFocus(g_view);
	POINT pt;
	pt.x = LOWORD(lp);
	pt.y = HIWORD(lp);
	BOOL highbits = FALSE;
	g_addr_type = GetPointArea(pt);
	if (em_hex == g_addr_type || em_ascii == g_addr_type)
	{
		GetAddressFromPoint(pt, g_current_address, g_addr_type);
		g_selection_begin = NOSECTION_VAL;
		g_selecting_end = NOSECTION_VAL;
		g_selecting_begin = g_current_address;
		SetCapture(g_view);
	}
	else
	{
		g_selection_begin = NOSECTION_VAL;
		g_selecting_end = NOSECTION_VAL;
		g_addr_type = em_undefine;
	}
 }

VOID WINAPI OnLButtonUp(DWORD wp, DWORD lp)
{
	if (g_view == GetCapture())
	{
		ReleaseCapture();
	}
	InvalidateRect(g_view, NULL, TRUE);
}

VOID WINAPI OnMouseMove(DWORD wp, DWORD lp)
{
	if (g_data.empty() || 0 == g_length)
	{
		return;
	}
	UINT flag = wp;
	if ((flag & MK_LBUTTON) && (g_selecting_begin != NOSECTION_VAL))
	{
		if (em_hex == g_addr_type || em_ascii == g_addr_type)
		{
			POINT pt;
			pt.x = LOWORD(lp);
			pt.y = HIWORD(lp);
			GetAddressFromPoint(pt, g_current_address, g_addr_type);
			g_selecting_end = g_current_address;
			g_selection_begin = g_selecting_begin;
			g_selection_end = g_selecting_end;
			NORMALIZE_SELECTION(g_selection_begin, g_selection_end);
			InvalidateRect(g_view, NULL, TRUE);
		}
	}
}

VOID WINAPI SetScrollbarRanges()
{
	SCROLLINFO scr = {0};
	scr.cbSize = sizeof(scr);
	if (g_ScrollRangeY > 0)
	{
		ShowScrollBar(g_view, SB_VERT, TRUE);
		EnableScrollBar(g_view, SB_VERT, ESB_ENABLE_BOTH);
		scr.fMask = SIF_ALL;
		scr.nPage = g_paint_details.nFullVisibleLines;
		scr.nMax = g_ScrollRangeY + scr.nPage - 1;
		if(g_ScrollPostionY > g_ScrollRangeY)
		{
			g_ScrollPostionY = g_ScrollRangeY;
		}
		scr.nPos = g_ScrollPostionY;
		SetScrollInfo(g_view, SB_VERT, &scr, TRUE);
	}
	else
	{
		ShowScrollBar(g_view, SB_VERT, FALSE);
	}

	if (g_ScrollRangeX > 0)
	{
		EnableScrollBar(g_view, SB_HORZ, ESB_ENABLE_BOTH);
		ShowScrollBar(g_view, SB_HORZ, TRUE);
		scr.fMask = SIF_ALL;
		scr.nPage = g_paint_details.cPaintingRect.right - g_paint_details.cPaintingRect.left;
		scr.nMax = g_ScrollRangeX + scr.nPage - 1;
		if (g_ScrollPostionX > g_ScrollRangeX)
		{
			g_ScrollPostionX = g_ScrollRangeX;
		}
		scr.nPos = g_ScrollPostionX;
		SetScrollInfo(g_view, SB_HORZ, &scr, TRUE);
	}
	else
	{
		ShowScrollBar(g_view, SB_HORZ, FALSE);
	}
}

VOID WINAPI OnSize(WPARAM wp, LPARAM lp)
{
	CalculatePaintingDetails(g_view);
	SetScrollbarRanges();
	InvalidateRect(g_view, NULL, TRUE);
}

VOID SetScrollPositionY(UINT pos, BOOL update)
{
	if (pos > g_ScrollRangeY)
	{
		pos = g_ScrollRangeY;
	}
	SetScrollPos(g_view, SB_VERT, pos, TRUE);
	if (pos != g_ScrollPostionY && update && IsWindow(g_view))
	{
		g_ScrollPostionY = pos;
		InvalidateRect(g_view, NULL, TRUE);
	}
	g_ScrollPostionY = pos;
}

VOID MoveScrollPostionY(int delta, BOOL update)
{
	if (delta > 0)
	{
		SetScrollPositionY(g_ScrollPostionY + delta, update);
	}
	else
	{
		int pos = (int)g_ScrollPostionY;
		pos += delta;
		if (pos < 0)
		{
			pos = 0;
		}
		SetScrollPositionY(pos, update);
	}
}

VOID WINAPI OnVsCroll(WPARAM wp, LPARAM lp)
{
	if (g_data.empty() || !g_length)
	{
		return;
	}
	int code = LOWORD(wp);
	switch(code)
	{
	case  SB_LINEDOWN:
		MoveScrollPostionY(1, TRUE);
		break;
	case  SB_LINEUP:
		MoveScrollPostionY(-1, TRUE);
		break;
	case  SB_PAGEDOWN:
		MoveScrollPostionY(g_paint_details.nFullVisibleLines, TRUE);
		break;
	case  SB_PAGEUP:
		MoveScrollPostionY(-(int)g_paint_details.nFullVisibleLines, TRUE);
		break;
	case  SB_THUMBTRACK:
		{
			SCROLLINFO scr = {0};
			scr.cbSize = sizeof(scr);
			scr.fMask = SIF_TRACKPOS;
			if (GetScrollInfo(g_view, SB_VERT, &scr))
			{
				SetScrollPositionY(scr.nTrackPos, TRUE);
			}
		}
		break;
	}
}

VOID OnLButtonDBlclk(WPARAM wp, LPARAM lp)
{
	POINT pt;
	pt.x = LOWORD(lp);
	pt.y = HIWORD(lp);
	g_addr_type = GetPointArea(pt);
	if (em_hex == g_addr_type || em_ascii == g_addr_type)
	{
		GetAddressFromPoint(pt, g_current_address, g_addr_type);
		if (g_current_address >= g_highlightedbegin && g_current_address <= g_highlightedend)
		{
			g_selection_begin = g_highlightedbegin;
			g_selection_end = g_highlightedend;
		}
		InvalidateRect(g_view, NULL, TRUE);
	}
}

VOID OnMouseWheel(WPARAM wp, LPARAM lp)
{
	MoveScrollPostionY(-((short)HIWORD(wp) / WHEEL_DELTA), TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
	case  WM_CREATE:
		{
			HexViewInit(hwnd);
		}
		break;
	case  WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			OnPaint(hwnd, hdc, wp, lp);
			EndPaint(hwnd, &ps);
		}
		break;
	case  WM_ERASEBKGND:
		{
			return 1;
		}
		break;
	case WM_SIZE:
		{
			OnSize(wp, lp);
		}
		break;
	case  WM_LBUTTONDOWN:
		OnLButtonDown(wp, lp);
		break;
	case  WM_LBUTTONUP:
		OnLButtonUp(wp, lp);
		break;
	case  WM_LBUTTONDBLCLK:
		OnLButtonDBlclk(wp, lp);
		break;
	case  WM_MOUSEMOVE:
		OnMouseMove(wp, lp);
		break;
	case WM_MOUSEWHEEL:
		OnMouseWheel(wp, lp);
		break;
	case  WM_VSCROLL:
		OnVsCroll(wp, lp);
		break;
	case  WM_CLOSE:
		break;
	default:
		return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}

BOOL WINAPI RegistHexClass()
{
	WNDCLASSA wndclass = {0};
	if (!::GetClassInfoA(g_m, HEXEDITBASECTRL_CLASSNAME, &wndclass))
	{
		wndclass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.hInstance = g_m;
		wndclass.hCursor = ::LoadCursor(NULL, IDC_IBEAM);
		wndclass.lpszClassName = HEXEDITBASECTRL_CLASSNAME;
		if(!RegisterClassA(&wndclass))
		{
			return FALSE;
		}
	}
	return TRUE;
}

VOID WINAPI SetData(const BYTE *data, size_t length)
{
	g_selection_begin = NOSECTION_VAL;
	g_selection_end = NOSECTION_VAL;
	g_selecting_begin = NOSECTION_VAL;
	g_selecting_end = NOSECTION_VAL;
	g_highlightedbegin = NOSECTION_VAL;
	g_highlightedend = NOSECTION_VAL;
	g_ScrollPostionX = 0;
	g_ScrollRangeX = 0;
	g_ScrollPostionY = 0;
	g_ScrollRangeY = 0;
	g_data.clear();
	g_data.append((const char *)data, length);
	g_length = length;
	CalculatePaintingDetails(g_view);
	SetScrollbarRanges();
	if (IsWindow(g_view))
	{
		InvalidateRect(g_view, NULL, TRUE);
	}
}

VOID WINAPI SetHighlighted(UINT begin, UINT end)
{
	if (end < begin)
	{
		return;
	}
	if (end >= g_length)
	{
		end = g_length - 1;
	}
	if (begin >= g_length)
	{
		begin = g_length - 1;
	}
	g_highlightedbegin = begin;
	g_highlightedend = end;
	InvalidateRect(g_view, NULL, TRUE);
	//UpdateHexSelect(g_selection_begin, g_selection_end, g_highlightedbegin, g_highlightedend);
    SendUserSelectMessage(g_highlightedbegin, g_highlightedend, g_selection_begin, g_selection_end);
}

HWND WINAPI CreateHexView(HWND parent, int width, int hight)
{
	if (!RegistHexClass())
	{
		return FALSE;
	}
	g_view = CreateWindowA(HEXEDITBASECTRL_CLASSNAME, "", WS_CHILD | WS_VISIBLE | ES_MULTILINE,  0, 0, width, hight, parent, NULL, g_m, NULL);
	return g_view;
}

VOID WINAPI DestroyHexView()
{
	if (IsWindow(g_view))
	{
		DestroyWindow(g_view);
		g_view = NULL;
	}
}

VOID WINAPI GetHexSelectData(mstring &strData)
{
    if (NOSECTION_VAL == g_selection_begin || NOSECTION_VAL == g_selection_end)
    {
        return;
    }
    if (g_selection_end < g_selection_begin)
    {
        return;
    }
    strData = g_data.substr(g_selection_begin, g_selection_end - g_selection_begin + 1);
}