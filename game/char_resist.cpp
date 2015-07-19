#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "locale_service.h"

// 독
const int poison_damage_rate[MOB_RANK_MAX_NUM] = 
{
	80, 50, 40, 30, 25, 1
};

int GetPoisonDamageRate(LPCHARACTER ch)
{
	int iRate;

	if (ch->IsPC())
	{
		if (LC_IsYMIR())
			iRate = 40;
		else
			iRate = 50;
	}
	else
		iRate = poison_damage_rate[ch->GetMobRank()];

	iRate = MAX(0, iRate - ch->GetPoint(POINT_POISON_REDUCE));
	return iRate;
}

EVENTINFO(TPoisonEventInfo)
{
	DynamicCharacterPtr ch;
	int		count;
	DWORD	attacker_pid;

	TPoisonEventInfo()
	: ch()
	, count(0)
	, attacker_pid(0)
	{
	}
};

EVENTFUNC(poison_event)
{
	TPoisonEventInfo * info = dynamic_cast<TPoisonEventInfo *>( event->info );
	
	if ( info == NULL )
	{
		sys_err( "poison_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	LPCHARACTER pkAttacker = CHARACTER_MANAGER::instance().FindByPID(info->attacker_pid);

	int dam = ch->GetMaxHP() * GetPoisonDamageRate(ch) / 1000;
	if (test_server) ch->ChatPacket(CHAT_TYPE_NOTICE, "Poison Damage %d", dam);

	if (ch->Damage(pkAttacker, dam, DAMAGE_TYPE_POISON))
	{
		ch->m_pkPoisonEvent = NULL;
		return 0;
	}

	--info->count;

	if (info->count)
		return PASSES_PER_SEC(3);
	else
	{
		ch->m_pkPoisonEvent = NULL;
		return 0;
	}
}

EVENTINFO(TFireEventInfo)
{
	DynamicCharacterPtr ch;
	int		count;
	int		amount;
	DWORD	attacker_pid;

	TFireEventInfo()
	: ch()
	, count(0)
	, amount(0)
	, attacker_pid(0)
	{
	}
};

EVENTFUNC(fire_event)
{
	TFireEventInfo * info = dynamic_cast<TFireEventInfo *>( event->info );

	if ( info == NULL )
	{
		sys_err( "fire_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	LPCHARACTER pkAttacker = CHARACTER_MANAGER::instance().FindByPID(info->attacker_pid);

	int dam = info->amount;
	if (test_server) ch->ChatPacket(CHAT_TYPE_NOTICE, "Fire Damage %d", dam);

	if (ch->Damage(pkAttacker, dam, DAMAGE_TYPE_FIRE))
	{
		ch->m_pkFireEvent = NULL;
		return 0;
	}

	--info->count;

	if (info->count)
		return PASSES_PER_SEC(3);
	else
	{
		ch->m_pkFireEvent = NULL;
		return 0;
	}
}

/*

   LEVEL에 의한..

   +8   0%
   +7   5%
   +6  10%
   +5  30%
   +4  50%
   +3  70%
   +2  80%
   +1  90%
   +0 100%
   -1 100%
   -2 100%
   -3 100%
   -4 100%
   -5 100%
   -6 100%
   -7 100%
   -8 100%

 */

static int poison_level_adjust[9] =
{
	100, 90, 80, 70, 50, 30, 10, 5, 0
};

void CHARACTER::AttackedByFire(LPCHARACTER pkAttacker, int amount, int count)
{
	if (m_pkFireEvent)
		return;

	AddAffect(AFFECT_FIRE, POINT_NONE, 0, AFF_FIRE, count*3+1, 0, true);

	TFireEventInfo* info = AllocEventInfo<TFireEventInfo>();

	info->ch = this;
	info->count = count;
	info->amount = amount;
	info->attacker_pid = pkAttacker->GetPlayerID();

	m_pkFireEvent = event_create(fire_event, info, 1);
}

void CHARACTER::AttackedByPoison(LPCHARACTER pkAttacker)
{
	if (m_pkPoisonEvent)
		return;

	if (m_bHasPoisoned && !IsPC()) // 몬스터는 독이 한번만 걸린다.
		return;

	if (pkAttacker && pkAttacker->GetLevel() < GetLevel())
	{
		int delta = GetLevel() - pkAttacker->GetLevel();

		if (delta > 8)
			delta = 8;

		if (number(1, 100) > poison_level_adjust[delta])
			return;
	}

	/*if (IsImmune(IMMUNE_POISON))
	  return;*/

	// 독 내성 굴림 실패, 독에 걸렸다!
	m_bHasPoisoned = true;

	AddAffect(AFFECT_POISON, POINT_NONE, 0, AFF_POISON, POISON_LENGTH + 1, 0, true);

	TPoisonEventInfo* info = AllocEventInfo<TPoisonEventInfo>();

	info->ch = this;
	info->count = 10;
	info->attacker_pid = pkAttacker?pkAttacker->GetPlayerID():0;

	m_pkPoisonEvent = event_create(poison_event, info, 1);

	if (test_server && pkAttacker)
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "POISON %s -> %s", pkAttacker->GetName(), GetName());
		pkAttacker->ChatPacket(CHAT_TYPE_INFO, "%s", buf);
	}
}

void CHARACTER::RemoveFire()
{
	RemoveAffect(AFFECT_FIRE);
	event_cancel(&m_pkFireEvent);
}

void CHARACTER::RemovePoison()
{
	RemoveAffect(AFFECT_POISON);
	event_cancel(&m_pkPoisonEvent);
}

void CHARACTER::ApplyMobAttribute(const TMobTable* table)
{
	for (int i = 0; i < MOB_ENCHANTS_MAX_NUM; ++i)
	{
		if (table->cEnchants[i] != 0)
			ApplyPoint(aiMobEnchantApplyIdx[i], table->cEnchants[i]);
	}

	for (int i = 0; i < MOB_RESISTS_MAX_NUM; ++i)
	{
		if (table->cResists[i] != 0)
			ApplyPoint(aiMobResistsApplyIdx[i], table->cResists[i]);
	}
}

bool CHARACTER::IsImmune(DWORD dwImmuneFlag)
{
	if (IS_SET(m_pointsInstant.dwImmuneFlag, dwImmuneFlag))
	{
		int immune_pct = 90;
		int	percent = number(1, 100);

		if (percent <= immune_pct)	// 90% Immune
		{
			if (test_server && IsPC())
				ChatPacket(CHAT_TYPE_PARTY, "<IMMUNE_SUCCESS> (%s)", GetName()); 

			return true;
		}
		else
		{
			if (test_server && IsPC())
				ChatPacket(CHAT_TYPE_PARTY, "<IMMUNE_FAIL> (%s)", GetName());

			return false;
		}
	}

	if (test_server && IsPC())
		ChatPacket(CHAT_TYPE_PARTY, "<IMMUNE_FAIL> (%s) NO_IMMUNE_FLAG", GetName());

	return false;
}

