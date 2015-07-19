#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
#include <io.h>
#include <windows.h>
#include <tchar.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <XTrap_S_Interface.h>

#include "char.h"
#include "config.h"
#include "event.h"
#include "log.h"
#include "desc.h"
#include "packet.h"
#include "XTrapManager.h"

#define CSFILE_NUM				2
#define XTRAP_CS1_CHECK_CYCLE	PASSES_PER_SEC(20) // per 20sec

unsigned char g_XTrap_ClientMap[CSFILE_NUM][XTRAP_CS4_BUFSIZE_MAP];


struct CXTrapManager::sXTrapContext
{
	//API function pointers
	PFN_XTrap_S_Start			XTrap_S_Start;
	PFN_XTrap_S_SessionInit		XTrap_S_SessionInit;
	PFN_XTrap_CS_Step1			XTrap_CS_Step1;
	PFN_XTrap_CS_Step3			XTrap_CS_Step3;

	PFN_XTrap_S_SetActiveCode	XTrap_S_SetActiveCode; 
	PFN_XTrap_S_SetOption		XTrap_S_SetOption;
	PFN_XTrap_S_SetAllowDelay	XTrap_S_SetAllowDelay;
	PFN_XTrap_S_SendGamePacket	XTrap_S_SendGamePacket;
	PFN_XTrap_S_RecvGamePacket	XTrap_S_RecvGamePacket;

	//handle
	void*		  hXTrap4Server;
};


CXTrapManager::CXTrapManager()
{
	m_pImpl = M2_NEW sXTrapContext;
	memset( m_pImpl, 0x00, sizeof(sXTrapContext) );
}

CXTrapManager::~CXTrapManager()
{
#ifdef __FreeBSD__
	if (m_pImpl->hXTrap4Server)
	{
		dlclose(m_pImpl->hXTrap4Server);
	}
#endif

	M2_DELETE(m_pImpl);
}

#ifdef __FreeBSD__
void CXTrapManager::MapReloadSignalHandler( int signal )
{
	for(int i=0; i<CSFILE_NUM; ++i )
	{
		if( Instance().LoadClientMapFile(i) )
			sys_log(0, "client map file(map%d).CS3 is reloaded", i+1 );
	}
}

void CXTrapManager::NotifyMapFileChanged( const std::string& fileName, eFileUpdatedOptions eUpdateOption )
{
	MapReloadSignalHandler(1);
}


#endif

bool CXTrapManager::LoadXTrapModule()
{
#ifdef __FreeBSD__
	//first load client mapfile
	bool bClientMapFileLoaded = false;
	for(int i=0; i<CSFILE_NUM; ++i )
	{
		if( LoadClientMapFile(i) )
		{
			bClientMapFileLoaded = true;
		}
	}

	if( !bClientMapFileLoaded )
	{
		sys_err("XTrap-failed to load at least one client map file. map file name should be map1.CS3 or map2.CS3");
		return false;
	}


	//load shared objects
	char sDllBinFile[]	="./libXTrap4Server.so";

	m_pImpl->hXTrap4Server = dlopen(sDllBinFile, RTLD_LAZY);

	if (m_pImpl->hXTrap4Server == 0) 
	{
		sys_err("XTrap-failed to load so reason:%s", dlerror()) ;
		return false;	
	}

	void* hXTrapHandle = m_pImpl->hXTrap4Server;

	m_pImpl->XTrap_S_Start		    = (PFN_XTrap_S_Start)			dlsym(hXTrapHandle, "XTrap_S_Start");
	m_pImpl->XTrap_S_SessionInit    = (PFN_XTrap_S_SessionInit)		dlsym(hXTrapHandle, "XTrap_S_SessionInit");
	m_pImpl->XTrap_CS_Step1		    = (PFN_XTrap_CS_Step1)			dlsym(hXTrapHandle, "XTrap_CS_Step1");
	m_pImpl->XTrap_CS_Step3		    = (PFN_XTrap_CS_Step3)			dlsym(hXTrapHandle, "XTrap_CS_Step3");
	m_pImpl->XTrap_S_SetActiveCode  = (PFN_XTrap_S_SetActiveCode)	dlsym(hXTrapHandle, "XTrap_S_SetActiveCode");
	m_pImpl->XTrap_S_SetOption	    = (PFN_XTrap_S_SetOption)		dlsym(hXTrapHandle, "XTrap_S_SetOption");
	m_pImpl->XTrap_S_SetAllowDelay  = (PFN_XTrap_S_SetAllowDelay)	dlsym(hXTrapHandle, "XTrap_S_SetAllowDelay");
	m_pImpl->XTrap_S_SendGamePacket = (PFN_XTrap_S_SendGamePacket)	dlsym(hXTrapHandle, "XTrap_S_SendGamePacket");
	m_pImpl->XTrap_S_RecvGamePacket = (PFN_XTrap_S_RecvGamePacket)	dlsym(hXTrapHandle, "XTrap_S_RecvGamePacket");

	if (m_pImpl->XTrap_S_Start			== NULL ||
		m_pImpl->XTrap_S_SessionInit	== NULL ||
		m_pImpl->XTrap_CS_Step1			== NULL ||
		m_pImpl->XTrap_CS_Step3			== NULL ||
		m_pImpl->XTrap_S_SetOption		== NULL ||
		m_pImpl->XTrap_S_SetAllowDelay	== NULL ||
		m_pImpl->XTrap_S_SendGamePacket	== NULL	||
		m_pImpl->XTrap_S_RecvGamePacket	== NULL)
	{
		sys_err("XTrap-failed to load function ptrs");
		return	false;
	}

	//start server module
	m_pImpl->XTrap_S_Start( 600, CSFILE_NUM, g_XTrap_ClientMap, NULL ); 

	//NOTE : 일단 XProtect모듈에 버그가 있어서 코드영역 체크를 끈다.
	m_pImpl->XTrap_S_SetActiveCode( XTRAP_ACTIVE_CODE_THEMIDA );

	//setup signal
	signal(SIGUSR2, CXTrapManager::MapReloadSignalHandler);

#endif 
	return true;
}

bool CXTrapManager::LoadClientMapFile( unsigned int iMapIndex )
{
#ifdef __FreeBSD__
	//index check
	if( iMapIndex >= CSFILE_NUM )
	{
		return false;
	}

	char szFileName[1024] = {0,};
	snprintf(szFileName, sizeof(szFileName), "map%d.CS3", iMapIndex+1);

	FILE* fi = 0;
	fi = fopen(szFileName, "rb");
	if (fi == NULL) 
	{
		return false;
	}

	fread(g_XTrap_ClientMap[iMapIndex], XTRAP_CS4_BUFSIZE_MAP, 1, fi);
	fclose(fi);
#endif

	return true;
}

EVENTINFO(xtrap_cs1_check_info)
{
	DynamicCharacterPtr ptrPC;
}; 

EVENTFUNC(xtrap_cs1_check_event)
{
	xtrap_cs1_check_info* info = dynamic_cast<xtrap_cs1_check_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "<xtrap_event> info null pointer" );
		return 0;
	}

	TPacketXTrapCSVerify pack;
	pack.bHeader = HEADER_GC_XTRAP_CS1_REQUEST;

	bool bSuccess = CXTrapManager::instance().Verify_CSStep1( info->ptrPC, pack.bPacketData );

	LPDESC lpClientDesc = info->ptrPC.Get()->GetDesc();
	if( !lpClientDesc )
	{
		sys_err( "<xtrap_event> client session is invalid" );
		return 0;
	}

	lpClientDesc->Packet( &pack, sizeof(pack) );

	if( bSuccess )
	{
		return XTRAP_CS1_CHECK_CYCLE;
	}

	sys_err( "XTrap: hack is detected %s", lpClientDesc->GetHostName() );
	
	info->ptrPC.Get()->Disconnect("XTrapCheckInvalid");
	lpClientDesc->SetPhase(PHASE_CLOSE);

	return 0;
}

bool CXTrapManager::CreateClientSession( LPCHARACTER lpCharSession )
{
	if( !bXTrapEnabled )
		return true;

	if( !lpCharSession ) 
		return false;

	DWORD dwSessionID = lpCharSession->GetPlayerID();

	ClientSessionMap::iterator it = m_mapClientSessions.find( dwSessionID );
	if( it != m_mapClientSessions.end() )
	{
		sys_err("XTrap: client session is alreay registered");
		return false;
	}

	//init session info
	sSessionInfo			infoData;

	//xtrap session init
	DWORD dwReturn = m_pImpl->XTrap_S_SessionInit( 600, CSFILE_NUM, g_XTrap_ClientMap, infoData.szSessionBuf );
	if( dwReturn != 0 )
	{
		sys_err("XTrap: client session init failed");
	}

	xtrap_cs1_check_info*  event_info = AllocEventInfo<xtrap_cs1_check_info>();
	event_info->ptrPC = lpCharSession;

	infoData.m_pCheckEvent = event_create(xtrap_cs1_check_event, event_info, XTRAP_CS1_CHECK_CYCLE); 
	m_mapClientSessions[dwSessionID] = infoData;

	return true;
}

void CXTrapManager::DestroyClientSession( LPCHARACTER lpCharSession )
{
	if( !bXTrapEnabled )
		return;

	if( !lpCharSession ) 
		return;

	DWORD dwSessionID = lpCharSession->GetPlayerID();

	ClientSessionMap::iterator it = m_mapClientSessions.find( dwSessionID );
	if( it == m_mapClientSessions.end() )
	{
		sys_err("XTrap: client session is already destroyed");
		return;
	}

	event_cancel(&(it->second.m_pCheckEvent) );
	m_mapClientSessions.erase(it);
}


bool CXTrapManager::Verify_CSStep1( LPCHARACTER lpCharSession, BYTE* pBufData )
{
	if( !bXTrapEnabled )
		return false;

	if( !lpCharSession ) 
		return false;

	DWORD dwSessionID = lpCharSession->GetPlayerID();

	ClientSessionMap::iterator it = m_mapClientSessions.find( dwSessionID );
	if( it == m_mapClientSessions.end() )
	{
		sys_err("XTrap: client session is already destroyed");
		return false;
	}

	int nReturn = m_pImpl->XTrap_CS_Step1( it->second.szSessionBuf, it->second.szPackBuf );
	
	memcpy( pBufData, it->second.szPackBuf, VERIFY_PACK_LEN );

	return (nReturn == 0) ? true : false;
}

void CXTrapManager::Verify_CSStep3( LPCHARACTER lpCharSession, BYTE* pBufData )
{
	if( !bXTrapEnabled )
		return;

	if( !lpCharSession ) 
		return;

	DWORD dwSessionID = lpCharSession->GetPlayerID();

	ClientSessionMap::iterator it = m_mapClientSessions.find( dwSessionID );
	if( it == m_mapClientSessions.end() )
	{
		sys_log(0, "XTrap: client session is alreay destroyed");
		return;
	}

	memcpy( it->second.szPackBuf, pBufData, VERIFY_PACK_LEN );
	m_pImpl->XTrap_CS_Step3( it->second.szSessionBuf, it->second.szPackBuf );

	//if( XTRAP_API_RETURN_DETECTHACK == m_pImpl->XTrap_CS_Step3( it->second.szSessionBuf, pBufData ) )
	//{
	//	sys_error(0, "XTrap: client session is alreay destroyed");
	//}
}
