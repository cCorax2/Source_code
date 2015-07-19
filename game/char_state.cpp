#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "battle.h"
#include "char_manager.h"
#include "packet.h"
#include "motion.h"
#include "party.h"
#include "affect.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "p2p.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "exchange.h"
#include "sectree_manager.h"
#include "xmas_event.h"
#include "guild_manager.h"
#include "war_map.h"
#include "locale_service.h"
#include "BlueDragon.h"

#include "../../common/VnumHelper.h"

BOOL g_test_server;
extern LPCHARACTER FindVictim(LPCHARACTER pkChr, int iMaxDistance);

namespace
{
	class FuncFindChrForFlag
	{
		public:
			FuncFindChrForFlag(LPCHARACTER pkChr) :
				m_pkChr(pkChr), m_pkChrFind(NULL), m_iMinDistance(INT_MAX)
				{
				}

			void operator () (LPENTITY ent)
			{
				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				if (ent->IsObserverMode())
					return;

				LPCHARACTER pkChr = (LPCHARACTER) ent;

				if (!pkChr->IsPC())
					return;

				if (!pkChr->GetGuild())
					return;

				if (pkChr->IsDead())
					return;

				int iDist = DISTANCE_APPROX(pkChr->GetX()-m_pkChr->GetX(), pkChr->GetY()-m_pkChr->GetY());

				if (iDist <= 500 && m_iMinDistance > iDist &&
						!pkChr->IsAffectFlag(AFF_WAR_FLAG1) &&
						!pkChr->IsAffectFlag(AFF_WAR_FLAG2) &&
						!pkChr->IsAffectFlag(AFF_WAR_FLAG3))
				{
					// 우리편 깃발일 경우
					if ((DWORD) m_pkChr->GetPoint(POINT_STAT) == pkChr->GetGuild()->GetID())
					{
						CWarMap * pMap = pkChr->GetWarMap();
						BYTE idx;

						if (!pMap || !pMap->GetTeamIndex(pkChr->GetGuild()->GetID(), idx))
							return;

						// 우리편 기지에 깃발이 없을 때만 깃발을 뽑는다. 안그러면 기지에 있는 깃발을
						// 가만히 두고 싶은데도 뽑힐수가 있으므로..
						if (!pMap->IsFlagOnBase(idx))
						{
							m_pkChrFind = pkChr;
							m_iMinDistance = iDist;
						}
					}
					else
					{
						// 상대편 깃발인 경우 무조건 뽑는다.
						m_pkChrFind = pkChr;
						m_iMinDistance = iDist;
					}
				}
			}

			LPCHARACTER	m_pkChr;
			LPCHARACTER m_pkChrFind;
			int		m_iMinDistance;
	};

	class FuncFindChrForFlagBase
	{
		public:
			FuncFindChrForFlagBase(LPCHARACTER pkChr) : m_pkChr(pkChr)
			{
			}

			void operator () (LPENTITY ent)
			{
				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				if (ent->IsObserverMode())
					return;

				LPCHARACTER pkChr = (LPCHARACTER) ent;

				if (!pkChr->IsPC())
					return;

				CGuild * pkGuild = pkChr->GetGuild();

				if (!pkGuild)
					return;

				int iDist = DISTANCE_APPROX(pkChr->GetX()-m_pkChr->GetX(), pkChr->GetY()-m_pkChr->GetY());

				if (iDist <= 500 &&
						(pkChr->IsAffectFlag(AFF_WAR_FLAG1) || 
						 pkChr->IsAffectFlag(AFF_WAR_FLAG2) ||
						 pkChr->IsAffectFlag(AFF_WAR_FLAG3)))
				{
					CAffect * pkAff = pkChr->FindAffect(AFFECT_WAR_FLAG);

					sys_log(0, "FlagBase %s dist %d aff %p flag gid %d chr gid %u",
							pkChr->GetName(), iDist, pkAff, m_pkChr->GetPoint(POINT_STAT),
							pkChr->GetGuild()->GetID());

					if (pkAff)
					{
						if ((DWORD) m_pkChr->GetPoint(POINT_STAT) == pkGuild->GetID() &&
								m_pkChr->GetPoint(POINT_STAT) != pkAff->lApplyValue)
						{
							CWarMap * pMap = pkChr->GetWarMap();
							BYTE idx;

							if (!pMap || !pMap->GetTeamIndex(pkGuild->GetID(), idx))
								return;

							//if (pMap->IsFlagOnBase(idx))
							{
								BYTE idx_opp = idx == 0 ? 1 : 0;

								SendGuildWarScore(m_pkChr->GetPoint(POINT_STAT), pkAff->lApplyValue, 1);
								//SendGuildWarScore(pkAff->lApplyValue, m_pkChr->GetPoint(POINT_STAT), -1);

								pMap->ResetFlag();
								//pMap->AddFlag(idx_opp);
								//pkChr->RemoveAffect(AFFECT_WAR_FLAG);

								char buf[256];
								snprintf(buf, sizeof(buf), LC_TEXT("%s 길드가 %s 길드의 깃발을 빼앗았습니다!"), pMap->GetGuild(idx)->GetName(), pMap->GetGuild(idx_opp)->GetName());
								pMap->Notice(buf);
							}
						}
					}
				}
			}

			LPCHARACTER m_pkChr;
	};

	class FuncFindGuardVictim
	{
		public:
			FuncFindGuardVictim(LPCHARACTER pkChr, int iMaxDistance) :
				m_pkChr(pkChr),
			m_iMinDistance(INT_MAX),
			m_iMaxDistance(iMaxDistance),
			m_lx(pkChr->GetX()),
			m_ly(pkChr->GetY()),
			m_pkChrVictim(NULL)
			{
			};

			void operator () (LPENTITY ent)
			{
				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				LPCHARACTER pkChr = (LPCHARACTER) ent;

				// 일단 PC 공격안함
				if (pkChr->IsPC())
					return;


				if (pkChr->IsNPC() && !pkChr->IsMonster())
					return;

				if (pkChr->IsDead())
					return;

				if (pkChr->IsAffectFlag(AFF_EUNHYUNG) || 
						pkChr->IsAffectFlag(AFF_INVISIBILITY) ||
						pkChr->IsAffectFlag(AFF_REVIVE_INVISIBLE))
					return;

				// 왜구는 패스
				if (pkChr->GetRaceNum() == 5001)
					return;

				int iDistance = DISTANCE_APPROX(m_lx - pkChr->GetX(), m_ly - pkChr->GetY());

				if (iDistance < m_iMinDistance && iDistance <= m_iMaxDistance)
				{
					m_pkChrVictim = pkChr;
					m_iMinDistance = iDistance;
				}
			}

			LPCHARACTER GetVictim()
			{
				return (m_pkChrVictim);
			}

		private:
			LPCHARACTER	m_pkChr;

			int		m_iMinDistance;
			int		m_iMaxDistance;
			long	m_lx;
			long	m_ly;

			LPCHARACTER	m_pkChrVictim;
	};

}

bool CHARACTER::IsAggressive() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_AGGRESSIVE);
}

void CHARACTER::SetAggressive()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_AGGRESSIVE);
}

bool CHARACTER::IsCoward() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_COWARD);
}

void CHARACTER::SetCoward()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_COWARD);
}

bool CHARACTER::IsBerserker() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_BERSERK);
}

bool CHARACTER::IsStoneSkinner() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_STONESKIN);
}

bool CHARACTER::IsGodSpeeder() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_GODSPEED);
}

bool CHARACTER::IsDeathBlower() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_DEATHBLOW);
}

bool CHARACTER::IsReviver() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_REVIVE);
}

void CHARACTER::CowardEscape()
{
	int iDist[4] = {500, 1000, 3000, 5000};

	for (int iDistIdx = 2; iDistIdx >= 0; --iDistIdx)
		for (int iTryCount = 0; iTryCount < 8; ++iTryCount)
		{
			SetRotation(number(0, 359));        // 방향은 랜덤으로 설정

			float fx, fy;
			float fDist = number(iDist[iDistIdx], iDist[iDistIdx+1]);

			GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

			bool bIsWayBlocked = false;
			for (int j = 1; j <= 100; ++j)
			{
				if (!SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx*j/100, GetY() + (int) fy*j/100))
				{
					bIsWayBlocked = true;
					break;
				}
			}

			if (bIsWayBlocked)
				continue;

			m_dwStateDuration = PASSES_PER_SEC(1);

			int iDestX = GetX() + (int) fx;
			int iDestY = GetY() + (int) fy;

			if (Goto(iDestX, iDestY))
				SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

			sys_log(0, "WAEGU move to %d %d (far)", iDestX, iDestY);
			return;
		}
}

void  CHARACTER::SetNoAttackShinsu()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKSHINSU);
}
bool CHARACTER::IsNoAttackShinsu() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKSHINSU);
}

void CHARACTER::SetNoAttackChunjo()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKCHUNJO);
}

bool CHARACTER::IsNoAttackChunjo() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKCHUNJO);
}

void CHARACTER::SetNoAttackJinno()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKJINNO);
}

bool CHARACTER::IsNoAttackJinno() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKJINNO);
}

void CHARACTER::SetAttackMob()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_ATTACKMOB);
}

bool CHARACTER::IsAttackMob() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_ATTACKMOB);
}

// STATE_IDLE_REFACTORING
void CHARACTER::StateIdle()
{
	if (IsStone())
	{
		__StateIdle_Stone();
		return;
	}
	else if (IsWarp() || IsGoto())
	{
		// 워프는 이벤트로 처리
		m_dwStateDuration = 60 * passes_per_sec;
		return;
	}

	if (IsPC())
		return;

	// NPC 처리
	if (!IsMonster())
	{
		__StateIdle_NPC();
		return;
	}

	__StateIdle_Monster();
}

void CHARACTER::__StateIdle_Stone()
{
	m_dwStateDuration = PASSES_PER_SEC(1);

	int iPercent = (GetHP() * 100) / GetMaxHP();
	DWORD dwVnum = number(MIN(GetMobTable().sAttackSpeed, GetMobTable().sMovingSpeed ), MAX(GetMobTable().sAttackSpeed, GetMobTable().sMovingSpeed));

	if (iPercent <= 10 && GetMaxSP() < 10)
	{
		SetMaxSP(10);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1500, GetY() - 1500, GetX() + 1500, GetY() + 1500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 20 && GetMaxSP() < 9)
	{
		SetMaxSP(9);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1500, GetY() - 1500, GetX() + 1500, GetY() + 1500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 30 && GetMaxSP() < 8)
	{
		SetMaxSP(8);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 40 && GetMaxSP() < 7)
	{
		SetMaxSP(7);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 50 && GetMaxSP() < 6)
	{
		SetMaxSP(6);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 60 && GetMaxSP() < 5)
	{
		SetMaxSP(5);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 70 && GetMaxSP() < 4)
	{
		SetMaxSP(4);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 80 && GetMaxSP() < 3)
	{
		SetMaxSP(3);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 90 && GetMaxSP() < 2)
	{
		SetMaxSP(2);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 99 && GetMaxSP() < 1)
	{
		SetMaxSP(1);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else
		return;

	UpdatePacket();
	return;
}

void CHARACTER::__StateIdle_NPC()
{
	MonsterChat(MONSTER_CHAT_WAIT);
	m_dwStateDuration = PASSES_PER_SEC(5);

	// 펫 시스템의 Idle 처리는 기존 거의 모든 종류의 캐릭터들이 공유해서 사용하는 상태머신이 아닌 CPetActor::Update에서 처리함.
	if (IsPet())
		return;
	else if (IsGuardNPC())
	{
		if (!quest::CQuestManager::instance().GetEventFlag("noguard"))
		{
			FuncFindGuardVictim f(this, 50000);

			if (GetSectree())
				GetSectree()->ForEachAround(f);

			LPCHARACTER victim = f.GetVictim();

			if (victim)
			{
				m_dwStateDuration = passes_per_sec/2;

				if (CanBeginFight())
					BeginFight(victim);
			}
		}
	}
	else
	{
		if (GetRaceNum() == xmas::MOB_SANTA_VNUM) // 산타
		{
			if (get_dword_time() > m_dwPlayStartTime)
			{
				int	next_warp_time = 2 * 1000;	// 2초

				m_dwPlayStartTime = get_dword_time() + next_warp_time;

				// 시간이 넘었으니 워프합시다.
				/*
				 * 산타용
				const int WARP_MAP_INDEX_NUM = 4;
				static const long c_lWarpMapIndexs[WARP_MAP_INDEX_NUM] = {61, 62, 63, 64};
				*/
				// 신선자 노해용
				const int WARP_MAP_INDEX_NUM = 7;
				static const long c_lWarpMapIndexs[WARP_MAP_INDEX_NUM] = { 61, 62, 63, 64, 3, 23, 43 };
				long lNextMapIndex;
				lNextMapIndex = c_lWarpMapIndexs[number(1, WARP_MAP_INDEX_NUM) - 1];

				if (map_allow_find(lNextMapIndex))
				{
					// 이곳입니다.
					M2_DESTROY_CHARACTER(this);
					int iNextSpawnDelay = 0;
					if (LC_IsYMIR())
						iNextSpawnDelay = 20 * 60;
					else
						iNextSpawnDelay = 50 * 60;

					xmas::SpawnSanta(lNextMapIndex, iNextSpawnDelay);
				}
				else
				{
					// 다른 서버 입니다.
					TPacketGGXmasWarpSanta p;
					p.bHeader   = HEADER_GG_XMAS_WARP_SANTA;
					p.bChannel  = g_bChannel;
					p.lMapIndex = lNextMapIndex;
					P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGXmasWarpSanta));
				}
				return;
			}
		}

		if (!IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
		{
			//
			// 이 곳 저 곳 이동한다.
			// 
			LPCHARACTER pkChrProtege = GetProtege();

			if (pkChrProtege)
			{
				if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 500)
				{
					if (Follow(pkChrProtege, number(100, 300)))
						return;
				}
			}

			if (!number(0, 6))
			{
				SetRotation(number(0, 359));        // 방향은 랜덤으로 설정

				float fx, fy;
				float fDist = number(200, 400);

				GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

				// 느슨한 못감 속성 체크; 최종 위치와 중간 위치가 갈수없다면 가지 않는다.
				if (!(SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx, GetY() + (int) fy) 
					&& SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx / 2, GetY() + (int) fy / 2)))
					return;

				SetNowWalking(true);

				if (Goto(GetX() + (int) fx, GetY() + (int) fy))
					SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

				return;
			}
		}
	}
}

void CHARACTER::__StateIdle_Monster()
{
	if (IsStun())
		return;

	if (!CanMove())
		return;

	if (IsCoward())
	{
		// 겁쟁이 몬스터는 도망만 다닙니다.
		if (!IsDead())
			CowardEscape();

		return;
	}

	if (IsBerserker())
		if (IsBerserk())
			SetBerserk(false);

	if (IsGodSpeeder())
		if (IsGodSpeed())
			SetGodSpeed(false);

	LPCHARACTER victim = GetVictim();

	if (!victim || victim->IsDead())
	{
		SetVictim(NULL);
		victim = NULL;
		m_dwStateDuration = PASSES_PER_SEC(1);
	}

	if (!victim || victim->IsBuilding())
	{
		// 돌 보호 처리
		if (m_pkChrStone)
		{
			victim = m_pkChrStone->GetNearestVictim(m_pkChrStone);
		}
		// 선공 몬스터 처리
		else if (!no_wander && IsAggressive())
		{
			if (GetMapIndex() == 61 && quest::CQuestManager::instance().GetEventFlag("xmas_tree"));
			// 서한산에서 나무가 있으면 선공하지않는다.
			else
				victim = FindVictim(this, m_pkMobData->m_table.wAggressiveSight);
		}
	}

	if (victim && !victim->IsDead())
	{
		if (CanBeginFight())
			BeginFight(victim);

		return;
	}

	if (IsAggressive() && !victim)
		m_dwStateDuration = PASSES_PER_SEC(number(1, 3));
	else
		m_dwStateDuration = PASSES_PER_SEC(number(3, 5));

	LPCHARACTER pkChrProtege = GetProtege();

	// 보호할 것(돌, 파티장)에게로 부터 멀다면 따라간다.
	if (pkChrProtege)
	{
		if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 1000)
		{
			if (Follow(pkChrProtege, number(150, 400)))
			{
				MonsterLog("[IDLE] 리더로부터 너무 멀리 떨어졌다! 복귀한다.");
				return;
			}
		}
	}

	//
	// 그냥 왔다리 갔다리 한다.
	//
	if (!no_wander && !IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
	{
		if (!number(0, 6))
		{
			SetRotation(number(0, 359));        // 방향은 랜덤으로 설정

			float fx, fy;
			float fDist = number(300, 700);

			GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

			// 느슨한 못감 속성 체크; 최종 위치와 중간 위치가 갈수없다면 가지 않는다.
			if (!(SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx, GetY() + (int) fy) 
						&& SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx/2, GetY() + (int) fy/2)))
				return;

			// NOTE: 몬스터가 IDLE 상태에서 주변을 서성거릴 때, 현재 무조건 뛰어가게 되어 있음. (절대로 걷지 않음)
			// 그래픽 팀에서 몬스터가 걷는 모습도 보고싶다고 해서 임시로 특정확률로 걷거나 뛰게 함. (게임의 전반적인 느낌이 틀려지기 때문에 일단 테스트 모드에서만 작동)
			if (g_test_server)
			{
				if (number(0, 100) < 60)
					SetNowWalking(false);
				else
					SetNowWalking(true);
			}

			if (Goto(GetX() + (int) fx, GetY() + (int) fy))
				SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

			return;
		}
	}

	MonsterChat(MONSTER_CHAT_WAIT);
}
// END_OF_STATE_IDLE_REFACTORING

bool __CHARACTER_GotoNearTarget(LPCHARACTER self, LPCHARACTER victim)
{
	if (IS_SET(self->GetAIFlag(), AIFLAG_NOMOVE))
		return false;

	switch (self->GetMobBattleType())
	{
		case BATTLE_TYPE_RANGE:
		case BATTLE_TYPE_MAGIC:
			// 마법사나 궁수는 공격 거리의 80%까지 가서 공격을 시작한다.
			if (self->Follow(victim, self->GetMobAttackRange() * 8 / 10))
				return true;
			break;

		default:
			// 나머지는 90%?
			if (self->Follow(victim, self->GetMobAttackRange() * 9 / 10))
				return true;
	}

	return false;
}

void CHARACTER::StateMove()
{
	DWORD dwElapsedTime = get_dword_time() - m_dwMoveStartTime;
	float fRate = (float) dwElapsedTime / (float) m_dwMoveDuration;

	if (fRate > 1.0f)
		fRate = 1.0f;

	int x = (int) ((float) (m_posDest.x - m_posStart.x) * fRate + m_posStart.x);
	int y = (int) ((float) (m_posDest.y - m_posStart.y) * fRate + m_posStart.y);

	Move(x, y);

	if (IsPC() && (thecore_pulse() & 15) == 0)
	{
		UpdateSectree();

		if (GetExchange())
		{
			LPCHARACTER victim = GetExchange()->GetCompany()->GetOwner();
			int iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

			// 거리 체크
			if (iDist >= EXCHANGE_MAX_DISTANCE)
			{
				GetExchange()->Cancel();
			}
		}
	}

	// 스테미나가 0 이상이어야 한다.
	if (IsPC())
	{
		if (IsWalking() && GetStamina() < GetMaxStamina())
		{
			// 5초 후 부터 스테미너 증가
			if (get_dword_time() - GetWalkStartTime() > 5000)
				PointChange(POINT_STAMINA, GetMaxStamina() / 1);
		}

		// 전투 중이면서 뛰는 중이면
		if (!IsWalking() && !IsRiding())
			if ((get_dword_time() - GetLastAttackTime()) < 20000)
			{
				StartAffectEvent();

				if (IsStaminaHalfConsume())
				{
					if (thecore_pulse()&1)
						PointChange(POINT_STAMINA, -STAMINA_PER_STEP);
				}
				else
					PointChange(POINT_STAMINA, -STAMINA_PER_STEP);

				StartStaminaConsume();

				if (GetStamina() <= 0)
				{
					// 스테미나가 모자라 걸어야함
					SetStamina(0);
					SetNowWalking(true);
					StopStaminaConsume();
				}
			}
			else if (IsStaminaConsume())
			{
				StopStaminaConsume();
			}
	}
	else
	{
		// XXX AGGRO 
		if (IsMonster() && GetVictim())
		{
			LPCHARACTER victim = GetVictim();
			UpdateAggrPoint(victim, DAMAGE_TYPE_NORMAL, -(victim->GetLevel() / 3 + 1));

			if (g_test_server)
			{
				// 몬스터가 적을 쫓아가는 것이면 무조건 뛰어간다.
				SetNowWalking(false);
			}
		}

		if (IsMonster() && GetMobRank() >= MOB_RANK_BOSS && GetVictim())
		{
			LPCHARACTER victim = GetVictim();

			// 거대 거북
			if (GetRaceNum() == 2191 && number(1, 20) == 1 && get_dword_time() - m_pkMobInst->m_dwLastWarpTime > 1000)
			{
				// 워프 테스트
				float fx, fy;
				GetDeltaByDegree(victim->GetRotation(), 400, &fx, &fy);
				long new_x = victim->GetX() + (long)fx;
				long new_y = victim->GetY() + (long)fy;
				SetRotation(GetDegreeFromPositionXY(new_x, new_y, victim->GetX(), victim->GetY()));
				Show(victim->GetMapIndex(), new_x, new_y, 0, true);
				GotoState(m_stateBattle);
				m_dwStateDuration = 1;
				ResetMobSkillCooltime();
				m_pkMobInst->m_dwLastWarpTime = get_dword_time();
				return;
			}

			// TODO 방향전환을 해서 덜 바보가 되자!
			if (number(0, 3) == 0)
			{
				if (__CHARACTER_GotoNearTarget(this, victim))
					return;
			}
		}
	}

	if (1.0f == fRate)
	{
		if (IsPC())
		{
			sys_log(1, "도착 %s %d %d", GetName(), x, y);
			GotoState(m_stateIdle);
			StopStaminaConsume();
		}
		else
		{
			if (GetVictim() && !IsCoward())
			{
				if (!IsState(m_stateBattle))
					MonsterLog("[BATTLE] 근처에 왔으니 공격시작 %s", GetVictim()->GetName());

				GotoState(m_stateBattle);
				m_dwStateDuration = 1;
			}
			else
			{
				if (!IsState(m_stateIdle))
					MonsterLog("[IDLE] 대상이 없으니 쉬자");

				GotoState(m_stateIdle);

				LPCHARACTER rider = GetRider();

				m_dwStateDuration = PASSES_PER_SEC(number(1, 3));
			}
		}
	}
}

void CHARACTER::StateBattle()
{
	if (IsStone())
	{
		sys_err("Stone must not use battle state (name %s)", GetName());
		return;
	}

	if (IsPC())
		return; 

	if (!CanMove())
		return;

	if (IsStun())
		return;

	LPCHARACTER victim = GetVictim();

	if (IsCoward())
	{
		if (IsDead())
			return;

		SetVictim(NULL);

		if (number(1, 50) != 1)
		{
			GotoState(m_stateIdle);
			m_dwStateDuration = 1;
		}
		else
			CowardEscape();

		return;
	}

	if (!victim || (victim->IsStun() && IsGuardNPC()) || victim->IsDead())
	{
		if (victim && victim->IsDead() &&
				!no_wander && IsAggressive() && (!GetParty() || GetParty()->GetLeader() == this))
		{
			LPCHARACTER new_victim = FindVictim(this, m_pkMobData->m_table.wAggressiveSight);

			SetVictim(new_victim);
			m_dwStateDuration = PASSES_PER_SEC(1);

			if (!new_victim)
			{
				switch (GetMobBattleType())
				{
					case BATTLE_TYPE_MELEE:
					case BATTLE_TYPE_SUPER_POWER:
					case BATTLE_TYPE_SUPER_TANKER:
					case BATTLE_TYPE_POWER:
					case BATTLE_TYPE_TANKER:
						{
							float fx, fy;
							float fDist = number(400, 1500);

							GetDeltaByDegree(number(0, 359), fDist, &fx, &fy);

							if (SECTREE_MANAGER::instance().IsMovablePosition(victim->GetMapIndex(),
										victim->GetX() + (int) fx, 
										victim->GetY() + (int) fy) && 
									SECTREE_MANAGER::instance().IsMovablePosition(victim->GetMapIndex(),
										victim->GetX() + (int) fx/2,
										victim->GetY() + (int) fy/2))
							{
								float dx = victim->GetX() + fx;
								float dy = victim->GetY() + fy;

								SetRotation(GetDegreeFromPosition(dx, dy));

								if (Goto((long) dx, (long) dy))
								{
									sys_log(0, "KILL_AND_GO: %s distance %.1f", GetName(), fDist);
									SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
								}
							}
						}
				}
			}
			return;
		}

		SetVictim(NULL);

		if (IsGuardNPC())
			Return();

		m_dwStateDuration = PASSES_PER_SEC(1);
		return;
	}

	if (IsSummonMonster() && !IsDead() && !IsStun())
	{
		if (!GetParty())
		{
			// 서몬해서 채워둘 파티를 만들어 둡니다.
			CPartyManager::instance().CreateParty(this);
		}

		LPPARTY pParty = GetParty();
		bool bPct = !number(0, 3);

		if (bPct && pParty->CountMemberByVnum(GetSummonVnum()) < SUMMON_MONSTER_COUNT)
		{
			MonsterLog("부하 몬스터 소환!");
			// 모자라는 녀석을 불러내 채웁시다.
			int sx = GetX() - 300;
			int sy = GetY() - 300;
			int ex = GetX() + 300;
			int ey = GetY() + 300;

			LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(GetSummonVnum(), GetMapIndex(), sx, sy, ex, ey, true, true);

			if (tch)
			{
				pParty->Join(tch->GetVID());
				pParty->Link(tch);
			}
		}
	}

	LPCHARACTER pkChrProtege = GetProtege();

	float fDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (fDist >= 4000.0f)   // 40미터 이상 멀어지면 포기
	{
		MonsterLog("타겟이 멀어서 포기");
		SetVictim(NULL);

		// 보호할 것(돌, 파티장) 주변으로 간다.
		if (pkChrProtege)
			if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 1000)
				Follow(pkChrProtege, number(150, 400));

		return;
	}

	if (fDist >= GetMobAttackRange() * 1.15)
	{
		__CHARACTER_GotoNearTarget(this, victim);
		return;
	}

	if (m_pkParty)
		m_pkParty->SendMessage(this, PM_ATTACKED_BY, 0, 0);

	if (2493 == m_pkMobData->m_table.dwVnum)
	{
		// 수룡(2493) 특수 처리
		m_dwStateDuration = BlueDragon_StateBattle(this);
		return;
	}

	DWORD dwCurTime = get_dword_time();
	DWORD dwDuration = CalculateDuration(GetLimitPoint(POINT_ATT_SPEED), 2000);

	if ((dwCurTime - m_dwLastAttackTime) < dwDuration) // 2초 마다 공격해야 한다.
	{
		m_dwStateDuration = MAX(1, (passes_per_sec * (dwDuration - (dwCurTime - m_dwLastAttackTime)) / 1000));
		return;
	}

	if (IsBerserker() == true)
		if (GetHPPct() < m_pkMobData->m_table.bBerserkPoint)
			if (IsBerserk() != true)
				SetBerserk(true);

	if (IsGodSpeeder() == true)
		if (GetHPPct() < m_pkMobData->m_table.bGodSpeedPoint)
			if (IsGodSpeed() != true)
				SetGodSpeed(true);

	//
	// 몹 스킬 처리
	//
	if (HasMobSkill())
	{
		for (unsigned int iSkillIdx = 0; iSkillIdx < MOB_SKILL_MAX_NUM; ++iSkillIdx)
		{
			if (CanUseMobSkill(iSkillIdx))
			{
				SetRotationToXY(victim->GetX(), victim->GetY());

				if (UseMobSkill(iSkillIdx))
				{
					SendMovePacket(FUNC_MOB_SKILL, iSkillIdx, GetX(), GetY(), 0, dwCurTime);

					float fDuration = CMotionManager::instance().GetMotionDuration(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_SPECIAL_1 + iSkillIdx));
					m_dwStateDuration = (DWORD) (fDuration == 0.0f ? PASSES_PER_SEC(2) : PASSES_PER_SEC(fDuration));

					if (test_server)
						sys_log(0, "USE_MOB_SKILL: %s idx %u motion %u duration %.0f", GetName(), iSkillIdx, MOTION_SPECIAL_1 + iSkillIdx, fDuration);

					return;
				}
			}
		}
	}

	if (!Attack(victim))    // 공격 실패라면? 왜 실패했지? TODO
		m_dwStateDuration = passes_per_sec / 2;
	else
	{
		// 적을 바라보게 만든다.
		SetRotationToXY(victim->GetX(), victim->GetY());

		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0, dwCurTime);

		float fDuration = CMotionManager::instance().GetMotionDuration(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_NORMAL_ATTACK));
		m_dwStateDuration = (DWORD) (fDuration == 0.0f ? PASSES_PER_SEC(2) : PASSES_PER_SEC(fDuration));
	}
}

void CHARACTER::StateFlag()
{
	m_dwStateDuration = (DWORD) PASSES_PER_SEC(0.5);

	CWarMap * pMap = GetWarMap();

	if (!pMap)
		return;

	FuncFindChrForFlag f(this);
	GetSectree()->ForEachAround(f);

	if (!f.m_pkChrFind)
		return;

	if (NULL == f.m_pkChrFind->GetGuild())
		return;

	char buf[256];
	BYTE idx;

	if (!pMap->GetTeamIndex(GetPoint(POINT_STAT), idx))
		return;

	f.m_pkChrFind->AddAffect(AFFECT_WAR_FLAG, POINT_NONE, GetPoint(POINT_STAT), idx == 0 ? AFF_WAR_FLAG1 : AFF_WAR_FLAG2, INFINITE_AFFECT_DURATION, 0, false);
	f.m_pkChrFind->AddAffect(AFFECT_WAR_FLAG, POINT_MOV_SPEED, 50 - f.m_pkChrFind->GetPoint(POINT_MOV_SPEED), 0, INFINITE_AFFECT_DURATION, 0, false);

	pMap->RemoveFlag(idx);

	snprintf(buf, sizeof(buf), LC_TEXT("%s 길드의 깃발을 %s 님이 획득하였습니다."), pMap->GetGuild(idx)->GetName(), f.m_pkChrFind->GetName());
	pMap->Notice(buf);
}

void CHARACTER::StateFlagBase()
{
	m_dwStateDuration = (DWORD) PASSES_PER_SEC(0.5);

	FuncFindChrForFlagBase f(this);
	GetSectree()->ForEachAround(f);
}

void CHARACTER::StateHorse()
{
	float	START_FOLLOW_DISTANCE = 400.0f;		// 이 거리 이상 떨어지면 쫓아가기 시작함
	float	START_RUN_DISTANCE = 700.0f;		// 이 거리 이상 떨어지면 뛰어서 쫓아감.
	int		MIN_APPROACH = 150;					// 최소 접근 거리
	int		MAX_APPROACH = 300;					// 최대 접근 거리	

	DWORD	STATE_DURATION = (DWORD)PASSES_PER_SEC(0.5);	// 상태 지속 시간

	bool bDoMoveAlone = true;					// 캐릭터와 가까이 있을 때 혼자 여기저기 움직일건지 여부 -_-;
	bool bRun = true;							// 뛰어야 하나?

	if (IsDead())
		return;

	m_dwStateDuration = STATE_DURATION;

	LPCHARACTER victim = GetRider();

	// ! 아님 // 대상이 없는 경우 소환자가 직접 나를 클리어할 것임
	if (!victim)
	{
		M2_DESTROY_CHARACTER(this);
		return;
	}

	m_pkMobInst->m_posLastAttacked = GetXYZ();

	float fDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if (fDist > START_RUN_DISTANCE)
			SetNowWalking(!bRun);		// NOTE: 함수 이름보고 멈추는건줄 알았는데 SetNowWalking(false) 하면 뛰는거임.. -_-;

		Follow(victim, number(MIN_APPROACH, MAX_APPROACH));

		m_dwStateDuration = STATE_DURATION;
	}
	else if (bDoMoveAlone && (get_dword_time() > m_dwLastAttackTime))
	{
		// wondering-.-
		m_dwLastAttackTime = get_dword_time() + number(5000, 12000);

		SetRotation(number(0, 359));        // 방향은 랜덤으로 설정

		float fx, fy;
		float fDist = number(200, 400);

		GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

		// 느슨한 못감 속성 체크; 최종 위치와 중간 위치가 갈수없다면 가지 않는다.
		if (!(SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx, GetY() + (int) fy) 
					&& SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx/2, GetY() + (int) fy/2)))
			return;

		SetNowWalking(true);

		if (Goto(GetX() + (int) fx, GetY() + (int) fy))
			SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}
}

