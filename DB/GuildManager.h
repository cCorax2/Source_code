// vim:ts=8 sw=4
#ifndef __INC_GUILD_MANAGER_H
#define __INC_GUILD_MANAGER_H

#include "Peer.h"
#include <queue>
#include <utility>
#include "../../libsql/libsql.h"
#include "../../libpoly/Poly.h"

enum
{
    GUILD_WARP_WAR_CHANNEL = 99
};

class CGuildWarReserve;

struct TGuildDeclareInfo
{
	BYTE bType;
	DWORD dwGuildID[2];

	TGuildDeclareInfo(BYTE _bType, DWORD _dwGuildID1, DWORD _dwGuildID2)
		: bType(_bType)
	{
		dwGuildID[0] = _dwGuildID1;
		dwGuildID[1] = _dwGuildID2;
	}

	bool operator < (const TGuildDeclareInfo& r) const
	{
		return dwGuildID[0] < r.dwGuildID[0] || dwGuildID[0] == r.dwGuildID[0] && dwGuildID[1] < r.dwGuildID[1];
	}

	TGuildDeclareInfo& operator = (const TGuildDeclareInfo& r)
	{
		bType = r.bType;
		dwGuildID[0] = r.dwGuildID[0];
		dwGuildID[1] = r.dwGuildID[1];
		return *this;
	}
};

struct TGuildWaitStartInfo
{
	BYTE			bType;
	DWORD			GID[2];
	long			lWarPrice;
	long			lInitialScore;
	CGuildWarReserve *	pkReserve;

	TGuildWaitStartInfo(BYTE _bType,
			DWORD _g1,
			DWORD _g2,
			long _lWarPrice,
			long _lInitialScore,
			CGuildWarReserve * _pkReserve)
		: bType(_bType), lWarPrice(_lWarPrice), lInitialScore(_lInitialScore), pkReserve(_pkReserve)
	{
		GID[0] = _g1;
		GID[1] = _g2;
	}

	bool operator < (const TGuildWaitStartInfo& r) const
	{
		return GID[0] < r.GID[0] || GID[0] == r.GID[0] && GID[1] < r.GID[1];
	}
};

struct TGuildWarPQElement
{
	bool	bEnd;
	BYTE	bType;
	DWORD	GID[2];
	DWORD	iScore[2];
	DWORD	iBetScore[2];

	TGuildWarPQElement(BYTE _bType, DWORD GID1, DWORD GID2) : bEnd(false), bType(_bType)
	{
		bType = _bType;
		GID[0] = GID1;
		GID[1] = GID2;
		iScore[0] = iScore[1] = 0;
		iBetScore[0] = iBetScore[1] = 0;
	}
};

struct TGuildSkillUsed
{
    DWORD GID;
    DWORD dwSkillVnum;

    // GUILD_SKILL_COOLTIME_BUG_FIX
    TGuildSkillUsed(DWORD _GID, DWORD _dwSkillVnum) : GID(_GID), dwSkillVnum(_dwSkillVnum)
    {
    }
    // END_OF_GUILD_SKILL_COOLTIME_BUG_FIX
};

inline bool operator < (const TGuildSkillUsed& a, const TGuildSkillUsed& b)
{
    return a.GID < b.GID || a.GID == b.GID && a.dwSkillVnum < b.dwSkillVnum;
}

typedef struct SGuild
{
	SGuild() : ladder_point(0), win(0), draw(0), loss(0), gold(0), level(0)
	{
		memset(szName, 0, sizeof(szName));
	}

	char szName[GUILD_NAME_MAX_LEN+1];
	int	ladder_point;
	int	win;
	int	draw;
	int	loss;
	int	gold;
	int	level;
} TGuild;

typedef struct SGuildWarInfo
{
    time_t		tEndTime;
    TGuildWarPQElement * pElement;
    CGuildWarReserve * pkReserve;

    SGuildWarInfo() : pElement(NULL)
    {
    }
} TGuildWarInfo;

class CGuildWarReserve
{
    public:
	CGuildWarReserve(const TGuildWarReserve& rTable);

	void Initialize();

	TGuildWarReserve & GetDataRef()
	{
	    return m_data;
	}

	void	OnSetup(CPeer * peer);
	bool	Bet(const char * pszLogin, DWORD dwGold, DWORD dwGuild);
	void	Draw();
	void	End(int iScoreFrom, int iScoreTo);

	int	GetLastNoticeMin() { return m_iLastNoticeMin; }
	void	SetLastNoticeMin(int iMin) { m_iLastNoticeMin = iMin; }

    private:
	CGuildWarReserve();  // 기본 생성자를 사용하지 못하도록 의도적으로 구현하지 않음

	TGuildWarReserve				m_data;
	// <login, <guild, gold>>
	std::map<std::string, std::pair<DWORD, DWORD> > mapBet;
	int						m_iLastNoticeMin;
};

class CGuildManager : public singleton<CGuildManager>
{
    public:
	CGuildManager();
	virtual ~CGuildManager();

	void	Initialize();

	void	Load(DWORD dwGuildID);

	TGuild & TouchGuild(DWORD GID);

	void	Update();

	void	OnSetup(CPeer * peer);
	void	StartWar(BYTE bType, DWORD GID1, DWORD GID2, CGuildWarReserve * pkReserve = NULL);

	void	UpdateScore(DWORD guild_gain_point, DWORD guild_opponent, int iScore, int iBetScore);

	void	AddDeclare(BYTE bType, DWORD guild_from, DWORD guild_to);
	void	RemoveDeclare(DWORD guild_from, DWORD guild_to);

	bool	TakeBetPrice(DWORD dwGuildTo, DWORD dwGuildFrom, long lWarPrice);

	bool	WaitStart(TPacketGuildWar * p);

	void	RecvWarEnd(DWORD GID1, DWORD GID2);
	void	RecvWarOver(DWORD dwGuildWinner, DWORD dwGuildLoser, bool bDraw, long lWarPrice);

	void	ChangeLadderPoint(DWORD GID, int change);

	void	UseSkill(DWORD dwGuild, DWORD dwSkillVnum, DWORD dwCooltime);

	INT		GetGuildGold(DWORD dwGuild);
	void	DepositMoney(DWORD dwGuild, INT lGold);
	void	WithdrawMoney(CPeer* peer, DWORD dwGuild, INT lGold);
	void	WithdrawMoneyReply(DWORD dwGuild, BYTE bGiveSuccess, INT lGold);

	void	MoneyChange(DWORD dwGuild, DWORD dwGold);

	void	QueryRanking();
	void	ResultRanking(MYSQL_RES * pRes);
	int	GetRanking(DWORD dwGID);

	//
	// Reserve War
	//
	void	BootReserveWar();
	bool	ReserveWar(TPacketGuildWar * p);
	void	ProcessReserveWar();
	bool	Bet(DWORD dwID, const char * c_pszLogin, DWORD dwGold, DWORD dwGuild);

	void	CancelWar(DWORD GID1, DWORD GID2);

	bool	ChangeMaster(DWORD dwGID, DWORD dwFrom, DWORD dwTo);

    private:
	void ParseResult(SQLResult * pRes);

	void RemoveWar(DWORD GID1, DWORD GID2);	// erase war from m_WarMap and set end on priority queue

	void WarEnd(DWORD GID1, DWORD GID2, bool bDraw = false);

	int GetLadderPoint(DWORD GID);

	void GuildWarWin(DWORD GID);
	void GuildWarDraw(DWORD GID);
	void GuildWarLose(DWORD GID);

	void ProcessDraw(DWORD dwGuildID1, DWORD dwGuildID2);
	void ProcessWinLose(DWORD dwGuildWinner, DWORD dwGuildLoser);

	bool IsHalfWinLadderPoint(DWORD dwGuildWinner, DWORD dwGuildLoser);

	std::map<DWORD, TGuild>					m_map_kGuild;
	std::map<DWORD, std::map<DWORD, time_t> >		m_mapGuildWarEndTime;

	std::set<TGuildDeclareInfo>				m_DeclareMap; // 선전 포고 상태를 저장
	std::map<DWORD, std::map<DWORD, TGuildWarInfo> >	m_WarMap;

	typedef std::pair<time_t, TGuildWarPQElement *>	stPairGuildWar;
	typedef std::pair<time_t, TGuildSkillUsed>	stPairSkillUsed;
	typedef std::pair<time_t, TGuildWaitStartInfo>	stPairWaitStart;

	std::priority_queue<stPairGuildWar, std::vector<stPairGuildWar>, std::greater<stPairGuildWar> >
	    m_pqOnWar;
	std::priority_queue<stPairWaitStart, std::vector<stPairWaitStart>, std::greater<stPairWaitStart> >
	    m_pqWaitStart;
	std::priority_queue<stPairSkillUsed, std::vector<stPairSkillUsed>, std::greater<stPairSkillUsed> >
	    m_pqSkill;

	std::map<DWORD, CGuildWarReserve *>			m_map_kWarReserve;
	CPoly							polyPower;
	CPoly							polyHandicap;

	// GID Ranking
	std::map<DWORD, int>					map_kLadderPointRankingByGID;
};

#endif
