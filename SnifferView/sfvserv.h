#ifndef SFVSERV_H_H_
#define SFVSERV_H_H_
#include <WinSock2.h>
#include <Windows.h>

#define SFV_SERVICE_NAME           L"SfvServ"
#define SFV_SERVICE_DISPLAY_NAME   L"SfvServ"
#define SFV_SERVICE_DESCRIPTION    L"SfvServ·þÎñ"

#define SFV_NOTIFY_NAME L"Global\\{784BC5BC-25D1-4861-8FED-38C2A9428856}"

VOID RunInUser(LPCWSTR wszImage, LPCWSTR wszCmd, BOOL bSession = TRUE);

VOID RunSinfferServ();
#endif
