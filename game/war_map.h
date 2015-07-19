#ifndef __GUILD_WAR_MAP_MANAGER_H
#define __GUILD_WAR_MAP_MANAGER_H

#include "constants.h"
#include "guild.h"

enum EWarMapTypes
{
	WAR_MAP_TYPE_NORMAL,
	WAR_MAP_TYPE_FLAG,
};

typedef struct SWarMapInfo
{
	BYTE		bType;
	long		lMapIndex;
	PIXEL_POSITION	posStart[3];
} TWarMapInfo;

namespace warmap
{
	enum
	{
		WAR_FLAG_VNUM_START = 20035,
		WAR_FLAG_VNUM_END = 20037,

		WAR_FLAG_VNUM0 = 20035,
		WAR_FLAG_VNUM1 = 20036,
		WAR_FLAG_VNUM2 = 20037,

		WAR_FLAG_BASE_VNUM = 20038
	};

	inline bool IsWarFlag(DWORD dwVnum)
	{
		if (dwVnum >= WAR_FLAG_VNUM_START && dwVnum <= WAR_FLAG_VNUM_END)
			return true;

		return false;
	}

	inline bool IsWarFlagBase(DWORD dwVnum)
	{
		return dwVnum == WAR_FLAG_BASE_VNUM ? true : false;
	}
};

class CWarMap
{
	public:
		friend class CGuild;

		CWarMap(long lMapIndex, const TGuildWarInfo & r_info, TWarMapInfo * pkWarMapInfo, DWORD dwGuildID1, DWORD dwGuildID2);
		~CWarMap();

		bool	GetTeamIndex(DWORD dwGuild, BYTE & bIdx);

		void	IncMember(LPCHARACTER ch);
		void	DecMember(LPCHARACTER ch);

		CGuild * GetGuild(BYTE bIdx);
		DWORD	GetGuildID(BYTE bIdx);

		BYTE	GetType();
		long	GetMapIndex();
		DWORD	GetGuildOpponent(LPCHARACTER ch);

		DWORD	GetWinnerGuild();
		void	UsePotion(LPCHARACTER ch, LPITEM item);

		void	Draw();	// 강제 무승부 처리
		void	Timeout();
		void	CheckWarEnd();
		bool	SetEnded();
		void	ExitAll();

		void	SetBeginEvent(LPEVENT pkEv);
		void	SetTimeoutEvent(LPEVENT pkEv);
		void	SetEndEvent(LPEVENT pkEv);
		void	SetResetFlagEvent(LPEVENT pkEv);

		void	UpdateScore(DWORD g1, int score1, DWORD g2, int score2);
		bool	CheckScore();

		int	GetRewardGold(BYTE bWinnerIdx);

		bool	GetGuildIndex(DWORD dwGuild, int& iIndex);

		void	Packet(const void * pv, int size);
		void	Notice(const char * psz);
		void	SendWarPacket(LPDESC d);
		void	SendScorePacket(BYTE bIdx, LPDESC d = NULL);

		void	OnKill(LPCHARACTER killer, LPCHARACTER ch);

		void	AddFlag(BYTE bIdx, DWORD x=0, DWORD y=0);
		void	AddFlagBase(BYTE bIdx, DWORD x=0, DWORD y=0);
		void	RemoveFlag(BYTE bIdx);
		bool	IsFlagOnBase(BYTE bIdx);
		void	ResetFlag();

	private:
		void	UpdateUserCount();

	private:
		TWarMapInfo	m_kMapInfo;
		bool		m_bEnded;

		LPEVENT m_pkBeginEvent;
		LPEVENT m_pkTimeoutEvent;
		LPEVENT m_pkEndEvent;
		LPEVENT	m_pkResetFlagEvent;

		typedef struct STeamData
		{
			DWORD	dwID;
			CGuild * pkGuild;
			int		iMemberCount;
			int		iUsePotionPrice;
			int		iScore;
			LPCHARACTER pkChrFlag;
			LPCHARACTER pkChrFlagBase;

			std::set<DWORD> set_pidJoiner;

			void Initialize();

			int GetAccumulatedJoinerCount(); // 누적된 참가자 수
			int GetCurJointerCount(); // 현재 참가자 수

			void AppendMember(LPCHARACTER ch);
			void RemoveMember(LPCHARACTER ch);
		} TeamData;

		TeamData	m_TeamData[2];
		int		m_iObserverCount;
		DWORD		m_dwStartTime;
		BYTE		m_bTimeout;

		TGuildWarInfo	m_WarInfo;

		CHARACTER_SET m_set_pkChr;
};

class CWarMapManager : public singleton<CWarMapManager>
{
	public:
		CWarMapManager();
		virtual ~CWarMapManager();

		bool		LoadWarMapInfo(const char * c_pszFileName);
		bool		IsWarMap(long lMapIndex);
		TWarMapInfo *	GetWarMapInfo(long lMapIndex);
		bool		GetStartPosition(long lMapIndex, BYTE bIdx, PIXEL_POSITION & pos);

		template <typename Func> Func for_each(Func f);
		long		CreateWarMap(const TGuildWarInfo & r_WarInfo, DWORD dwGuildID1, DWORD dwGuildID2);
		void		DestroyWarMap(CWarMap* pMap);
		CWarMap *	Find(long lMapIndex);
		int		CountWarMap() { return m_mapWarMap.size(); }

		void		OnShutdown();

	private:
		std::map<long, TWarMapInfo *> m_map_kWarMapInfo;
		std::map<long, CWarMap *> m_mapWarMap;
};

template <typename Func> Func CWarMapManager::for_each(Func f)
{
	for (itertype(m_mapWarMap) it = m_mapWarMap.begin(); it != m_mapWarMap.end(); ++it)
		f(it->second);

	return f;
}

#endif
