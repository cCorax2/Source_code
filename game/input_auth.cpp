#include "stdafx.h" 
#include "constants.h"
#include "config.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "protocol.h"
#include "matrix_card.h"
#include "passpod.h"
#include "locale_service.h"
#include "auth_brazil.h"
#include "db.h"

#ifndef __WIN32__
	#include "limit_time.h"
#endif

extern time_t get_global_time();
extern int openid_server;

bool FN_IS_VALID_LOGIN_STRING(const char *str)
{
	const char*	tmp;

	if (!str || !*str)
		return false;

	if (strlen(str) < 2)
		return false;

	for (tmp = str; *tmp; ++tmp)
	{
		// 알파벳과 수자만 허용
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;

		// 캐나다는 몇몇 특수문자 허용
		if (LC_IsCanada())
		{
			switch (*tmp)
			{
				case ' ':
				case '_':
				case '-':
				case '.':
				case '!':
				case '@':
				case '#':
				case '$':
				case '%':
				case '^':
				case '&':
				case '*':
				case '(':
				case ')':
					continue;
			}
		}

		if (LC_IsYMIR() == true || LC_IsKorea() == true)
		{
			switch (*tmp)
			{
				case '-' :
				case '_' :
					continue;
			}
		}

		if (LC_IsBrazil() == true)
		{
			switch (*tmp)
			{
				case '_' :
				case '-' :
				case '=' :
					continue;
			}
		}

		if (LC_IsJapan() == true)
		{
			switch (*tmp)
			{
				case '-' :
				case '_' :
				case '@':
				case '#':
					continue;
			}
		}

		return false;
	}

	return true;
}

bool Login_IsInChannelService(const char* c_login)
{
	if (c_login[0] == '[')
		return true;
	return false;
}

CInputAuth::CInputAuth()
{
}

void CInputAuth::Login(LPDESC d, const char * c_pData)
{
#ifdef ENABLE_LIMIT_TIME
#endif
	TPacketCGLogin3 * pinfo = (TPacketCGLogin3 *) c_pData;

	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.", 
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return;
	}

	// string 무결성을 위해 복사
	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	char passwd[PASSWD_MAX_LEN + 1];
	strlcpy(passwd, pinfo->passwd, sizeof(passwd));

	sys_log(0, "InputAuth::Login : %s(%d) desc %p",
			login, strlen(login), get_pointer(d));

	// check login string
	if (false == FN_IS_VALID_LOGIN_STRING(login))
	{
		sys_log(0, "InputAuth::Login : IS_NOT_VALID_LOGIN_STRING(%s) desc %p",
				login, get_pointer(d));
		LoginFailure(d, "NOID");
		return;
	}

	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));

		d->Packet(&failurePacket, sizeof(failurePacket));
		return;
	}

	if (DESC_MANAGER::instance().FindByLoginName(login))
	{
		LoginFailure(d, "ALREADY");
		return;
	}

	DWORD dwKey = DESC_MANAGER::instance().CreateLoginKey(d);
	DWORD dwPanamaKey = dwKey ^ pinfo->adwClientKey[0] ^ pinfo->adwClientKey[1] ^ pinfo->adwClientKey[2] ^ pinfo->adwClientKey[3];
	d->SetPanamaKey(dwPanamaKey);

	sys_log(0, "InputAuth::Login : key %u:0x%x login %s", dwKey, dwPanamaKey, login);

	// BRAZIL_AUTH
	if (LC_IsBrazil() && !test_server)
	{
		int result = auth_brazil(login, passwd);

		switch (result)
		{
			case AUTH_BRAZIL_SERVER_ERR:
			case AUTH_BRAZIL_NOID:
				LoginFailure(d, "NOID");
				return;
			case AUTH_BRAZIL_WRONGPWD:
				LoginFailure(d, "WRONGPWD");
				return;
			case AUTH_BRAZIL_FLASHUSER:
				LoginFailure(d, "FLASH");
				return;
		}
	}

	TPacketCGLogin3 * p = M2_NEW TPacketCGLogin3;
	thecore_memcpy(p, pinfo, sizeof(TPacketCGLogin3));

	char szPasswd[PASSWD_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	// CHANNEL_SERVICE_LOGIN
	if (Login_IsInChannelService(szLogin))
	{
		sys_log(0, "ChannelServiceLogin [%s]", szLogin);

		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
				"SELECT '%s',password,securitycode,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",

				szPasswd, szLogin);
	}
	// END_OF_CHANNEL_SERVICE_LOGIN
	else
	{
		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p, 
				"SELECT PASSWORD('%s'),password,securitycode,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",
				szPasswd, szLogin);
	}
}

void CInputAuth::LoginOpenID(LPDESC d, const char * c_pData)
{
#ifdef ENABLE_LIMIT_TIME
#endif
	//OpenID test code.
	TPacketCGLogin5 *tempInfo1 = (TPacketCGLogin5 *)c_pData;

	//일본 웹 서버에 인증키 확인 요청을 보낸다.
	char* authKey = tempInfo1->authKey;
	char returnID[LOGIN_MAX_LEN + 1] = {0};

	int test_url_get_protocol = auth_OpenID(authKey, inet_ntoa(d->GetAddr().sin_addr), returnID);

	//인증 실패. 에러 처리
	if (0!=test_url_get_protocol)
	{
		LoginFailure(d, "OpenID Fail");
		return;
	}

	TPacketCGLogin3 tempInfo2;
	strncpy(tempInfo2.login, returnID, LOGIN_MAX_LEN);
	strncpy(tempInfo2.passwd, "0000", PASSWD_MAX_LEN);
	for(int i=0;i<4;i++)
		tempInfo2.adwClientKey[i] = tempInfo1->adwClientKey[i];
	TPacketCGLogin3 * pinfo = &tempInfo2;

	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.", 
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return;
	}

	// string 무결성을 위해 복사
	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login, login, sizeof(login));

	char passwd[PASSWD_MAX_LEN + 1];
	strlcpy(passwd, pinfo->passwd, sizeof(passwd));

	sys_log(0, "InputAuth::Login : %s(%d) desc %p",
			login, strlen(login), get_pointer(d));

	// check login string
	if (false == FN_IS_VALID_LOGIN_STRING(login))
	{
		sys_log(0, "InputAuth::Login : IS_NOT_VALID_LOGIN_STRING(%s) desc %p",
				login, get_pointer(d));
		LoginFailure(d, "NOID");
		return;
	}

	if (g_bNoMoreClient)
	{
		TPacketGCLoginFailure failurePacket;

		failurePacket.header = HEADER_GC_LOGIN_FAILURE;
		strlcpy(failurePacket.szStatus, "SHUTDOWN", sizeof(failurePacket.szStatus));

		d->Packet(&failurePacket, sizeof(failurePacket));
		return;
	}

	if (DESC_MANAGER::instance().FindByLoginName(login))
	{
		LoginFailure(d, "ALREADY");
		return;
	}

	DWORD dwKey = DESC_MANAGER::instance().CreateLoginKey(d);
	DWORD dwPanamaKey = dwKey ^ pinfo->adwClientKey[0] ^ pinfo->adwClientKey[1] ^ pinfo->adwClientKey[2] ^ pinfo->adwClientKey[3];
	d->SetPanamaKey(dwPanamaKey);

	sys_log(0, "InputAuth::Login : key %u:0x%x login %s", dwKey, dwPanamaKey, login);

	// BRAZIL_AUTH
	if (LC_IsBrazil() && !test_server)
	{
		int result = auth_brazil(login, passwd);

		switch (result)
		{
			case AUTH_BRAZIL_SERVER_ERR:
			case AUTH_BRAZIL_NOID:
				LoginFailure(d, "NOID");
				return;
			case AUTH_BRAZIL_WRONGPWD:
				LoginFailure(d, "WRONGPWD");
				return;
			case AUTH_BRAZIL_FLASHUSER:
				LoginFailure(d, "FLASH");
				return;
		}
	}

	TPacketCGLogin3 * p = M2_NEW TPacketCGLogin3;
	thecore_memcpy(p, pinfo, sizeof(TPacketCGLogin3));

	char szPasswd[PASSWD_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	// CHANNEL_SERVICE_LOGIN
	if (Login_IsInChannelService(szLogin))
	{
		sys_log(0, "ChannelServiceLogin [%s]", szLogin);

		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN_OPENID, dwKey, p,
				"SELECT '%s',password,securitycode,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",

				szPasswd, szLogin);
	}
	// END_OF_CHANNEL_SERVICE_LOGIN
	else
	{
		DBManager::instance().ReturnQuery(QID_AUTH_LOGIN_OPENID, dwKey, p, 
				"SELECT PASSWORD('%s'),password,securitycode,social_id,id,status,availDt - NOW() > 0,"
				"UNIX_TIMESTAMP(silver_expire),"
				"UNIX_TIMESTAMP(gold_expire),"
				"UNIX_TIMESTAMP(safebox_expire),"
				"UNIX_TIMESTAMP(autoloot_expire),"
				"UNIX_TIMESTAMP(fish_mind_expire),"
				"UNIX_TIMESTAMP(marriage_fast_expire),"
				"UNIX_TIMESTAMP(money_drop_rate_expire),"
				"UNIX_TIMESTAMP(create_time)"
				" FROM account WHERE login='%s'",
				szPasswd, szLogin);
	}
}

extern void socket_timeout(socket_t s, long sec, long usec);

//OpenID
int CInputAuth::auth_OpenID(const char *authKey, const char *ipAddr, char *rID)
{
	//return value
	//0 : normal execution
	//1 : cannot connect to openid auth server 
	//2 : socket_write failed
	//3 : openid auth server not reply
	//4 : Reply Error
	//5 : Incorrect auth key.

	extern char openid_host[256];
	extern char openid_uri[256];

    int		port	= 80;

    socket_t fd = socket_connect(openid_host, port);
    if (fd < 0)
    {
		sys_err("[auth_OpenID] : could not connect to OpenID server(%s)", openid_host);
		return 1;
    }

    socket_block(fd);
    socket_timeout(fd, 3, 0);

    // send request
    {
		char request[512];
		int len = snprintf(request, sizeof(request),
						//"GET /kyw/gameauth.php?auth_key=%s&ip=%s HTTP/1.1\r\n"
						"GET %s?auth_key=%s&ip=%s HTTP/1.1\r\n"
						"Host: %s\r\n"
						"Connection: Close\r\n\r\n",
						//openid_uri, authKey,ipAddr);//"aaaaa", "202.31.212.73");
						//authKey,ipAddr);
						//"/kyw/gameauth.php", authKey, ipAddr);
						openid_uri, authKey, ipAddr, openid_host);

//#ifndef __WIN32__
//		if (write(fd, request, len) < 0)
//#else
		if (socket_write(fd, request, len) < 0)
//#endif
		{
			sys_err("[auth_OpenID] : could not send auth-request (%s)", authKey);
			socket_close(fd);
			return 2;
		}
    }

    // read reply
	char reply[1024] = {0};
	int len;
//#ifndef __WIN32__
//	len = read(fd, reply, sizeof(reply));
//#else
	len = socket_read(fd, reply, sizeof(reply));
//#endif
	socket_close(fd);

	if (len <= 0)
	{
	    sys_err("[auth_OpenID] : could not recv auth-reply (%s)", authKey);
	    return 3;
	}

	//결과값 파싱
	char buffer[1024];
	strcpy(buffer, reply);

	const char *delim = "\r\n";
	char *last = 0;
	char *v = strtok(buffer, delim);
	char *result = 0;

	while (v)
	{
		result = v;
		v = strtok(NULL, delim);
	}


	char *id = strtok(result, "%");
	char *success = strtok(NULL, "%");

	if (!*id || !*success)
	{
	    sys_err("[auth_OpenID] : OpenID AuthServer Reply Error (%s)", reply);
		return 4;
	}

	if (0 != strcmp("OK", success))		//에러 처리
	{
		int returnNumber = 0;
		str_to_number(returnNumber, id);
		switch (returnNumber)
		{
		case 1:
			sys_err("[auth_OpenID] : AuthKey incorrect");
			break;
		case 2:
			sys_err("[auth_OpenID] : ip incorrect");
			break;
		case 3:
			sys_err("[auth_OpenID] : used AuthKey");
			break;
		case 4:
			sys_err("[auth_OpenID] : AuthKey not delivered");
			break;
		case 5:
			sys_err("[auth_OpenID] : ip not delivered");
			break;
		case 6:
			sys_err("[auth_OpenID] : AuthKey time over");
			break;
		default:
			break;

		return 5;
		}
	}

	strcpy(rID, id);

	return 0;
}


int CInputAuth::Analyze(LPDESC d, BYTE bHeader, const char * c_pData)
{

	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.", 
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return 0;
	}

	int iExtraLen = 0;

	if (test_server)
		sys_log(0, " InputAuth Analyze Header[%d] ", bHeader);

	switch (bHeader)
	{
		case HEADER_CG_PONG:
			Pong(d);
			break;

		case HEADER_CG_LOGIN3:
			Login(d, c_pData);
			break;

		//2012.07.19 OpenID : 김용욱
		case HEADER_CG_LOGIN5_OPENID:
			if (openid_server)
				LoginOpenID(d, c_pData);
			else
				sys_err("HEADER_CG_LOGIN5_OPENID : wrong client access");
			break;

		case HEADER_CG_PASSPOD_ANSWER:
			PasspodAnswer(d, c_pData);
			break;

		case HEADER_CG_HANDSHAKE:
			break;

		default:
			sys_err("This phase does not handle this header %d (0x%x)(phase: AUTH)", bHeader, bHeader);
			break;
	}

	return iExtraLen;
}

void CInputAuth::PasspodAnswer(LPDESC d, const char * c_pData)
{

	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.", 
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);		
		return;
	}

	TPacketCGPasspod * packet = (TPacketCGPasspod*)c_pData;

	RequestConfirmPasspod Confirm;

	memcpy(Confirm.passpod, packet->szAnswer, MAX_PASSPOD + 1);
	memcpy(Confirm.login, d->GetAccountTable().login, LOGIN_MAX_LEN + 1);
	

	if (!d->GetAccountTable().id)
	{
		sys_err("HEADER_CG_PASSPOD_ANSWER received to desc with no account table binded");
		return;
	}   

	int ret_code = 1;
	sys_log(0, "Passpod start %s %s", d->GetAccountTable().login, packet->szAnswer);
	ret_code = CPasspod::instance().ConfirmPasspod(d->GetAccountTable().login, packet->szAnswer);
	
	if (ret_code != 0)
	{
		sys_log(0, "PASSPOD: wrong answer: %s ret_code %d", d->GetAccountTable().login, ret_code);
	
		LoginFailure(d, ERR_MESSAGE[ret_code]);

		if (!d->CheckMatrixTryCount())
		{
			LoginFailure(d, "QUIT");
			d->SetPhase(PHASE_CLOSE);
		}
	}
	else
	{
		sys_log(0, "PASSPOD: success: %s", d->GetAccountTable().login);
		DBManager::instance().SendAuthLogin(d);
	}
//	g_PasspodDesc->DBPacket(HEADER_GP_CONFIRM_PASSPOD,  0, &Confirm, sizeof(Confirm));

//	sys_log(0, "PASSPOD %s %d", Confirm.login, Confirm.passpod);	
}
