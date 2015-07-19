#ifndef __INC_METIN_II_GAME_TARGET_H__
#define __INC_METIN_II_GAME_TARGET_H__

enum ETargetTypes
{
	TARGET_TYPE_POS	= (1 << 0),
	TARGET_TYPE_VID	= (1 << 1),
};

EVENTINFO(TargetInfo)
{
	// <Factor> Removed unsafe copy of CHARACTER* here
	//LPCHARACTER pkChr;

	int         iID;

	DWORD       dwPID;
	DWORD       dwQuestIndex;

	char        szTargetName[32+1]; // 퀘스트에서 사용하는 이름
	char        szTargetDesc[32+1]; // 실제 클라이언트에 전송되는 이름

	int         iType;
	int         iArg1;
	int         iArg2;

	int		iMapIndex;
	int         iOldX;
	int         iOldY;

	bool	bSendToClient;

	TargetInfo() 
	: iID( 0 )
	, dwPID( 0 )
	, dwQuestIndex( 0 )
	, iType( 0 )
	, iArg1( 0 )
	, iArg2( 0 )
	, iMapIndex( 0 )
	, iOldX( 0 )
	, iOldY( 0 )
	, bSendToClient( false )
	{
		::memset(szTargetName, 0, 32+1);
		::memset(szTargetDesc, 0, 32+1);
	}
};

class CTargetManager : public singleton<CTargetManager>
{
	public:
		CTargetManager();
		virtual ~CTargetManager();

		void CreateTarget(DWORD dwPID, DWORD dwQuestIndex, const char * c_pszTargetName, int iType, int iArg1, int iArg2, int iMapIndex, const char * c_pszTargetDesc = NULL, int iSendFlag = 1);
		void DeleteTarget(DWORD dwPID, DWORD dwQuestIndex, const char * c_pszTargetName);

		void Logout(DWORD dwPID);
		TargetInfo * GetTargetInfo(DWORD dwPID, int iType, int iArg1);

		LPEVENT GetTargetEvent(DWORD dwPID, DWORD dwQuestIndex, const char * c_pszTargetName);

	protected:
		// first: PID
		std::map<DWORD, std::list<LPEVENT> > m_map_kListEvent;
		int m_iID;
};

#endif
