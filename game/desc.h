#ifndef __INC_METIN_II_GAME_DESC_H__
#define __INC_METIN_II_GAME_DESC_H__

#include "constants.h"
#include "input.h"
#ifdef _IMPROVED_PACKET_ENCRYPTION_
#include "cipher.h"
#endif

#define MAX_ALLOW_USER                  4096
//#define MAX_INPUT_LEN			2048
#define MAX_INPUT_LEN			65536

#define HANDSHAKE_RETRY_LIMIT		32

class CInputProcessor;

enum EDescType
{
	DESC_TYPE_ACCEPTOR,
	DESC_TYPE_CONNECTOR
};

class CLoginKey
{
	public:
		CLoginKey(DWORD dwKey, LPDESC pkDesc) : m_dwKey(dwKey), m_pkDesc(pkDesc)
		{
			m_dwExpireTime = 0;
		}

		void Expire()
		{
			m_dwExpireTime = get_dword_time();
			m_pkDesc = NULL;
		}

		operator DWORD() const
		{
			return m_dwKey;
		}

		DWORD   m_dwKey;
		DWORD   m_dwExpireTime;
		LPDESC  m_pkDesc;
};


// sequence 버그 찾기용 데이타
struct seq_t
{
	BYTE	hdr;
	BYTE	seq;
};
typedef std::vector<seq_t>	seq_vector_t;
// sequence 버그 찾기용 데이타

class DESC
{
	public:
		EVENTINFO(desc_event_info)
		{
			LPDESC desc;

			desc_event_info() 
			: desc(0)
			{
			}
		};

	public:
		DESC();
		virtual ~DESC();

		virtual BYTE		GetType() { return DESC_TYPE_ACCEPTOR; }
		virtual void		Destroy();
		virtual void		SetPhase(int _phase);

		void			FlushOutput();

		bool			Setup(LPFDWATCH _fdw, socket_t _fd, const struct sockaddr_in & c_rSockAddr, DWORD _handle, DWORD _handshake);

		socket_t		GetSocket() const	{ return m_sock; }
		const char *	GetHostName()		{ return m_stHost.c_str(); }
		WORD			GetPort()	{ return m_wPort; }

		void			SetP2P(const char * h, WORD w, BYTE b) { m_stP2PHost = h; m_wP2PPort = w; m_bP2PChannel = b; }
		const char *	GetP2PHost()		{ return m_stP2PHost.c_str();	}
		WORD			GetP2PPort() const		{ return m_wP2PPort; }
		BYTE			GetP2PChannel() const	{ return m_bP2PChannel;	}

		void			BufferedPacket(const void * c_pvData, int iSize);
		void			Packet(const void * c_pvData, int iSize);
		void			LargePacket(const void * c_pvData, int iSize);

		int			ProcessInput();		// returns -1 if error
		int			ProcessOutput();	// returns -1 if error

		CInputProcessor	*	GetInputProcessor()	{ return m_pInputProcessor; }

		DWORD			GetHandle() const	{ return m_dwHandle; }
		LPBUFFER		GetOutputBuffer()	{ return m_lpOutputBuffer; }

		void			BindAccountTable(TAccountTable * pTable);
		TAccountTable &		GetAccountTable()	{ return m_accountTable; }

		void			BindCharacter(LPCHARACTER ch);
		LPCHARACTER		GetCharacter()		{ return m_lpCharacter; }

		bool			IsPhase(int phase) const	{ return m_iPhase == phase ? true : false; }

		const struct sockaddr_in & GetAddr()		{ return m_SockAddr;	}

		void			   UDPGrant(const struct sockaddr_in & c_rSockAddr);
		const struct sockaddr_in & GetUDPAddr()		{ return m_UDPSockAddr; }

		void			Log(const char * format, ...);

		// 핸드쉐이크 (시간 동기화)
		void			StartHandshake(DWORD _dw);
		void			SendHandshake(DWORD dwCurTime, long lNewDelta);
		bool			HandshakeProcess(DWORD dwTime, long lDelta, bool bInfiniteRetry=false);
		bool			IsHandshaking();

		DWORD			GetHandshake() const	{ return m_dwHandshake; }
		DWORD			GetClientTime();

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		void SendKeyAgreement();
		void SendKeyAgreementCompleted();
		bool FinishHandshake(size_t agreed_length, const void* buffer, size_t length);
		bool IsCipherPrepared();
#else
		// Obsolete encryption stuff here
		void			SetSecurityKey(const DWORD * c_pdwKey);
		const DWORD *	GetEncryptionKey() const { return &m_adwEncryptionKey[0]; }
		const DWORD *	GetDecryptionKey() const { return &m_adwDecryptionKey[0]; }
#endif

		// 제국
		BYTE			GetEmpire();

		// for p2p
		void			SetRelay(const char * c_pszName);
		bool			DelayedDisconnect(int iSec);
		void			DisconnectOfSameLogin();

		void			SetAdminMode();
		bool			IsAdminMode();		// Handshake 에서 어드민 명령을 쓸수있나?

		void			SetPong(bool b);
		bool			IsPong();

		BYTE			GetSequence();
		void			SetNextSequence();

		void			SendLoginSuccessPacket();
		//void			SendServerStatePacket(int nIndex);

		void			SetMatrixCode(const char * c_psz) { m_stMatrixCode = c_psz; }
		const char *		GetMatrixCode() { return m_stMatrixCode.c_str(); }

		void			SetMatrixCardRowsAndColumns(unsigned long rows, unsigned long cols);
		unsigned long		GetMatrixRows();
		unsigned long		GetMatrixCols();
		bool			CheckMatrixTryCount();

		void			SetPanamaKey(DWORD dwKey)	{m_dwPanamaKey = dwKey;}
		DWORD			GetPanamaKey() const		{ return m_dwPanamaKey; }

		void			SetLoginKey(DWORD dwKey);
		void			SetLoginKey(CLoginKey * pkKey);
		DWORD			GetLoginKey();

		void			AssembleCRCMagicCube(BYTE bProcPiece, BYTE bFilePiece);

		void			SetBillingExpireSecond(DWORD dwSec);
		DWORD			GetBillingExpireSecond();

		void			SetClientVersion(const char * c_pszTimestamp) { m_stClientVersion = c_pszTimestamp; }
		const char *		GetClientVersion() { return m_stClientVersion.c_str(); }

		bool			isChannelStatusRequested() const { return m_bChannelStatusRequested; }
		void			SetChannelStatusRequested(bool bChannelStatusRequested) { m_bChannelStatusRequested = bChannelStatusRequested; }

	protected:
		void			Initialize();

	protected:
		CInputProcessor *	m_pInputProcessor;
		CInputClose		m_inputClose;
		CInputHandshake	m_inputHandshake;
		CInputLogin		m_inputLogin;
		CInputMain		m_inputMain;
		CInputDead		m_inputDead;
		CInputAuth		m_inputAuth;


		LPFDWATCH		m_lpFdw;
		socket_t		m_sock;
		int				m_iPhase;
		DWORD			m_dwHandle;

		std::string		m_stHost;
		WORD			m_wPort;
		time_t			m_LastTryToConnectTime;

		LPBUFFER		m_lpInputBuffer;
		int				m_iMinInputBufferLen;
	
		DWORD			m_dwHandshake;
		DWORD			m_dwHandshakeSentTime;
		int				m_iHandshakeRetry;
		DWORD			m_dwClientTime;
		bool			m_bHandshaking;

		LPBUFFER		m_lpBufferedOutputBuffer;
		LPBUFFER		m_lpOutputBuffer;

		LPEVENT			m_pkPingEvent;
		LPCHARACTER		m_lpCharacter;
		TAccountTable		m_accountTable;

		struct sockaddr_in	m_SockAddr;
		struct sockaddr_in 	m_UDPSockAddr;

		FILE *			m_pLogFile;
		std::string		m_stRelayName;

		std::string		m_stP2PHost;
		WORD			m_wP2PPort;
		BYTE			m_bP2PChannel;

		bool			m_bAdminMode; // Handshake 에서 어드민 명령을 쓸수있나?
		bool			m_bPong;

		int			m_iCurrentSequence;

		DWORD			m_dwMatrixRows;
		DWORD			m_dwMatrixCols;
		BYTE			m_bMatrixTryCount;

		CLoginKey *		m_pkLoginKey;
		DWORD			m_dwLoginKey;
		DWORD			m_dwPanamaKey;

		BYTE                    m_bCRCMagicCubeIdx;
		DWORD                   m_dwProcCRC;
		DWORD                   m_dwFileCRC;
		bool			m_bHackCRCQuery;

		DWORD			m_dwBillingExpireSecond;
		std::string		m_stClientVersion;
		std::string		m_stMatrixCode;

		std::string		m_Login;
		int				m_outtime;
		int				m_playtime;
		int				m_offtime;

		bool			m_bDestroyed;
		bool			m_bChannelStatusRequested;

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		Cipher cipher_;
#else
		// Obsolete encryption stuff here
		bool			m_bEncrypted;
		DWORD			m_adwDecryptionKey[4];
		DWORD			m_adwEncryptionKey[4];
#endif

	public:
		LPEVENT			m_pkDisconnectEvent;

	protected:
		std::string m_stMatrixCardID;
		std::string m_stMatrixQuiz;

	public:
		void SetMatrixCardID( const char * szCardID ) { m_stMatrixCardID = szCardID;} 
		const char * GetMatrixCardID() { return m_stMatrixCardID.c_str();}
		void SetMatrixQuiz( const char * szCode ) { m_stMatrixCode = szCode; }
		const char * GetMatrixQuiz() { return m_stMatrixCode.c_str(); }

		void SetLogin( const std::string & login ) { m_Login = login; }
		void SetLogin( const char * login ) { m_Login = login; }
		const std::string& GetLogin() { return m_Login; }

		void SetOutTime( int outtime ) { m_outtime = outtime; }
		void SetOffTime( int offtime ) { m_offtime = offtime; }
		void SetPlayTime( int playtime ) { m_playtime = playtime; }

		void RawPacket(const void * c_pvData, int iSize);
		void ChatPacket(BYTE type, const char * format, ...);

		/* 시퀀스 버그 찾기용 코드 */
	public:
		seq_vector_t	m_seq_vector;
		void			push_seq (BYTE hdr, BYTE seq);
		
};

#endif
