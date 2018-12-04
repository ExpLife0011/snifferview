#include <WinSock2.h>
#include <common.h>
#include <base64.h>
#include "dump.h"

static size_t s_check_size = 0;

#define  DUMP_SECTION_MARK					("\r\r")
#define  DUMP_DATA_MARK						("\n\n")

//字段间标识 /r/r，数据间标识/n/n
BOOL WINAPI DumpPacketsToFile(IN const char *path, IN vector<PPacketContent> &packets)
{
	BOOL status = FALSE;
	HANDLE file = NULL;
	do 
	{
		file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!file || INVALID_HANDLE_VALUE == file)
		{
			break;
		}

		DumpFileVersion vers;
		mstring ms;
		ms.append((const char *)&vers, sizeof(vers));
		ms = base64encode(ms);
		ms += '\0';
		if (!WriteFileBuffer(file, ms.c_str(), ms.size()))
		{
			break;
		}
		vector<PPacketContent>::iterator itm;
		mstring vv;
		mstring uu;
		list<mstring>::iterator its;
		PPacketContent vm;
		for (itm = packets.begin() ; itm != packets.end() ; itm++)
		{
			vm = *itm;
			vv.clear();
			uu.clear();
			uu.append((const char *)&(vm->m_packet_init), sizeof(vm->m_packet_init));
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());

			uu.format("%hs%hs", vm->m_packet_mark.c_str(), DUMP_SECTION_MARK);
			vv.append(uu.c_str(), uu.size());

			uu.format("%hs%hs", vm-> m_dec_mark.c_str(), DUMP_SECTION_MARK);
			vv.append(uu.c_str(), uu.size());

			uu.clear();
			uu.append((const char *)&(vm->m_ip_header), sizeof(vm->m_ip_header));
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());

			uu.clear();
			uu.append((const char *)&(vm->m_tls_type), sizeof(vm->m_tls_type));
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());

			uu.clear();
			uu.append((const char *)&(vm->m_tls_header), sizeof(vm->m_tls_header));
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());

			uu.clear();
			uu.append((const char *)&(vm->m_user_type), sizeof(vm->m_user_type));
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());

			uu.clear();
			uu.append((const char *)&(vm->m_user_content), sizeof(vm->m_user_content));
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());

			int size = vm->m_urls.size();
			uu.clear();
			uu.append((const char *)&size, sizeof(size));
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());
			for (its = vm->m_urls.begin() ; its != vm->m_urls.end() ; its++)
			{
				uu.clear();
				uu.append(its->c_str(), its->size());
				uu.repsub("\n", "#\n");
				uu.repsub("\r", "#\r");
				uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
				vv.append(uu.c_str(), uu.size());
			}
			
			uu.clear();
			uu.append(vm->m_packet.c_str(), vm->m_packet.size());
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());

			uu.clear();
			uu.append(vm->m_show.c_str(), vm->m_show.size());
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());

			uu.clear();
			uu.append(vm->m_time.c_str(), vm->m_time.size());
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());

			uu.clear();
			uu.append((const char *)&vm->m_colour, sizeof(vm->m_colour));
			uu.repsub("\n", "#\n");
			uu.repsub("\r", "#\r");
			uu.append(DUMP_SECTION_MARK, lstrlenA(DUMP_SECTION_MARK));
			vv.append(uu.c_str(), uu.size());
			
			vv.append(DUMP_DATA_MARK);
			if (!WriteFileBuffer(file, vv.c_str(), vv.size()))
			{
				break;
			}
		}
		status = TRUE;
	} while (FALSE);
	
	if (file && INVALID_HANDLE_VALUE != file)
	{
		CloseHandle(file);
	}
	return status;
}

static PPacketContent WINAPI AnalysisSinglePacket(const char *buffer, size_t length)
{
	mstring vv(buffer, length);
	PacketContent tmp;
	mstring ms;
	int itm = 0;
	int m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	memcpy(&tmp.m_packet_init, ms.c_str(), sizeof(tmp.m_packet_init));
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	tmp.m_packet_mark = ms;
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	tmp.m_dec_mark = ms;
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	memcpy(&tmp.m_ip_header, ms.c_str(), ms.size());
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	memcpy(&tmp.m_tls_type, ms.c_str(), ms.size());
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	memcpy(&tmp.m_tls_header, ms.c_str(), ms.size());
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	memcpy(&tmp.m_user_type, ms.c_str(), ms.size());
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	memcpy(&tmp.m_user_content, ms.c_str(), ms.size());
	itm = m + 2;

	int size = 0;
	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	memcpy(&size, ms.c_str(), ms.size());
	itm = m + 2;

	int itk = 0;
	mstring uu;
	for (itk = 0 ; itk < size ; itk++)
	{
		m = vv.find(DUMP_SECTION_MARK, itm);
		if (mstring::npos == m)
		{
			return NULL;
		}
		ms.assign(vv, itm, m - itm);
		ms.repsub("#\n", "\n");
		ms.repsub("#\r", "\r");
		tmp.m_urls.push_back(ms);
		itm = m + 2;
	}

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	tmp.m_packet.append(ms.c_str(), ms.size());
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	tmp.m_show.append(ms.c_str(), ms.size());
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	tmp.m_time.append(ms.c_str(), ms.size());
	itm = m + 2;

	m = vv.find(DUMP_SECTION_MARK, itm);
	if (mstring::npos == m)
	{
		return NULL;
	}
	ms.assign(vv, itm, m - itm);
	ms.repsub("#\n", "\n");
	ms.repsub("#\r", "\r");
	memcpy(&tmp.m_colour, ms.c_str(), ms.size());
	itm = m + 2;
	PPacketContent vs = new PacketContent;
	*vs = tmp;
	return vs;
}

BOOL WINAPI DumpPacketsFromFile(IN const char *path, OUT vector<PPacketContent> &packets)
{
	BOOL status = FALSE;
	HANDLE file = NULL;
	LARGE_INTEGER size;
	size.QuadPart = 0;
	HANDLE mv = NULL;
	LPVOID vv = NULL;
	do
	{
		file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!file || INVALID_HANDLE_VALUE == file)
		{
			break;
		}
		GetFileSizeEx(file, &size);
		if (size.QuadPart > 0xffffffff)
		{
			break;
		}
		mv = CreateFileMappingA(file, NULL, PAGE_READWRITE, size.u.HighPart, size.u.LowPart, NULL);
		if (!mv || INVALID_HANDLE_VALUE == mv)
		{
			break;
		}
		vv = MapViewOfFile(mv, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
		mstring vt((const char *)vv, size.u.LowPart);
		size_t itm = 0;
		size_t itk = 0;
		if (mstring::npos == (itk = vt.find('\0', itm)))
		{
			break;
		}

		if (0 ==s_check_size)
		{
			DumpFileVersion vers;
			mstring ms;
			ms.append((const char *)&vers, sizeof(vers));
			ms = base64encode(ms);
			s_check_size = ms.size();
		}

		if (itk != s_check_size)
		{
			dp(L"dump file error");
			break;
		}

		mstring uv;
		uv.assign(vt, itm, itk - itm);
		uv = base64decode(uv);
		DumpFileVersion ver;
		if (uv.size() != sizeof(ver))
		{
			break;
		}
		memcpy(&ver, uv.c_str(), uv.size());
		if (ver.m_version != DUMP_FILE_VER)
		{
			break;
		}
		itm = itk + 1;
		PPacketContent vs = NULL;
		while(mstring::npos != (itk = vt.find(DUMP_DATA_MARK, itm)))
		{
			if (vs = AnalysisSinglePacket(vt.c_str() + itm, itk - itm))
			{
				packets.push_back(vs);
			}
			else
			{
				MessageBoxA(0, "file error", "error", 0);
			}
			itm = itk + lstrlenA(DUMP_DATA_MARK);
		}
		status = TRUE;
	} while (FALSE);

	if (vv)
	{
		UnmapViewOfFile(vv);
	}

	if (mv && INVALID_HANDLE_VALUE != mv)
	{
		CloseHandle(mv);
	}

	if (file && INVALID_HANDLE_VALUE != file)
	{
		CloseHandle(file);
	}
	return status;
}