#ifndef __INC_METIN_II_GAME_PARTY_H__
#define __INC_METIN_II_GAME_PARTY_H__

#include "char.h"

enum // unit : minute
{
	PARTY_ENOUGH_MINUTE_FOR_EXP_BONUS = 60, // 파티 결성 후 60분 후 부터 추가 경험치 보너스
	PARTY_HEAL_COOLTIME_LONG = 60,
	PARTY_HEAL_COOLTIME_SHORT = 30,
	PARTY_MAX_MEMBER = 8,
	PARTY_DEFAULT_RANGE = 5000,
};

enum EPartyRole
{   
	PARTY_ROLE_NORMAL,  
	PARTY_ROLE_LEADER,
	PARTY_ROLE_ATTACKER,
	PARTY_ROLE_TANKER,
	PARTY_ROLE_BUFFER,  
	PARTY_ROLE_SKILL_MASTER,
	PARTY_ROLE_HASTE,   
	PARTY_ROLE_DEFENDER,
	PARTY_ROLE_MAX_NUM, 
};  

enum EPartyExpDistributionModes
{
	PARTY_EXP_DISTRIBUTION_NON_PARITY,
	PARTY_EXP_DISTRIBUTION_PARITY,
	PARTY_EXP_DISTRIBUTION_MAX_NUM
};

class CParty;
class CDungeon;

class CPartyManager : public singleton<CPartyManager>
{
	public:
		typedef std::map<DWORD, LPPARTY> TPartyMap;
		typedef std::set<LPPARTY> TPCPartySet;

	public:
		CPartyManager();
		virtual ~CPartyManager();

		void		Initialize();

		//void		SendPartyToDB();

		void		EnablePCParty() { m_bEnablePCParty = true; sys_log(0,"PARTY Enable"); }
		void		DisablePCParty() { m_bEnablePCParty = false; sys_log(0,"PARTY Disable"); }
		bool		IsEnablePCParty() { return m_bEnablePCParty; }

		LPPARTY		CreateParty(LPCHARACTER pkLeader);
		void		DeleteParty(LPPARTY pParty);
		void		DeleteAllParty();
		bool		SetParty(LPCHARACTER pkChr);

		void		SetPartyMember(DWORD dwPID, LPPARTY pParty);

		void		P2PLogin(DWORD pid, const char* name);
		void		P2PLogout(DWORD pid);

		LPPARTY		P2PCreateParty(DWORD pid);
		void		P2PDeleteParty(DWORD pid);
		void		P2PJoinParty(DWORD leader, DWORD pid, BYTE role = 0);
		void		P2PQuitParty(DWORD pid);

	private:
		TPartyMap	m_map_pkParty;		// PID로 어느 파티에 있나 검색하기 위한 컨테이너
		TPartyMap	m_map_pkMobParty;	// Mob 파티는 PID 대신 VID 로 따로 관리한다.

		TPCPartySet	m_set_pkPCParty;	// 사람들의 파티 전체 집합

		bool		m_bEnablePCParty;	// 디비가 켜져있지 않으면 사람들의 파티 상태가 변경불가
};

enum EPartyMessages
{
	PM_ATTACK,		// Attack him
	PM_RETURN,		// Return back to position
	PM_ATTACKED_BY,	// I was attacked by someone
	PM_AGGRO_INCREASE,	// My aggro is increased
};

class CParty
{
	public:
		typedef struct SMember
		{
			LPCHARACTER	pCharacter;
			bool	bNear;
			BYTE	bRole;
			int		bLevel;
			std::string strName;
		} TMember;

		typedef std::map<DWORD, TMember> TMemberMap;

		typedef std::map<std::string, int> TFlagMap;

	public:
		CParty();
		virtual ~CParty();

		void		P2PJoin(DWORD dwPID);
		void		P2PQuit(DWORD dwPID);
		virtual void	Join(DWORD dwPID);
		void		Quit(DWORD dwPID);
		void		Link(LPCHARACTER pkChr);
		void		Unlink(LPCHARACTER pkChr);

		void		ChatPacketToAllMember(BYTE type, const char* format, ...);	

		void		UpdateOnlineState(DWORD dwPID, const char* name);
		void		UpdateOfflineState(DWORD dwPID);

		DWORD		GetLeaderPID();
		LPCHARACTER	GetLeaderCharacter();
		LPCHARACTER	GetLeader() { return m_pkChrLeader; }

		DWORD		GetMemberCount();
		DWORD		GetNearMemberCount()	{ return m_iCountNearPartyMember; }

		bool		IsMember(DWORD pid) { return m_memberMap.find(pid) != m_memberMap.end(); }

		bool		IsNearLeader(DWORD pid);

		bool		IsPositionNearLeader(LPCHARACTER ch);

		void		SendMessage(LPCHARACTER ch, BYTE bMsg, DWORD dwArg1, DWORD dwArg2);

		void		SendPartyJoinOneToAll(DWORD dwPID);
		void		SendPartyJoinAllToOne(LPCHARACTER ch);
		void		SendPartyRemoveOneToAll(DWORD dwPID);

		void		SendPartyInfoOneToAll(DWORD pid);
		void		SendPartyInfoOneToAll(LPCHARACTER ch);
		void		SendPartyInfoAllToOne(LPCHARACTER ch);

		void		SendPartyLinkOneToAll(LPCHARACTER ch);
		void		SendPartyLinkAllToOne(LPCHARACTER ch);
		void		SendPartyUnlinkOneToAll(LPCHARACTER ch);

		int		GetPartyBonusExpPercent()	{ return m_iExpBonus; }
		int		GetPartyBonusAttackGrade()	{ return m_iAttBonus; }
		int		GetPartyBonusDefenseGrade()	{ return m_iDefBonus; }

		int	ComputePartyBonusExpPercent();
		inline int	ComputePartyBonusAttackGrade();
		inline int	ComputePartyBonusDefenseGrade();

		template <class Func> void ForEachMember(Func & f);
		template <class Func> void ForEachMemberPtr(Func & f);
		template <class Func> void ForEachOnlineMember(Func & f);
		template <class Func> void ForEachNearMember(Func & f);
		template <class Func> void ForEachOnMapMember (Func & f, long lMapIndex);
		template <class Func> bool ForEachOnMapMemberBool (Func & f, long lMapIndex);

		void		Update();

		int		GetExpBonusPercent();

		bool		SetRole(DWORD pid, BYTE bRole, bool on);
		BYTE		GetRole(DWORD pid);
		bool		IsRole(DWORD pid, BYTE bRole);

		int			GetMemberMaxLevel();
		int			GetMemberMinLevel();

		void		ComputeRolePoint(LPCHARACTER ch, BYTE bRole, bool bAdd);

		void		HealParty();
		void		SummonToLeader(DWORD pid);

		void		SetPCParty(bool b) { m_bPCParty = b; }

		LPCHARACTER	GetNextOwnership(LPCHARACTER ch, long x, long y);

		void		SetFlag(const std::string& name, int value);
		int		GetFlag(const std::string& name);

		void		SetDungeon(LPDUNGEON pDungeon);
		LPDUNGEON	GetDungeon();

		BYTE		CountMemberByVnum(DWORD dwVnum);

		void		SetParameter(int iMode);
		int		GetExpDistributionMode();

		void		SetExpCentralizeCharacter(DWORD pid);
		LPCHARACTER	GetExpCentralizeCharacter();

		void		RequestSetMemberLevel(DWORD pid, int level);
		void		P2PSetMemberLevel(DWORD pid, int level);

	protected:
		void		IncreaseOwnership();

		virtual void	Initialize();
		void		Destroy();
		void		RemovePartyBonus();

		void		RemoveBonus();
		void		RemoveBonusForOne(DWORD pid);

		void		SendParameter(LPCHARACTER ch);
		void		SendParameterToAll();

		TMemberMap	m_memberMap;
		DWORD		m_dwLeaderPID;
		LPCHARACTER	m_pkChrLeader;

		LPEVENT		m_eventUpdate;

		TMemberMap::iterator m_itNextOwner;

	private:
		int		m_iExpDistributionMode;
		LPCHARACTER	m_pkChrExpCentralize;

		DWORD		m_dwPartyStartTime;

		DWORD		m_dwPartyHealTime;
		bool		m_bPartyHealReady;
		bool		m_bCanUsePartyHeal;

		int		m_anRoleCount[PARTY_ROLE_MAX_NUM];
		int		m_anMaxRole[PARTY_ROLE_MAX_NUM];

		int		m_iLongTimeExpBonus;

		// used in Update
		int		m_iLeadership;
		int		m_iExpBonus;
		int		m_iAttBonus;
		int		m_iDefBonus;

		// changed only in Update
		int		m_iCountNearPartyMember;

		bool		m_bPCParty;

		TFlagMap	m_map_iFlag;

		LPDUNGEON	m_pkDungeon;
		// 아귀 동굴용 dungeon 멤버 변수.
		// 정말 이렇게까지 하고 싶진 않았는데, 던전에서 party 관리가 정말로 개판이라
		// 그거 고치기 전까지는 이렇게 임시로 해놓는다.
		LPDUNGEON	m_pkDungeon_for_Only_party;
	public:
		void SetDungeon_for_Only_party(LPDUNGEON pDungeon);
		LPDUNGEON GetDungeon_for_Only_party();
};

template <class Func> void CParty::ForEachMember(Func & f)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		f(it->first);
}

template <class Func> void CParty::ForEachMemberPtr(Func & f)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		f(it->second.pCharacter);
}

template <class Func> void CParty::ForEachOnlineMember(Func & f)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		if (it->second.pCharacter)
			f(it->second.pCharacter);
}

template <class Func> void CParty::ForEachNearMember(Func & f)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		if (it->second.pCharacter && it->second.bNear)
			f(it->second.pCharacter);
}

template <class Func> void CParty::ForEachOnMapMember (Func & f, long lMapIndex)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch = it->second.pCharacter;
		if (ch)
		{
			if (ch->GetMapIndex () == lMapIndex)
				f(ch);
		}
	}
}

template <class Func> bool CParty::ForEachOnMapMemberBool(Func & f, long lMapIndex)
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch = it->second.pCharacter;
		if (ch)
		{
			if (ch->GetMapIndex () == lMapIndex)
			{
				if(f(ch) == false)
				{
					return false;
			
				}
			}
		}
	}
	return true;
}

inline int CParty::ComputePartyBonusAttackGrade()
{
	/*
	   if (GetNearMemberCount() <= 1)
	   return 0;

	   int leadership = GetLeaderCharacter()->GetLeadershipSkillLevel();
	   int n = GetNearMemberCount();

	   if (n >= 3 && leadership >= 10)
	   return 2;

	   if (n >= 2 && leadership >= 4)
	   return 1;
	 */
	return 0;
}

inline int CParty::ComputePartyBonusDefenseGrade()
{
	/*
	   if (GetNearMemberCount() <= 1)
	   return 0;

	   int leadership = GetLeaderCharacter()->GetLeadershipSkillLevel();
	   int n = GetNearMemberCount();

	   if (n >= 5 && leadership >= 24)
	   return 2;

	   if (n >= 4 && leadership >= 16)
	   return 1;
	 */
	return 0;
}
#endif
