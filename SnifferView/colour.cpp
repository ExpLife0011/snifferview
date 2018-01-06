/*
 *filename:   colour.cpp
 *author:     lougd
 *created:    2015-6-30 9:10
 *version:    1.0.0.1
 *desc:       —’…´∑÷≈‰À„∑®
 *history:
*/
#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <set>

using namespace std;

static vector<DWORD> s_colours;
static DWORD s_count = 0;

VOID ClearColour()
{
	s_colours.clear();
	s_count = 0;
}

DWORD GetColourValue()
{
	if (s_colours.size() == 0)
	{
		s_colours.push_back(RGB(0xff, 0xff, 0xff));
		s_colours.push_back(RGB(0xff, 0xd0, 0xd0));
		s_colours.push_back(RGB(0xd0, 0xd0, 0xff));
		s_colours.push_back(RGB(0xd0, 0xff, 0xd0));
		s_colours.push_back(RGB(0xcd, 0xcd, 0x00));
		s_colours.push_back(RGB(0xca, 0xff, 0x70));
		s_colours.push_back(RGB(0xb0, 0xe2, 0xff));
		s_colours.push_back(RGB(0x97, 0xff, 0xff));
	}
	
	if (s_count >= s_colours.size())
	{
		srand(GetTickCount() + s_count);
		BYTE r = rand() % 128 + 128;
		BYTE g = rand() % 128 + 128;
		BYTE b = rand() % 128 + 128;
		s_colours.push_back(RGB(r, g, b));
	}
	DWORD ret = s_colours[s_count];
	s_count++;
	return ret;
}