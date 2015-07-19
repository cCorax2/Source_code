#include "stdafx.h"
#include "constants.h"
#include "passpod.h"

extern BOOL g_test_server;
extern int test_server;
const char ERR_STRINGS[6][32] = 
{
	"AUTH_SUCCESS" ,
	"AUTH_FAILURE:PASSPOD_ERROR" ,
	"AUTH_FAILURE:USER_NOT_FOUND" ,
	"AUTH_FAILURE:SYSTEM_NOT_FOUND" ,
	"AUTH_FAILURE:TOKEN_DISABLED" ,
	"AUTH_FAILURE:EMPTY",
};

const char ERR_MESSAGE[6][64] =
{
	"SUCCESS",
	"PASERR1",
	"PASERR2",
	"PASERR3",
	"PASERR4",
	"PASERR5"
};

CPasspod::CPasspod()
	: m_sock(INVALID_SOCKET), m_lpFDW(NULL)
{
}


CPasspod::~CPasspod()
{
}

int CPasspod::ConfirmPasspod( const char * account, const char * passpod )
{
	const char * servername = "1001";
	const char * algname = "plaintext";
	const char * posspod_server = "/passpod-server";
	const char * auth = "/auth.s";


	char szRequest[1024];
	char szResult[1024];
	char szTmp[128];

	int ret_code = 1; // 0 이 성공 

	snprintf( szRequest, sizeof(szRequest), "username=%s&systemname=%s&passpod=%s&algname=%s", account, servername, passpod, algname );
	snprintf( szResult, sizeof(szResult), "POST %s%s HTTP/1.0\r\n", posspod_server, auth );
	snprintf( szTmp, sizeof(szTmp), "Host: %s\r\n", "218.99.6.103" );
	strlcat( szResult, szTmp, sizeof(szResult) );
	strlcat( szResult, "Content-type: application/x-www-form-urlencoded\r\n", sizeof(szResult) ); 
	snprintf( szTmp, sizeof(szTmp), "Content-length: %d\r\n", strlen(szRequest));
	strlcat( szResult, szTmp, sizeof(szResult) );
	strlcat( szResult, "Connection: Close\r\n\r\n", sizeof(szResult) );
	strlcat( szResult, szRequest, sizeof(szResult) );

	if ( !Connect( NULL ) )
	{
		sys_log( 0, "PASSPOD : Can not connect to passpod server" );
		Disconnect();
		return ret_code; 
	}
	
	int ret = socket_write( m_sock, (const char *)szResult, strlen(szResult));

	sys_log( 0, "PASSPOD : Write End %s %s", account, passpod );
	if ( test_server )
	{
		sys_log( 0, "PASSPOD : %s", szResult );
		
	}
	char Read[1024];
	int nCount = 5;
	while (--nCount)
	{
		ret = recv(m_sock, Read, 1024, 0);

		if ( ret > 0 )
		{
			if ( test_server )
			{
				sys_log( 0, "PASSPOD : %d", ret );
			}
			break;
		}
		else
		{	
			if ( test_server )
			{
				sys_log( 0, "PASSPOD : %d", ret );
			}
			Disconnect();
			return ret_code;
		}
	}
	sys_log( 0, "PASSPOD : Read End %s %s \n %s\n", account, passpod, Read );
	
	char * pos = Read;
	nCount = 15;
	while ( --nCount )
	{
		int n = 0;

		//라인넘기기
		for (; pos[n]!='\n'; ++n  ) {}
	
		//\n에서 멈추기 하나더 남겨주자 	
		pos = pos+n+1;

		
		//Return Value 를 비교 
		if ( 0 == strncmp( pos, "AUTH_SUCCESS", strlen(ERR_STRINGS[0]) ) )
		{
			ret_code = E_PASSPOD_SUCCESS;
			break;
		}
		if ( 0 == strncmp( pos, "AUTH_FAILURE:PASSPOD_ERROR", strlen(ERR_STRINGS[1]) ) )
		{
			ret_code = E_PASSPOD_FAILED_PASSPOD_ERROR;
			break;
		}
		if ( 0 == strncmp( pos, "AUTH_FAILURE:USER_NOT_FOUND", strlen(ERR_STRINGS[2] ) ) )
		{
			ret_code = E_PASSPOD_FAILED_USER_NOT_FOUND;
			break;
		}
		if ( 0 == strncmp( pos, "AUTH_FAILURE:SYSTEM_NOT_FOUND", strlen(ERR_STRINGS[3] ) ) )
		{
			ret_code = E_PASSPOD_FAILED_SYSTEM_NOT_FOUND;
			break;
		}
		if ( 0 == strncmp( pos, "AUTH_FAILURE:TOKEN_DISABLED", strlen(ERR_STRINGS[4] ) ) )
		{
			ret_code = E_PASSPOD_FAILED_TOKEN_DISABLED;
			break;
		}
		if ( 0 == strncmp( pos, "AUTH_FAILURE:EMPTY", strlen(ERR_STRINGS[5] )  ) )
		{
			ret_code = E_PASSPOD_FAILED_EMPTY;
			break;
		}

	}

	sys_log(0, "PASSPOD Ret Value = %s ", ERR_STRINGS[ret_code] );
	

	this->Disconnect();
	return ret_code;
	
}

bool CPasspod::Connect( LPFDWATCH fdw )
{

//	m_lpFDW = fdw;

	if ( m_sock != INVALID_SOCKET )
	{
		sys_err( "Sock != INVALID_SOCKET " );
		return false;
	}
		
	m_sock = socket_connect( "218.99.6.103", 8080 );

	if ( m_sock == INVALID_SOCKET )
	{
		sys_err( "Sock == INVALID_SOCKET " );
		return false;
	}



	//fdwatch_add_fd( m_lpFDW, m_sock, this, FDW_READ, false );
	//fdwatch_add_fd( m_lpFDW, m_sock, this, FDW_WRITE, false );
	
	
	return true;
}

bool CPasspod::Disconnect()
{
	//fdwatch_del_fd( m_lpFDW, m_sock );
	socket_close(m_sock);
	m_sock = INVALID_SOCKET;
	return true;
}

bool CPasspod::IConv( const char * src, char * desc )
{
	return true;
}
