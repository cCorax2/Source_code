
#ifndef _XTRAP_MANAGER_H_
#define _XTRAP_MANAGER_H_

#include "IFileMonitor.h"

#define SESSION_BUF_LEN		320
#define VERIFY_PACK_LEN		128
#define SESSION_CSSTEP1_LEN	256

#pragma pack(1)

typedef struct PacketXTrapVerify
{
	BYTE	bHeader;
	BYTE	bPacketData[VERIFY_PACK_LEN];

} TPacketXTrapCSVerify;

#pragma pack()

class CXTrapManager : public singleton<CXTrapManager>
{
public:
	CXTrapManager();
	virtual ~CXTrapManager();

	bool LoadXTrapModule();
	bool LoadClientMapFile( unsigned int iMapIndex );

	bool CreateClientSession( LPCHARACTER lpCharSession );
	void DestroyClientSession( LPCHARACTER lpCharSession );

	bool Verify_CSStep1( LPCHARACTER lpCharSession, BYTE* pOutBufData );
	void Verify_CSStep3( LPCHARACTER lpCharSession, BYTE* pBufData );
#ifdef __FreeBSD__
	static void MapReloadSignalHandler( int signal );

	static void NotifyMapFileChanged( const std::string& fileName, eFileUpdatedOptions eUpdateOption );
#endif

private:
	//pimpl`s idiom
	struct			sXTrapContext;
	sXTrapContext*	m_pImpl;
	
	struct sSessionInfo
	{
		sSessionInfo()
		{
			m_pCheckEvent = NULL;
			memset(szSessionBuf, 0x00, sizeof(szSessionBuf) );
			memset(szPackBuf, 0x00, sizeof(szPackBuf) );
		}

		BYTE	szSessionBuf[SESSION_BUF_LEN];
		BYTE	szPackBuf[VERIFY_PACK_LEN];
		LPEVENT m_pCheckEvent;
	};

	typedef boost::unordered_map<DWORD, sSessionInfo> ClientSessionMap;

	ClientSessionMap	m_mapClientSessions;

};

#endif /* _XTRAP_MANAGER_H_ */
