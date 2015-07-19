#ifndef __INC_METIN_II_GAME_BATTLE_H__
#define __INC_METIN_II_GAME_BATTLE_H__

#include "char.h"

enum EBattleTypes       // 상대방 기준
{
	BATTLE_NONE,
	BATTLE_DAMAGE,
	BATTLE_DEFENSE,
	BATTLE_DEAD
};

extern int	CalcAttBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iAtk);
extern int	CalcBattleDamage(int iDam, int iAttackerLev, int iVictimLev);
extern int	CalcMeleeDamage(LPCHARACTER pAttacker, LPCHARACTER pVictim, bool bIgnoreDefense = false, bool bIgnoreTargetRating = false);
extern int	CalcMagicDamage(LPCHARACTER pAttacker, LPCHARACTER pVictim);
extern int	CalcArrowDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, LPITEM pkBow, LPITEM pkArrow, bool bIgnoreDefense = false);
extern float	CalcAttackRating(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreTargetRating = false);

extern bool	battle_is_attackable(LPCHARACTER ch, LPCHARACTER victim);
extern int	battle_melee_attack(LPCHARACTER ch, LPCHARACTER victim);
extern void	battle_end(LPCHARACTER ch);

extern bool	battle_distance_valid_by_xy(long x, long y, long tx, long ty);
extern bool	battle_distance_valid(LPCHARACTER ch, LPCHARACTER victim);
extern int	battle_count_attackers(LPCHARACTER ch);

extern void	NormalAttackAffect(LPCHARACTER pkAttacker, LPCHARACTER pkVictim);

// 특성 공격
inline void AttackAffect(LPCHARACTER pkAttacker,
		LPCHARACTER pkVictim,
		BYTE att_point,
		DWORD immune_flag,
		DWORD affect_idx,
		BYTE affect_point,
		long affect_amount,
		DWORD affect_flag,
		int time,
		const char* name)
{
	if (pkAttacker->GetPoint(att_point) && !pkVictim->IsAffectFlag(affect_flag))
	{
		if (number(1, 100) <= pkAttacker->GetPoint(att_point) && !pkVictim->IsImmune(immune_flag))
		{
			pkVictim->AddAffect(affect_idx, affect_point, affect_amount, affect_flag, time, 0, true);

			if (test_server)
			{
				pkVictim->ChatPacket(CHAT_TYPE_PARTY, "%s %s(%ld%%) SUCCESS", pkAttacker->GetName(), name, pkAttacker->GetPoint(att_point));
			}
		}
		else if (test_server)
		{
			pkVictim->ChatPacket(CHAT_TYPE_PARTY, "%s %s(%ld%%) FAIL", pkAttacker->GetName(), name, pkAttacker->GetPoint(att_point));
		}
	}
}

inline void SkillAttackAffect(LPCHARACTER pkVictim,
		int success_pct,
		DWORD immune_flag,
		DWORD affect_idx,
		BYTE affect_point,
		long affect_amount,
		DWORD affect_flag,
		int time,
		const char* name)
{
	if (success_pct && !pkVictim->IsAffectFlag(affect_flag))
	{
		if (number(1, 1000) <= success_pct && !pkVictim->IsImmune(immune_flag))
		{
			pkVictim->AddAffect(affect_idx, affect_point, affect_amount, affect_flag, time, 0, true);

			// SKILL_ATTACK_NO_LOG_TARGET_NAME_FIX
			if (test_server)
				pkVictim->ChatPacket(CHAT_TYPE_PARTY, 
						"%s(%d%%) -> %s SUCCESS", name, success_pct, name);
			// END_OF_SKILL_ATTACK_LOG_NO_TARGET_NAME_FIX
		}
		else if (test_server)
		{
			// SKILL_ATTACK_NO_LOG_TARGET_NAME_FIX
			pkVictim->ChatPacket(CHAT_TYPE_PARTY, "%s(%d%%) -> %s FAIL", name, success_pct, name);
			// END_OF_SKILL_ATTACK_LOG_NO_TARGET_NAME_FIX
		}
	}
} 


#define GET_SPEED_HACK_COUNT(ch)		((ch)->m_speed_hack_count)
#define INCREASE_SPEED_HACK_COUNT(ch)	(++GET_SPEED_HACK_COUNT(ch))
DWORD	GET_ATTACK_SPEED(LPCHARACTER ch);
void	SET_ATTACK_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time);
void	SET_ATTACKED_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time);
bool	IS_SPEED_HACK(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time);

#endif
